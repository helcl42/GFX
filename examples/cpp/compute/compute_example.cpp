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
static constexpr uint32_t COMPUTE_TEXTURE_WIDTH = 800;
static constexpr uint32_t COMPUTE_TEXTURE_HEIGHT = 600;
static constexpr size_t MAX_FRAMES_IN_FLIGHT = 3;
static constexpr gfx::TextureFormat COLOR_FORMAT = gfx::TextureFormat::B8G8R8A8UnormSrgb;

#if defined(__EMSCRIPTEN__)
static constexpr gfx::Backend BACKEND_API = gfx::Backend::WebGPU;
#else
// here we can choose between VULKAN, WEBGPU
static constexpr gfx::Backend BACKEND_API = gfx::Backend::Vulkan;
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

// Uniform structures
struct ComputeUniformData {
    float time;
    float padding[3]; // WebGPU requires 16-byte alignment for uniform buffers
};

struct RenderUniformData {
    float postProcessStrength;
    float padding[3]; // WebGPU requires 16-byte alignment for uniform buffers
};

class ComputeApp {
public:
    bool initialize();
    void run();
    void cleanup();

private:
    bool initializeGLFW();
    bool initializeGraphics();
    bool createComputeResources();
    bool createRenderResources();
    bool createSyncObjects();
    void cleanupSizeDependentResources();
    bool createSizeDependentResources(uint32_t width, uint32_t height);
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

private:
    static void framebufferResizeCallback(GLFWwindow* window, int width, int height);
    static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);

private:
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
    std::array<std::shared_ptr<gfx::BindGroup>, MAX_FRAMES_IN_FLIGHT> computeBindGroups;
    std::array<std::shared_ptr<gfx::Buffer>, MAX_FRAMES_IN_FLIGHT> computeUniformBuffers;

    // Render resources (fullscreen quad)
    std::shared_ptr<gfx::Shader> vertexShader;
    std::shared_ptr<gfx::Shader> fragmentShader;
    std::shared_ptr<gfx::RenderPipeline> renderPipeline;
    std::shared_ptr<gfx::BindGroupLayout> renderBindGroupLayout;
    std::shared_ptr<gfx::Sampler> sampler;
    std::array<std::shared_ptr<gfx::BindGroup>, MAX_FRAMES_IN_FLIGHT> renderBindGroups;
    std::array<std::shared_ptr<gfx::Buffer>, MAX_FRAMES_IN_FLIGHT> renderUniformBuffers;
    std::shared_ptr<gfx::RenderPass> renderPass;
    std::vector<std::shared_ptr<gfx::Framebuffer>> framebuffers;

    uint32_t windowWidth = WINDOW_WIDTH;
    uint32_t windowHeight = WINDOW_HEIGHT;
    uint32_t previousWidth = WINDOW_WIDTH;
    uint32_t previousHeight = WINDOW_HEIGHT;

    // Per-frame synchronization
    std::array<std::shared_ptr<gfx::Semaphore>, MAX_FRAMES_IN_FLIGHT> imageAvailableSemaphores;
    std::array<std::shared_ptr<gfx::Semaphore>, MAX_FRAMES_IN_FLIGHT> renderFinishedSemaphores;
    std::array<std::shared_ptr<gfx::Fence>, MAX_FRAMES_IN_FLIGHT> inFlightFences;
    std::array<std::shared_ptr<gfx::CommandEncoder>, MAX_FRAMES_IN_FLIGHT> commandEncoders;

    size_t currentFrame = 0;
    float elapsedTime = 0.0f;

    // FPS tracking
    uint32_t fpsFrameCount = 0;
    float fpsTimeAccumulator = 0.0f;
    float fpsFrameTimeMin = FLT_MAX;
    float fpsFrameTimeMax = 0.0f;
};

bool ComputeApp::initialize()
{
    if (!initializeGLFW()) {
        return false;
    }

    if (!initializeGraphics()) {
        return false;
    }

    if (!createComputeResources()) {
        return false;
    }

    if (!createRenderResources()) {
        return false;
    }

    if (!createSyncObjects()) {
        return false;
    }

    std::cout << "Application initialized successfully!" << std::endl;
    std::cout << "Press ESC to exit" << std::endl;

    return true;
}

bool ComputeApp::initializeGLFW()
{
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return false;
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

    window = glfwCreateWindow(windowWidth, windowHeight, "Compute & Postprocess Example (C++)", nullptr, nullptr);
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

bool ComputeApp::initializeGraphics()
{
    // Set up logging callback
    gfx::setLogCallback(logCallback);

    try {
        gfx::InstanceDescriptor instanceDesc{};
        instanceDesc.applicationName = "Compute & Postprocess Example (C++)";
        instanceDesc.applicationVersion = 1;
        instanceDesc.backend = gfx::Backend::WebGPU;
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

        createSizeDependentResources(static_cast<uint32_t>(width), static_cast<uint32_t>(height));
        if (!swapchain) {
            std::cerr << "Failed to create swapchain" << std::endl;
            return false;
        }

        return true;
    } catch (const std::exception& e) {
        std::cerr << "Failed to initialize graphics: " << e.what() << std::endl;
        return false;
    }
}

bool ComputeApp::createComputeResources()
{
    try {
        // Create compute output texture (storage image)
        gfx::TextureDescriptor textureDesc{};
        textureDesc.type = gfx::TextureType::Texture2D;
        textureDesc.size = { COMPUTE_TEXTURE_WIDTH, COMPUTE_TEXTURE_HEIGHT, 1 };
        textureDesc.format = gfx::TextureFormat::R8G8B8A8Unorm;
        textureDesc.usage = gfx::TextureUsage::StorageBinding | gfx::TextureUsage::TextureBinding;
        textureDesc.mipLevelCount = 1;
        textureDesc.sampleCount = gfx::SampleCount::Count1;

        computeTexture = device->createTexture(textureDesc);
        if (!computeTexture) {
            std::cerr << "Failed to create compute texture" << std::endl;
            return false;
        }

        gfx::TextureViewDescriptor viewDesc{};
        viewDesc.format = gfx::TextureFormat::R8G8B8A8Unorm;
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

        gfx::ShaderSourceType shaderSourceType;
        std::vector<uint8_t> computeShaderCode;

        if (device->supportsShaderFormat(gfx::ShaderSourceType::SPIRV)) {
            shaderSourceType = gfx::ShaderSourceType::SPIRV;
            std::cout << "Loading SPIR-V compute shader..." << std::endl;
            computeShaderCode = loadBinaryFile("generate.comp.spv");
        } else if (device->supportsShaderFormat(gfx::ShaderSourceType::WGSL)) {
            shaderSourceType = gfx::ShaderSourceType::WGSL;
            std::cout << "Loading WGSL compute shader..." << std::endl;
            auto wgsl = loadTextFile("shaders/generate.comp.wgsl");
            computeShaderCode.assign(wgsl.begin(), wgsl.end());
        } else {
            std::cerr << "Error: No supported shader format found" << std::endl;
            return false;
        }

        if (computeShaderCode.empty()) {
            std::cerr << "Failed to load compute shader" << std::endl;
            return false;
        }

        gfx::ShaderDescriptor computeShaderDesc{};
        computeShaderDesc.label = "Compute Shader";
        computeShaderDesc.sourceType = shaderSourceType;
        computeShaderDesc.code = computeShaderCode;
        computeShaderDesc.entryPoint = "main";

        computeShader = device->createShader(computeShaderDesc);
        if (!computeShader) {
            std::cerr << "Failed to create compute shader" << std::endl;
            return false;
        }

        // Create compute uniform buffers (one per frame in flight)
        gfx::BufferDescriptor computeUniformBufferDesc{};
        computeUniformBufferDesc.label = "Compute Uniform Buffer";
        computeUniformBufferDesc.size = sizeof(ComputeUniformData);
        computeUniformBufferDesc.usage = gfx::BufferUsage::Uniform | gfx::BufferUsage::CopyDst;

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
            computeUniformBuffers[i] = device->createBuffer(computeUniformBufferDesc);
            if (!computeUniformBuffers[i]) {
                std::cerr << "Failed to create compute uniform buffer " << i << std::endl;
                return false;
            }
        }

        // Create compute bind group layout
        gfx::BindGroupLayoutEntry storageTextureEntry{
            .binding = 0,
            .visibility = gfx::ShaderStage::Compute,
            .resource = gfx::BindGroupLayoutEntry::StorageTextureBinding{
                .format = gfx::TextureFormat::R8G8B8A8Unorm,
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
        computeLayoutDesc.entries = { storageTextureEntry, uniformBufferEntry };

        computeBindGroupLayout = device->createBindGroupLayout(computeLayoutDesc);
        if (!computeBindGroupLayout) {
            std::cerr << "Failed to create compute bind group layout" << std::endl;
            return false;
        }

        // Create compute bind groups (one per frame in flight)
        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
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
            computeBindGroupDesc.entries = { textureEntry, bufferEntry };

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
        computePipelineDesc.bindGroupLayouts = { computeBindGroupLayout };

        computePipeline = device->createComputePipeline(computePipelineDesc);
        if (!computePipeline) {
            std::cerr << "Failed to create compute pipeline" << std::endl;
            return false;
        }

        // Transition compute texture to SHADER_READ_ONLY layout initially
        auto initEncoder = device->createCommandEncoder({ "Init Layout Transition" });
        if (initEncoder) {
            initEncoder->begin();

            gfx::TextureBarrier initBarrier{
                .texture = computeTexture,
                .oldLayout = gfx::TextureLayout::Undefined,
                .newLayout = gfx::TextureLayout::ShaderReadOnly,
                .srcStageMask = gfx::PipelineStage::TopOfPipe,
                .dstStageMask = gfx::PipelineStage::FragmentShader,
                .srcAccessMask = gfx::AccessFlags::None,
                .dstAccessMask = gfx::AccessFlags::ShaderRead,
                .baseMipLevel = 0,
                .mipLevelCount = 1,
                .baseArrayLayer = 0,
                .arrayLayerCount = 1
            };

            initEncoder->pipelineBarrier({ .textureBarriers = { initBarrier } });
            initEncoder->end();

            gfx::FenceDescriptor initFenceDesc{};
            initFenceDesc.signaled = false;
            auto initFence = device->createFence(initFenceDesc);

            gfx::SubmitDescriptor submitDescriptor{};
            submitDescriptor.commandEncoders = { initEncoder };
            submitDescriptor.signalFence = initFence;

            queue->submit(submitDescriptor);
            initFence->wait(UINT64_MAX);
        }

        std::cout << "Compute resources created successfully" << std::endl;
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Failed to create compute resources: " << e.what() << std::endl;
        return false;
    }
}

bool ComputeApp::createRenderResources()
{
    try {
        // Load shaders - try SPIR-V first, then WGSL
        gfx::ShaderSourceType shaderSourceType;
        std::vector<uint8_t> vertexShaderCode, fragmentShaderCode;

        if (device->supportsShaderFormat(gfx::ShaderSourceType::SPIRV)) {
            shaderSourceType = gfx::ShaderSourceType::SPIRV;
            std::cout << "Loading SPIR-V shaders..." << std::endl;
            vertexShaderCode = loadBinaryFile("fullscreen.vert.spv");
            fragmentShaderCode = loadBinaryFile("postprocess.frag.spv");
        } else if (device->supportsShaderFormat(gfx::ShaderSourceType::WGSL)) {
            shaderSourceType = gfx::ShaderSourceType::WGSL;
            std::cout << "Loading WGSL shaders..." << std::endl;
            auto vertexWgsl = loadTextFile("shaders/fullscreen.vert.wgsl");
            auto fragmentWgsl = loadTextFile("shaders/postprocess.frag.wgsl");
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

        // Create render uniform buffers (one per frame in flight)
        gfx::BufferDescriptor renderUniformBufferDesc{};
        renderUniformBufferDesc.label = "Render Uniform Buffer";
        renderUniformBufferDesc.size = sizeof(RenderUniformData);
        renderUniformBufferDesc.usage = gfx::BufferUsage::Uniform | gfx::BufferUsage::CopyDst;

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
            renderUniformBuffers[i] = device->createBuffer(renderUniformBufferDesc);
            if (!renderUniformBuffers[i]) {
                std::cerr << "Failed to create render uniform buffer " << i << std::endl;
                return false;
            }
        }

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
        renderLayoutDesc.entries = { samplerEntry, textureEntry, uniformBufferEntry };

        renderBindGroupLayout = device->createBindGroupLayout(renderLayoutDesc);
        if (!renderBindGroupLayout) {
            std::cerr << "Failed to create render bind group layout" << std::endl;
            return false;
        }

        // Create render bind groups (one per frame in flight)
        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
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
            renderBindGroupDesc.entries = { samplerBindEntry, textureBindEntry, bufferBindEntry };

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
        fragmentState.targets = { colorTarget };

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
        pipelineDesc.bindGroupLayouts = { renderBindGroupLayout };
        pipelineDesc.renderPass = renderPass;

        renderPipeline = device->createRenderPipeline(pipelineDesc);
        if (!renderPipeline) {
            std::cerr << "Failed to create render pipeline" << std::endl;
            return false;
        }

        std::cout << "Render resources created successfully" << std::endl;
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Failed to create render resources: " << e.what() << std::endl;
        return false;
    }
}

bool ComputeApp::createSyncObjects()
{
    try {
        gfx::SemaphoreDescriptor semaphoreDesc{};
        semaphoreDesc.type = gfx::SemaphoreType::Binary;

        gfx::FenceDescriptor fenceDesc{};
        fenceDesc.signaled = true;

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
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

void ComputeApp::cleanupSizeDependentResources()
{
    swapchain.reset();
}

bool ComputeApp::createSizeDependentResources(uint32_t width, uint32_t height)
{
    try {
        gfx::SwapchainDescriptor swapchainDesc{};
        swapchainDesc.surface = surface;
        swapchainDesc.width = width;
        swapchainDesc.height = height;
        swapchainDesc.format = COLOR_FORMAT;
        swapchainDesc.usage = gfx::TextureUsage::RenderAttachment;
        swapchainDesc.presentMode = gfx::PresentMode::Fifo;
        swapchainDesc.imageCount = MAX_FRAMES_IN_FLIGHT;

        swapchain = device->createSwapchain(swapchainDesc);
        if (!swapchain) {
            std::cerr << "Failed to create swapchain" << std::endl;
            return false;
        }

        auto swapchainInfo = swapchain->getInfo();

        // Create render pass
        gfx::RenderPassCreateDescriptor renderPassDesc{};
        renderPassDesc.label = "Main Render Pass";

        // Color attachment
        gfx::RenderPassColorAttachment colorAttachment{};
        colorAttachment.target.format = swapchainInfo.format;
        colorAttachment.target.sampleCount = gfx::SampleCount::Count1;
        colorAttachment.target.loadOp = gfx::LoadOp::Clear;
        colorAttachment.target.storeOp = gfx::StoreOp::Store;
        colorAttachment.target.finalLayout = gfx::TextureLayout::PresentSrc;

        renderPassDesc.colorAttachments.push_back(colorAttachment);

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
            framebufferDesc.width = width;
            framebufferDesc.height = height;

            // Color attachment
            framebufferDesc.colorAttachments.push_back({ swapchain->getTextureView(i) });

            framebuffers[i] = device->createFramebuffer(framebufferDesc);
            if (!framebuffers[i]) {
                std::cerr << "Failed to create framebuffer " << i << std::endl;
                return false;
            }
        }

        return true;
    } catch (const std::exception& e) {
        std::cerr << "Failed to recreate swapchain: " << e.what() << std::endl;
        return false;
    }
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
        inFlightFences[frameIndex]->wait(UINT64_MAX);
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

            renderPassEncoder->setViewport(0.0f, 0.0f,
                static_cast<float>(windowWidth),
                static_cast<float>(windowHeight),
                0.0f, 1.0f);
            renderPassEncoder->setScissorRect(0, 0, windowWidth, windowHeight);

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

        queue->submit(submitDescriptor);

        // Present
        gfx::PresentDescriptor presentDescriptor{};
        presentDescriptor.waitSemaphores = { renderFinishedSemaphores[frameIndex] };

        result = swapchain->present(presentDescriptor);

        currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
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

    // Sync objects
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        commandEncoders[i].reset();
        inFlightFences[i].reset();
        renderFinishedSemaphores[i].reset();
        imageAvailableSemaphores[i].reset();
    }

    // Render resources
    renderPipeline.reset();
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        renderBindGroups[i].reset();
        renderUniformBuffers[i].reset();
    }
    renderBindGroupLayout.reset();
    sampler.reset();
    fragmentShader.reset();
    vertexShader.reset();

    // Compute resources
    computePipeline.reset();
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        computeBindGroups[i].reset();
        computeUniformBuffers[i].reset();
    }
    computeBindGroupLayout.reset();
    computeShader.reset();
    computeTextureView.reset();
    computeTexture.reset();

    // Core resources
    cleanupSizeDependentResources();
    surface.reset();
    queue.reset();
    device.reset();
    adapter.reset();
    instance.reset();

    if (window) {
        glfwDestroyWindow(window);
        window = nullptr;
    }
    glfwTerminate();
}

gfx::PlatformWindowHandle ComputeApp::extractNativeHandle()
{
    gfx::PlatformWindowHandle handle{};

#if defined(__EMSCRIPTEN__)
    handle = gfx::PlatformWindowHandle::fromEmscripten("#canvas");

#elif defined(_WIN32)
    // Windows: Get HWND and HINSTANCE
    handle = gfx::PlatformWindowHandle::fromWin32(glfwGetWin32Window(window), GetModuleHandle(nullptr));
    std::cout << "Extracted Win32 handle: HWND=" << handle.handle.win32.hwnd << ", HINSTANCE=" << handle.handle.win32.hinstance << std::endl;

#elif defined(__linux__)
    // Linux: Get X11 Window and Display (assuming X11, not Wayland)
    handle = gfx::PlatformWindowHandle::fromXlib(glfwGetX11Display(), glfwGetX11Window(window));
    std::cout << "Extracted X11 handle: Window=" << handle.handle.xlib.window << ", Display=" << handle.handle.xlib.display << std::endl;

#elif defined(__APPLE__)
    // macOS: Get NSWindow
    handle = gfx::PlatformWindowHandle::fromMetal(glfwGetMetalLayer(window));
    std::cout << "Extracted Metal handle: Layer=" << handle.handle.metal.layer << std::endl;
#endif

    return handle;
}

std::vector<uint8_t> ComputeApp::loadBinaryFile(const char* filepath)
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

std::string ComputeApp::loadTextFile(const char* filepath)
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

int main()
{
    std::cout << "=== Compute & Postprocess Example (C++) ===" << std::endl;

    ComputeApp app;

    if (!app.initialize()) {
        app.cleanup();
        return -1;
    }

    app.run();
    app.cleanup();

    std::cout << "Application terminated successfully" << std::endl;
    return 0;
}
