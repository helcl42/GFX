#include <gfx_cpp/Gfx.hpp>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

// Include platform-specific GLFW headers to get native handles
#ifdef _WIN32
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
#elif defined(__linux__)
#define GLFW_EXPOSE_NATIVE_X11
#include <GLFW/glfw3native.h>
// X11 headers define "Success" and "None" as macros which conflict with our enums
#ifdef Success
#undef Success
#endif
#ifdef None
#undef None
#endif
#elif defined(__APPLE__)
#define GLFW_EXPOSE_NATIVE_COCOA
#include <GLFW/glfw3native.h>
#endif

#include <array>
#include <cmath>
#include <fstream>
#include <iostream>
#include <memory>
#include <vector>

using namespace gfx;

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

static constexpr uint32_t WINDOW_WIDTH = 800;
static constexpr uint32_t WINDOW_HEIGHT = 600;
static constexpr uint32_t COMPUTE_TEXTURE_WIDTH = 800;
static constexpr uint32_t COMPUTE_TEXTURE_HEIGHT = 600;
static constexpr size_t MAX_FRAMES_IN_FLIGHT = 3;
static constexpr TextureFormat COLOR_FORMAT = TextureFormat::B8G8R8A8UnormSrgb;

// Uniform structures
struct ComputeUniformData {
    float time;
};

struct RenderUniformData {
    float postProcessStrength;
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
    bool recreateSizeDependentResources(uint32_t width, uint32_t height);
    void update(float deltaTime);
    void drawFrame();
    PlatformWindowHandle extractNativeHandle();
    std::vector<uint8_t> loadBinaryFile(const char* filepath);

private:
    static void framebufferResizeCallback(GLFWwindow* window, int width, int height);
    static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);

private:
    GLFWwindow* window = nullptr;

    std::shared_ptr<Instance> instance;
    std::shared_ptr<Adapter> adapter;
    std::shared_ptr<Device> device;
    std::shared_ptr<Queue> queue;
    std::shared_ptr<Surface> surface;
    std::shared_ptr<Swapchain> swapchain;

    // Compute resources
    std::shared_ptr<Texture> computeTexture;
    std::shared_ptr<TextureView> computeTextureView;
    std::shared_ptr<Shader> computeShader;
    std::shared_ptr<ComputePipeline> computePipeline;
    std::shared_ptr<BindGroupLayout> computeBindGroupLayout;
    std::array<std::shared_ptr<BindGroup>, MAX_FRAMES_IN_FLIGHT> computeBindGroups;
    std::array<std::shared_ptr<Buffer>, MAX_FRAMES_IN_FLIGHT> computeUniformBuffers;

    // Render resources (fullscreen quad)
    std::shared_ptr<Shader> vertexShader;
    std::shared_ptr<Shader> fragmentShader;
    std::shared_ptr<RenderPipeline> renderPipeline;
    std::shared_ptr<BindGroupLayout> renderBindGroupLayout;
    std::shared_ptr<Sampler> sampler;
    std::array<std::shared_ptr<BindGroup>, MAX_FRAMES_IN_FLIGHT> renderBindGroups;
    std::array<std::shared_ptr<Buffer>, MAX_FRAMES_IN_FLIGHT> renderUniformBuffers;

    uint32_t windowWidth = WINDOW_WIDTH;
    uint32_t windowHeight = WINDOW_HEIGHT;

    // Per-frame synchronization
    std::array<std::shared_ptr<Semaphore>, MAX_FRAMES_IN_FLIGHT> imageAvailableSemaphores;
    std::array<std::shared_ptr<Semaphore>, MAX_FRAMES_IN_FLIGHT> renderFinishedSemaphores;
    std::array<std::shared_ptr<Fence>, MAX_FRAMES_IN_FLIGHT> inFlightFences;
    std::array<std::shared_ptr<CommandEncoder>, MAX_FRAMES_IN_FLIGHT> commandEncoders;

    size_t currentFrame = 0;
    float elapsedTime = 0.0f;
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
    try {
        // Get required extensions from GLFW
        uint32_t glfwExtensionCount = 0;
        const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

        std::vector<std::string> extensions;
        for (uint32_t i = 0; i < glfwExtensionCount; ++i) {
            extensions.emplace_back(glfwExtensions[i]);
        }

        InstanceDescriptor instanceDesc{};
        instanceDesc.applicationName = "Compute & Postprocess Example (C++)";
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

        // Set debug callback
        instance->setDebugCallback([](DebugMessageSeverity severity, DebugMessageType type, const std::string& message) {
            if (severity == DebugMessageSeverity::Verbose) {
                return; // Skip verbose
            }

            const char* severityStr = "";
            switch (severity) {
            case DebugMessageSeverity::Info:
                severityStr = "INFO";
                break;
            case DebugMessageSeverity::Warning:
                severityStr = "WARNING";
                break;
            case DebugMessageSeverity::Error:
                severityStr = "ERROR";
                break;
            default:
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

        // Create surface using native platform handles
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
        swapchainDesc.width = windowWidth;
        swapchainDesc.height = windowHeight;
        swapchainDesc.format = COLOR_FORMAT;
        swapchainDesc.usage = TextureUsage::RenderAttachment;
        swapchainDesc.presentMode = PresentMode::Fifo;
        swapchainDesc.bufferCount = MAX_FRAMES_IN_FLIGHT;

        swapchain = device->createSwapchain(surface, swapchainDesc);
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
        TextureDescriptor textureDesc{};
        textureDesc.type = TextureType::Texture2D;
        textureDesc.size = { COMPUTE_TEXTURE_WIDTH, COMPUTE_TEXTURE_HEIGHT, 1 };
        textureDesc.format = TextureFormat::R8G8B8A8Unorm;
        textureDesc.usage = TextureUsage::StorageBinding | TextureUsage::TextureBinding;
        textureDesc.mipLevelCount = 1;
        textureDesc.sampleCount = SampleCount::Count1;

        computeTexture = device->createTexture(textureDesc);
        if (!computeTexture) {
            std::cerr << "Failed to create compute texture" << std::endl;
            return false;
        }

        TextureViewDescriptor viewDesc{};
        viewDesc.format = TextureFormat::R8G8B8A8Unorm;
        viewDesc.viewType = TextureViewType::View2D;
        viewDesc.baseMipLevel = 0;
        viewDesc.mipLevelCount = 1;
        viewDesc.baseArrayLayer = 0;
        viewDesc.arrayLayerCount = 1;

        computeTextureView = computeTexture->createView(viewDesc);
        if (!computeTextureView) {
            std::cerr << "Failed to create compute texture view" << std::endl;
            return false;
        }

        // Load compute shader
        auto computeShaderCode = loadBinaryFile("generate.comp.spv");
        if (computeShaderCode.empty()) {
            std::cerr << "Failed to load compute shader" << std::endl;
            return false;
        }

        ShaderDescriptor computeShaderDesc{};
        computeShaderDesc.label = "Compute Shader";
        computeShaderDesc.code = std::string(reinterpret_cast<const char*>(computeShaderCode.data()), computeShaderCode.size());
        computeShaderDesc.entryPoint = "main";

        computeShader = device->createShader(computeShaderDesc);
        if (!computeShader) {
            std::cerr << "Failed to create compute shader" << std::endl;
            return false;
        }

        // Create compute uniform buffers (one per frame in flight)
        BufferDescriptor computeUniformBufferDesc{};
        computeUniformBufferDesc.label = "Compute Uniform Buffer";
        computeUniformBufferDesc.size = sizeof(ComputeUniformData);
        computeUniformBufferDesc.usage = BufferUsage::Uniform | BufferUsage::CopyDst;
        computeUniformBufferDesc.mappedAtCreation = false;

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
            computeUniformBuffers[i] = device->createBuffer(computeUniformBufferDesc);
            if (!computeUniformBuffers[i]) {
                std::cerr << "Failed to create compute uniform buffer " << i << std::endl;
                return false;
            }
        }

        // Create compute bind group layout
        BindGroupLayoutEntry storageTextureEntry{
            .binding = 0,
            .visibility = ShaderStage::Compute,
            .resource = BindGroupLayoutEntry::StorageTextureBinding{
                .format = TextureFormat::R8G8B8A8Unorm,
                .writeOnly = true,
            }
        };

        BindGroupLayoutEntry uniformBufferEntry{
            .binding = 1,
            .visibility = ShaderStage::Compute,
            .resource = BindGroupLayoutEntry::BufferBinding{
                .hasDynamicOffset = false,
                .minBindingSize = sizeof(ComputeUniformData),
            }
        };

        BindGroupLayoutDescriptor computeLayoutDesc{};
        computeLayoutDesc.label = "Compute Bind Group Layout";
        computeLayoutDesc.entries = { storageTextureEntry, uniformBufferEntry };

        computeBindGroupLayout = device->createBindGroupLayout(computeLayoutDesc);
        if (!computeBindGroupLayout) {
            std::cerr << "Failed to create compute bind group layout" << std::endl;
            return false;
        }

        // Create compute bind groups (one per frame in flight)
        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
            BindGroupEntry textureEntry{};
            textureEntry.binding = 0;
            textureEntry.resource = computeTextureView;

            BindGroupEntry bufferEntry{};
            bufferEntry.binding = 1;
            bufferEntry.resource = computeUniformBuffers[i];
            bufferEntry.offset = 0;
            bufferEntry.size = sizeof(ComputeUniformData);

            BindGroupDescriptor computeBindGroupDesc{};
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
        ComputePipelineDescriptor computePipelineDesc{};
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
        auto initEncoder = device->createCommandEncoder("Init Layout Transition");
        if (initEncoder) {
            initEncoder->begin();

            TextureBarrier initBarrier{
                .texture = computeTexture,
                .oldLayout = TextureLayout::Undefined,
                .newLayout = TextureLayout::ShaderReadOnly,
                .srcStageMask = PipelineStage::TopOfPipe,
                .dstStageMask = PipelineStage::FragmentShader,
                .srcAccessMask = AccessFlags::None,
                .dstAccessMask = AccessFlags::ShaderRead,
                .baseMipLevel = 0,
                .mipLevelCount = 1,
                .baseArrayLayer = 0,
                .arrayLayerCount = 1
            };

            initEncoder->pipelineBarrier({}, {}, { initBarrier });
            initEncoder->end();

            FenceDescriptor initFenceDesc{};
            initFenceDesc.signaled = false;
            auto initFence = device->createFence(initFenceDesc);

            SubmitInfo submitInfo{};
            submitInfo.commandEncoders = { initEncoder };
            submitInfo.signalFence = initFence;

            queue->submit(submitInfo);
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
        // Load shaders
        auto vertexShaderCode = loadBinaryFile("fullscreen.vert.spv");
        if (vertexShaderCode.empty()) {
            std::cerr << "Failed to load vertex shader" << std::endl;
            return false;
        }

        ShaderDescriptor vertexShaderDesc{};
        vertexShaderDesc.label = "Vertex Shader";
        vertexShaderDesc.code = std::string(reinterpret_cast<const char*>(vertexShaderCode.data()), vertexShaderCode.size());
        vertexShaderDesc.entryPoint = "main";

        vertexShader = device->createShader(vertexShaderDesc);
        if (!vertexShader) {
            std::cerr << "Failed to create vertex shader" << std::endl;
            return false;
        }

        auto fragmentShaderCode = loadBinaryFile("postprocess.frag.spv");
        if (fragmentShaderCode.empty()) {
            std::cerr << "Failed to load fragment shader" << std::endl;
            return false;
        }

        ShaderDescriptor fragmentShaderDesc{};
        fragmentShaderDesc.label = "Fragment Shader";
        fragmentShaderDesc.code = std::string(reinterpret_cast<const char*>(fragmentShaderCode.data()), fragmentShaderCode.size());
        fragmentShaderDesc.entryPoint = "main";

        fragmentShader = device->createShader(fragmentShaderDesc);
        if (!fragmentShader) {
            std::cerr << "Failed to create fragment shader" << std::endl;
            return false;
        }

        // Create sampler
        SamplerDescriptor samplerDesc{};
        samplerDesc.magFilter = FilterMode::Linear;
        samplerDesc.minFilter = FilterMode::Linear;
        samplerDesc.addressModeU = AddressMode::ClampToEdge;
        samplerDesc.addressModeV = AddressMode::ClampToEdge;

        sampler = device->createSampler(samplerDesc);
        if (!sampler) {
            std::cerr << "Failed to create sampler" << std::endl;
            return false;
        }

        // Create render uniform buffers (one per frame in flight)
        BufferDescriptor renderUniformBufferDesc{};
        renderUniformBufferDesc.label = "Render Uniform Buffer";
        renderUniformBufferDesc.size = sizeof(RenderUniformData);
        renderUniformBufferDesc.usage = BufferUsage::Uniform | BufferUsage::CopyDst;
        renderUniformBufferDesc.mappedAtCreation = false;

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
            renderUniformBuffers[i] = device->createBuffer(renderUniformBufferDesc);
            if (!renderUniformBuffers[i]) {
                std::cerr << "Failed to create render uniform buffer " << i << std::endl;
                return false;
            }
        }

        // Create render bind group layout
        BindGroupLayoutEntry samplerEntry{
            .binding = 0,
            .visibility = ShaderStage::Fragment,
            .resource = BindGroupLayoutEntry::SamplerBinding{
                .comparison = false,
            }
        };

        BindGroupLayoutEntry textureEntry{
            .binding = 1,
            .visibility = ShaderStage::Fragment,
            .resource = BindGroupLayoutEntry::TextureBinding{
                .multisampled = false,
            }
        };

        BindGroupLayoutEntry uniformBufferEntry{
            .binding = 2,
            .visibility = ShaderStage::Fragment,
            .resource = BindGroupLayoutEntry::BufferBinding{
                .hasDynamicOffset = false,
                .minBindingSize = sizeof(RenderUniformData),
            }
        };

        BindGroupLayoutDescriptor renderLayoutDesc{};
        renderLayoutDesc.label = "Render Bind Group Layout";
        renderLayoutDesc.entries = { samplerEntry, textureEntry, uniformBufferEntry };

        renderBindGroupLayout = device->createBindGroupLayout(renderLayoutDesc);
        if (!renderBindGroupLayout) {
            std::cerr << "Failed to create render bind group layout" << std::endl;
            return false;
        }

        // Create render bind groups (one per frame in flight)
        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
            BindGroupEntry samplerBindEntry{};
            samplerBindEntry.binding = 0;
            samplerBindEntry.resource = sampler;

            BindGroupEntry textureBindEntry{};
            textureBindEntry.binding = 1;
            textureBindEntry.resource = computeTextureView;

            BindGroupEntry bufferBindEntry{};
            bufferBindEntry.binding = 2;
            bufferBindEntry.resource = renderUniformBuffers[i];
            bufferBindEntry.offset = 0;
            bufferBindEntry.size = sizeof(RenderUniformData);

            BindGroupDescriptor renderBindGroupDesc{};
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
        VertexState vertexState{};
        vertexState.module = vertexShader;
        vertexState.entryPoint = "main";
        vertexState.buffers = {};

        ColorTargetState colorTarget{};
        colorTarget.format = COLOR_FORMAT;
        colorTarget.writeMask = 0xF;

        FragmentState fragmentState{};
        fragmentState.module = fragmentShader;
        fragmentState.entryPoint = "main";
        fragmentState.targets = { colorTarget };

        PrimitiveState primitiveState{};
        primitiveState.topology = PrimitiveTopology::TriangleList;
        primitiveState.frontFace = FrontFace::CounterClockwise;
        primitiveState.cullMode = CullMode::None;
        primitiveState.polygonMode = PolygonMode::Fill;

        RenderPipelineDescriptor pipelineDesc{};
        pipelineDesc.label = "Render Pipeline";
        pipelineDesc.vertex = vertexState;
        pipelineDesc.fragment = fragmentState;
        pipelineDesc.primitive = primitiveState;
        pipelineDesc.sampleCount = SampleCount::Count1;
        pipelineDesc.bindGroupLayouts = { renderBindGroupLayout };

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
        SemaphoreDescriptor semaphoreDesc{};
        semaphoreDesc.type = SemaphoreType::Binary;

        FenceDescriptor fenceDesc{};
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

            commandEncoders[i] = device->createCommandEncoder("Command Encoder " + std::to_string(i));
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

bool ComputeApp::recreateSizeDependentResources(uint32_t width, uint32_t height)
{
    try {
        SwapchainDescriptor swapchainDesc{};
        swapchainDesc.width = width;
        swapchainDesc.height = height;
        swapchainDesc.format = COLOR_FORMAT;
        swapchainDesc.usage = TextureUsage::RenderAttachment;
        swapchainDesc.presentMode = PresentMode::Fifo;
        swapchainDesc.bufferCount = MAX_FRAMES_IN_FLIGHT;

        swapchain = device->createSwapchain(surface, swapchainDesc);
        if (!swapchain) {
            std::cerr << "Failed to create swapchain" << std::endl;
            return false;
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

void ComputeApp::drawFrame()
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

        if (result != Result::Success) {
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
        TextureBarrier readToWriteBarrier{
            .texture = computeTexture,
            .oldLayout = TextureLayout::ShaderReadOnly,
            .newLayout = TextureLayout::General,
            .srcStageMask = PipelineStage::FragmentShader,
            .dstStageMask = PipelineStage::ComputeShader,
            .srcAccessMask = AccessFlags::ShaderRead,
            .dstAccessMask = AccessFlags::ShaderWrite,
            .baseMipLevel = 0,
            .mipLevelCount = 1,
            .baseArrayLayer = 0,
            .arrayLayerCount = 1
        };
        encoder->pipelineBarrier({}, {}, { readToWriteBarrier });

        // Compute pass: Generate pattern
        auto computePass = encoder->beginComputePass("Generate Pattern");
        computePass->setPipeline(computePipeline);
        computePass->setBindGroup(0, computeBindGroups[frameIndex]);

        uint32_t workGroupsX = (COMPUTE_TEXTURE_WIDTH + 15) / 16;
        uint32_t workGroupsY = (COMPUTE_TEXTURE_HEIGHT + 15) / 16;
        computePass->dispatchWorkgroups(workGroupsX, workGroupsY, 1);

        computePass->end();

        // Transition compute texture for shader read
        TextureBarrier computeToReadBarrier{
            .texture = computeTexture,
            .oldLayout = TextureLayout::General,
            .newLayout = TextureLayout::ShaderReadOnly,
            .srcStageMask = PipelineStage::ComputeShader,
            .dstStageMask = PipelineStage::FragmentShader,
            .srcAccessMask = AccessFlags::ShaderWrite,
            .dstAccessMask = AccessFlags::ShaderRead,
            .baseMipLevel = 0,
            .mipLevelCount = 1,
            .baseArrayLayer = 0,
            .arrayLayerCount = 1
        };
        encoder->pipelineBarrier({}, {}, { computeToReadBarrier });

        // Render pass: Post-process and display
        auto swapchainView = swapchain->getImageView(imageIndex);
        if (!swapchainView) {
            return;
        }

        Color clearColor{ 0.0f, 0.0f, 0.0f, 1.0f };

        auto renderPass = encoder->beginRenderPass(
            { swapchainView },
            { clearColor },
            { TextureLayout::PresentSrc },
            nullptr,
            0.0f,
            0,
            TextureLayout::Undefined);

        renderPass->setPipeline(renderPipeline);
        renderPass->setBindGroup(0, renderBindGroups[frameIndex]);

        renderPass->setViewport(0.0f, 0.0f,
            static_cast<float>(windowWidth),
            static_cast<float>(windowHeight),
            0.0f, 1.0f);
        renderPass->setScissorRect(0, 0, windowWidth, windowHeight);

        // Draw fullscreen quad (6 vertices, no buffers needed)
        renderPass->draw(6, 1, 0, 0);

        renderPass->end();

        encoder->end();

        // Submit
        SubmitInfo submitInfo{};
        submitInfo.commandEncoders = { encoder };
        submitInfo.waitSemaphores = { imageAvailableSemaphores[frameIndex] };
        submitInfo.signalSemaphores = { renderFinishedSemaphores[frameIndex] };
        submitInfo.signalFence = inFlightFences[frameIndex];

        queue->submit(submitInfo);

        // Present
        PresentInfo presentInfo{};
        presentInfo.waitSemaphores = { renderFinishedSemaphores[frameIndex] };

        result = swapchain->present(presentInfo);

        currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
    } catch (const std::exception& e) {
        std::cerr << "Render error: " << e.what() << std::endl;
    }
}

void ComputeApp::run()
{
    std::cout << "\nStarting render loop..." << std::endl;
    std::cout << "Press ESC to exit\n"
              << std::endl;

    float lastTime = static_cast<float>(glfwGetTime());

    uint32_t previousWidth = swapchain->getWidth();
    uint32_t previousHeight = swapchain->getHeight();

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        // Handle framebuffer resize
        if (previousWidth != windowWidth || previousHeight != windowHeight) {
            device->waitIdle();

            cleanupSizeDependentResources();
            if (!recreateSizeDependentResources(windowWidth, windowHeight)) {
                std::cerr << "Failed to recreate size-dependent resources after resize" << std::endl;
                break;
            }

            previousWidth = windowWidth;
            previousHeight = windowHeight;

            std::cout << "Window resized: " << windowWidth << "x" << windowHeight << std::endl;
            continue;
        }

        float currentTime = static_cast<float>(glfwGetTime());
        float deltaTime = currentTime - lastTime;
        lastTime = currentTime;

        update(deltaTime);
        drawFrame();
    }
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
    swapchain.reset();
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

PlatformWindowHandle ComputeApp::extractNativeHandle()
{
    PlatformWindowHandle handle{};

#ifdef _WIN32
    handle.hwnd = glfwGetWin32Window(window);
    handle.hinstance = GetModuleHandle(nullptr);
    std::cout << "Extracted Win32 handle" << std::endl;
#elif defined(__linux__)
    handle.windowingSystem = WindowingSystem::X11;
    handle.x11.window = reinterpret_cast<void*>(glfwGetX11Window(window));
    handle.x11.display = glfwGetX11Display();
    std::cout << "Extracted X11 handle" << std::endl;
#elif defined(__APPLE__)
    handle.nsWindow = glfwGetCocoaWindow(window);
    std::cout << "Extracted Cocoa handle" << std::endl;
#else
    handle.window = window;
    std::cout << "Using GLFW window handle directly (fallback)" << std::endl;
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
