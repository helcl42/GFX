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

#ifdef None
#undef None
#endif

#include <array>
#include <cfloat>
#include <cmath>
#include <fstream>
#include <iostream>
#include <memory>
#include <vector>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

static constexpr uint32_t WINDOW_WIDTH = 800;
static constexpr uint32_t WINDOW_HEIGHT = 600;
static constexpr uint32_t COMPUTE_TEXTURE_WIDTH = 512;
static constexpr uint32_t COMPUTE_TEXTURE_HEIGHT = 512;
static constexpr gfx::Format COLOR_FORMAT = gfx::Format::B8G8R8A8UnormSrgb;

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

// Uniform structures
struct ComputeUniformData {
    float time;
    float padding[3]; // WebGPU requires 16-byte alignment for uniform buffers
};

struct RenderUniformData {
    float postProcessStrength;
    float padding[3]; // WebGPU requires 16-byte alignment for uniform buffers
};

// Application settings/configuration
struct Settings {
    gfx::Backend backend;
    bool vsync;
};

namespace util {
std::vector<uint8_t> loadBinaryFile(const char* filepath);
std::string loadTextFile(const char* filepath);
} // namespace util

class ComputeApp {
public:
    explicit ComputeApp(const Settings& settings);
    ~ComputeApp() = default;

    bool init();
    void run();
    void cleanup();

private:
    bool createWindow(uint32_t width, uint32_t height);
    void destroyWindow();
    bool createGraphics();
    void destroyGraphics();
    bool createPerFrameResources();
    void destroyPerFrameResources();
    bool createSizeDependentResources(uint32_t width, uint32_t height);
    void destroySizeDependentResources();
    bool createSwapchain(uint32_t width, uint32_t height);
    void destroySwapchain();
    bool createRenderPass();
    void destroyRenderPass();
    bool createFramebuffers();
    void destroyFramebuffers();

    bool createComputeTexture();
    void destroyComputeTexture();
    bool createComputeShaders();
    void destroyComputeShaders();
    bool createComputeBindGroupLayout();
    void destroyComputeBindGroupLayout();
    bool createComputePipeline();
    void destroyComputePipeline();
    bool transitionComputeTexture();
    bool createComputeResources();
    void destroyComputeResources();

    bool createSampler();
    void destroySampler();
    bool createRenderShaders();
    void destroyRenderShaders();
    bool createRenderBindGroupLayout();
    void destroyRenderBindGroupLayout();
    bool createRenderPipeline();
    void destroyRenderPipeline();
    bool createRenderResources();
    void destroyRenderResources();

    void update(float deltaTime);
    void render();
    float getCurrentTime();
    bool mainLoopIteration();
#if defined(__EMSCRIPTEN__)
    static void emscriptenMainLoop(void* userData);
#endif
    gfx::PlatformWindowHandle getPlatformWindowHandle();

    static void errorCallback(int error, const char* description);
    static void framebufferResizeCallback(GLFWwindow* window, int width, int height);
    static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);

private:
    Settings settings;

    GLFWwindow* window = nullptr;

    std::shared_ptr<gfx::Instance> instance;
    std::shared_ptr<gfx::Adapter> adapter;
    gfx::AdapterInfo adapterInfo; // Cached adapter info
    std::shared_ptr<gfx::Device> device;
    std::shared_ptr<gfx::Queue> queue;
    std::shared_ptr<gfx::Surface> surface;
    std::shared_ptr<gfx::Swapchain> swapchain;

    // Compute resources
    std::shared_ptr<gfx::Texture> computeTexture;
    std::shared_ptr<gfx::TextureView> computeTextureView;
    std::shared_ptr<gfx::Shader> computeShader;
    std::shared_ptr<gfx::ComputePipeline> computePipeline;
    std::shared_ptr<gfx::BindGroupLayout> computeBindGroupLayout;
    std::vector<std::shared_ptr<gfx::BindGroup>> computeBindGroups; // Dynamic
    std::vector<std::shared_ptr<gfx::Buffer>> computeUniformBuffers; // Dynamic

    // Render resources (fullscreen quad)
    std::shared_ptr<gfx::Shader> vertexShader;
    std::shared_ptr<gfx::Shader> fragmentShader;
    std::shared_ptr<gfx::RenderPipeline> renderPipeline;
    std::shared_ptr<gfx::BindGroupLayout> renderBindGroupLayout;
    std::shared_ptr<gfx::Sampler> sampler;
    std::vector<std::shared_ptr<gfx::BindGroup>> renderBindGroups; // Dynamic
    std::vector<std::shared_ptr<gfx::Buffer>> renderUniformBuffers; // Dynamic
    std::shared_ptr<gfx::RenderPass> renderPass;
    std::vector<std::shared_ptr<gfx::Framebuffer>> framebuffers;

    uint32_t windowWidth = WINDOW_WIDTH;
    uint32_t windowHeight = WINDOW_HEIGHT;
    uint32_t previousWidth = WINDOW_WIDTH;
    uint32_t previousHeight = WINDOW_HEIGHT;
    size_t framesInFlightCount = 0; // Dynamic based on surface capabilities

    // Per-frame synchronization (dynamic)
    std::vector<std::shared_ptr<gfx::Semaphore>> imageAvailableSemaphores;
    std::vector<std::shared_ptr<gfx::Semaphore>> renderFinishedSemaphores;
    std::vector<std::shared_ptr<gfx::Fence>> inFlightFences;
    std::vector<std::shared_ptr<gfx::CommandEncoder>> commandEncoders;

    size_t currentFrame = 0;
    float elapsedTime = 0.0f;

    // FPS tracking
    uint32_t fpsFrameCount = 0;
    float fpsTimeAccumulator = 0.0f;
    float fpsFrameTimeMin = FLT_MAX;
    float fpsFrameTimeMax = 0.0f;
};

ComputeApp::ComputeApp(const Settings& settings)
    : settings(settings)
{
}

bool ComputeApp::init()
{
    if (!createWindow(WINDOW_WIDTH, WINDOW_HEIGHT)) {
        return false;
    }

    if (!createGraphics()) {
        return false;
    }

    if (!createSizeDependentResources(windowWidth, windowHeight)) {
        return false;
    }

    if (!createComputeResources()) {
        return false;
    }

    if (!createRenderResources()) {
        return false;
    }

    if (!createPerFrameResources()) {
        return false;
    }

    std::cout << "Application initialized successfully!" << std::endl;
    std::cout << "Press ESC to exit" << std::endl;

    return true;
}

void ComputeApp::run()
{
    // Run main loop (platform-specific)
#if defined(__EMSCRIPTEN__)
    // Note: emscripten_set_main_loop_arg returns immediately and never blocks
    // Cleanup happens in emscriptenMainLoop when the loop exits
    // Execution continues in the browser event loop
    emscripten_set_main_loop_arg(ComputeApp::emscriptenMainLoop, this, 0, 1);
#else
    while (mainLoopIteration()) {
        // Loop continues until mainLoopIteration returns false
    }
#endif
}

void ComputeApp::cleanup()
{
    if (device) {
        device->waitIdle();
    }

    // Destroy in reverse order of creation
    destroyPerFrameResources();
    destroyRenderResources();
    destroyComputeResources();
    destroySizeDependentResources();
    destroyGraphics();
    destroyWindow();
}

bool ComputeApp::createWindow(uint32_t width, uint32_t height)
{
    glfwSetErrorCallback(errorCallback);

    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return false;
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

    const char* backendName = (settings.backend == gfx::Backend::Vulkan) ? "Vulkan" : "WebGPU";
    std::string windowTitle = std::string("Compute & Postprocess Example (C++) - ") + backendName;
    window = glfwCreateWindow(windowWidth, windowHeight, windowTitle.c_str(), nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return false;
    }

    glfwSetWindowUserPointer(window, this);
    glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
    glfwSetKeyCallback(window, keyCallback);

    return true;
}

void ComputeApp::destroyWindow()
{
    if (window) {
        glfwDestroyWindow(window);
        window = nullptr;
    }
    glfwTerminate();
}

bool ComputeApp::createGraphics()
{
    // Set up logging callback
    gfx::setLogCallback(logCallback);

    auto result = gfx::loadBackend(settings.backend);
    if (!gfx::isSuccess(result)) {
        std::cerr << "Failed to load graphics backend: " << static_cast<int32_t>(result) << std::endl;
        return false;
    }

    try {
        gfx::InstanceDescriptor instanceDesc{};
        instanceDesc.applicationName = "Compute & Postprocess Example (C++)";
        instanceDesc.applicationVersion = 1;
        instanceDesc.backend = settings.backend;
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

        // Create surface using native platform handles
        gfx::SurfaceDescriptor surfaceDesc{};
        surfaceDesc.label = "Main Surface";
        surfaceDesc.windowHandle = getPlatformWindowHandle();

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

void ComputeApp::destroyGraphics()
{
    surface.reset();
    queue.reset();
    device.reset();
    adapter.reset();
    instance.reset();
    gfx::unloadBackend(settings.backend);
}

bool ComputeApp::createPerFrameResources()
{
    try {
        gfx::SemaphoreDescriptor semaphoreDesc{};
        semaphoreDesc.type = gfx::SemaphoreType::Binary;

        gfx::FenceDescriptor fenceDesc{};
        fenceDesc.signaled = true;

        imageAvailableSemaphores.resize(framesInFlightCount);
        renderFinishedSemaphores.resize(framesInFlightCount);
        inFlightFences.resize(framesInFlightCount);
        commandEncoders.resize(framesInFlightCount);

        for (size_t i = 0; i < framesInFlightCount; ++i) {
            imageAvailableSemaphores[i] = device->createSemaphore(semaphoreDesc);
            if (!imageAvailableSemaphores[i]) {
                std::cerr << "Failed to create image available semaphore " << i << std::endl;
                return false;
            }

            renderFinishedSemaphores[i] = device->createSemaphore(semaphoreDesc);
            if (!renderFinishedSemaphores[i]) {
                std::cerr << "Failed to create render finished semaphore " << i << std::endl;
                return false;
            }

            inFlightFences[i] = device->createFence(fenceDesc);
            if (!inFlightFences[i]) {
                std::cerr << "Failed to create fence " << i << std::endl;
                return false;
            }

            commandEncoders[i] = device->createCommandEncoder({ .label = "Command Encoder " + std::to_string(i) });
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

void ComputeApp::destroyPerFrameResources()
{
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
    commandEncoders.clear();
    inFlightFences.clear();
    renderFinishedSemaphores.clear();
    imageAvailableSemaphores.clear();
}

bool ComputeApp::createSizeDependentResources(uint32_t width, uint32_t height)
{
    if (!createSwapchain(width, height)) {
        return false;
    }

    if (!createRenderPass()) {
        return false;
    }

    if (!createFramebuffers()) {
        return false;
    }

    return true;
}

void ComputeApp::destroySizeDependentResources()
{
    destroyFramebuffers();
    destroyRenderPass();
    destroySwapchain();
}

bool ComputeApp::createSwapchain(uint32_t width, uint32_t height)
{
    try {
        // Query surface capabilities
        auto surfaceInfo = surface->getInfo();
        std::cout << "Surface Info:" << std::endl;
        std::cout << "  Image Count: min " << surfaceInfo.minImageCount << ", max " << surfaceInfo.maxImageCount << std::endl;

        // Calculate frames in flight
        framesInFlightCount = surfaceInfo.minImageCount;
        if (framesInFlightCount < 2) {
            framesInFlightCount = 2;
        }
        if (framesInFlightCount > 4) {
            framesInFlightCount = 4;
        }
        std::cout << "Frames in flight: " << framesInFlightCount << std::endl;

        gfx::SwapchainDescriptor swapchainDesc{};
        swapchainDesc.label = "Main Swapchain";
        swapchainDesc.surface = surface;
        swapchainDesc.extent.width = width;
        swapchainDesc.extent.height = height;
        swapchainDesc.format = COLOR_FORMAT;
        swapchainDesc.usage = gfx::TextureUsage::RenderAttachment;
        swapchainDesc.presentMode = settings.vsync ? gfx::PresentMode::Fifo : gfx::PresentMode::Immediate;
        swapchainDesc.imageCount = static_cast<uint32_t>(framesInFlightCount);

        swapchain = device->createSwapchain(swapchainDesc);
        if (!swapchain) {
            std::cerr << "Failed to create swapchain" << std::endl;
            return false;
        }

        return true;
    } catch (const std::exception& e) {
        std::cerr << "Swapchain creation error: " << e.what() << std::endl;
        return false;
    }
}

void ComputeApp::destroySwapchain()
{
    swapchain.reset();
}

bool ComputeApp::createRenderPass()
{
    try {
        auto swapchainInfo = swapchain->getInfo();

        gfx::RenderPassCreateDescriptor renderPassDesc{};
        renderPassDesc.label = "Main Render Pass";

        gfx::RenderPassColorAttachment colorAttachment{};
        colorAttachment.target.format = swapchainInfo.format;
        colorAttachment.target.sampleCount = gfx::SampleCount::Count1;
        colorAttachment.target.ops.load = gfx::LoadOp::Clear;
        colorAttachment.target.ops.store = gfx::StoreOp::Store;
        colorAttachment.target.finalLayout = gfx::TextureLayout::PresentSrc;

        renderPassDesc.colorAttachments.push_back(colorAttachment);

        renderPass = device->createRenderPass(renderPassDesc);
        if (!renderPass) {
            std::cerr << "Failed to create render pass" << std::endl;
            return false;
        }

        return true;
    } catch (const std::exception& e) {
        std::cerr << "Render pass creation error: " << e.what() << std::endl;
        return false;
    }
}

void ComputeApp::destroyRenderPass()
{
    renderPass.reset();
}

bool ComputeApp::createFramebuffers()
{
    try {
        auto swapchainInfo = swapchain->getInfo();
        framebuffers.resize(swapchainInfo.imageCount);

        for (uint32_t i = 0; i < swapchainInfo.imageCount; ++i) {
            gfx::FramebufferDescriptor framebufferDesc{};
            framebufferDesc.label = "Framebuffer " + std::to_string(i);
            framebufferDesc.renderPass = renderPass;
            framebufferDesc.extent.width = swapchainInfo.extent.width;
            framebufferDesc.extent.height = swapchainInfo.extent.height;
            framebufferDesc.colorAttachments.push_back({ swapchain->getTextureView(i) });

            framebuffers[i] = device->createFramebuffer(framebufferDesc);
            if (!framebuffers[i]) {
                std::cerr << "Failed to create framebuffer " << i << std::endl;
                return false;
            }
        }

        return true;
    } catch (const std::exception& e) {
        std::cerr << "Framebuffer creation error: " << e.what() << std::endl;
        return false;
    }
}

void ComputeApp::destroyFramebuffers()
{
    framebuffers.clear();
}

bool ComputeApp::createComputeTexture()
{
    try {
        // Create compute output texture (storage image)
        gfx::TextureDescriptor textureDesc{};
        textureDesc.type = gfx::TextureType::Texture2D;
        textureDesc.size = { COMPUTE_TEXTURE_WIDTH, COMPUTE_TEXTURE_HEIGHT, 1 };
        textureDesc.format = gfx::Format::R8G8B8A8Unorm;
        textureDesc.usage = gfx::TextureUsage::StorageBinding | gfx::TextureUsage::TextureBinding;
        textureDesc.mipLevelCount = 1;
        textureDesc.sampleCount = gfx::SampleCount::Count1;

        computeTexture = device->createTexture(textureDesc);
        if (!computeTexture) {
            std::cerr << "Failed to create compute texture" << std::endl;
            return false;
        }

        gfx::TextureViewDescriptor viewDesc{};
        viewDesc.format = gfx::Format::R8G8B8A8Unorm;
        viewDesc.viewType = gfx::TextureViewType::View2D;
        viewDesc.baseMipLevel = 0;
        viewDesc.mipLevelCount = 1;
        viewDesc.baseArrayLayer = 0;
        viewDesc.arrayLayerCount = 1;

        computeTextureView = computeTexture->createView(viewDesc);
        if (!computeTextureView) {
            std::cerr << "Failed to create compute texture view" << std::endl;
            return false;
        }

        return true;
    } catch (const std::exception& e) {
        std::cerr << "Failed to create compute texture: " << e.what() << std::endl;
        return false;
    }
}

void ComputeApp::destroyComputeTexture()
{
    computeTextureView.reset();
    computeTexture.reset();
}

bool ComputeApp::createComputeShaders()
{
    try {
        // Load shader - try SPIR-V first, then WGSL
        gfx::ShaderSourceType shaderSourceType;
        std::vector<uint8_t> shaderCode;

        if (device->supportsShaderFormat(gfx::ShaderSourceType::SPIRV)) {
            shaderSourceType = gfx::ShaderSourceType::SPIRV;
            std::cout << "Loading SPIR-V compute shader..." << std::endl;
            shaderCode = util::loadBinaryFile("shaders/generate.comp.spv");
        } else if (device->supportsShaderFormat(gfx::ShaderSourceType::WGSL)) {
            shaderSourceType = gfx::ShaderSourceType::WGSL;
            std::cout << "Loading WGSL compute shader..." << std::endl;
            auto wgsl = util::loadTextFile("shaders/generate.comp.wgsl");
            shaderCode.assign(wgsl.begin(), wgsl.end());
        } else {
            std::cerr << "Error: No supported shader format found" << std::endl;
            return false;
        }

        if (shaderCode.empty()) {
            std::cerr << "Failed to load compute shader" << std::endl;
            return false;
        }

        gfx::ShaderDescriptor shaderDesc{};
        shaderDesc.label = "Compute Shader";
        shaderDesc.sourceType = shaderSourceType;
        shaderDesc.code = shaderCode;
        shaderDesc.entryPoint = "main";

        computeShader = device->createShader(shaderDesc);
        if (!computeShader) {
            std::cerr << "Failed to create compute shader" << std::endl;
            return false;
        }

        return true;
    } catch (const std::exception& e) {
        std::cerr << "Failed to create compute shaders: " << e.what() << std::endl;
        return false;
    }
}

void ComputeApp::destroyComputeShaders()
{
    computeShader.reset();
}

bool ComputeApp::createComputeBindGroupLayout()
{
    try {
        // Create compute bind group layout
        gfx::BindGroupLayoutEntry storageTextureEntry{
            .binding = 0,
            .visibility = gfx::ShaderStage::Compute,
            .resource = gfx::BindGroupLayoutEntry::StorageTextureBinding{
                .format = gfx::Format::R8G8B8A8Unorm,
                .writeOnly = true,
                .viewDimension = gfx::TextureViewType::View2D,
            }
        };

        gfx::BindGroupLayoutEntry uniformBufferEntry{
            .binding = 1,
            .visibility = gfx::ShaderStage::Compute,
            .resource = gfx::BindGroupLayoutEntry::BufferBinding{
                .hasDynamicOffset = false,
                .minBindingSize = sizeof(ComputeUniformData),
            }
        };

        gfx::BindGroupLayoutDescriptor computeLayoutDesc{};
        computeLayoutDesc.label = "Compute Bind Group Layout";
        computeLayoutDesc.entries.push_back(storageTextureEntry);
        computeLayoutDesc.entries.push_back(uniformBufferEntry);

        computeBindGroupLayout = device->createBindGroupLayout(computeLayoutDesc);
        if (!computeBindGroupLayout) {
            std::cerr << "Failed to create compute bind group layout" << std::endl;
            return false;
        }

        return true;
    } catch (const std::exception& e) {
        std::cerr << "Failed to create compute bind group layout: " << e.what() << std::endl;
        return false;
    }
}

void ComputeApp::destroyComputeBindGroupLayout()
{
    computeBindGroupLayout.reset();
}

bool ComputeApp::createComputePipeline()
{
    try {
        // Create compute uniform buffers (one per frame in flight)
        gfx::BufferDescriptor computeUniformBufferDesc{};
        computeUniformBufferDesc.label = "Compute Uniform Buffer";
        computeUniformBufferDesc.size = sizeof(ComputeUniformData);
        computeUniformBufferDesc.usage = gfx::BufferUsage::Uniform | gfx::BufferUsage::CopyDst;

        computeUniformBuffers.resize(framesInFlightCount);
        for (size_t i = 0; i < framesInFlightCount; ++i) {
            computeUniformBuffers[i] = device->createBuffer(computeUniformBufferDesc);
            if (!computeUniformBuffers[i]) {
                std::cerr << "Failed to create compute uniform buffer " << i << std::endl;
                return false;
            }
        }

        // Create compute bind groups (one per frame in flight)
        computeBindGroups.resize(framesInFlightCount);
        for (size_t i = 0; i < framesInFlightCount; ++i) {
            gfx::BindGroupEntry textureEntry{};
            textureEntry.binding = 0;
            textureEntry.resource = computeTextureView;

            gfx::BindGroupEntry bufferEntry{};
            bufferEntry.binding = 1;
            bufferEntry.resource = computeUniformBuffers[i];
            bufferEntry.offset = 0;
            bufferEntry.size = sizeof(ComputeUniformData);

            gfx::BindGroupDescriptor computeBindGroupDesc{};
            computeBindGroupDesc.label = "Compute Bind Group " + std::to_string(i);
            computeBindGroupDesc.layout = computeBindGroupLayout;
            computeBindGroupDesc.entries.push_back(textureEntry);
            computeBindGroupDesc.entries.push_back(bufferEntry);

            computeBindGroups[i] = device->createBindGroup(computeBindGroupDesc);
            if (!computeBindGroups[i]) {
                std::cerr << "Failed to create compute bind group " << i << std::endl;
                return false;
            }
        }

        // Create compute pipeline
        gfx::ComputePipelineDescriptor computePipelineDesc{};
        computePipelineDesc.label = "Compute Pipeline";
        computePipelineDesc.compute = computeShader;
        computePipelineDesc.entryPoint = "main";
        computePipelineDesc.bindGroupLayouts.push_back(computeBindGroupLayout);

        computePipeline = device->createComputePipeline(computePipelineDesc);
        if (!computePipeline) {
            std::cerr << "Failed to create compute pipeline" << std::endl;
            return false;
        }

        return true;
    } catch (const std::exception& e) {
        std::cerr << "Failed to create compute pipeline: " << e.what() << std::endl;
        return false;
    }
}

void ComputeApp::destroyComputePipeline()
{
    computePipeline.reset();
    for (size_t i = 0; i < computeBindGroups.size(); ++i) {
        computeBindGroups[i].reset();
    }
    for (size_t i = 0; i < computeUniformBuffers.size(); ++i) {
        computeUniformBuffers[i].reset();
    }
    computeBindGroups.clear();
    computeUniformBuffers.clear();
}

bool ComputeApp::transitionComputeTexture()
{
    try {
        // Transition compute texture from Undefined to ShaderReadOnly layout
        gfx::CommandEncoderDescriptor initEncoderDesc{};
        initEncoderDesc.label = "Init Texture Transition";
        auto initEncoder = device->createCommandEncoder(initEncoderDesc);
        if (!initEncoder) {
            std::cerr << "Failed to create command encoder for texture transition" << std::endl;
            return false;
        }

        initEncoder->begin();

        gfx::TextureBarrier initBarrier{};
        initBarrier.texture = computeTexture;
        initBarrier.oldLayout = gfx::TextureLayout::Undefined;
        initBarrier.newLayout = gfx::TextureLayout::ShaderReadOnly;
        initBarrier.srcStageMask = gfx::PipelineStage::TopOfPipe;
        initBarrier.dstStageMask = gfx::PipelineStage::FragmentShader;
        initBarrier.srcAccessMask = gfx::AccessFlags::None;
        initBarrier.dstAccessMask = gfx::AccessFlags::ShaderRead;
        initBarrier.baseMipLevel = 0;
        initBarrier.mipLevelCount = 1;
        initBarrier.baseArrayLayer = 0;
        initBarrier.arrayLayerCount = 1;

        gfx::PipelineBarrierDescriptor barrierDesc{};
        barrierDesc.textureBarriers.push_back(initBarrier);
        initEncoder->pipelineBarrier(barrierDesc);
        initEncoder->end();

        gfx::FenceDescriptor initFenceDesc{};
        initFenceDesc.signaled = false;
        auto initFence = device->createFence(initFenceDesc);
        if (!initFence) {
            std::cerr << "Failed to create fence for texture transition" << std::endl;
            return false;
        }

        gfx::SubmitDescriptor submitDescriptor{};
        submitDescriptor.commandEncoders.push_back(initEncoder);
        submitDescriptor.signalFence = initFence;

        queue->submit(submitDescriptor);
        auto waitResult = initFence->wait(gfx::TimeoutInfinite);
        if (!gfx::isSuccess(waitResult)) {
            std::cerr << "Failed to wait for texture transition fence" << std::endl;
            return false;
        }

        return true;
    } catch (const std::exception& e) {
        std::cerr << "Failed to transition compute texture: " << e.what() << std::endl;
        return false;
    }
}

bool ComputeApp::createComputeResources()
{
    if (!createComputeTexture()) {
        return false;
    }

    if (!createComputeShaders()) {
        return false;
    }

    if (!createComputeBindGroupLayout()) {
        return false;
    }

    if (!createComputePipeline()) {
        return false;
    }

    if (!transitionComputeTexture()) {
        return false;
    }

    std::cout << "Compute resources created successfully" << std::endl;
    return true;
}

void ComputeApp::destroyComputeResources()
{
    destroyComputePipeline();
    destroyComputeBindGroupLayout();
    destroyComputeShaders();
    destroyComputeTexture();
}

bool ComputeApp::createSampler()
{
    try {
        // Create sampler
        gfx::SamplerDescriptor samplerDesc{};
        samplerDesc.magFilter = gfx::FilterMode::Linear;
        samplerDesc.minFilter = gfx::FilterMode::Linear;
        samplerDesc.addressModeU = gfx::AddressMode::ClampToEdge;
        samplerDesc.addressModeV = gfx::AddressMode::ClampToEdge;

        sampler = device->createSampler(samplerDesc);
        if (!sampler) {
            std::cerr << "Failed to create sampler" << std::endl;
            return false;
        }

        return true;
    } catch (const std::exception& e) {
        std::cerr << "Failed to create sampler: " << e.what() << std::endl;
        return false;
    }
}

void ComputeApp::destroySampler()
{
    sampler.reset();
}

bool ComputeApp::createRenderShaders()
{
    try {
        // Load shaders - try SPIR-V first, then WGSL
        gfx::ShaderSourceType shaderSourceType;
        std::vector<uint8_t> vertexShaderCode, fragmentShaderCode;

        if (device->supportsShaderFormat(gfx::ShaderSourceType::SPIRV)) {
            shaderSourceType = gfx::ShaderSourceType::SPIRV;
            std::cout << "Loading SPIR-V shaders..." << std::endl;
            vertexShaderCode = util::loadBinaryFile("shaders/fullscreen.vert.spv");
            fragmentShaderCode = util::loadBinaryFile("shaders/postprocess.frag.spv");
        } else if (device->supportsShaderFormat(gfx::ShaderSourceType::WGSL)) {
            shaderSourceType = gfx::ShaderSourceType::WGSL;
            std::cout << "Loading WGSL shaders..." << std::endl;
            auto vertexWgsl = util::loadTextFile("shaders/fullscreen.vert.wgsl");
            auto fragmentWgsl = util::loadTextFile("shaders/postprocess.frag.wgsl");
            vertexShaderCode.assign(vertexWgsl.begin(), vertexWgsl.end());
            fragmentShaderCode.assign(fragmentWgsl.begin(), fragmentWgsl.end());
        } else {
            std::cerr << "Error: No supported shader format found" << std::endl;
            return false;
        }

        if (vertexShaderCode.empty() || fragmentShaderCode.empty()) {
            std::cerr << "Failed to load shaders" << std::endl;
            return false;
        }

        gfx::ShaderDescriptor vertexShaderDesc{};
        vertexShaderDesc.label = "Vertex Shader";
        vertexShaderDesc.sourceType = shaderSourceType;
        vertexShaderDesc.code = vertexShaderCode;
        vertexShaderDesc.entryPoint = "main";

        vertexShader = device->createShader(vertexShaderDesc);
        if (!vertexShader) {
            std::cerr << "Failed to create vertex shader" << std::endl;
            return false;
        }

        gfx::ShaderDescriptor fragmentShaderDesc{};
        fragmentShaderDesc.label = "Fragment Shader";
        fragmentShaderDesc.sourceType = shaderSourceType;
        fragmentShaderDesc.code = fragmentShaderCode;
        fragmentShaderDesc.entryPoint = "main";

        fragmentShader = device->createShader(fragmentShaderDesc);
        if (!fragmentShader) {
            std::cerr << "Failed to create fragment shader" << std::endl;
            return false;
        }

        return true;
    } catch (const std::exception& e) {
        std::cerr << "Failed to create render shaders: " << e.what() << std::endl;
        return false;
    }
}

void ComputeApp::destroyRenderShaders()
{
    fragmentShader.reset();
    vertexShader.reset();
}

bool ComputeApp::createRenderBindGroupLayout()
{
    try {
        // Create render bind group layout
        gfx::BindGroupLayoutEntry samplerEntry{
            .binding = 0,
            .visibility = gfx::ShaderStage::Fragment,
            .resource = gfx::BindGroupLayoutEntry::SamplerBinding{
                .comparison = false,
            }
        };

        gfx::BindGroupLayoutEntry textureEntry{
            .binding = 1,
            .visibility = gfx::ShaderStage::Fragment,
            .resource = gfx::BindGroupLayoutEntry::TextureBinding{
                .multisampled = false,
                .viewDimension = gfx::TextureViewType::View2D,
            }
        };

        gfx::BindGroupLayoutEntry uniformBufferEntry{
            .binding = 2,
            .visibility = gfx::ShaderStage::Fragment,
            .resource = gfx::BindGroupLayoutEntry::BufferBinding{
                .hasDynamicOffset = false,
                .minBindingSize = sizeof(RenderUniformData),
            }
        };

        gfx::BindGroupLayoutDescriptor renderLayoutDesc{};
        renderLayoutDesc.label = "Render Bind Group Layout";
        renderLayoutDesc.entries.push_back(samplerEntry);
        renderLayoutDesc.entries.push_back(textureEntry);
        renderLayoutDesc.entries.push_back(uniformBufferEntry);

        renderBindGroupLayout = device->createBindGroupLayout(renderLayoutDesc);
        if (!renderBindGroupLayout) {
            std::cerr << "Failed to create render bind group layout" << std::endl;
            return false;
        }

        return true;
    } catch (const std::exception& e) {
        std::cerr << "Failed to create render bind group layout: " << e.what() << std::endl;
        return false;
    }
}

void ComputeApp::destroyRenderBindGroupLayout()
{
    renderBindGroupLayout.reset();
}

bool ComputeApp::createRenderPipeline()
{
    try {
        // Create render uniform buffers (one per frame in flight)
        gfx::BufferDescriptor renderUniformBufferDesc{};
        renderUniformBufferDesc.label = "Render Uniform Buffer";
        renderUniformBufferDesc.size = sizeof(RenderUniformData);
        renderUniformBufferDesc.usage = gfx::BufferUsage::Uniform | gfx::BufferUsage::CopyDst;

        renderUniformBuffers.resize(framesInFlightCount);
        for (size_t i = 0; i < framesInFlightCount; ++i) {
            renderUniformBuffers[i] = device->createBuffer(renderUniformBufferDesc);
            if (!renderUniformBuffers[i]) {
                std::cerr << "Failed to create render uniform buffer " << i << std::endl;
                return false;
            }
        }

        // Create render bind groups (one per frame in flight)
        renderBindGroups.resize(framesInFlightCount);
        for (size_t i = 0; i < framesInFlightCount; ++i) {
            gfx::BindGroupEntry samplerBindEntry{};
            samplerBindEntry.binding = 0;
            samplerBindEntry.resource = sampler;

            gfx::BindGroupEntry textureBindEntry{};
            textureBindEntry.binding = 1;
            textureBindEntry.resource = computeTextureView;

            gfx::BindGroupEntry bufferBindEntry{};
            bufferBindEntry.binding = 2;
            bufferBindEntry.resource = renderUniformBuffers[i];
            bufferBindEntry.offset = 0;
            bufferBindEntry.size = sizeof(RenderUniformData);

            gfx::BindGroupDescriptor renderBindGroupDesc{};
            renderBindGroupDesc.label = "Render Bind Group " + std::to_string(i);
            renderBindGroupDesc.layout = renderBindGroupLayout;
            renderBindGroupDesc.entries.push_back(samplerBindEntry);
            renderBindGroupDesc.entries.push_back(textureBindEntry);
            renderBindGroupDesc.entries.push_back(bufferBindEntry);

            renderBindGroups[i] = device->createBindGroup(renderBindGroupDesc);
            if (!renderBindGroups[i]) {
                std::cerr << "Failed to create render bind group " << i << std::endl;
                return false;
            }
        }

        // Create render pipeline
        gfx::VertexState vertexState{};
        vertexState.module = vertexShader;
        vertexState.entryPoint = "main";
        vertexState.buffers = {};

        gfx::ColorTargetState colorTarget{};
        colorTarget.format = swapchain->getInfo().format;
        colorTarget.writeMask = gfx::ColorWriteMask::All;

        gfx::FragmentState fragmentState{};
        fragmentState.module = fragmentShader;
        fragmentState.entryPoint = "main";
        fragmentState.targets.push_back(colorTarget);

        gfx::PrimitiveState primitiveState{};
        primitiveState.topology = gfx::PrimitiveTopology::TriangleList;
        primitiveState.frontFace = gfx::FrontFace::CounterClockwise;
        primitiveState.cullMode = gfx::CullMode::None;
        primitiveState.polygonMode = gfx::PolygonMode::Fill;

        gfx::RenderPipelineDescriptor pipelineDesc{};
        pipelineDesc.label = "Render Pipeline";
        pipelineDesc.vertex = vertexState;
        pipelineDesc.fragment = fragmentState;
        pipelineDesc.primitive = primitiveState;
        pipelineDesc.sampleCount = gfx::SampleCount::Count1;
        pipelineDesc.bindGroupLayouts.push_back(renderBindGroupLayout);
        pipelineDesc.renderPass = renderPass;

        renderPipeline = device->createRenderPipeline(pipelineDesc);
        if (!renderPipeline) {
            std::cerr << "Failed to create render pipeline" << std::endl;
            return false;
        }

        return true;
    } catch (const std::exception& e) {
        std::cerr << "Failed to create render pipeline: " << e.what() << std::endl;
        return false;
    }
}

void ComputeApp::destroyRenderPipeline()
{
    renderPipeline.reset();
    for (size_t i = 0; i < renderBindGroups.size(); ++i) {
        renderBindGroups[i].reset();
    }
    for (size_t i = 0; i < renderUniformBuffers.size(); ++i) {
        renderUniformBuffers[i].reset();
    }
    renderBindGroups.clear();
    renderUniformBuffers.clear();
}

bool ComputeApp::createRenderResources()
{
    if (!createRenderShaders()) {
        return false;
    }

    if (!createSampler()) {
        return false;
    }

    if (!createRenderBindGroupLayout()) {
        return false;
    }

    if (!createRenderPipeline()) {
        return false;
    }

    std::cout << "Render resources created successfully" << std::endl;
    return true;
}

void ComputeApp::destroyRenderResources()
{
    destroyRenderPipeline();
    destroyRenderBindGroupLayout();
    destroyRenderShaders();
    destroySampler();
}

void ComputeApp::update(float deltaTime)
{
    elapsedTime += deltaTime;
}

void ComputeApp::render()
{
    try {
        size_t frameIndex = currentFrame;

        // Wait for previous frame
        auto waitResult = inFlightFences[frameIndex]->wait(gfx::TimeoutInfinite);
        if (!gfx::isSuccess(waitResult)) {
            throw std::runtime_error("Failed to wait for frame fence");
        }
        inFlightFences[frameIndex]->reset();

        // Acquire swapchain image
        uint32_t imageIndex = 0;
        auto result = swapchain->acquireNextImage(
            UINT64_MAX,
            imageAvailableSemaphores[frameIndex],
            nullptr,
            &imageIndex);

        if (result != gfx::Result::Success) {
            std::cerr << "Failed to acquire swapchain image" << std::endl;
            return;
        }

        // Update compute uniforms for current frame
        ComputeUniformData computeUniforms{ .time = elapsedTime };
        queue->writeBuffer(computeUniformBuffers[frameIndex], 0, &computeUniforms, sizeof(computeUniforms));

        // Update render uniforms for current frame
        RenderUniformData renderUniforms{
            .postProcessStrength = 0.5f + 0.5f * std::sin(elapsedTime * 0.5f)
        };
        queue->writeBuffer(renderUniformBuffers[frameIndex], 0, &renderUniforms, sizeof(renderUniforms));

        // Begin command encoder
        auto encoder = commandEncoders[frameIndex];
        encoder->begin();

        // Transition compute texture to GENERAL layout for compute shader write
        gfx::TextureBarrier readToWriteBarrier{
            .texture = computeTexture,
            .oldLayout = gfx::TextureLayout::ShaderReadOnly,
            .newLayout = gfx::TextureLayout::General,
            .srcStageMask = gfx::PipelineStage::FragmentShader,
            .dstStageMask = gfx::PipelineStage::ComputeShader,
            .srcAccessMask = gfx::AccessFlags::ShaderRead,
            .dstAccessMask = gfx::AccessFlags::ShaderWrite,
            .baseMipLevel = 0,
            .mipLevelCount = 1,
            .baseArrayLayer = 0,
            .arrayLayerCount = 1
        };
        encoder->pipelineBarrier({ .textureBarriers = { readToWriteBarrier } });

        // Compute pass: Generate pattern
        {
            gfx::ComputePassBeginDescriptor computePassDesc;
            computePassDesc.label = "Generate Pattern";
            auto computePass = encoder->beginComputePass(computePassDesc);
            computePass->setPipeline(computePipeline);
            computePass->setBindGroup(0, computeBindGroups[frameIndex]);

            uint32_t workGroupsX = (COMPUTE_TEXTURE_WIDTH + 15) / 16;
            uint32_t workGroupsY = (COMPUTE_TEXTURE_HEIGHT + 15) / 16;
            computePass->dispatch(workGroupsX, workGroupsY, 1);
        } // computePass destroyed here

        // Transition compute texture for shader read
        gfx::TextureBarrier computeToReadBarrier{
            .texture = computeTexture,
            .oldLayout = gfx::TextureLayout::General,
            .newLayout = gfx::TextureLayout::ShaderReadOnly,
            .srcStageMask = gfx::PipelineStage::ComputeShader,
            .dstStageMask = gfx::PipelineStage::FragmentShader,
            .srcAccessMask = gfx::AccessFlags::ShaderWrite,
            .dstAccessMask = gfx::AccessFlags::ShaderRead,
            .baseMipLevel = 0,
            .mipLevelCount = 1,
            .baseArrayLayer = 0,
            .arrayLayerCount = 1
        };
        encoder->pipelineBarrier({ .textureBarriers = { computeToReadBarrier } });

        // Render pass: Post-process and display
        gfx::Color clearColor{ 0.0f, 0.0f, 0.0f, 1.0f };

        gfx::RenderPassBeginDescriptor renderPassBeginDesc{};
        renderPassBeginDesc.framebuffer = framebuffers[imageIndex];
        renderPassBeginDesc.colorClearValues = { clearColor };

        {
            auto renderPassEncoder = encoder->beginRenderPass(renderPassBeginDesc);

            renderPassEncoder->setPipeline(renderPipeline);
            renderPassEncoder->setBindGroup(0, renderBindGroups[frameIndex]);

            renderPassEncoder->setViewport({ 0.0f, 0.0f, static_cast<float>(windowWidth), static_cast<float>(windowHeight), 0.0f, 1.0f });
            renderPassEncoder->setScissorRect({ 0, 0, windowWidth, windowHeight });

            // Draw fullscreen quad (6 vertices, no buffers needed)
            renderPassEncoder->draw(6, 1, 0, 0);
        } // renderPassEncoder destroyed here

        encoder->end();

        // Submit
        gfx::SubmitDescriptor submitDescriptor{};
        submitDescriptor.commandEncoders = { encoder };
        submitDescriptor.waitSemaphores = { imageAvailableSemaphores[frameIndex] };
        submitDescriptor.signalSemaphores = { renderFinishedSemaphores[frameIndex] };
        submitDescriptor.signalFence = inFlightFences[frameIndex];

        auto submitResult = queue->submit(submitDescriptor);
        if (!gfx::isSuccess(submitResult)) {
            throw std::runtime_error("Failed to submit compute commands");
        }

        // Present
        gfx::PresentDescriptor presentDescriptor{};
        presentDescriptor.waitSemaphores = { renderFinishedSemaphores[frameIndex] };

        result = swapchain->present(presentDescriptor);

        currentFrame = (currentFrame + 1) % framesInFlightCount;
    } catch (const std::exception& e) {
        std::cerr << "Render error: " << e.what() << std::endl;
    }
}

float ComputeApp::getCurrentTime()
{
#if defined(__EMSCRIPTEN__)
    return (float)emscripten_get_now() / 1000.0f;
#else
    return (float)glfwGetTime();
#endif
}

bool ComputeApp::mainLoopIteration()
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
        destroySizeDependentResources();
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
    float deltaTime = currentTime - elapsedTime;

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
void ComputeApp::emscriptenMainLoop(void* userData)
{
    ComputeApp* app = (ComputeApp*)userData;
    if (!app->mainLoopIteration()) {
        emscripten_cancel_main_loop();
        app->cleanup();
    }
}
#endif

gfx::PlatformWindowHandle ComputeApp::getPlatformWindowHandle()
{
    gfx::PlatformWindowHandle handle{};

#if defined(__EMSCRIPTEN__)
    handle = gfx::PlatformWindowHandle::fromEmscripten("#canvas");
#elif defined(_WIN32)
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

void ComputeApp::errorCallback(int error, const char* description)
{
    std::cerr << "GLFW Error " << error << ": " << description << std::endl;
}

void ComputeApp::framebufferResizeCallback(GLFWwindow* window, int width, int height)
{
    auto* app = static_cast<ComputeApp*>(glfwGetWindowUserPointer(window));
    if (app) {
        app->windowWidth = static_cast<uint32_t>(width);
        app->windowHeight = static_cast<uint32_t>(height);
    }
}

void ComputeApp::keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    (void)scancode;
    (void)mods;

    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    }
}

namespace util {
std::vector<uint8_t> loadBinaryFile(const char* filepath)
{
    std::ifstream file(filepath, std::ios::binary | std::ios::ate);
    if (!file) {
        std::cerr << "Failed to open file: " << filepath << std::endl;
        return {};
    }

    size_t fileSize = static_cast<size_t>(file.tellg());
    file.seekg(0);

    std::vector<uint8_t> buffer(fileSize);
    file.read(reinterpret_cast<char*>(buffer.data()), fileSize);

    if (!file) {
        std::cerr << "Failed to read complete file: " << filepath << std::endl;
        return {};
    }

    return buffer;
}

std::string loadTextFile(const char* filepath)
{
    std::FILE* file = std::fopen(filepath, "r");
    if (!file) {
        std::cerr << "Failed to open file: " << filepath << std::endl;
        return {};
    }

    std::fseek(file, 0, SEEK_END);
    long fileSize = std::ftell(file);
    std::fseek(file, 0, SEEK_SET);

    if (fileSize <= 0) {
        std::cerr << "Invalid file size for: " << filepath << std::endl;
        std::fclose(file);
        return {};
    }

    std::string buffer(fileSize, '\0');
    size_t bytesRead = std::fread(buffer.data(), 1, fileSize, file);
    std::fclose(file);

    if (bytesRead != static_cast<size_t>(fileSize)) {
        std::cerr << "Failed to read complete file: " << filepath << std::endl;
        return {};
    }

    return buffer;
}
} // namespace util

static bool parseArguments(int argc, char** argv, Settings& settings)
{
#if defined(__EMSCRIPTEN__)
    settings.backend = gfx::Backend::WebGPU;
#else
    settings.backend = gfx::Backend::Vulkan;
#endif
    settings.vsync = true; // VSync on by default

    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--backend") == 0 && i + 1 < argc) {
            i++;
            if (std::strcmp(argv[i], "vulkan") == 0) {
                settings.backend = gfx::Backend::Vulkan;
            } else if (std::strcmp(argv[i], "webgpu") == 0) {
                settings.backend = gfx::Backend::WebGPU;
            } else {
                std::cerr << "Unknown backend: " << argv[i] << std::endl;
                return false;
            }
        } else if (std::strcmp(argv[i], "--vsync") == 0 && i + 1 < argc) {
            i++;
            int vsync = std::atoi(argv[i]);
            if (vsync == 0) {
                settings.vsync = false;
            } else if (vsync == 1) {
                settings.vsync = true;
            } else {
                std::cerr << "Invalid vsync value: " << argv[i] << std::endl;
                std::cerr << "Valid values: 0 (off), 1 (on)" << std::endl;
                return false;
            }
        } else if (std::strcmp(argv[i], "--help") == 0) {
            std::cout << "Usage: " << argv[0] << " [options]" << std::endl;
            std::cout << "Options:" << std::endl;
            std::cout << "  --backend [vulkan|webgpu]   Select graphics backend" << std::endl;
            std::cout << "  --vsync [0|1]               VSync: 0=off, 1=on" << std::endl;
            std::cout << "  --help                      Show this help message" << std::endl;
            return false;
        } else {
            std::cerr << "Unknown argument: " << argv[i] << std::endl;
            return false;
        }
    }

    return true;
}

int main(int argc, char** argv)
{
    std::cout << "=== Compute & Postprocess Example (C++) ===" << std::endl;

    Settings settings;
    if (!parseArguments(argc, argv, settings)) {
        return 0;
    }

    ComputeApp app(settings);

    if (!app.init()) {
        app.cleanup();
        return -1;
    }

    app.run();
    app.cleanup();

    std::cout << "Application terminated successfully" << std::endl;
    return 0;
}
