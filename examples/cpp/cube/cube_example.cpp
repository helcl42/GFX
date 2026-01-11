#include <gfx_cpp/Gfx.hpp>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#if defined(__EMSCRIPTEN__)
#include <emscripten/emscripten.h>
#else
#if defined(_WIN32)
#define GLFW_EXPOSE_NATIVE_WIN32
#elif defined(__linux__)
#define GLFW_EXPOSE_NATIVE_X11
#elif defined(__APPLE__)
#define GLFW_EXPOSE_NATIVE_COCOA
#endif
#include <GLFW/glfw3native.h>
#endif

#ifdef Success
#undef Success
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

static constexpr uint32_t WINDOW_WIDTH = 800;
static constexpr uint32_t WINDOW_HEIGHT = 600;
static constexpr size_t MAX_FRAMES_IN_FLIGHT = 3;
static constexpr size_t CUBE_COUNT = 3;
static constexpr SampleCount MSAA_SAMPLE_COUNT = SampleCount::Count4;
static constexpr TextureFormat COLOR_FORMAT = TextureFormat::B8G8R8A8UnormSrgb;
static constexpr TextureFormat DEPTH_FORMAT = TextureFormat::Depth32Float;

#if defined(__EMSCRIPTEN__)
static constexpr Backend BACKEND_API = Backend::WebGPU;
#else
// here we can choose between VULKAN, WEBGPU
static constexpr Backend BACKEND_API = Backend::Vulkan;
#endif

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
    void cleanupRenderingResources();
    bool createSizeDependentResources(uint32_t width, uint32_t height);
    void cleanupSizeDependentResources();
    bool createRenderPipeline();
    void updateCube(int cubeIndex);
    void update(float deltaTime);
    void render();
    float getCurrentTime();
    bool mainLoopIteration();
#if defined(__EMSCRIPTEN__)
    static void emscriptenMainLoop(void* userData);
#endif
    PlatformWindowHandle extractNativeHandle();
    std::vector<uint8_t> loadBinaryFile(const char* filepath);
    std::string loadTextFile(const char* filepath);

    // Matrix math utilities
    void matrixIdentity(std::array<std::array<float, 4>, 4>& matrix);
    void matrixPerspective(std::array<std::array<float, 4>, 4>& matrix, float fovy, float aspect, float nearPlane, float farPlane, Backend backend);
    void matrixLookAt(std::array<std::array<float, 4>, 4>& matrix,
        float eyeX, float eyeY, float eyeZ,
        float centerX, float centerY, float centerZ,
        float upX, float upY, float upZ);
    void matrixRotateX(std::array<std::array<float, 4>, 4>& matrix, float angle);
    void matrixRotateY(std::array<std::array<float, 4>, 4>& matrix, float angle);
    void matrixMultiply(std::array<std::array<float, 4>, 4>& result,
        const std::array<std::array<float, 4>, 4>& a,
        const std::array<std::array<float, 4>, 4>& b);
    bool vectorNormalize(float& x, float& y, float& z);

    static void framebufferResizeCallback(GLFWwindow* window, int width, int height);
    static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);

    GLFWwindow* window = nullptr;
    std::shared_ptr<Instance> instance;
    std::shared_ptr<Adapter> adapter;
    AdapterInfo adapterInfo; // Cached adapter info
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

    // Depth buffer
    std::shared_ptr<Texture> depthTexture;
    std::shared_ptr<TextureView> depthTextureView;

    // MSAA color buffer
    std::shared_ptr<Texture> msaaColorTexture;
    std::shared_ptr<TextureView> msaaColorTextureView;

    // Render pass and framebuffers
    std::shared_ptr<RenderPass> renderPass;
    std::vector<std::shared_ptr<Framebuffer>> framebuffers;

    uint32_t windowWidth = WINDOW_WIDTH;
    uint32_t windowHeight = WINDOW_HEIGHT;
    uint32_t previousWidth = WINDOW_WIDTH;
    uint32_t previousHeight = WINDOW_HEIGHT;

    // Per-frame resources (for frames in flight)
    std::shared_ptr<Buffer> sharedUniformBuffer; // Single buffer for all frames and cubes
    size_t uniformAlignedSize = 0; // Aligned size per uniform buffer
    std::array<std::array<std::shared_ptr<BindGroup>, CUBE_COUNT>, MAX_FRAMES_IN_FLIGHT> uniformBindGroups;
    std::array<std::shared_ptr<CommandEncoder>, MAX_FRAMES_IN_FLIGHT> commandEncoders;

    // Per-frame synchronization
    std::array<std::shared_ptr<Semaphore>, MAX_FRAMES_IN_FLIGHT> imageAvailableSemaphores;
    std::array<std::shared_ptr<Semaphore>, MAX_FRAMES_IN_FLIGHT> renderFinishedSemaphores;
    std::array<std::shared_ptr<Fence>, MAX_FRAMES_IN_FLIGHT> inFlightFences;
    size_t currentFrame = 0;

    // Animation state
    float rotationAngleX = 0.0f;
    float rotationAngleY = 0.0f;
    float lastTime = 0.0f;
};

bool CubeApp::initialize()
{
    if (!initializeGLFW()) {
        return false;
    }

    if (!initializeGraphics()) {
        return false;
    }

    if (!createSizeDependentResources(windowWidth, windowHeight)) {
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

    window = glfwCreateWindow(windowWidth, windowHeight, "Rotating Cube Example (C++ API)", nullptr, nullptr);
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
        std::vector<std::string> extensions;
#if !defined(__EMSCRIPTEN__)
        uint32_t glfwExtensionCount = 0;
        const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
        for (uint32_t i = 0; i < glfwExtensionCount; ++i) {
            extensions.emplace_back(glfwExtensions[i]);
        }
#endif
        InstanceDescriptor instanceDesc{};
        instanceDesc.applicationName = "Rotating Cube Example (C++)";
        instanceDesc.applicationVersion = 1;
        instanceDesc.enableValidation = true;
        instanceDesc.enabledHeadless = false;
        instanceDesc.requiredExtensions = extensions;
        instanceDesc.backend = BACKEND_API;

        instance = createInstance(instanceDesc);
        if (!instance) {
            std::cerr << "Failed to create graphics instance" << std::endl;
            return false;
        }

        // Set debug callback after instance creation
        instance->setDebugCallback([](DebugMessageSeverity severity, DebugMessageType type, const std::string& message) {
            const char* severityStr = "";
            switch (severity) {
            case DebugMessageSeverity::Verbose:
                severityStr = "VERBOSE";
                break;
            case DebugMessageSeverity::Info:
                severityStr = "INFO";
                break;
            case DebugMessageSeverity::Warning:
                severityStr = "WARNING";
                break;
            case DebugMessageSeverity::Error:
                severityStr = "ERROR";
                break;
            }

            const char* typeStr = "";
            switch (type) {
            case DebugMessageType::General:
                typeStr = "GENERAL";
                break;
            case DebugMessageType::Validation:
                typeStr = "VALIDATION";
                break;
            case DebugMessageType::Performance:
                typeStr = "PERFORMANCE";
                break;
            }

            std::cout << "[" << severityStr << "|" << typeStr << "] " << message << std::endl;
        });

        // Get adapter
        AdapterDescriptor adapterDesc{};
        adapterDesc.preference = AdapterPreference::HighPerformance;

        adapter = instance->requestAdapter(adapterDesc);
        if (!adapter) {
            std::cerr << "Failed to get graphics adapter" << std::endl;
            return false;
        }

        // Query and store adapter info
        adapterInfo = adapter->getInfo();
        std::cout << "Using adapter: " << adapterInfo.name << std::endl;
        std::cout << "Backend: " << (adapterInfo.backend == Backend::Vulkan ? "Vulkan" : "WebGPU") << std::endl;
        std::cout << "  Vendor ID: 0x" << std::hex << adapterInfo.vendorID << std::dec
                  << ", Device ID: 0x" << std::hex << adapterInfo.deviceID << std::dec << std::endl;

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

        return true;
    } catch (const std::exception& e) {
        std::cerr << "Failed to initialize graphics: " << e.what() << std::endl;
        return false;
    }
}

bool CubeApp::createSizeDependentResources(uint32_t width, uint32_t height)
{
    try {
        // Create swapchain
        SwapchainDescriptor swapchainDesc{};
        swapchainDesc.label = "Main Swapchain";
        swapchainDesc.width = static_cast<uint32_t>(width);
        swapchainDesc.height = static_cast<uint32_t>(height);
        swapchainDesc.format = COLOR_FORMAT;
        swapchainDesc.usage = TextureUsage::RenderAttachment;
        swapchainDesc.presentMode = PresentMode::Fifo;
        swapchainDesc.imageCount = MAX_FRAMES_IN_FLIGHT;

        swapchain = device->createSwapchain(surface, swapchainDesc);
        if (!swapchain) {
            std::cerr << "Failed to create swapchain" << std::endl;
            return false;
        }

        // Create depth texture with MSAA
        TextureDescriptor depthTextureDesc{};
        depthTextureDesc.label = "Depth Buffer";
        depthTextureDesc.type = TextureType::Texture2D;
        depthTextureDesc.size = { static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1 };
        depthTextureDesc.arrayLayerCount = 1;
        depthTextureDesc.mipLevelCount = 1;
        depthTextureDesc.sampleCount = MSAA_SAMPLE_COUNT;
        depthTextureDesc.format = DEPTH_FORMAT;
        depthTextureDesc.usage = TextureUsage::RenderAttachment;

        depthTexture = device->createTexture(depthTextureDesc);
        if (!depthTexture) {
            std::cerr << "Failed to create depth texture" << std::endl;
            return false;
        }

        // Create depth texture view
        TextureViewDescriptor depthViewDesc{};
        depthViewDesc.label = "Depth Buffer View";
        depthViewDesc.viewType = TextureViewType::View2D;
        depthViewDesc.format = DEPTH_FORMAT;
        depthViewDesc.baseMipLevel = 0;
        depthViewDesc.mipLevelCount = 1;
        depthViewDesc.baseArrayLayer = 0;
        depthViewDesc.arrayLayerCount = 1;

        depthTextureView = depthTexture->createView(depthViewDesc);
        if (!depthTextureView) {
            std::cerr << "Failed to create depth texture view" << std::endl;
            return false;
        }

        // Create MSAA color texture
        auto swapchainInfo = swapchain->getInfo();
        TextureDescriptor msaaColorTextureDesc{};
        msaaColorTextureDesc.label = "MSAA Color Buffer";
        msaaColorTextureDesc.type = TextureType::Texture2D;
        msaaColorTextureDesc.size = { static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1 };
        msaaColorTextureDesc.arrayLayerCount = 1;
        msaaColorTextureDesc.mipLevelCount = 1;
        msaaColorTextureDesc.sampleCount = MSAA_SAMPLE_COUNT;
        msaaColorTextureDesc.format = swapchainInfo.format;
        msaaColorTextureDesc.usage = TextureUsage::RenderAttachment;

        msaaColorTexture = device->createTexture(msaaColorTextureDesc);
        if (!msaaColorTexture) {
            std::cerr << "Failed to create MSAA color texture" << std::endl;
            return false;
        }

        // Create MSAA color texture view
        TextureViewDescriptor msaaColorViewDesc{};
        msaaColorViewDesc.label = "MSAA Color Buffer View";
        msaaColorViewDesc.viewType = TextureViewType::View2D;
        msaaColorViewDesc.format = swapchainInfo.format;
        msaaColorViewDesc.baseMipLevel = 0;
        msaaColorViewDesc.mipLevelCount = 1;
        msaaColorViewDesc.baseArrayLayer = 0;
        msaaColorViewDesc.arrayLayerCount = 1;

        msaaColorTextureView = msaaColorTexture->createView(msaaColorViewDesc);
        if (!msaaColorTextureView) {
            std::cerr << "Failed to create MSAA color texture view" << std::endl;
            return false;
        }

        // Create render pass
        RenderPassCreateDescriptor renderPassDesc{};
        renderPassDesc.label = "Main Render Pass";

        // Color attachment
        RenderPassColorAttachment colorAttachment{};
        RenderPassColorAttachmentTarget resolveTarget{}; // Declare outside to prevent dangling pointer
        
        colorAttachment.target.format = swapchainInfo.format;
        colorAttachment.target.sampleCount = MSAA_SAMPLE_COUNT;
        colorAttachment.target.loadOp = LoadOp::Clear;
        colorAttachment.target.storeOp = StoreOp::DontCare; // MSAA buffer doesn't need to be stored
        colorAttachment.target.finalLayout = TextureLayout::ColorAttachment;

        if (MSAA_SAMPLE_COUNT != SampleCount::Count1) {
            // MSAA: Add resolve target
            resolveTarget.format = swapchainInfo.format;
            resolveTarget.sampleCount = SampleCount::Count1;
            resolveTarget.loadOp = LoadOp::DontCare;
            resolveTarget.storeOp = StoreOp::Store;
            resolveTarget.finalLayout = TextureLayout::PresentSrc;
            colorAttachment.resolveTarget = &resolveTarget;
        } else {
            // No MSAA: Store directly
            colorAttachment.target.storeOp = StoreOp::Store;
            colorAttachment.target.finalLayout = TextureLayout::PresentSrc;
        }

        renderPassDesc.colorAttachments.push_back(colorAttachment);

        // Depth/stencil attachment
        RenderPassDepthStencilAttachment depthAttachment{};
        depthAttachment.target.format = DEPTH_FORMAT;
        depthAttachment.target.sampleCount = MSAA_SAMPLE_COUNT;
        depthAttachment.target.depthLoadOp = LoadOp::Clear;
        depthAttachment.target.depthStoreOp = StoreOp::DontCare;
        depthAttachment.target.stencilLoadOp = LoadOp::DontCare;
        depthAttachment.target.stencilStoreOp = StoreOp::DontCare;
        depthAttachment.target.finalLayout = TextureLayout::DepthStencilAttachment;

        renderPassDesc.depthStencilAttachment = &depthAttachment;

        renderPass = device->createRenderPass(renderPassDesc);
        if (!renderPass) {
            std::cerr << "Failed to create render pass" << std::endl;
            return false;
        }

        // Create framebuffers for each swapchain image
        framebuffers.resize(swapchainInfo.imageCount);

        for (uint32_t i = 0; i < swapchainInfo.imageCount; ++i) {
            FramebufferDescriptor framebufferDesc{};
            framebufferDesc.label = "Framebuffer " + std::to_string(i);
            framebufferDesc.renderPass = renderPass;
            framebufferDesc.width = width;
            framebufferDesc.height = height;

            // Color attachment
            if (MSAA_SAMPLE_COUNT != SampleCount::Count1) {
                // MSAA: Single attachment with MSAA buffer and resolve target
                framebufferDesc.colorAttachments.push_back({ msaaColorTextureView, swapchain->getTextureView(i) });
            } else {
                // No MSAA: Attach swapchain image directly
                framebufferDesc.colorAttachments.push_back({ swapchain->getTextureView(i) });
            }

            // Depth attachment (must be a pointer)
            FramebufferDepthStencilAttachment depthAttachment{ depthTextureView };
            framebufferDesc.depthStencilAttachment = &depthAttachment;

            framebuffers[i] = device->createFramebuffer(framebufferDesc);
            if (!framebuffers[i]) {
                std::cerr << "Failed to create framebuffer " << i << std::endl;
                return false;
            }
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
        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
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

            // Create command encoder for this frame
            commandEncoders[i] = device->createCommandEncoder({ "Command Encoder Frame " + std::to_string(i) });
            if (!commandEncoders[i]) {
                std::cerr << "Failed to create command encoder " << i << std::endl;
                return false;
            }
        }

        return true;
    } catch (const std::exception& e) {
        std::cerr << "Failed to create sync objects: " << e.what() << std::endl;
        return false;
    }
}

void CubeApp::cleanupSizeDependentResources()
{
    // Clean up framebuffers and render pass
    framebuffers.clear();
    renderPass.reset();
    
    // Clean up size-dependent resources
    msaaColorTextureView.reset();
    msaaColorTexture.reset();
    depthTextureView.reset();
    depthTexture.reset();

    // Also destroy swapchain to fully recreate it
    swapchain.reset();
}

void CubeApp::cleanupRenderingResources()
{
    // Clean up size-independent rendering resources
    renderPipeline.reset();
    fragmentShader.reset();
    vertexShader.reset();
    uniformBindGroupLayout.reset();
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        for (size_t cubeIdx = 0; cubeIdx < CUBE_COUNT; ++cubeIdx) {
            uniformBindGroups[i][cubeIdx].reset();
        }
    }
    sharedUniformBuffer.reset();
    indexBuffer.reset();
    vertexBuffer.reset();
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

        indexBuffer = device->createBuffer(indexBufferDesc);
        if (!indexBuffer) {
            std::cerr << "Failed to create index buffer" << std::endl;
            return false;
        }

        // Upload vertex and index data
        queue->writeBuffer(vertexBuffer, 0, vertices.data(), sizeof(vertices));
        queue->writeBuffer(indexBuffer, 0, indices.data(), sizeof(indices));

        // Create single large uniform buffer for all frames and cubes with proper alignment
        auto limits = device->getLimits();

        size_t uniformSize = sizeof(UniformData);
        uniformAlignedSize = utils::alignUp(uniformSize, limits.minUniformBufferOffsetAlignment);
        size_t totalBufferSize = uniformAlignedSize * MAX_FRAMES_IN_FLIGHT * CUBE_COUNT;

        BufferDescriptor uniformBufferDesc{};
        uniformBufferDesc.label = "Shared Transform Uniforms";
        uniformBufferDesc.size = totalBufferSize;
        uniformBufferDesc.usage = BufferUsage::Uniform | BufferUsage::CopyDst;

        sharedUniformBuffer = device->createBuffer(uniformBufferDesc);
        if (!sharedUniformBuffer) {
            std::cerr << "Failed to create shared uniform buffer" << std::endl;
            return false;
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

        // Create bind groups (one per frame per cube) using offsets into shared buffer
        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
            for (size_t cubeIdx = 0; cubeIdx < CUBE_COUNT; ++cubeIdx) {
                BindGroupEntry uniformEntry{};
                uniformEntry.binding = 0;
                uniformEntry.resource = sharedUniformBuffer;
                uniformEntry.offset = (i * CUBE_COUNT + cubeIdx) * uniformAlignedSize;
                uniformEntry.size = sizeof(UniformData);

                BindGroupDescriptor uniformBindGroupDesc{};
                uniformBindGroupDesc.label = "Uniform Bind Group Frame " + std::to_string(i) + " Cube " + std::to_string(cubeIdx);
                uniformBindGroupDesc.layout = uniformBindGroupLayout;
                uniformBindGroupDesc.entries = { uniformEntry };

                uniformBindGroups[i][cubeIdx] = device->createBindGroup(uniformBindGroupDesc);
                if (!uniformBindGroups[i][cubeIdx]) {
                    std::cerr << "Failed to create uniform bind group " << i << " cube " << cubeIdx << std::endl;
                    return false;
                }
            }
        }

        // Load shaders (WGSL for WebGPU, SPIR-V for Vulkan)
        ShaderSourceType shaderSourceType;
        std::string vertexShaderCode;
        std::string fragmentShaderCode;

        if (adapterInfo.backend == Backend::WebGPU) {
            shaderSourceType = ShaderSourceType::WGSL;
            // Load WGSL shaders for WebGPU
            vertexShaderCode = loadTextFile("shaders/cube.vert.wgsl");
            fragmentShaderCode = loadTextFile("shaders/cube.frag.wgsl");
            if (vertexShaderCode.empty() || fragmentShaderCode.empty()) {
                std::cerr << "Failed to load WGSL shader files" << std::endl;
                return false;
            }
        } else {
            shaderSourceType = ShaderSourceType::SPIRV;
            // Load SPIR-V shaders for Vulkan
            auto vertexSpirv = loadBinaryFile("cube.vert.spv");
            auto fragmentSpirv = loadBinaryFile("cube.frag.spv");
            if (vertexSpirv.empty() || fragmentSpirv.empty()) {
                std::cerr << "Failed to load SPIR-V shader files" << std::endl;
                return false;
            }
            vertexShaderCode = std::string(reinterpret_cast<const char*>(vertexSpirv.data()), vertexSpirv.size());
            fragmentShaderCode = std::string(reinterpret_cast<const char*>(fragmentSpirv.data()), fragmentSpirv.size());
        }

        // Create vertex shader
        ShaderDescriptor vertexShaderDesc{};
        vertexShaderDesc.label = "Cube Vertex Shader";
        vertexShaderDesc.sourceType = shaderSourceType;
        vertexShaderDesc.code = vertexShaderCode;
        vertexShaderDesc.entryPoint = "main";

        vertexShader = device->createShader(vertexShaderDesc);
        if (!vertexShader) {
            std::cerr << "Failed to create vertex shader" << std::endl;
            return false;
        }

        // Create fragment shader
        ShaderDescriptor fragmentShaderDesc{};
        fragmentShaderDesc.label = "Cube Fragment Shader";
        fragmentShaderDesc.sourceType = shaderSourceType;
        fragmentShaderDesc.code = fragmentShaderCode;
        fragmentShaderDesc.entryPoint = "main";

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

        auto swapchainInfo = swapchain->getInfo();
        ColorTargetState colorTarget{};
        colorTarget.format = swapchainInfo.format;
        colorTarget.writeMask = ColorWriteMask::All;

        FragmentState fragmentState{};
        fragmentState.module = fragmentShader;
        fragmentState.entryPoint = "main";
        fragmentState.targets = { colorTarget };

        PrimitiveState primitiveState{};
        primitiveState.topology = PrimitiveTopology::TriangleList;
        primitiveState.frontFace = FrontFace::CounterClockwise;
        primitiveState.cullMode = CullMode::Back; // Enable back-face culling for 3D
        primitiveState.polygonMode = PolygonMode::Fill;

        // Depth/stencil state - enable depth testing
        DepthStencilState depthStencilState{};
        depthStencilState.format = TextureFormat::Depth32Float;
        depthStencilState.depthWriteEnabled = true;
        depthStencilState.depthCompare = CompareFunction::Less;

        RenderPipelineDescriptor pipelineDesc{};
        pipelineDesc.label = "Cube Pipeline";
        pipelineDesc.vertex = vertexState;
        pipelineDesc.fragment = fragmentState;
        pipelineDesc.primitive = primitiveState;
        pipelineDesc.depthStencil = depthStencilState;
        pipelineDesc.sampleCount = MSAA_SAMPLE_COUNT;
        pipelineDesc.bindGroupLayouts = { uniformBindGroupLayout }; // Pass the bind group layout
        pipelineDesc.renderPass = renderPass;

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

void CubeApp::updateCube(int cubeIndex)
{
    UniformData uniforms{};

    // Create rotation matrices (combine X and Y rotations)
    // Each cube rotates slightly differently
    std::array<std::array<float, 4>, 4> rotX, rotY, tempModel, translation;
    matrixRotateX(rotX, (rotationAngleX + cubeIndex * 30.0f) * M_PI / 180.0f);
    matrixRotateY(rotY, (rotationAngleY + cubeIndex * 45.0f) * M_PI / 180.0f);
    matrixMultiply(tempModel, rotY, rotX);

    // Position cubes side by side: left (-3, 0, 0), center (0, 0, 0), right (3, 0, 0)
    matrixIdentity(translation);
    translation[3][0] = (cubeIndex - 1) * 3.0f; // x offset: -3, 0, 3

    // Apply translation after rotation
    matrixMultiply(uniforms.model, tempModel, translation);

    // Create view matrix (camera positioned at 0, 0, 10 looking at origin)
    matrixLookAt(uniforms.view,
        0.0f, 0.0f, 10.0f, // eye position - pulled back to see all 3 cubes
        0.0f, 0.0f, 0.0f, // look at point
        0.0f, 1.0f, 0.0f); // up vector

    // Create projection matrix
    auto swapchainInfo = swapchain->getInfo();
    float aspect = (float)swapchainInfo.width / (float)swapchainInfo.height;
    matrixPerspective(uniforms.projection, 45.0f * M_PI / 180.0f, aspect, 0.1f, 100.0f, adapterInfo.backend);

    // Upload uniform data to buffer at aligned offset
    // Formula: (frame * CUBE_COUNT + cube) * alignedSize
    size_t offset = (currentFrame * CUBE_COUNT + cubeIndex) * uniformAlignedSize;
    queue->writeBuffer(sharedUniformBuffer, offset, &uniforms, sizeof(uniforms));
}

void CubeApp::update(float deltaTime)
{
    // Update rotation angles (both X and Y axes)
    rotationAngleX += deltaTime * 45.0f; // 45 degrees per second around X
    rotationAngleY += deltaTime * 30.0f; // 30 degrees per second around Y
    if (rotationAngleX >= 360.0f) {
        rotationAngleX -= 360.0f;
    }
    if (rotationAngleY >= 360.0f) {
        rotationAngleY -= 360.0f;
    }

    // Update uniforms for all CUBE_COUNT cubes BEFORE encoding
    for (int i = 0; i < CUBE_COUNT; ++i) {
        updateCube(i);
    }
}

void CubeApp::render()
{
    try {
        // Wait for this frame's fence to be signaled
        inFlightFences[currentFrame]->wait(UINT64_MAX);
        inFlightFences[currentFrame]->reset();

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

        // Begin command encoder for reuse
        auto commandEncoder = commandEncoders[currentFrame];
        commandEncoder->begin();

        // Begin render pass with the new API
        Color clearColor{ 0.1f, 0.2f, 0.3f, 1.0f }; // Dark blue background

        RenderPassBeginDescriptor renderPassBeginDesc{};
        renderPassBeginDesc.framebuffer = framebuffers[imageIndex];
        renderPassBeginDesc.colorClearValues = { clearColor };
        renderPassBeginDesc.depthClearValue = 1.0f;
        renderPassBeginDesc.stencilClearValue = 0;

        {
            auto renderPassEncoder = commandEncoder->beginRenderPass(renderPassBeginDesc);

            // Set pipeline, bind groups, and buffers (using current frame's bind group)
            renderPassEncoder->setPipeline(renderPipeline);

            // Set viewport and scissor to fill the entire render target
            auto swapchainInfo = swapchain->getInfo();
            renderPassEncoder->setViewport(0.0f, 0.0f, static_cast<float>(swapchainInfo.width), static_cast<float>(swapchainInfo.height), 0.0f, 1.0f);
            renderPassEncoder->setScissorRect(0, 0, swapchainInfo.width, swapchainInfo.height);

            renderPassEncoder->setVertexBuffer(0, vertexBuffer);
            renderPassEncoder->setIndexBuffer(indexBuffer, IndexFormat::Uint16);

            // Draw CUBE_COUNT cubes at different positions
            for (int i = 0; i < CUBE_COUNT; ++i) {
                // Bind the specific cube's bind group (no dynamic offsets)
                renderPassEncoder->setBindGroup(0, uniformBindGroups[currentFrame][i]);

                // Draw indexed (36 indices for the cube)
                renderPassEncoder->drawIndexed(36, 1, 0, 0, 0);
            }
        } // renderPassEncoder destroyed here, ending the render pass

        // Finish command encoding
        commandEncoder->end();

        // Submit with explicit synchronization
        SubmitInfo submitInfo{};
        submitInfo.commandEncoders = { commandEncoder };
        submitInfo.waitSemaphores = { imageAvailableSemaphores[currentFrame] };
        submitInfo.signalSemaphores = { renderFinishedSemaphores[currentFrame] };
        submitInfo.signalFence = inFlightFences[currentFrame];

        queue->submit(submitInfo);

        // Present with explicit synchronization
        PresentInfo presentInfo{};
        presentInfo.waitSemaphores = { renderFinishedSemaphores[currentFrame] };

        result = swapchain->present(presentInfo);
        if (result != gfx::Result::Success) {
            std::cerr << "Failed to present" << std::endl;
        }

        // Advance to next frame
        currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
    } catch (const std::exception& e) {
        std::cerr << "Render error: " << e.what() << std::endl;
    }
}

PlatformWindowHandle CubeApp::extractNativeHandle()
{
    PlatformWindowHandle handle{};

#if defined(__EMSCRIPTEN__)
    handle = PlatformWindowHandle::fromEmscripten("#canvas");

#elif defined(_WIN32)
    // Windows: Get HWND and HINSTANCE
    handle = PlatformWindowHandle::fromWin32(glfwGetWin32Window(window), GetModuleHandle(nullptr));
    std::cout << "Extracted Win32 handle: HWND=" << handle.handle.win32.hwnd << ", HINSTANCE=" << handle.handle.win32.hinstance << std::endl;

#elif defined(__linux__)
    // Linux: Get X11 Window and Display (assuming X11, not Wayland)
    handle = PlatformWindowHandle::fromXlib(glfwGetX11Display(), glfwGetX11Window(window));
    std::cout << "Extracted X11 handle: Window=" << handle.handle.xlib.window << ", Display=" << handle.handle.xlib.display << std::endl;

#elif defined(__APPLE__)
    // macOS: Get NSWindow
    handle = PlatformWindowHandle::fromMetal(glfwGetMetalLayer(window));
    std::cout << "Extracted Metal handle: Layer=" << handle.handle.metal.layer << std::endl;
#endif

    return handle;
}

void CubeApp::framebufferResizeCallback(GLFWwindow* window, int width, int height)
{
    auto* app = static_cast<CubeApp*>(glfwGetWindowUserPointer(window));
    if (app) {
        app->windowWidth = static_cast<uint32_t>(width);
        app->windowHeight = static_cast<uint32_t>(height);
    }
}

void CubeApp::keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    (void)scancode;
    (void)mods; // Suppress unused parameter warnings

    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    }
}

float CubeApp::getCurrentTime()
{
#if defined(__EMSCRIPTEN__)
    return (float)emscripten_get_now() / 1000.0f;
#else
    return (float)glfwGetTime();
#endif
}

bool CubeApp::mainLoopIteration()
{
    if (glfwWindowShouldClose(window)) {
        return false;
    }

    glfwPollEvents();

    // Handle framebuffer resize
    if (previousWidth != windowWidth || previousHeight != windowHeight) {
        // Wait for all in-flight frames to complete
        device->waitIdle();

        // Recreate only size-dependent resources (including swapchain)
        cleanupSizeDependentResources();
        if (!createSizeDependentResources(windowWidth, windowHeight)) {
            std::cerr << "Failed to recreate size-dependent resources after resize" << std::endl;
            return false;
        }

        previousWidth = windowWidth;
        previousHeight = windowHeight;
        auto swapchainInfo = swapchain->getInfo();
        std::cout << "Window resized: " << swapchainInfo.width << "x" << swapchainInfo.height << std::endl;
        return true; // Skip rendering this frame
    }

    // Calculate delta time
    float currentTime = getCurrentTime();
    float deltaTime = currentTime - lastTime;
    lastTime = currentTime;

    update(deltaTime);
    render();

    return true;
}

#if defined(__EMSCRIPTEN__)
void CubeApp::emscriptenMainLoop(void* userData)
{
    CubeApp* app = (CubeApp*)userData;
    if (!app->mainLoopIteration()) {
        emscripten_cancel_main_loop();
        app->cleanup();
    }
}
#endif

void CubeApp::run()
{
    // Run main loop (platform-specific)
#if defined(__EMSCRIPTEN__)
    // Note: emscripten_set_main_loop_arg returns immediately and never blocks
    // Cleanup happens in emscriptenMainLoop when the loop exits
    // Execution continues in the browser event loop
    emscripten_set_main_loop_arg(CubeApp::emscriptenMainLoop, this, 0, 1);
#else
    while (mainLoopIteration()) {
        // Loop continues until mainLoopIteration returns false
    }
#endif
}

void CubeApp::cleanup()
{
    // Wait for device to finish
    if (device) {
        device->waitIdle();
    }

    // Clean up size-dependent resources
    cleanupSizeDependentResources();

    // Clean up rendering resources
    cleanupRenderingResources();

    // Clean up per-frame resources
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        commandEncoders[i].reset();
        inFlightFences[i].reset();
        renderFinishedSemaphores[i].reset();
        imageAvailableSemaphores[i].reset();
        for (size_t cubeIdx = 0; cubeIdx < CUBE_COUNT; ++cubeIdx) {
            uniformBindGroups[i][cubeIdx].reset();
        }
    }

    // Destroy shared uniform buffer
    sharedUniformBuffer.reset();

    // C++ destructors will handle cleanup automatically
    // But we can explicitly reset shared_ptrs if needed
    renderPipeline.reset();
    fragmentShader.reset();
    vertexShader.reset();
    uniformBindGroupLayout.reset();
    indexBuffer.reset();
    vertexBuffer.reset();
    framebuffers.clear();
    renderPass.reset();
    msaaColorTextureView.reset();
    msaaColorTexture.reset();
    depthTextureView.reset();
    depthTexture.reset();
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

void CubeApp::matrixPerspective(std::array<std::array<float, 4>, 4>& matrix, float fovy, float aspect, float nearPlane, float farPlane, Backend backend)
{
    matrix = { { { { 0.0f, 0.0f, 0.0f, 0.0f } },
        { { 0.0f, 0.0f, 0.0f, 0.0f } },
        { { 0.0f, 0.0f, 0.0f, 0.0f } },
        { { 0.0f, 0.0f, 0.0f, 0.0f } } } };

    float f = 1.0f / std::tan(fovy / 2.0f);
    matrix[0][0] = f / aspect;
    if (backend == Backend::Vulkan) {
        matrix[1][1] = -f;
    } else {
        matrix[1][1] = f;
    }
    matrix[2][2] = (farPlane + nearPlane) / (nearPlane - farPlane);
    matrix[2][3] = -1.0f;
    matrix[3][2] = (2.0f * farPlane * nearPlane) / (nearPlane - farPlane);
}

void CubeApp::matrixLookAt(std::array<std::array<float, 4>, 4>& matrix,
    float eyeX, float eyeY, float eyeZ,
    float centerX, float centerY, float centerZ,
    float upX, float upY, float upZ)
{
    // Calculate forward vector
    float fx = centerX - eyeX;
    float fy = centerY - eyeY;
    float fz = centerZ - eyeZ;

    // Check for zero-length forward vector
    if (!vectorNormalize(fx, fy, fz)) {
        matrixIdentity(matrix);
        return;
    }

    // Calculate right vector (cross product of forward and up)
    float rx = fy * upZ - fz * upY;
    float ry = fz * upX - fx * upZ;
    float rz = fx * upY - fy * upX;

    // Check for zero-length right vector (forward and up are parallel)
    if (!vectorNormalize(rx, ry, rz)) {
        matrixIdentity(matrix);
        return;
    }

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
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            temp[i][j] = 0.0f;
            for (int k = 0; k < 4; k++) {
                temp[i][j] += a[i][k] * b[k][j];
            }
        }
    }
    result = temp;
}

bool CubeApp::vectorNormalize(float& x, float& y, float& z)
{
    const float epsilon = 1e-6f;
    float len = sqrtf(x * x + y * y + z * z);

    if (len < epsilon) {
        return false;
    }

    x /= len;
    y /= len;
    z /= len;
    return true;
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

std::string CubeApp::loadTextFile(const char* filepath)
{
    std::FILE* file = std::fopen(filepath, "r");
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

    // Read file into string
    std::string buffer(fileSize, '\0');
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