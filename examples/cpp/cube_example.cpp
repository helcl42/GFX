#include "GfxApi.hpp"
#include <GLFW/glfw3.h>

// Include platform-specific GLFW headers to get native handles
#ifdef _WIN32
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
#elif defined(__linux__)
#define GLFW_EXPOSE_NATIVE_X11
#include <GLFW/glfw3native.h>
// X11 headers define "Success" as a macro which conflicts with our enum
#ifdef Success
#undef Success
#endif
#elif defined(__APPLE__)
#define GLFW_EXPOSE_NATIVE_COCOA
#include <GLFW/glfw3native.h>
#endif

#include <array>
#include <cmath>
#include <cstring>
#include <iostream>
#include <memory>
#include <vector>

using namespace gfx;

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

static constexpr size_t MAX_FRAMES_IN_FLIGHT = 3;
static constexpr uint32_t WINDOW_WIDTH = 800;
static constexpr uint32_t WINDOW_HEIGHT = 600;

// Vertex structure for cube
struct Vertex {
    std::array<float, 3> position;
    std::array<float, 3> color;
};

// Uniform buffer structure for transformations
struct UniformData {
    std::array<std::array<float, 4>, 4> model; // Model matrix
    std::array<std::array<float, 4>, 4> view; // View matrix
    std::array<std::array<float, 4>, 4> projection; // Projection matrix
};

class CubeApp {
public:
    bool initialize();
    void run();
    void cleanup();

private:
    bool initializeGLFW();
    bool initializeGraphics();
    bool createSyncObjects();
    bool createRenderingResources();
    bool createRenderPipeline();
    void updateUniforms();
    void render();
    void recreateSwapchain();
    PlatformWindowHandle extractNativeHandle();
    std::vector<uint8_t> loadBinaryFile(const char* filepath);

    // Matrix math utilities
    void matrixIdentity(std::array<std::array<float, 4>, 4>& matrix);
    void matrixPerspective(std::array<std::array<float, 4>, 4>& matrix, float fovy, float aspect, float nearPlane, float farPlane);
    void matrixLookAt(std::array<std::array<float, 4>, 4>& matrix,
        float eyeX, float eyeY, float eyeZ,
        float centerX, float centerY, float centerZ,
        float upX, float upY, float upZ);
    void matrixRotateX(std::array<std::array<float, 4>, 4>& matrix, float angle);
    void matrixRotateY(std::array<std::array<float, 4>, 4>& matrix, float angle);
    void matrixMultiply(std::array<std::array<float, 4>, 4>& result,
        const std::array<std::array<float, 4>, 4>& a,
        const std::array<std::array<float, 4>, 4>& b);

    static void framebufferResizeCallback(GLFWwindow* window, int width, int height);
    static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);

    GLFWwindow* window = nullptr;
    std::shared_ptr<Instance> instance;
    std::shared_ptr<Adapter> adapter;
    std::shared_ptr<Device> device;
    std::shared_ptr<Queue> queue;
    std::shared_ptr<Surface> surface;
    std::shared_ptr<Swapchain> swapchain;

    std::shared_ptr<Buffer> vertexBuffer;
    std::shared_ptr<Buffer> indexBuffer;
    std::shared_ptr<Shader> vertexShader;
    std::shared_ptr<Shader> fragmentShader;
    std::shared_ptr<RenderPipeline> renderPipeline;
    std::shared_ptr<BindGroupLayout> uniformBindGroupLayout;

    // Per-frame resources (for frames in flight)
    std::array<std::shared_ptr<Buffer>, MAX_FRAMES_IN_FLIGHT> uniformBuffers;
    std::array<std::shared_ptr<BindGroup>, MAX_FRAMES_IN_FLIGHT> uniformBindGroups;
    std::array<std::shared_ptr<CommandEncoder>, MAX_FRAMES_IN_FLIGHT> commandEncoders;

    // Per-frame synchronization
    std::array<std::shared_ptr<Semaphore>, MAX_FRAMES_IN_FLIGHT> imageAvailableSemaphores;
    std::array<std::shared_ptr<Semaphore>, MAX_FRAMES_IN_FLIGHT> renderFinishedSemaphores;
    std::array<std::shared_ptr<Fence>, MAX_FRAMES_IN_FLIGHT> inFlightFences;
    size_t currentFrame = 0;

    // Animation state
    float rotationAngleX = 0.0f;
    float rotationAngleY = 0.0f;
    double lastTime = 0.0;
};

bool CubeApp::initialize()
{
    if (!initializeGLFW()) {
        return false;
    }

    if (!initializeGraphics()) {
        return false;
    }

    if (!createSyncObjects()) {
        return false;
    }

    if (!createRenderingResources()) {
        return false;
    }

    std::cout << "Application initialized successfully!" << std::endl;
    std::cout << "Press ESC or close window to exit" << std::endl;

    return true;
}

bool CubeApp::initializeGLFW()
{
    // Initialize GLFW
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return false;
    }

    // Create window
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API); // No OpenGL context
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

    window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Rotating Cube Example (C++ API)", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return false;
    }

    // Set up window resize callback
    glfwSetWindowUserPointer(window, this);
    glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
    glfwSetKeyCallback(window, keyCallback);

    return true;
}

bool CubeApp::initializeGraphics()
{
    try {
        // Create graphics instance with required extensions from GLFW
        uint32_t glfwExtensionCount = 0;
        const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

        std::vector<std::string> extensions;
        for (uint32_t i = 0; i < glfwExtensionCount; ++i) {
            extensions.emplace_back(glfwExtensions[i]);
        }

        InstanceDescriptor instanceDesc{};
        instanceDesc.applicationName = "Rotating Cube Example (C++)";
        instanceDesc.applicationVersion = 1;
        instanceDesc.enableValidation = true;
        instanceDesc.enabledHeadless = false;
        instanceDesc.requiredExtensions = extensions;
        instanceDesc.backend = Backend::Auto;

        instance = createInstance(instanceDesc);
        if (!instance) {
            std::cerr << "Failed to create graphics instance" << std::endl;
            return false;
        }

        // Get adapter
        AdapterDescriptor adapterDesc{};
        adapterDesc.powerPreference = PowerPreference::HighPerformance;
        adapterDesc.forceFallbackAdapter = false;

        adapter = instance->requestAdapter(adapterDesc);
        if (!adapter) {
            std::cerr << "Failed to get graphics adapter" << std::endl;
            return false;
        }

        std::cout << "Using adapter: " << adapter->getName() << std::endl;
        std::cout << "Backend: " << (adapter->getBackend() == Backend::Vulkan ? "Vulkan" : "WebGPU") << std::endl;

        // Create device
        DeviceDescriptor deviceDesc{};
        deviceDesc.label = "Main Device";

        device = adapter->createDevice(deviceDesc);
        if (!device) {
            std::cerr << "Failed to create device" << std::endl;
            return false;
        }

        queue = device->getQueue();

        // Create surface using native platform handles extracted from GLFW
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);

        SurfaceDescriptor surfaceDesc{};
        surfaceDesc.label = "Main Surface";
        surfaceDesc.windowHandle = extractNativeHandle();
        surfaceDesc.width = static_cast<uint32_t>(width);
        surfaceDesc.height = static_cast<uint32_t>(height);

        surface = device->createSurface(surfaceDesc);
        if (!surface) {
            std::cerr << "Failed to create surface" << std::endl;
            return false;
        }

        // Create swapchain
        SwapchainDescriptor swapchainDesc{};
        swapchainDesc.label = "Main Swapchain";
        swapchainDesc.width = static_cast<uint32_t>(width);
        swapchainDesc.height = static_cast<uint32_t>(height);
        swapchainDesc.format = TextureFormat::B8G8R8A8Unorm;
        swapchainDesc.usage = TextureUsage::RenderAttachment;
        swapchainDesc.presentMode = PresentMode::Fifo; // VSync
        swapchainDesc.bufferCount = 3;

        swapchain = device->createSwapchain(surface, swapchainDesc);
        if (!swapchain) {
            std::cerr << "Failed to create swapchain" << std::endl;
            return false;
        }

        return true;
    } catch (const std::exception& e) {
        std::cerr << "Graphics initialization error: " << e.what() << std::endl;
        return false;
    }
}

bool CubeApp::createSyncObjects()
{
    try {
        // Create synchronization objects for each frame in flight
        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            // Create binary semaphores for image availability and render completion
            SemaphoreDescriptor semDesc{};
            semDesc.label = "Image Available Semaphore Frame " + std::to_string(i);
            semDesc.type = SemaphoreType::Binary;

            imageAvailableSemaphores[i] = device->createSemaphore(semDesc);
            if (!imageAvailableSemaphores[i]) {
                std::cerr << "Failed to create image available semaphore " << i << std::endl;
                return false;
            }

            semDesc.label = "Render Finished Semaphore Frame " + std::to_string(i);
            renderFinishedSemaphores[i] = device->createSemaphore(semDesc);
            if (!renderFinishedSemaphores[i]) {
                std::cerr << "Failed to create render finished semaphore " << i << std::endl;
                return false;
            }

            // Create fence (start signaled so first frame doesn't wait)
            FenceDescriptor fenceDesc{};
            fenceDesc.label = "In Flight Fence Frame " + std::to_string(i);
            fenceDesc.signaled = true;

            inFlightFences[i] = device->createFence(fenceDesc);
            if (!inFlightFences[i]) {
                std::cerr << "Failed to create in flight fence " << i << std::endl;
                return false;
            }
        }

        return true;
    } catch (const std::exception& e) {
        std::cerr << "Failed to create sync objects: " << e.what() << std::endl;
        return false;
    }
}

bool CubeApp::createRenderingResources()
{
    try {
        // Create cube vertices (8 vertices for a cube)
        std::array<Vertex, 8> vertices = { {
            // Front face
            { { { -1.0f, -1.0f, 1.0f } }, { { 1.0f, 0.0f, 0.0f } } }, // 0: Bottom-left
            { { { 1.0f, -1.0f, 1.0f } }, { { 0.0f, 1.0f, 0.0f } } }, // 1: Bottom-right
            { { { 1.0f, 1.0f, 1.0f } }, { { 0.0f, 0.0f, 1.0f } } }, // 2: Top-right
            { { { -1.0f, 1.0f, 1.0f } }, { { 1.0f, 1.0f, 0.0f } } }, // 3: Top-left

            // Back face
            { { { -1.0f, -1.0f, -1.0f } }, { { 1.0f, 0.0f, 1.0f } } }, // 4: Bottom-left
            { { { 1.0f, -1.0f, -1.0f } }, { { 0.0f, 1.0f, 1.0f } } }, // 5: Bottom-right
            { { { 1.0f, 1.0f, -1.0f } }, { { 1.0f, 1.0f, 1.0f } } }, // 6: Top-right
            { { { -1.0f, 1.0f, -1.0f } }, { { 0.5f, 0.5f, 0.5f } } } // 7: Top-left
        } };

        // Create cube indices (36 indices for 12 triangles)
        // All faces wound clockwise when viewed from outside
        std::array<uint16_t, 36> indices = { { // Front face (Z+) - vertices 0,1,2,3
            0, 1, 2, 2, 3, 0,
            // Back face (Z-) - vertices 4,5,6,7 - FIXED
            5, 4, 7, 7, 6, 5,
            // Left face (X-) - vertices 4,0,3,7
            4, 0, 3, 3, 7, 4,
            // Right face (X+) - vertices 1,5,6,2
            1, 5, 6, 6, 2, 1,
            // Top face (Y+) - vertices 3,2,6,7
            3, 2, 6, 6, 7, 3,
            // Bottom face (Y-) - vertices 4,5,1,0
            4, 5, 1, 1, 0, 4 } };

        // Create vertex buffer
        BufferDescriptor vertexBufferDesc{};
        vertexBufferDesc.label = "Cube Vertices";
        vertexBufferDesc.size = sizeof(vertices);
        vertexBufferDesc.usage = BufferUsage::Vertex | BufferUsage::CopyDst;
        vertexBufferDesc.mappedAtCreation = false;

        vertexBuffer = device->createBuffer(vertexBufferDesc);
        if (!vertexBuffer) {
            std::cerr << "Failed to create vertex buffer" << std::endl;
            return false;
        }

        // Create index buffer
        BufferDescriptor indexBufferDesc{};
        indexBufferDesc.label = "Cube Indices";
        indexBufferDesc.size = sizeof(indices);
        indexBufferDesc.usage = BufferUsage::Index | BufferUsage::CopyDst;
        indexBufferDesc.mappedAtCreation = false;

        indexBuffer = device->createBuffer(indexBufferDesc);
        if (!indexBuffer) {
            std::cerr << "Failed to create index buffer" << std::endl;
            return false;
        }

        // Upload vertex and index data
        queue->writeBuffer(vertexBuffer, 0, vertices.data(), sizeof(vertices));
        queue->writeBuffer(indexBuffer, 0, indices.data(), sizeof(indices));

        // Create uniform buffers (one per frame in flight)
        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            BufferDescriptor uniformBufferDesc{};
            uniformBufferDesc.label = "Transform Uniforms Frame " + std::to_string(i);
            uniformBufferDesc.size = sizeof(UniformData);
            uniformBufferDesc.usage = BufferUsage::Uniform | BufferUsage::CopyDst;
            uniformBufferDesc.mappedAtCreation = false;

            uniformBuffers[i] = device->createBuffer(uniformBufferDesc);
            if (!uniformBuffers[i]) {
                std::cerr << "Failed to create uniform buffer " << i << std::endl;
                return false;
            }
        }

        // Create bind group layout for uniforms
        BindGroupLayoutEntry uniformLayoutEntry{
            .binding = 0,
            .visibility = ShaderStage::Vertex,
            .resource = BindGroupLayoutEntry::BufferBinding{
                .hasDynamicOffset = false,
                .minBindingSize = sizeof(UniformData) }
        };

        BindGroupLayoutDescriptor uniformLayoutDesc{};
        uniformLayoutDesc.label = "Uniform Bind Group Layout";
        uniformLayoutDesc.entries = { uniformLayoutEntry };

        uniformBindGroupLayout = device->createBindGroupLayout(uniformLayoutDesc);
        if (!uniformBindGroupLayout) {
            std::cerr << "Failed to create uniform bind group layout" << std::endl;
            return false;
        }

        // Create bind groups (one per frame in flight)
        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            BindGroupEntry uniformEntry{};
            uniformEntry.binding = 0;
            uniformEntry.resource = uniformBuffers[i];
            uniformEntry.offset = 0;
            uniformEntry.size = sizeof(UniformData);

            BindGroupDescriptor uniformBindGroupDesc{};
            uniformBindGroupDesc.label = "Uniform Bind Group Frame " + std::to_string(i);
            uniformBindGroupDesc.layout = uniformBindGroupLayout;
            uniformBindGroupDesc.entries = { uniformEntry };

            uniformBindGroups[i] = device->createBindGroup(uniformBindGroupDesc);
            if (!uniformBindGroups[i]) {
                std::cerr << "Failed to create uniform bind group " << i << std::endl;
                return false;
            }
        }

        // Load SPIR-V shaders
        auto vertexShaderCode = loadBinaryFile("cube.vert.spv");
        auto fragmentShaderCode = loadBinaryFile("cube.frag.spv");

        if (vertexShaderCode.empty() || fragmentShaderCode.empty()) {
            std::cerr << "Failed to load SPIR-V shader files" << std::endl;
            return false;
        }

        // Create vertex shader with SPIR-V binary
        ShaderDescriptor vertexShaderDesc{};
        vertexShaderDesc.label = "Cube Vertex Shader";
        vertexShaderDesc.code = std::string(reinterpret_cast<const char*>(vertexShaderCode.data()), vertexShaderCode.size());
        vertexShaderDesc.entryPoint = "main"; // SPIR-V uses "main" as entry point

        vertexShader = device->createShader(vertexShaderDesc);
        if (!vertexShader) {
            std::cerr << "Failed to create vertex shader" << std::endl;
            return false;
        }

        // Create fragment shader with SPIR-V binary
        ShaderDescriptor fragmentShaderDesc{};
        fragmentShaderDesc.label = "Cube Fragment Shader";
        fragmentShaderDesc.code = std::string(reinterpret_cast<const char*>(fragmentShaderCode.data()), fragmentShaderCode.size());
        fragmentShaderDesc.entryPoint = "main"; // SPIR-V uses "main" as entry point

        fragmentShader = device->createShader(fragmentShaderDesc);
        if (!fragmentShader) {
            std::cerr << "Failed to create fragment shader" << std::endl;
            return false;
        }

        // Initialize animation state
        rotationAngleX = 0.0f;
        rotationAngleY = 0.0f;

        // Create render pipeline
        return createRenderPipeline();
    } catch (const std::exception& e) {
        std::cerr << "Resource creation error: " << e.what() << std::endl;
        return false;
    }
}

bool CubeApp::createRenderPipeline()
{
    try {
        // Define vertex buffer layout
        std::vector<VertexAttribute> attributes = {
            { .format = TextureFormat::R32G32B32Float,
                .offset = offsetof(Vertex, position),
                .shaderLocation = 0 },
            { .format = TextureFormat::R32G32B32Float,
                .offset = offsetof(Vertex, color),
                .shaderLocation = 1 }
        };

        VertexBufferLayout vertexLayout{};
        vertexLayout.arrayStride = sizeof(Vertex);
        vertexLayout.attributes = attributes;
        vertexLayout.stepModeInstance = false; // Vertex mode

        // Create render pipeline descriptor
        VertexState vertexState{};
        vertexState.module = vertexShader;
        vertexState.entryPoint = "main";
        vertexState.buffers = { vertexLayout };

        ColorTargetState colorTarget{};
        colorTarget.format = swapchain->getFormat();
        colorTarget.writeMask = 0xF; // All channels

        FragmentState fragmentState{};
        fragmentState.module = fragmentShader;
        fragmentState.entryPoint = "main";
        fragmentState.targets = { colorTarget };

        PrimitiveState primitiveState{};
        primitiveState.topology = PrimitiveTopology::TriangleList;
        primitiveState.frontFaceCounterClockwise = false;
        primitiveState.cullBackFace = true; // Enable back-face culling for 3D

        RenderPipelineDescriptor pipelineDesc{};
        pipelineDesc.label = "Cube Pipeline";
        pipelineDesc.vertex = vertexState;
        pipelineDesc.fragment = fragmentState;
        pipelineDesc.primitive = primitiveState;
        pipelineDesc.sampleCount = 1;
        pipelineDesc.bindGroupLayouts = { uniformBindGroupLayout }; // Pass the bind group layout

        renderPipeline = device->createRenderPipeline(pipelineDesc);
        if (!renderPipeline) {
            std::cerr << "Failed to create render pipeline" << std::endl;
            return false;
        }

        return true;
    } catch (const std::exception& e) {
        std::cerr << "Pipeline creation error: " << e.what() << std::endl;
        return false;
    }
}

void CubeApp::updateUniforms()
{
    double currentTime = glfwGetTime();
    float deltaTime = static_cast<float>(currentTime - lastTime);
    lastTime = currentTime;

    // Update rotation angles (both X and Y axes)
    rotationAngleX += deltaTime * 45.0f; // 45 degrees per second around X
    rotationAngleY += deltaTime * 30.0f; // 30 degrees per second around Y
    if (rotationAngleX >= 360.0f) {
        rotationAngleX -= 360.0f;
    }
    if (rotationAngleY >= 360.0f) {
        rotationAngleY -= 360.0f;
    }

    UniformData uniforms;

    // Create rotation matrices (combine X and Y rotations)
    std::array<std::array<float, 4>, 4> rotX, rotY;
    matrixRotateX(rotX, rotationAngleX * M_PI / 180.0f);
    matrixRotateY(rotY, rotationAngleY * M_PI / 180.0f);
    matrixMultiply(uniforms.model, rotY, rotX);

    // Create view matrix (camera looking at origin from distance)
    matrixLookAt(uniforms.view, 0.0f, 0.0f, 5.0f, // eye
        0.0f, 0.0f, 0.0f, // center
        0.0f, 1.0f, 0.0f); // up

    // Create projection matrix
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    float aspect = static_cast<float>(width) / static_cast<float>(height);
    matrixPerspective(uniforms.projection, 45.0f * M_PI / 180.0f, aspect, 0.1f, 100.0f);

    // Upload uniform data to current frame's uniform buffer
    queue->writeBuffer(uniformBuffers[currentFrame], 0, &uniforms, sizeof(uniforms));
}

void CubeApp::render()
{
    try {
        // Wait for this frame's fence to be signaled
        inFlightFences[currentFrame]->wait(UINT64_MAX);
        inFlightFences[currentFrame]->reset();

        // Deferred cleanup: destroy the old command encoder for this frame slot
        // Now that the fence is signaled, we know the GPU is done with it
        if (commandEncoders[currentFrame]) {
            commandEncoders[currentFrame].reset();
        }

        // Acquire next image with explicit synchronization
        uint32_t imageIndex;
        auto result = swapchain->acquireNextImage(
            UINT64_MAX,
            imageAvailableSemaphores[currentFrame],
            nullptr,
            &imageIndex);

        if (result != gfx::Result::Success) {
            std::cerr << "Failed to acquire next image" << std::endl;
            return;
        }

        // Update uniform buffer with current transformation matrices
        updateUniforms();

        // Get the texture view for the acquired image
        auto backbuffer = swapchain->getImageView(imageIndex);
        if (!backbuffer) {
            return; // Skip frame if no backbuffer available
        }

        // Create command encoder for this frame
        auto commandEncoder = device->createCommandEncoder("Cube Render Frame " + std::to_string(currentFrame));

        // Begin render pass
        Color clearColor{ 0.1f, 0.2f, 0.3f, 1.0f }; // Dark blue background

        auto renderPass = commandEncoder->beginRenderPass(
            { backbuffer },
            { clearColor });

        // Set pipeline, bind groups, and buffers (using current frame's bind group)
        renderPass->setPipeline(renderPipeline);

        // Set viewport and scissor to fill the entire render target
        uint32_t swapWidth = swapchain->getWidth();
        uint32_t swapHeight = swapchain->getHeight();
        renderPass->setViewport(0.0f, 0.0f, static_cast<float>(swapWidth), static_cast<float>(swapHeight), 0.0f, 1.0f);
        renderPass->setScissorRect(0, 0, swapWidth, swapHeight);

        renderPass->setBindGroup(0, uniformBindGroups[currentFrame]);
        renderPass->setVertexBuffer(0, vertexBuffer);
        renderPass->setIndexBuffer(indexBuffer, IndexFormat::Uint16);

        // Draw cube (36 indices for 12 triangles)
        renderPass->drawIndexed(36, 1, 0, 0, 0);

        // End render pass
        renderPass->end();

        // Finish command encoding
        commandEncoder->finish();

        // Submit with explicit synchronization
        SubmitInfo submitInfo{};
        submitInfo.waitSemaphores = { imageAvailableSemaphores[currentFrame] };
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.signalSemaphores = { renderFinishedSemaphores[currentFrame] };
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.signalFence = inFlightFences[currentFrame];

        queue->submitWithSync(commandEncoder, submitInfo);

        // Present with explicit synchronization
        PresentInfo presentInfo{};
        presentInfo.waitSemaphores = { renderFinishedSemaphores[currentFrame] };
        presentInfo.waitSemaphoreCount = 1;

        result = swapchain->presentWithSync(presentInfo);
        if (result != gfx::Result::Success) {
            std::cerr << "Failed to present" << std::endl;
        }

        // Store the command encoder for deferred cleanup next time we use this frame slot
        commandEncoders[currentFrame] = commandEncoder;

        // Advance to next frame
        currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
    } catch (const std::exception& e) {
        std::cerr << "Render error: " << e.what() << std::endl;
    }
}

void CubeApp::recreateSwapchain()
{
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);

    // Wait if window is minimized
    while (width == 0 || height == 0) {
        glfwGetFramebufferSize(window, &width, &height);
        glfwWaitEvents();
    }

    // Recreate swapchain with new size
    swapchain->resize(static_cast<uint32_t>(width), static_cast<uint32_t>(height));

    std::cout << "Swapchain recreated: " << width << "x" << height << std::endl;
}

PlatformWindowHandle CubeApp::extractNativeHandle()
{
    PlatformWindowHandle handle{};

#ifdef _WIN32
    // Windows: Get HWND and HINSTANCE
    handle.hwnd = glfwGetWin32Window(window);
    handle.hinstance = GetModuleHandle(nullptr);
    std::cout << "Extracted Win32 handle: HWND=" << handle.hwnd << ", HINSTANCE=" << handle.hinstance << std::endl;

#elif defined(__linux__)
    // Linux: Get X11 Window and Display (assuming X11, not Wayland)
    handle.windowingSystem = gfx::WindowingSystem::X11;
    handle.x11.window = reinterpret_cast<void*>(glfwGetX11Window(window));
    handle.x11.display = glfwGetX11Display();
    std::cout << "Extracted X11 handle: Window=" << handle.x11.window << ", Display=" << handle.x11.display << std::endl;

#elif defined(__APPLE__)
    // macOS: Get NSWindow
    handle.nsWindow = glfwGetCocoaWindow(window);
    std::cout << "Extracted Cocoa handle: NSWindow=" << handle.nsWindow << std::endl;

#else
    // Fallback: Use GLFW window directly
    handle.window = window;
    std::cout << "Using GLFW window handle directly (fallback)" << std::endl;
#endif

    return handle;
}

void CubeApp::framebufferResizeCallback(GLFWwindow* window, int width, int height)
{
    auto* app = static_cast<CubeApp*>(glfwGetWindowUserPointer(window));
    // The swapchain recreation will be handled in the main loop
    (void)width;
    (void)height; // Suppress unused parameter warnings
}

void CubeApp::keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    (void)scancode;
    (void)mods; // Suppress unused parameter warnings

    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    }
}

void CubeApp::run()
{
    // Initialize timing for first frame
    lastTime = glfwGetTime();

    // Main loop
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        // Handle swapchain recreation if needed
        if (swapchain && swapchain->needsRecreation()) {
            recreateSwapchain();
        }

        render();
    }
}

void CubeApp::cleanup()
{
    // Wait for device to finish
    if (device) {
        device->waitIdle();
    }

    // Clean up per-frame resources
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        commandEncoders[i].reset();
        inFlightFences[i].reset();
        renderFinishedSemaphores[i].reset();
        imageAvailableSemaphores[i].reset();
        uniformBindGroups[i].reset();
        uniformBuffers[i].reset();
    }

    // C++ destructors will handle cleanup automatically
    // But we can explicitly reset shared_ptrs if needed
    renderPipeline.reset();
    fragmentShader.reset();
    vertexShader.reset();
    uniformBindGroupLayout.reset();
    indexBuffer.reset();
    vertexBuffer.reset();
    swapchain.reset();
    surface.reset();
    queue.reset();
    device.reset();
    adapter.reset();
    instance.reset();

    // Destroy GLFW resources
    if (window) {
        glfwDestroyWindow(window);
        window = nullptr;
    }
    glfwTerminate();
}

// Matrix math utility functions
void CubeApp::matrixIdentity(std::array<std::array<float, 4>, 4>& matrix)
{
    matrix = { { { { 1.0f, 0.0f, 0.0f, 0.0f } },
        { { 0.0f, 1.0f, 0.0f, 0.0f } },
        { { 0.0f, 0.0f, 1.0f, 0.0f } },
        { { 0.0f, 0.0f, 0.0f, 1.0f } } } };
}

void CubeApp::matrixPerspective(std::array<std::array<float, 4>, 4>& matrix, float fovy, float aspect, float nearPlane, float farPlane)
{
    matrix = { { { { 0.0f, 0.0f, 0.0f, 0.0f } },
        { { 0.0f, 0.0f, 0.0f, 0.0f } },
        { { 0.0f, 0.0f, 0.0f, 0.0f } },
        { { 0.0f, 0.0f, 0.0f, 0.0f } } } };

    float f = 1.0f / std::tan(fovy / 2.0f);
    matrix[0][0] = f / aspect;
    matrix[1][1] = f;
    matrix[2][2] = (farPlane + nearPlane) / (nearPlane - farPlane);
    matrix[2][3] = -1.0f;
    matrix[3][2] = (2.0f * farPlane * nearPlane) / (nearPlane - farPlane);
}

void CubeApp::matrixLookAt(std::array<std::array<float, 4>, 4>& matrix,
    float eyeX, float eyeY, float eyeZ,
    float centerX, float centerY, float centerZ,
    float upX, float upY, float upZ)
{
    const float epsilon = 1e-6f;

    // Calculate forward vector
    float fx = centerX - eyeX;
    float fy = centerY - eyeY;
    float fz = centerZ - eyeZ;
    float flen = std::sqrt(fx * fx + fy * fy + fz * fz);

    // Check for zero-length forward vector
    if (flen < epsilon) {
        matrixIdentity(matrix);
        return;
    }

    fx /= flen;
    fy /= flen;
    fz /= flen;

    // Calculate right vector (cross product of forward and up)
    float rx = fy * upZ - fz * upY;
    float ry = fz * upX - fx * upZ;
    float rz = fx * upY - fy * upX;
    float rlen = std::sqrt(rx * rx + ry * ry + rz * rz);

    // Check for zero-length right vector (forward and up are parallel)
    if (rlen < epsilon) {
        matrixIdentity(matrix);
        return;
    }

    rx /= rlen;
    ry /= rlen;
    rz /= rlen;

    // Calculate up vector (cross product of right and forward)
    float ux = ry * fz - rz * fy;
    float uy = rz * fx - rx * fz;
    float uz = rx * fy - ry * fx;

    matrix[0][0] = rx;
    matrix[0][1] = ux;
    matrix[0][2] = -fx;
    matrix[0][3] = 0.0f;
    matrix[1][0] = ry;
    matrix[1][1] = uy;
    matrix[1][2] = -fy;
    matrix[1][3] = 0.0f;
    matrix[2][0] = rz;
    matrix[2][1] = uz;
    matrix[2][2] = -fz;
    matrix[2][3] = 0.0f;
    matrix[3][0] = -(rx * eyeX + ry * eyeY + rz * eyeZ);
    matrix[3][1] = -(ux * eyeX + uy * eyeY + uz * eyeZ);
    matrix[3][2] = -(-fx * eyeX + -fy * eyeY + -fz * eyeZ);
    matrix[3][3] = 1.0f;
}

void CubeApp::matrixRotateY(std::array<std::array<float, 4>, 4>& matrix, float angle)
{
    float c = std::cos(angle);
    float s = std::sin(angle);

    matrixIdentity(matrix);
    matrix[0][0] = c;
    matrix[0][2] = s;
    matrix[2][0] = -s;
    matrix[2][2] = c;
}

void CubeApp::matrixRotateX(std::array<std::array<float, 4>, 4>& matrix, float angle)
{
    float c = std::cos(angle);
    float s = std::sin(angle);

    matrixIdentity(matrix);
    matrix[1][1] = c;
    matrix[1][2] = -s;
    matrix[2][1] = s;
    matrix[2][2] = c;
}

void CubeApp::matrixMultiply(std::array<std::array<float, 4>, 4>& result,
    const std::array<std::array<float, 4>, 4>& a,
    const std::array<std::array<float, 4>, 4>& b)
{
    std::array<std::array<float, 4>, 4> temp;
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            temp[i][j] = 0.0f;
            for (int k = 0; k < 4; k++) {
                temp[i][j] += a[i][k] * b[k][j];
            }
        }
    }
    result = temp;
}

std::vector<uint8_t> CubeApp::loadBinaryFile(const char* filepath)
{
    std::FILE* file = std::fopen(filepath, "rb");
    if (!file) {
        std::cerr << "Failed to open file: " << filepath << std::endl;
        return {};
    }

    // Get file size
    std::fseek(file, 0, SEEK_END);
    long fileSize = std::ftell(file);
    std::fseek(file, 0, SEEK_SET);

    if (fileSize <= 0) {
        std::cerr << "Invalid file size for: " << filepath << std::endl;
        std::fclose(file);
        return {};
    }

    // Read file into vector
    std::vector<uint8_t> buffer(fileSize);
    size_t bytesRead = std::fread(buffer.data(), 1, fileSize, file);
    std::fclose(file);

    if (bytesRead != static_cast<size_t>(fileSize)) {
        std::cerr << "Failed to read complete file: " << filepath << std::endl;
        return {};
    }

    return buffer;
}

int main()
{
    std::cout << "=== Cube Example with Unified Graphics API (C++) ===" << std::endl;

    CubeApp app;

    if (!app.initialize()) {
        app.cleanup();
        return -1;
    }

    app.run();
    app.cleanup();

    return 0;
}