#include <gfx_cpp/gfx.hpp>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#if defined(__EMSCRIPTEN__)
#include <emscripten/emscripten.h>
#else
#if defined(_WIN32)
#define GLFW_EXPOSE_NATIVE_WIN32
#elif defined(__linux__)
#define GLFW_EXPOSE_NATIVE_X11
#define GLFW_EXPOSE_NATIVE_WAYLAND
#elif defined(__APPLE__)
#define GLFW_EXPOSE_NATIVE_COCOA
#endif
#include <GLFW/glfw3native.h>
#endif

#ifdef Success
#undef Success
#endif

#include <array>
#include <cfloat>
#include <cmath>
#include <cstring>
#include <iostream>
#include <memory>
#include <vector>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

static constexpr uint32_t WINDOW_WIDTH = 800;
static constexpr uint32_t WINDOW_HEIGHT = 600;
// Frame count is dynamic based on surface capabilities
static constexpr size_t CUBE_COUNT = 3;
static constexpr gfx::SampleCount MSAA_SAMPLE_COUNT = gfx::SampleCount::Count4;
static constexpr gfx::TextureFormat COLOR_FORMAT = gfx::TextureFormat::B8G8R8A8UnormSrgb;
static constexpr gfx::TextureFormat DEPTH_FORMAT = gfx::TextureFormat::Depth32Float;

#if defined(__EMSCRIPTEN__)
static constexpr gfx::Backend BACKEND_API = gfx::Backend::WebGPU;
#else
// here we can choose between VULKAN, WEBGPU
static constexpr gfx::Backend BACKEND_API = gfx::Backend::WebGPU;
#endif

// Log callback function
static void logCallback(gfx::LogLevel level, const std::string& message)
{
    const char* levelStr = "UNKNOWN";
    switch (level) {
    case gfx::LogLevel::Error:
        levelStr = "ERROR";
        break;
    case gfx::LogLevel::Warning:
        levelStr = "WARNING";
        break;
    case gfx::LogLevel::Info:
        levelStr = "INFO";
        break;
    case gfx::LogLevel::Debug:
        levelStr = "DEBUG";
        break;
    default:
        levelStr = "UNKNOWN";
        break;
    }
    std::cout << "[" << levelStr << "] " << message << std::endl;
}

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
    gfx::PlatformWindowHandle extractNativeHandle();
    std::vector<uint8_t> loadBinaryFile(const char* filepath);
    std::string loadTextFile(const char* filepath);

    // Matrix math utilities
    void matrixIdentity(std::array<std::array<float, 4>, 4>& matrix);
    void matrixPerspective(std::array<std::array<float, 4>, 4>& matrix, float fovy, float aspect, float nearPlane, float farPlane, gfx::Backend backend);
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
    std::shared_ptr<gfx::Instance> instance;
    std::shared_ptr<gfx::Adapter> adapter;
    gfx::AdapterInfo adapterInfo; // Cached adapter info
    std::shared_ptr<gfx::Device> device;
    std::shared_ptr<gfx::Queue> queue;
    std::shared_ptr<gfx::Surface> surface;
    std::shared_ptr<gfx::Swapchain> swapchain;

    std::shared_ptr<gfx::Buffer> vertexBuffer;
    std::shared_ptr<gfx::Buffer> indexBuffer;
    std::shared_ptr<gfx::Shader> vertexShader;
    std::shared_ptr<gfx::Shader> fragmentShader;
    std::shared_ptr<gfx::RenderPipeline> renderPipeline;
    std::shared_ptr<gfx::BindGroupLayout> uniformBindGroupLayout;

    // Depth buffer
    std::shared_ptr<gfx::Texture> depthTexture;
    std::shared_ptr<gfx::TextureView> depthTextureView;

    // MSAA color buffer
    std::shared_ptr<gfx::Texture> msaaColorTexture;
    std::shared_ptr<gfx::TextureView> msaaColorTextureView;

    // Render pass and framebuffers
    std::shared_ptr<gfx::RenderPass> renderPass;
    std::vector<std::shared_ptr<gfx::Framebuffer>> framebuffers;

    uint32_t windowWidth = WINDOW_WIDTH;
    uint32_t windowHeight = WINDOW_HEIGHT;
    uint32_t previousWidth = WINDOW_WIDTH;
    uint32_t previousHeight = WINDOW_HEIGHT;

    // Per-frame resources (for frames in flight)
    size_t framesInFlightCount = 0; // Dynamic based on surface capabilities
    std::shared_ptr<gfx::Buffer> sharedUniformBuffer; // Single buffer for all frames and cubes
    size_t uniformAlignedSize = 0; // Aligned size per uniform buffer
    std::vector<std::vector<std::shared_ptr<gfx::BindGroup>>> uniformBindGroups; // Dynamic: [framesInFlightCount][CUBE_COUNT]
    std::vector<std::shared_ptr<gfx::CommandEncoder>> commandEncoders; // Dynamic: [framesInFlightCount]

    // Per-frame synchronization
    std::vector<std::shared_ptr<gfx::Semaphore>> imageAvailableSemaphores; // Dynamic: [framesInFlightCount]
    std::vector<std::shared_ptr<gfx::Semaphore>> renderFinishedSemaphores; // Dynamic: [framesInFlightCount]
    std::vector<std::shared_ptr<gfx::Fence>> inFlightFences; // Dynamic: [framesInFlightCount]
    size_t currentFrame = 0;

    // Animation state
    float rotationAngleX = 0.0f;
    float rotationAngleY = 0.0f;
    float lastTime = 0.0f;

    // FPS tracking
    uint32_t fpsFrameCount = 0;
    float fpsTimeAccumulator = 0.0f;
    float fpsFrameTimeMin = FLT_MAX;
    float fpsFrameTimeMax = 0.0f;
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

    // Initialize timing
    lastTime = getCurrentTime();

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
    // Set up logging callback
    gfx::setLogCallback(logCallback);

    auto result = gfx::loadBackend(BACKEND_API);
    if (!gfx::isSuccess(result)) {
        std::cerr << "Failed to load graphics backend: " << static_cast<int32_t>(result) << std::endl;
        return false;
    }

    try {
        gfx::InstanceDescriptor instanceDesc{};
        instanceDesc.applicationName = "Rotating Cube Example (C++)";
        instanceDesc.applicationVersion = 1;
        instanceDesc.backend = BACKEND_API;
        instanceDesc.enabledExtensions = { gfx::INSTANCE_EXTENSION_SURFACE, gfx::INSTANCE_EXTENSION_DEBUG };

        instance = gfx::createInstance(instanceDesc);
        if (!instance) {
            std::cerr << "Failed to create graphics instance" << std::endl;
            return false;
        }

        // Get adapter
        gfx::AdapterDescriptor adapterDesc{};
        adapterDesc.preference = gfx::AdapterPreference::HighPerformance;

        adapter = instance->requestAdapter(adapterDesc);
        if (!adapter) {
            std::cerr << "Failed to get graphics adapter" << std::endl;
            return false;
        }

        // Query and store adapter info
        adapterInfo = adapter->getInfo();
        std::cout << "Using adapter: " << adapterInfo.name << std::endl;
        std::cout << "Backend: " << (adapterInfo.backend == gfx::Backend::Vulkan ? "Vulkan" : "WebGPU") << std::endl;
        std::cout << "  Vendor ID: 0x" << std::hex << adapterInfo.vendorID << std::dec
                  << ", Device ID: 0x" << std::hex << adapterInfo.deviceID << std::dec << std::endl;

        // Create device
        gfx::DeviceDescriptor deviceDesc{};
        deviceDesc.label = "Main Device";
        deviceDesc.enabledExtensions = { gfx::DEVICE_EXTENSION_SWAPCHAIN };

        device = adapter->createDevice(deviceDesc);
        if (!device) {
            std::cerr << "Failed to create device" << std::endl;
            return false;
        }

        queue = device->getQueue();

        // Create surface using native platform handles extracted from GLFW
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);

        gfx::SurfaceDescriptor surfaceDesc{};
        surfaceDesc.label = "Main Surface";
        surfaceDesc.windowHandle = extractNativeHandle();

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
        // Query surface capabilities to determine frame count
        auto surfaceInfo = surface->getInfo();
        std::cout << "Surface Info:" << std::endl;
        std::cout << "  Image Count: min " << surfaceInfo.minImageCount << ", max " << surfaceInfo.maxImageCount << std::endl;
        std::cout << "  Extent: min (" << surfaceInfo.minExtent.width << ", " << surfaceInfo.minExtent.height << "), "
                  << "max (" << surfaceInfo.maxExtent.width << ", " << surfaceInfo.maxExtent.height << ")" << std::endl;

        // Calculate frames in flight based on surface capabilities
        // Use min image count, but clamp to reasonable values (2-4 is typical)
        framesInFlightCount = surfaceInfo.minImageCount;
        if (framesInFlightCount < 2) {
            framesInFlightCount = 2;
        }
        if (framesInFlightCount > 4) {
            framesInFlightCount = 4;
        }
        std::cout << "Frames in flight: " << framesInFlightCount << std::endl;

        // Create swapchain
        gfx::SwapchainDescriptor swapchainDesc{};
        swapchainDesc.label = "Main Swapchain";
        swapchainDesc.surface = surface;
        swapchainDesc.extent.width = width;
        swapchainDesc.extent.height = height;
        swapchainDesc.format = COLOR_FORMAT;
        swapchainDesc.usage = gfx::TextureUsage::RenderAttachment;
        swapchainDesc.presentMode = gfx::PresentMode::Fifo;
        swapchainDesc.imageCount = framesInFlightCount;

        swapchain = device->createSwapchain(swapchainDesc);
        if (!swapchain) {
            std::cerr << "Failed to create swapchain" << std::endl;
            return false;
        }

        // Get actual swapchain dimensions (may differ from requested)
        auto swapchainInfo = swapchain->getInfo();
        uint32_t actualWidth = swapchainInfo.extent.width;
        uint32_t actualHeight = swapchainInfo.extent.height;

        // Create depth texture with MSAA using actual swapchain dimensions
        gfx::TextureDescriptor depthTextureDesc{};
        depthTextureDesc.label = "Depth Buffer";
        depthTextureDesc.type = gfx::TextureType::Texture2D;
        depthTextureDesc.size = { actualWidth, actualHeight, 1 };
        depthTextureDesc.arrayLayerCount = 1;
        depthTextureDesc.mipLevelCount = 1;
        depthTextureDesc.sampleCount = MSAA_SAMPLE_COUNT;
        depthTextureDesc.format = DEPTH_FORMAT;
        depthTextureDesc.usage = gfx::TextureUsage::RenderAttachment;

        depthTexture = device->createTexture(depthTextureDesc);
        if (!depthTexture) {
            std::cerr << "Failed to create depth texture" << std::endl;
            return false;
        }

        // Create depth texture view
        gfx::TextureViewDescriptor depthViewDesc{};
        depthViewDesc.label = "Depth Buffer View";
        depthViewDesc.viewType = gfx::TextureViewType::View2D;
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

        // Create MSAA color texture using actual swapchain dimensions
        gfx::TextureDescriptor msaaColorTextureDesc{};
        msaaColorTextureDesc.label = "MSAA Color Buffer";
        msaaColorTextureDesc.type = gfx::TextureType::Texture2D;
        msaaColorTextureDesc.size = { actualWidth, actualHeight, 1 };
        msaaColorTextureDesc.arrayLayerCount = 1;
        msaaColorTextureDesc.mipLevelCount = 1;
        msaaColorTextureDesc.sampleCount = MSAA_SAMPLE_COUNT;
        msaaColorTextureDesc.format = swapchainInfo.format;
        msaaColorTextureDesc.usage = gfx::TextureUsage::RenderAttachment;

        msaaColorTexture = device->createTexture(msaaColorTextureDesc);
        if (!msaaColorTexture) {
            std::cerr << "Failed to create MSAA color texture" << std::endl;
            return false;
        }

        // Create MSAA color texture view
        gfx::TextureViewDescriptor msaaColorViewDesc{};
        msaaColorViewDesc.label = "MSAA Color Buffer View";
        msaaColorViewDesc.viewType = gfx::TextureViewType::View2D;
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
        gfx::RenderPassCreateDescriptor renderPassDesc{};
        renderPassDesc.label = "Main Render Pass";

        // Color attachment
        gfx::RenderPassColorAttachment colorAttachment{};
        gfx::RenderPassColorAttachmentTarget resolveTarget{}; // Declare outside to prevent dangling pointer

        colorAttachment.target.format = swapchainInfo.format;
        colorAttachment.target.sampleCount = MSAA_SAMPLE_COUNT;
        colorAttachment.target.ops.load = gfx::LoadOp::Clear;
        colorAttachment.target.ops.store = gfx::StoreOp::DontCare; // MSAA buffer doesn't need to be stored
        colorAttachment.target.finalLayout = gfx::TextureLayout::ColorAttachment;

        if (MSAA_SAMPLE_COUNT != gfx::SampleCount::Count1) {
            // MSAA: Add resolve target
            resolveTarget.format = swapchainInfo.format;
            resolveTarget.sampleCount = gfx::SampleCount::Count1;
            resolveTarget.ops.load = gfx::LoadOp::DontCare;
            resolveTarget.ops.store = gfx::StoreOp::Store;
            resolveTarget.finalLayout = gfx::TextureLayout::PresentSrc;
            colorAttachment.resolveTarget = resolveTarget;
        } else {
            // No MSAA: Store directly
            colorAttachment.target.ops.store = gfx::StoreOp::Store;
            colorAttachment.target.finalLayout = gfx::TextureLayout::PresentSrc;
        }

        renderPassDesc.colorAttachments.push_back(colorAttachment);

        // Depth/stencil attachment
        gfx::RenderPassDepthStencilAttachment depthAttachment{};
        depthAttachment.target.format = DEPTH_FORMAT;
        depthAttachment.target.sampleCount = MSAA_SAMPLE_COUNT;
        depthAttachment.target.depthOps.load = gfx::LoadOp::Clear;
        depthAttachment.target.depthOps.store = gfx::StoreOp::DontCare;
        depthAttachment.target.stencilOps.load = gfx::LoadOp::DontCare;
        depthAttachment.target.stencilOps.store = gfx::StoreOp::DontCare;
        depthAttachment.target.finalLayout = gfx::TextureLayout::DepthStencilAttachment;

        renderPassDesc.depthStencilAttachment = depthAttachment;

        renderPass = device->createRenderPass(renderPassDesc);
        if (!renderPass) {
            std::cerr << "Failed to create render pass" << std::endl;
            return false;
        }

        // Create framebuffers for each swapchain image
        framebuffers.resize(swapchainInfo.imageCount);

        for (uint32_t i = 0; i < swapchainInfo.imageCount; ++i) {
            gfx::FramebufferDescriptor framebufferDesc{};
            framebufferDesc.label = "Framebuffer " + std::to_string(i);
            framebufferDesc.renderPass = renderPass;
            framebufferDesc.extent.width = actualWidth;
            framebufferDesc.extent.height = actualHeight;

            // Color attachment
            if (MSAA_SAMPLE_COUNT != gfx::SampleCount::Count1) {
                // MSAA: Single attachment with MSAA buffer and resolve target
                framebufferDesc.colorAttachments.push_back({ msaaColorTextureView, swapchain->getTextureView(i) });
            } else {
                // No MSAA: Attach swapchain image directly
                framebufferDesc.colorAttachments.push_back({ swapchain->getTextureView(i) });
            }

            // Depth attachment (must be a pointer)
            gfx::FramebufferDepthStencilAttachment depthAttachment{ depthTextureView };
            framebufferDesc.depthStencilAttachment = depthAttachment;

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
        // Resize vectors for dynamic frame count
        imageAvailableSemaphores.resize(framesInFlightCount);
        renderFinishedSemaphores.resize(framesInFlightCount);
        inFlightFences.resize(framesInFlightCount);
        commandEncoders.resize(framesInFlightCount);

        // Create synchronization objects for each frame in flight
        for (size_t i = 0; i < framesInFlightCount; ++i) {
            // Create binary semaphores for image availability and render completion
            gfx::SemaphoreDescriptor semDesc{};
            semDesc.label = "Image Available Semaphore Frame " + std::to_string(i);
            semDesc.type = gfx::SemaphoreType::Binary;

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
            gfx::FenceDescriptor fenceDesc{};
            fenceDesc.label = "In Flight Fence Frame " + std::to_string(i);
            fenceDesc.signaled = true;

            inFlightFences[i] = device->createFence(fenceDesc);
            if (!inFlightFences[i]) {
                std::cerr << "Failed to create in flight fence " << i << std::endl;
                return false;
            }

            // Create command encoder for this frame
            commandEncoders[i] = device->createCommandEncoder({ .label = "Command Encoder Frame " + std::to_string(i) });
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
    for (size_t i = 0; i < framesInFlightCount; ++i) {
        for (size_t cubeIdx = 0; cubeIdx < CUBE_COUNT; ++cubeIdx) {
            uniformBindGroups[i][cubeIdx].reset();
        }
    }
    uniformBindGroups.clear();
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
        gfx::BufferDescriptor vertexBufferDesc{};
        vertexBufferDesc.label = "Cube Vertices";
        vertexBufferDesc.size = sizeof(vertices);
        vertexBufferDesc.usage = gfx::BufferUsage::Vertex | gfx::BufferUsage::CopyDst;

        vertexBuffer = device->createBuffer(vertexBufferDesc);
        if (!vertexBuffer) {
            std::cerr << "Failed to create vertex buffer" << std::endl;
            return false;
        }

        // Create index buffer
        gfx::BufferDescriptor indexBufferDesc{};
        indexBufferDesc.label = "Cube Indices";
        indexBufferDesc.size = sizeof(indices);
        indexBufferDesc.usage = gfx::BufferUsage::Index | gfx::BufferUsage::CopyDst;

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
        uniformAlignedSize = gfx::utils::alignUp(uniformSize, limits.minUniformBufferOffsetAlignment);
        size_t totalBufferSize = uniformAlignedSize * framesInFlightCount * CUBE_COUNT;

        gfx::BufferDescriptor uniformBufferDesc{};
        uniformBufferDesc.label = "Shared Transform Uniforms";
        uniformBufferDesc.size = totalBufferSize;
        uniformBufferDesc.usage = gfx::BufferUsage::Uniform | gfx::BufferUsage::CopyDst;

        sharedUniformBuffer = device->createBuffer(uniformBufferDesc);
        if (!sharedUniformBuffer) {
            std::cerr << "Failed to create shared uniform buffer" << std::endl;
            return false;
        }

        // Create bind group layout for uniforms
        gfx::BindGroupLayoutEntry uniformLayoutEntry{
            .binding = 0,
            .visibility = gfx::ShaderStage::Vertex,
            .resource = gfx::BindGroupLayoutEntry::BufferBinding{
                .hasDynamicOffset = false,
                .minBindingSize = sizeof(UniformData) }
        };

        gfx::BindGroupLayoutDescriptor uniformLayoutDesc{};
        uniformLayoutDesc.label = "Uniform Bind Group Layout";
        uniformLayoutDesc.entries = { uniformLayoutEntry };

        uniformBindGroupLayout = device->createBindGroupLayout(uniformLayoutDesc);
        if (!uniformBindGroupLayout) {
            std::cerr << "Failed to create uniform bind group layout" << std::endl;
            return false;
        }

        // Resize uniformBindGroups vector
        uniformBindGroups.resize(framesInFlightCount);
        for (auto& frameBindGroups : uniformBindGroups) {
            frameBindGroups.resize(CUBE_COUNT);
        }

        // Create bind groups (one per frame per cube) using offsets into shared buffer
        for (size_t i = 0; i < framesInFlightCount; ++i) {
            for (size_t cubeIdx = 0; cubeIdx < CUBE_COUNT; ++cubeIdx) {
                gfx::BindGroupEntry uniformEntry{};
                uniformEntry.binding = 0;
                uniformEntry.resource = sharedUniformBuffer;
                uniformEntry.offset = (i * CUBE_COUNT + cubeIdx) * uniformAlignedSize;
                uniformEntry.size = sizeof(UniformData);

                gfx::BindGroupDescriptor uniformBindGroupDesc{};
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
        gfx::ShaderSourceType shaderSourceType;
        std::vector<uint8_t> vertexShaderCode;
        std::vector<uint8_t> fragmentShaderCode;

        // Query shader format support and use the first supported format
        // Try SPIR-V first (generally better performance)
        if (device->supportsShaderFormat(gfx::ShaderSourceType::SPIRV)) {
            shaderSourceType = gfx::ShaderSourceType::SPIRV;
            std::cout << "Loading SPIR-V shaders..." << std::endl;
            auto vertexSpirv = loadBinaryFile("shaders/cube.vert.spv");
            auto fragmentSpirv = loadBinaryFile("shaders/cube.frag.spv");
            if (vertexSpirv.empty() || fragmentSpirv.empty()) {
                std::cerr << "Failed to load SPIR-V shader files" << std::endl;
                return false;
            }
            vertexShaderCode = vertexSpirv;
            fragmentShaderCode = fragmentSpirv;
        }
        // Fall back to WGSL
        else if (device->supportsShaderFormat(gfx::ShaderSourceType::WGSL)) {
            shaderSourceType = gfx::ShaderSourceType::WGSL;
            std::cout << "Loading WGSL shaders..." << std::endl;
            auto vertexWgsl = loadTextFile("shaders/cube.vert.wgsl");
            auto fragmentWgsl = loadTextFile("shaders/cube.frag.wgsl");
            if (vertexWgsl.empty() || fragmentWgsl.empty()) {
                std::cerr << "Failed to load WGSL shader files" << std::endl;
                return false;
            }
            vertexShaderCode.assign(vertexWgsl.begin(), vertexWgsl.end());
            fragmentShaderCode.assign(fragmentWgsl.begin(), fragmentWgsl.end());
        } else {
            std::cerr << "Error: No supported shader format found (neither SPIR-V nor WGSL)" << std::endl;
            return false;
        }

        // Create vertex shader
        gfx::ShaderDescriptor vertexShaderDesc{};
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
        gfx::ShaderDescriptor fragmentShaderDesc{};
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
        std::vector<gfx::VertexAttribute> attributes = {
            { .format = gfx::TextureFormat::R32G32B32Float,
                .offset = offsetof(Vertex, position),
                .shaderLocation = 0 },
            { .format = gfx::TextureFormat::R32G32B32Float,
                .offset = offsetof(Vertex, color),
                .shaderLocation = 1 }
        };

        gfx::VertexBufferLayout vertexLayout{};
        vertexLayout.arrayStride = sizeof(Vertex);
        vertexLayout.attributes = attributes;
        vertexLayout.stepMode = gfx::VertexStepMode::Vertex;

        // Create render pipeline descriptor
        gfx::VertexState vertexState{};
        vertexState.module = vertexShader;
        vertexState.entryPoint = "main";
        vertexState.buffers = { vertexLayout };

        auto swapchainInfo = swapchain->getInfo();
        gfx::ColorTargetState colorTarget{};
        colorTarget.format = swapchainInfo.format;
        colorTarget.writeMask = gfx::ColorWriteMask::All;

        gfx::FragmentState fragmentState{};
        fragmentState.module = fragmentShader;
        fragmentState.entryPoint = "main";
        fragmentState.targets = { colorTarget };

        gfx::PrimitiveState primitiveState{};
        primitiveState.topology = gfx::PrimitiveTopology::TriangleList;
        primitiveState.frontFace = gfx::FrontFace::CounterClockwise;
        primitiveState.cullMode = gfx::CullMode::Back; // Enable back-face culling for 3D
        primitiveState.polygonMode = gfx::PolygonMode::Fill;

        // Depth/stencil state - enable depth testing
        gfx::DepthStencilState depthStencilState{};
        depthStencilState.format = gfx::TextureFormat::Depth32Float;
        depthStencilState.depthWriteEnabled = true;
        depthStencilState.depthCompare = gfx::CompareFunction::Less;

        gfx::RenderPipelineDescriptor pipelineDesc{};
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
    float aspect = (float)swapchainInfo.extent.width / (float)swapchainInfo.extent.height;
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
        auto waitResult = inFlightFences[currentFrame]->wait(gfx::TimeoutInfinite);
        if (!gfx::isSuccess(waitResult)) {
            throw std::runtime_error("Failed to wait for fence");
        }
        inFlightFences[currentFrame]->reset();

        // Acquire next image with explicit synchronization
        uint32_t imageIndex;
        auto result = swapchain->acquireNextImage(
            gfx::TimeoutInfinite,
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
        gfx::Color clearColor{ 0.1f, 0.2f, 0.3f, 1.0f }; // Dark blue background

        gfx::RenderPassBeginDescriptor renderPassBeginDesc{};
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
            renderPassEncoder->setViewport({ 0.0f, 0.0f, static_cast<float>(swapchainInfo.extent.width), static_cast<float>(swapchainInfo.extent.height), 0.0f, 1.0f });
            renderPassEncoder->setScissorRect({ 0, 0, swapchainInfo.extent.width, swapchainInfo.extent.height });

            renderPassEncoder->setVertexBuffer(0, vertexBuffer, 0, vertexBuffer->getInfo().size);
            renderPassEncoder->setIndexBuffer(indexBuffer, gfx::IndexFormat::Uint16, 0, indexBuffer->getInfo().size);

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
        gfx::SubmitDescriptor submitDescriptor{};
        submitDescriptor.commandEncoders = { commandEncoder };
        submitDescriptor.waitSemaphores = { imageAvailableSemaphores[currentFrame] };
        submitDescriptor.signalSemaphores = { renderFinishedSemaphores[currentFrame] };
        submitDescriptor.signalFence = inFlightFences[currentFrame];

        auto submitResult = queue->submit(submitDescriptor);
        if (!gfx::isSuccess(submitResult)) {
            throw std::runtime_error("Failed to submit command buffer");
        }

        // Present with explicit synchronization
        gfx::PresentDescriptor presentDescriptor{};
        presentDescriptor.waitSemaphores = { renderFinishedSemaphores[currentFrame] };

        result = swapchain->present(presentDescriptor);
        if (result != gfx::Result::Success) {
            std::cerr << "Failed to present" << std::endl;
        }

        // Advance to next frame
        currentFrame = (currentFrame + 1) % framesInFlightCount;
    } catch (const std::exception& e) {
        std::cerr << "Render error: " << e.what() << std::endl;
    }
}

gfx::PlatformWindowHandle CubeApp::extractNativeHandle()
{
    gfx::PlatformWindowHandle handle{};
#if defined(__EMSCRIPTEN__)
    handle = gfx::PlatformWindowHandle::fromEmscripten("#canvas");
#elif defined(_WIN32)
    // Windows: Get HWND and HINSTANCE
    handle = gfx::PlatformWindowHandle::fromWin32(GetModuleHandle(nullptr), glfwGetWin32Window(window));
    std::cout << "Extracted Win32 handle: HWND=" << handle.handle.win32.hwnd << ", HINSTANCE=" << handle.handle.win32.hinstance << std::endl;
#elif defined(__linux__)
    // handle = gfx::PlatformWindowHandle::fromXlib(glfwGetX11Display(), glfwGetX11Window(window));
    // std::cout << "Extracted X11 handle: Window=" << handle.handle.xlib.window << ", Display=" << handle.handle.xlib.display << std::endl;
    handle = gfx::PlatformWindowHandle::fromWayland(glfwGetWaylandDisplay(), glfwGetWaylandWindow(window));
    std::cout << "Extracted Wayland handle: Surface=" << handle.handle.wayland.surface << ", Display=" << handle.handle.wayland.display << std::endl;
#elif defined(__APPLE__)
    handle = gfx::PlatformWindowHandle::fromMetal(glfwGetCocoaWindow(window));
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
        std::cout << "Window resized: " << swapchainInfo.extent.width << "x" << swapchainInfo.extent.height << std::endl;
        return true; // Skip rendering this frame
    }

    // Calculate delta time
    float currentTime = getCurrentTime();
    float deltaTime = currentTime - lastTime;
    lastTime = currentTime;

    // Track FPS
    if (deltaTime > 0.0f) {
        fpsFrameCount++;
        fpsTimeAccumulator += deltaTime;

        if (deltaTime < fpsFrameTimeMin) {
            fpsFrameTimeMin = deltaTime;
        }
        if (deltaTime > fpsFrameTimeMax) {
            fpsFrameTimeMax = deltaTime;
        }

        // Log FPS every second
        if (fpsTimeAccumulator >= 1.0f) {
            float avgFPS = static_cast<float>(fpsFrameCount) / fpsTimeAccumulator;
            float avgFrameTime = (fpsTimeAccumulator / static_cast<float>(fpsFrameCount)) * 1000.0f;
            float minFPS = 1.0f / fpsFrameTimeMax;
            float maxFPS = 1.0f / fpsFrameTimeMin;
            std::cout << "FPS - Avg: " << avgFPS << ", Min: " << minFPS << ", Max: " << maxFPS
                      << " | Frame Time - Avg: " << avgFrameTime << " ms, Min: " << (fpsFrameTimeMin * 1000.0f)
                      << " ms, Max: " << (fpsFrameTimeMax * 1000.0f) << " ms" << std::endl;

            // Reset for next second
            fpsFrameCount = 0;
            fpsTimeAccumulator = 0.0f;
            fpsFrameTimeMin = FLT_MAX;
            fpsFrameTimeMax = 0.0f;
        }
    }

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

    // Clean up per-frame resources (safely check sizes)
    for (size_t i = 0; i < commandEncoders.size(); ++i) {
        commandEncoders[i].reset();
    }
    for (size_t i = 0; i < inFlightFences.size(); ++i) {
        inFlightFences[i].reset();
    }
    for (size_t i = 0; i < renderFinishedSemaphores.size(); ++i) {
        renderFinishedSemaphores[i].reset();
    }
    for (size_t i = 0; i < imageAvailableSemaphores.size(); ++i) {
        imageAvailableSemaphores[i].reset();
    }

    // Clean up bind groups (safely check both dimensions)
    for (size_t i = 0; i < uniformBindGroups.size(); ++i) {
        for (size_t cubeIdx = 0; cubeIdx < uniformBindGroups[i].size(); ++cubeIdx) {
            uniformBindGroups[i][cubeIdx].reset();
        }
    }

    // Clear vectors
    commandEncoders.clear();
    inFlightFences.clear();
    renderFinishedSemaphores.clear();
    imageAvailableSemaphores.clear();
    uniformBindGroups.clear();

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

    gfx::unloadBackend(BACKEND_API);
}

// Matrix math utility functions
void CubeApp::matrixIdentity(std::array<std::array<float, 4>, 4>& matrix)
{
    matrix = { { { { 1.0f, 0.0f, 0.0f, 0.0f } },
        { { 0.0f, 1.0f, 0.0f, 0.0f } },
        { { 0.0f, 0.0f, 1.0f, 0.0f } },
        { { 0.0f, 0.0f, 0.0f, 1.0f } } } };
}

void CubeApp::matrixPerspective(std::array<std::array<float, 4>, 4>& matrix, float fovy, float aspect, float nearPlane, float farPlane, gfx::Backend backend)
{
    matrix = { { { { 0.0f, 0.0f, 0.0f, 0.0f } },
        { { 0.0f, 0.0f, 0.0f, 0.0f } },
        { { 0.0f, 0.0f, 0.0f, 0.0f } },
        { { 0.0f, 0.0f, 0.0f, 0.0f } } } };

    float f = 1.0f / std::tan(fovy / 2.0f);
    matrix[0][0] = f / aspect;
    if (backend == gfx::Backend::Vulkan) {
        matrix[1][1] = -f;
    } else {
        matrix[1][1] = f;
    }
    matrix[2][2] = (farPlane + nearPlane) / (nearPlane - farPlane);
    matrix[2][3] = -1.0f;
    matrix[3][2] = (2.0f * farPlane * nearPlane) / (nearPlane - farPlane);
}

void CubeApp::matrixLookAt(std::array<std::array<float, 4>, 4>& matrix, float eyeX, float eyeY, float eyeZ, float centerX, float centerY, float centerZ, float upX, float upY, float upZ)
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