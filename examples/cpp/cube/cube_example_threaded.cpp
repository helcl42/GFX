// Threaded Cube Example (C++ with C API) - Parallel Command Recording with ThreadPool
// Uses C API (gfx/gfx.h) but C++ language features and ThreadPool for threading

#include <gfx/gfx.h>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#if !defined(__EMSCRIPTEN__)
#if defined(_WIN32)
#define GLFW_EXPOSE_NATIVE_WIN32
#elif defined(__linux__)
#define GLFW_EXPOSE_NATIVE_X11
#elif defined(__APPLE__)
#define GLFW_EXPOSE_NATIVE_COCOA
#endif
#include <GLFW/glfw3native.h>
#endif

#include <algorithm>
#include <atomic>
#include <cmath>
#include <condition_variable>
#include <cstddef>
#include <cstring>
#include <format>
#include <fstream>
#include <functional>
#include <future>
#include <iostream>
#include <memory>
#include <mutex>
#include <queue>
#include <stdexcept>
#include <thread>
#include <vector>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Constants
constexpr uint32_t WINDOW_WIDTH = 800;
constexpr uint32_t WINDOW_HEIGHT = 600;
constexpr uint32_t MAX_FRAMES_IN_FLIGHT = 3;
constexpr uint32_t CUBE_COUNT = 12;
constexpr GfxSampleCount MSAA_SAMPLE_COUNT = GFX_SAMPLE_COUNT_4;
constexpr GfxTextureFormat COLOR_FORMAT = GFX_TEXTURE_FORMAT_B8G8R8A8_UNORM_SRGB;
constexpr GfxTextureFormat DEPTH_FORMAT = GFX_TEXTURE_FORMAT_DEPTH32_FLOAT;

#if defined(__EMSCRIPTEN__)
constexpr GfxBackend GFX_BACKEND_API = GFX_BACKEND_WEBGPU;
constexpr bool USE_THREADING = false;
#else
constexpr GfxBackend GFX_BACKEND_API = GFX_BACKEND_VULKAN;
constexpr bool USE_THREADING = true;
#endif

// ThreadPool class for parallel command recording
class ThreadPool {
public:
    explicit ThreadPool(size_t numThreads)
        : stop(false)
    {
        for (size_t i = 0; i < numThreads; ++i) {
            workers.emplace_back([this] {
                while (true) {
                    std::function<void()> task;
                    {
                        std::unique_lock<std::mutex> lock(queueMutex);
                        condition.wait(lock, [this] { return stop || !tasks.empty(); });

                        if (stop && tasks.empty()) {
                            return;
                        }

                        task = std::move(tasks.front());
                        tasks.pop();
                    }
                    task();
                }
            });
        }
    }

    ~ThreadPool()
    {
        {
            std::unique_lock<std::mutex> lock(queueMutex);
            stop = true;
        }
        condition.notify_all();
        for (std::thread& worker : workers) {
            worker.join();
        }
    }

    template <class F>
    auto Enqueue(F&& f) -> std::future<void>
    {
        auto task = std::make_shared<std::packaged_task<void()>>(std::forward<F>(f));
        std::future<void> res = task->get_future();
        {
            std::unique_lock<std::mutex> lock(queueMutex);
            if (stop) {
                throw std::runtime_error("Enqueue on stopped ThreadPool");
            }
            tasks.emplace([task]() { (*task)(); });
        }
        condition.notify_one();
        return res;
    }

private:
    std::vector<std::thread> workers;
    std::queue<std::function<void()>> tasks;
    std::mutex queueMutex;
    std::condition_variable condition;
    bool stop;
};

// Debug callback function
static void debugCallback(GfxDebugMessageSeverity severity, GfxDebugMessageType type,
    const char* message, void* userData)
{
    const char* severityStr = "";
    switch (severity) {
    case GFX_DEBUG_MESSAGE_SEVERITY_VERBOSE:
        severityStr = "VERBOSE";
        break;
    case GFX_DEBUG_MESSAGE_SEVERITY_INFO:
        severityStr = "INFO";
        break;
    case GFX_DEBUG_MESSAGE_SEVERITY_WARNING:
        severityStr = "WARNING";
        break;
    case GFX_DEBUG_MESSAGE_SEVERITY_ERROR:
        severityStr = "ERROR";
        break;
    }

    const char* typeStr = "";
    switch (type) {
    case GFX_DEBUG_MESSAGE_TYPE_GENERAL:
        typeStr = "GENERAL";
        break;
    case GFX_DEBUG_MESSAGE_TYPE_VALIDATION:
        typeStr = "VALIDATION";
        break;
    case GFX_DEBUG_MESSAGE_TYPE_PERFORMANCE:
        typeStr = "PERFORMANCE";
        break;
    }

    std::cout << "[" << severityStr << "|" << typeStr << "] " << message << std::endl;
}

// Vertex structure for cube
struct Vertex {
    float position[3];
    float color[3];
};

// Uniform buffer structure for transformations
struct UniformData {
    float model[16];
    float view[16];
    float projection[16];
};

// Forward declaration
class CubeApp;

// Matrix math function declarations
void matrixIdentity(float* matrix);
void matrixMultiply(float* result, const float* a, const float* b);
void matrixRotateX(float* matrix, float angle);
void matrixRotateY(float* matrix, float angle);
void matrixRotateZ(float* matrix, float angle);
void matrixPerspective(float* matrix, float fov, float aspect, float near, float far, GfxBackend backend);
void matrixLookAt(float* matrix, float eyeX, float eyeY, float eyeZ,
    float centerX, float centerY, float centerZ,
    float upX, float upY, float upZ);
bool vectorNormalize(float* x, float* y, float* z);

// Helper functions
static void* loadBinaryFile(const char* filepath, size_t* outSize);
static void* loadTextFile(const char* filepath, size_t* outSize);
static float getCurrentTime();

// Main application class
class CubeApp {
public:
    CubeApp();
    ~CubeApp();

    bool initialize();
    void run();

    // Public for GLFW callbacks
    GLFWwindow* window = nullptr;
    uint32_t windowWidth = WINDOW_WIDTH;
    uint32_t windowHeight = WINDOW_HEIGHT;

private:
    // Graphics resources
    GfxInstance instance = nullptr;
    GfxAdapter adapter = nullptr;
    GfxAdapterInfo adapterInfo = {};
    GfxDevice device = nullptr;
    GfxQueue queue = nullptr;
    GfxSurface surface = nullptr;
    GfxSwapchain swapchain = nullptr;
    GfxSwapchainInfo swapchainInfo = {};

    GfxBuffer vertexBuffer = nullptr;
    GfxBuffer indexBuffer = nullptr;
    GfxShader vertexShader = nullptr;
    GfxShader fragmentShader = nullptr;
    GfxRenderPass clearRenderPass = nullptr;
    GfxRenderPass renderPass = nullptr;
    GfxRenderPass resolveRenderPass = nullptr;
    GfxRenderPipeline renderPipeline = nullptr;
    GfxBindGroupLayout uniformBindGroupLayout = nullptr;

    // Depth and MSAA resources
    GfxTexture depthTexture = nullptr;
    GfxTextureView depthTextureView = nullptr;
    GfxTexture msaaColorTexture = nullptr;
    GfxTextureView msaaColorTextureView = nullptr;

    // Framebuffers
    std::vector<GfxFramebuffer> framebuffers;

    // Uniform buffers
    GfxBuffer sharedUniformBuffer = nullptr;
    size_t uniformAlignedSize = 0;
    std::vector<std::vector<GfxBindGroup>> uniformBindGroups; // [frame][cube]

    // Command encoders
    std::vector<GfxCommandEncoder> clearEncoders;
    std::vector<std::vector<GfxCommandEncoder>> cubeEncoders; // [frame][cube]
    std::vector<GfxCommandEncoder> resolveEncoders;

    // Synchronization
    std::vector<GfxSemaphore> imageAvailableSemaphores;
    std::vector<GfxSemaphore> clearFinishedSemaphores;
    std::vector<GfxSemaphore> renderFinishedSemaphores;
    std::vector<GfxFence> inFlightFences;
    uint32_t currentFrame = 0;

    // Animation state
    float rotationAngleX = 0.0f;
    float rotationAngleY = 0.0f;

    // Loop state
    uint32_t previousWidth = WINDOW_WIDTH;
    uint32_t previousHeight = WINDOW_HEIGHT;
    float lastTime = 0.0f;

    // Threading
    std::unique_ptr<ThreadPool> threadPool;
    std::atomic<uint32_t> currentImageIndex{ 0 };

    // Initialization methods
    bool initWindow();
    bool initializeGraphics();
    bool createSyncObjects();
    bool createSwapchain(uint32_t width, uint32_t height);
    bool createTextures(uint32_t width, uint32_t height);
    bool createFramebuffers(uint32_t width, uint32_t height);
    bool createRenderPass();
    bool createSizeDependentResources(uint32_t width, uint32_t height);
    bool createRenderingResources();
    bool createGeometry();
    bool createUniformBuffer();
    bool createBindGroup();
    bool createShaders();
    bool createRenderPipeline();

    // Cleanup methods
    void cleanupSizeDependentResources();
    void cleanupRenderingResources();

    // Update and render
    void update(float deltaTime);
    void updateCube(int cubeIndex);
    void render();
    void recordClearCommands(uint32_t imageIndex);
    void recordCubeCommands(int cubeIndex, uint32_t imageIndex);
    void recordResolveCommands(uint32_t imageIndex);

    // Main loop
    bool mainLoopIteration();

    // Platform-specific
    GfxPlatformWindowHandle getPlatformWindowHandle();
};

// GLFW callbacks
static void errorCallback(int error, const char* description)
{
    std::cerr << "GLFW Error " << error << ": " << description << std::endl;
}

static void framebufferSizeCallback(GLFWwindow* window, int width, int height)
{
    auto* app = static_cast<CubeApp*>(glfwGetWindowUserPointer(window));
    if (app) {
        app->windowWidth = static_cast<uint32_t>(width);
        app->windowHeight = static_cast<uint32_t>(height);
    }
}

static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    }
}

// Constructor
CubeApp::CubeApp()
{
    framebuffers.resize(MAX_FRAMES_IN_FLIGHT, nullptr);
    clearEncoders.resize(MAX_FRAMES_IN_FLIGHT, nullptr);
    resolveEncoders.resize(MAX_FRAMES_IN_FLIGHT, nullptr);
    imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT, nullptr);
    clearFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT, nullptr);
    renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT, nullptr);
    inFlightFences.resize(MAX_FRAMES_IN_FLIGHT, nullptr);

    // Resize 2D vectors
    cubeEncoders.resize(MAX_FRAMES_IN_FLIGHT);
    for (auto& frameEncoders : cubeEncoders) {
        frameEncoders.resize(CUBE_COUNT, nullptr);
    }

    uniformBindGroups.resize(MAX_FRAMES_IN_FLIGHT);
    for (auto& frameBindGroups : uniformBindGroups) {
        frameBindGroups.resize(CUBE_COUNT, nullptr);
    }
}

// Destructor - cleanup in reverse order of creation
CubeApp::~CubeApp()
{
    // Wait for device to finish
    if (device) {
        gfxDeviceWaitIdle(device);
    }

    // Clean up size-dependent resources
    cleanupSizeDependentResources();

    // Clean up rendering resources
    cleanupRenderingResources();

    // Destroy per-frame resources
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        if (imageAvailableSemaphores[i])
            gfxSemaphoreDestroy(imageAvailableSemaphores[i]);
        if (clearFinishedSemaphores[i])
            gfxSemaphoreDestroy(clearFinishedSemaphores[i]);
        if (renderFinishedSemaphores[i])
            gfxSemaphoreDestroy(renderFinishedSemaphores[i]);
        if (inFlightFences[i])
            gfxFenceDestroy(inFlightFences[i]);
        if (clearEncoders[i])
            gfxCommandEncoderDestroy(clearEncoders[i]);
        if (resolveEncoders[i])
            gfxCommandEncoderDestroy(resolveEncoders[i]);

        for (size_t cubeIdx = 0; cubeIdx < CUBE_COUNT; ++cubeIdx) {
            if (cubeEncoders[i][cubeIdx])
                gfxCommandEncoderDestroy(cubeEncoders[i][cubeIdx]);
            if (uniformBindGroups[i][cubeIdx])
                gfxBindGroupDestroy(uniformBindGroups[i][cubeIdx]);
        }
    }

    if (sharedUniformBuffer)
        gfxBufferDestroy(sharedUniformBuffer);
    if (renderPipeline)
        gfxRenderPipelineDestroy(renderPipeline);
    if (fragmentShader)
        gfxShaderDestroy(fragmentShader);
    if (vertexShader)
        gfxShaderDestroy(vertexShader);
    if (uniformBindGroupLayout)
        gfxBindGroupLayoutDestroy(uniformBindGroupLayout);
    if (indexBuffer)
        gfxBufferDestroy(indexBuffer);
    if (vertexBuffer)
        gfxBufferDestroy(vertexBuffer);
    if (surface)
        gfxSurfaceDestroy(surface);
    if (device)
        gfxDeviceDestroy(device);
    if (adapter)
        gfxAdapterDestroy(adapter);
    if (instance)
        gfxInstanceDestroy(instance);

    // Unload backend
    std::cout << "Unloading graphics backend..." << std::endl;
    gfxUnloadBackend(GFX_BACKEND_API);

    // Destroy window
    if (window) {
        glfwDestroyWindow(window);
    }
    glfwTerminate();
}

bool CubeApp::initWindow()
{
    glfwSetErrorCallback(errorCallback);

    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW\n";
        return false;
    }
    
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

    const char* title = USE_THREADING ? "Threaded Cube Example (C++ ThreadPool) - Parallel Command Recording" : "Cube Example (C++) - Unified Graphics API";

    window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, title, nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create GLFW window\n";
        glfwTerminate();
        return false;
    }

    glfwSetWindowUserPointer(window, this);
    glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);
    glfwSetKeyCallback(window, keyCallback);

    return true;
}

GfxPlatformWindowHandle CubeApp::getPlatformWindowHandle() {
    GfxPlatformWindowHandle handle = {}; // Use empty initializer for C++
#if defined(__EMSCRIPTEN__)
    handle = gfxPlatformWindowHandleFromEmscripten("#canvas");
#elif defined(_WIN32)
    handle = gfxPlatformWindowHandleFromWin32(glfwGetWin32Window(window), GetModuleHandle(NULL));
#elif defined(__linux__)
    handle = gfxPlatformWindowHandleFromXlib(glfwGetX11Display(), glfwGetX11Window(window));
#elif defined(__APPLE__)
    handle = gfxPlatformWindowHandleFromMetal(glfwGetMetalLayer(window));
#endif
    return handle;
}

bool CubeApp::initializeGraphics() {
    std::cout << "Loading graphics backend...\n";
    if (gfxLoadBackend(GFX_BACKEND_API) != GFX_RESULT_SUCCESS) {
        std::cerr << "Failed to load graphics backend\n";
        return false;
    }
    std::cout << "Graphics backend loaded successfully!\n";

    // Create instance
    GfxInstanceFeatureType instanceFeatures[] = { GFX_INSTANCE_FEATURE_TYPE_SURFACE };
    GfxInstanceDescriptor instanceDesc = {
        .backend = GFX_BACKEND_API,
        .enableValidation = true,
        .applicationName = "Cube Example (C++ ThreadPool)",
        .applicationVersion = 1,
        .enabledFeatures = instanceFeatures,
        .enabledFeatureCount = 1
    };

    if (gfxCreateInstance(&instanceDesc, &instance) != GFX_RESULT_SUCCESS) {
        std::cerr << "Failed to create graphics instance\n";
        return false;
    }

    gfxInstanceSetDebugCallback(instance, debugCallback, nullptr);

    // Get adapter
    GfxAdapterDescriptor adapterDesc = {
        .adapterIndex = UINT32_MAX,
        .preference = GFX_ADAPTER_PREFERENCE_HIGH_PERFORMANCE
    };

    if (gfxInstanceRequestAdapter(instance, &adapterDesc, &adapter) != GFX_RESULT_SUCCESS) {
        std::cerr << "Failed to get graphics adapter\n";
        return false;
    }

    gfxAdapterGetInfo(adapter, &adapterInfo);
    std::cout << "Using adapter: " << adapterInfo.name << std::endl;
    std::cout << "  Vendor ID: 0x" << std::hex << adapterInfo.vendorID
              << ", Device ID: 0x" << adapterInfo.deviceID << std::dec << std::endl;
    std::cout << "  Type: " << (adapterInfo.adapterType == GFX_ADAPTER_TYPE_DISCRETE_GPU ? "Discrete GPU" : adapterInfo.adapterType == GFX_ADAPTER_TYPE_INTEGRATED_GPU ? "Integrated GPU"
            : adapterInfo.adapterType == GFX_ADAPTER_TYPE_CPU                                                                                                          ? "CPU"
                                                                                                                                                                       : "Unknown")
              << std::endl;
    std::cout << "  Backend: " << (adapterInfo.backend == GFX_BACKEND_VULKAN ? "Vulkan" : "WebGPU") << std::endl;

    // Create device
    GfxDeviceFeatureType deviceFeatures[] = { GFX_DEVICE_FEATURE_TYPE_SWAPCHAIN };
    GfxDeviceDescriptor deviceDesc = {
        .label = "Main Device",
        .enabledFeatures = deviceFeatures,
        .enabledFeatureCount = 1
    };
    if (gfxAdapterCreateDevice(adapter, &deviceDesc, &device) != GFX_RESULT_SUCCESS) {
        std::cerr << "Failed to create device\n";
        return false;
    }
    
    // Query device limits
    GfxDeviceLimits limits;
    if (gfxDeviceGetLimits(device, &limits) != GFX_RESULT_SUCCESS) {
        std::cerr << "Failed to get device limits\n";
        return false;
    }
    std::cout << "Device Limits:\n";
    std::cout << std::format("  Min Uniform Buffer Offset Alignment: {} bytes\n", 
                             limits.minUniformBufferOffsetAlignment);
    std::cout << std::format("  Max Buffer Size: {} bytes\n", limits.maxBufferSize);
    
    // Get queue
    if (gfxDeviceGetQueue(device, &queue) != GFX_RESULT_SUCCESS) {
        std::cerr << "Failed to get device queue\n";
        return false;
    }

    // Create surface
    GfxPlatformWindowHandle windowHandle = getPlatformWindowHandle();
    GfxSurfaceDescriptor surfaceDesc = {
        .label = "Main Surface",
        .windowHandle = windowHandle
    };

    if (gfxDeviceCreateSurface(device, &surfaceDesc, &surface) != GFX_RESULT_SUCCESS) {
        std::cerr << "Failed to create surface\n";
        return false;
    }

    return true;
}

bool CubeApp::createSwapchain(uint32_t width, uint32_t height)
{
    GfxSwapchainDescriptor swapchainDesc = {
        .label = "Main Swapchain",
        .width = width,
        .height = height,
        .format = COLOR_FORMAT,
        .usage = GFX_TEXTURE_USAGE_RENDER_ATTACHMENT,
        .presentMode = GFX_PRESENT_MODE_IMMEDIATE,
        .imageCount = MAX_FRAMES_IN_FLIGHT
    };

    if (gfxDeviceCreateSwapchain(device, surface, &swapchainDesc, &swapchain) != GFX_RESULT_SUCCESS) {
        std::cerr << "Failed to create swapchain" << std::endl;
        return false;
    }

    if (gfxSwapchainGetInfo(swapchain, &swapchainInfo) != GFX_RESULT_SUCCESS) {
        std::cerr << "Failed to get swapchain info" << std::endl;
        return false;
    }

    std::cout << "Swapchain created: " << swapchainInfo.width << "x" << swapchainInfo.height
              << ", format: " << swapchainInfo.format << std::endl;
    return true;
}

bool CubeApp::createTextures(uint32_t width, uint32_t height)
{
    // Create depth texture
    GfxTextureDescriptor depthTextureDesc = {
        .label = "Depth Buffer",
        .type = GFX_TEXTURE_TYPE_2D,
        .size = { .width = width, .height = height, .depth = 1 },
        .arrayLayerCount = 1,
        .mipLevelCount = 1,
        .sampleCount = MSAA_SAMPLE_COUNT,
        .format = DEPTH_FORMAT,
        .usage = GFX_TEXTURE_USAGE_RENDER_ATTACHMENT
    };

    if (gfxDeviceCreateTexture(device, &depthTextureDesc, &depthTexture) != GFX_RESULT_SUCCESS) {
        std::cerr << "Failed to create depth texture\n";
        return false;
    }

    GfxTextureViewDescriptor depthViewDesc = {
        .label = "Depth Buffer View",
        .viewType = GFX_TEXTURE_VIEW_TYPE_2D,
        .format = DEPTH_FORMAT,
        .baseMipLevel = 0,
        .mipLevelCount = 1,
        .baseArrayLayer = 0,
        .arrayLayerCount = 1
    };

    if (gfxTextureCreateView(depthTexture, &depthViewDesc, &depthTextureView) != GFX_RESULT_SUCCESS) {
        std::cerr << "Failed to create depth texture view\n";
        return false;
    }

    // Create MSAA color texture
    GfxTextureDescriptor msaaColorTextureDesc = {
        .label = "MSAA Color Buffer",
        .type = GFX_TEXTURE_TYPE_2D,
        .size = { .width = width, .height = height, .depth = 1 },
        .arrayLayerCount = 1,
        .mipLevelCount = 1,
        .sampleCount = MSAA_SAMPLE_COUNT,
        .format = swapchainInfo.format,
        .usage = GFX_TEXTURE_USAGE_RENDER_ATTACHMENT
    };

    if (gfxDeviceCreateTexture(device, &msaaColorTextureDesc, &msaaColorTexture) != GFX_RESULT_SUCCESS) {
        std::cerr << "Failed to create MSAA color texture\n";
        return false;
    }

    GfxTextureViewDescriptor msaaColorViewDesc = {
        .label = "MSAA Color Buffer View",
        .viewType = GFX_TEXTURE_VIEW_TYPE_2D,
        .format = swapchainInfo.format,
        .baseMipLevel = 0,
        .mipLevelCount = 1,
        .baseArrayLayer = 0,
        .arrayLayerCount = 1
    };

    if (gfxTextureCreateView(msaaColorTexture, &msaaColorViewDesc, &msaaColorTextureView) != GFX_RESULT_SUCCESS) {
        std::cerr << "Failed to create MSAA color texture view\n";
        return false;
    }

    return true;
}

bool CubeApp::createFramebuffers(uint32_t width, uint32_t height)
{
    for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        GfxTextureView backbuffer = nullptr;
        if (gfxSwapchainGetTextureView(swapchain, i, &backbuffer) != GFX_RESULT_SUCCESS || !backbuffer) {
            std::cerr << std::format("Failed to get swapchain image view {}\n", i);
            return false;
        }
        
        GfxFramebufferAttachment fbColorAttachment = {
            .view = (MSAA_SAMPLE_COUNT > GFX_SAMPLE_COUNT_1) ? msaaColorTextureView : backbuffer,
            .resolveTarget = (MSAA_SAMPLE_COUNT > GFX_SAMPLE_COUNT_1) ? backbuffer : nullptr
        };
        
        GfxFramebufferAttachment fbDepthAttachment = {
            .view = depthTextureView,
            .resolveTarget = nullptr
        };
        
        std::string labelStr = std::format("Framebuffer {}", i);
        const char* label = labelStr.c_str();
        
        GfxFramebufferDescriptor fbDesc = {
            .label = label,
            .renderPass = resolveRenderPass,
            .colorAttachments = &fbColorAttachment,
            .colorAttachmentCount = 1,
            .depthStencilAttachment = fbDepthAttachment,
            .width = width,
            .height = height
        };

        if (gfxDeviceCreateFramebuffer(device, &fbDesc, &framebuffers[i]) != GFX_RESULT_SUCCESS) {
            std::cerr << std::format("Failed to create framebuffer {}\n", i);
            return false;
        }
    }

    return true;
}

bool CubeApp::createRenderPass()
{
    // Clear pass target
    GfxRenderPassColorAttachmentTarget clearColorTarget = {
        .format = swapchainInfo.format,
        .sampleCount = MSAA_SAMPLE_COUNT,
        .ops = { .loadOp = GFX_LOAD_OP_CLEAR, .storeOp = GFX_STORE_OP_STORE },
        .finalLayout = GFX_TEXTURE_LAYOUT_COLOR_ATTACHMENT
    };

    // Load pass target
    GfxRenderPassColorAttachmentTarget colorTarget = {
        .format = swapchainInfo.format,
        .sampleCount = MSAA_SAMPLE_COUNT,
        .ops = { .loadOp = GFX_LOAD_OP_LOAD, .storeOp = GFX_STORE_OP_STORE },
        .finalLayout = GFX_TEXTURE_LAYOUT_COLOR_ATTACHMENT
    };

    // Resolve target
    GfxRenderPassColorAttachmentTarget resolveTarget = {
        .format = swapchainInfo.format,
        .sampleCount = GFX_SAMPLE_COUNT_1,
        .ops = { .loadOp = GFX_LOAD_OP_DONT_CARE, .storeOp = GFX_STORE_OP_STORE },
        .finalLayout = GFX_TEXTURE_LAYOUT_PRESENT_SRC
    };

    GfxRenderPassColorAttachmentTarget dummyResolveTarget = {
        .format = swapchainInfo.format,
        .sampleCount = GFX_SAMPLE_COUNT_1,
        .ops = { .loadOp = GFX_LOAD_OP_DONT_CARE, .storeOp = GFX_STORE_OP_DONT_CARE },
        .finalLayout = GFX_TEXTURE_LAYOUT_PRESENT_SRC
    };

    GfxRenderPassDepthStencilAttachmentTarget depthTarget = {
        .format = DEPTH_FORMAT,
        .sampleCount = MSAA_SAMPLE_COUNT,
        .depthOps = { .loadOp = GFX_LOAD_OP_CLEAR, .storeOp = GFX_STORE_OP_DONT_CARE },
        .stencilOps = { .loadOp = GFX_LOAD_OP_DONT_CARE, .storeOp = GFX_STORE_OP_DONT_CARE },
        .finalLayout = GFX_TEXTURE_LAYOUT_DEPTH_STENCIL_ATTACHMENT
    };

    GfxRenderPassDepthStencilAttachment depthAttachment = {
        .target = depthTarget,
        .resolveTarget = nullptr
    };

    // Clear render pass
    GfxRenderPassColorAttachment clearColorAttachment = {
        .target = clearColorTarget,
        .resolveTarget = &dummyResolveTarget
    };

    GfxRenderPassDescriptor clearPassDesc = {
        .label = "Clear Render Pass",
        .colorAttachments = &clearColorAttachment,
        .colorAttachmentCount = 1,
        .depthStencilAttachment = &depthAttachment
    };

    if (gfxDeviceCreateRenderPass(device, &clearPassDesc, &clearRenderPass) != GFX_RESULT_SUCCESS) {
        std::cerr << "Failed to create clear render pass\n";
        return false;
    }

    // Main render pass
    GfxRenderPassColorAttachment colorAttachment = {
        .target = colorTarget,
        .resolveTarget = &dummyResolveTarget
    };

    GfxRenderPassDescriptor renderPassDesc = {
        .label = "Cube Render Pass (LOAD)",
        .colorAttachments = &colorAttachment,
        .colorAttachmentCount = 1,
        .depthStencilAttachment = &depthAttachment
    };

    if (gfxDeviceCreateRenderPass(device, &renderPassDesc, &renderPass) != GFX_RESULT_SUCCESS) {
        std::cerr << "Failed to create render pass\n";
        return false;
    }

    // Resolve render pass
    GfxRenderPassColorAttachment resolveColorAttachment = {
        .target = colorTarget,
        .resolveTarget = &resolveTarget
    };

    GfxRenderPassDepthStencilAttachmentTarget resolveDepthTarget = {
        .format = DEPTH_FORMAT,
        .sampleCount = MSAA_SAMPLE_COUNT,
        .depthOps = { .loadOp = GFX_LOAD_OP_LOAD, .storeOp = GFX_STORE_OP_DONT_CARE },
        .stencilOps = { .loadOp = GFX_LOAD_OP_DONT_CARE, .storeOp = GFX_STORE_OP_DONT_CARE },
        .finalLayout = GFX_TEXTURE_LAYOUT_DEPTH_STENCIL_ATTACHMENT
    };

    GfxRenderPassDepthStencilAttachment resolveDepthAttachment = {
        .target = resolveDepthTarget,
        .resolveTarget = nullptr
    };

    GfxRenderPassDescriptor resolvePassDesc = {
        .label = "Resolve Render Pass",
        .colorAttachments = &resolveColorAttachment,
        .colorAttachmentCount = 1,
        .depthStencilAttachment = &resolveDepthAttachment
    };

    if (gfxDeviceCreateRenderPass(device, &resolvePassDesc, &resolveRenderPass) != GFX_RESULT_SUCCESS) {
        std::cerr << "Failed to create resolve render pass\n";
        return false;
    }

    return true;
}

bool CubeApp::createSizeDependentResources(uint32_t width, uint32_t height)
{
    if (!createSwapchain(width, height))
        return false;

    uint32_t swapchainWidth = swapchainInfo.width;
    uint32_t swapchainHeight = swapchainInfo.height;

    if (!createTextures(swapchainWidth, swapchainHeight))
        return false;
    if (!createRenderPass())
        return false;
    if (!createFramebuffers(swapchainWidth, swapchainHeight))
        return false;

    return true;
}

void CubeApp::cleanupSizeDependentResources()
{
    for (auto& fb : framebuffers) {
        if (fb) {
            gfxFramebufferDestroy(fb);
            fb = nullptr;
        }
    }

    if (resolveRenderPass) {
        gfxRenderPassDestroy(resolveRenderPass);
        resolveRenderPass = nullptr;
    }
    if (clearRenderPass) {
        gfxRenderPassDestroy(clearRenderPass);
        clearRenderPass = nullptr;
    }
    if (renderPass) {
        gfxRenderPassDestroy(renderPass);
        renderPass = nullptr;
    }

    if (msaaColorTextureView) {
        gfxTextureViewDestroy(msaaColorTextureView);
        msaaColorTextureView = nullptr;
    }
    if (msaaColorTexture) {
        gfxTextureDestroy(msaaColorTexture);
        msaaColorTexture = nullptr;
    }
    if (depthTextureView) {
        gfxTextureViewDestroy(depthTextureView);
        depthTextureView = nullptr;
    }
    if (depthTexture) {
        gfxTextureDestroy(depthTexture);
        depthTexture = nullptr;
    }

    if (swapchain) {
        gfxSwapchainDestroy(swapchain);
        swapchain = nullptr;
    }
}

void CubeApp::cleanupRenderingResources()
{
    if (renderPipeline) {
        gfxRenderPipelineDestroy(renderPipeline);
        renderPipeline = nullptr;
    }
    if (fragmentShader) {
        gfxShaderDestroy(fragmentShader);
        fragmentShader = nullptr;
    }
    if (vertexShader) {
        gfxShaderDestroy(vertexShader);
        vertexShader = nullptr;
    }
    if (uniformBindGroupLayout) {
        gfxBindGroupLayoutDestroy(uniformBindGroupLayout);
        uniformBindGroupLayout = nullptr;
    }

    for (auto& frameBindGroups : uniformBindGroups) {
        for (auto& bindGroup : frameBindGroups) {
            if (bindGroup) {
                gfxBindGroupDestroy(bindGroup);
                bindGroup = nullptr;
            }
        }
    }

    if (sharedUniformBuffer) {
        gfxBufferDestroy(sharedUniformBuffer);
        sharedUniformBuffer = nullptr;
    }
    if (indexBuffer) {
        gfxBufferDestroy(indexBuffer);
        indexBuffer = nullptr;
    }
    if (vertexBuffer) {
        gfxBufferDestroy(vertexBuffer);
        vertexBuffer = nullptr;
    }
}

bool CubeApp::createGeometry()
{
    // Cube vertices
    Vertex vertices[] = {
        // Front face
        { { -1.0f, -1.0f, 1.0f }, { 1.0f, 0.0f, 0.0f } },
        { { 1.0f, -1.0f, 1.0f }, { 0.0f, 1.0f, 0.0f } },
        { { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 1.0f } },
        { { -1.0f, 1.0f, 1.0f }, { 1.0f, 1.0f, 0.0f } },
        // Back face
        { { -1.0f, -1.0f, -1.0f }, { 1.0f, 0.0f, 1.0f } },
        { { 1.0f, -1.0f, -1.0f }, { 0.0f, 1.0f, 1.0f } },
        { { 1.0f, 1.0f, -1.0f }, { 1.0f, 1.0f, 1.0f } },
        { { -1.0f, 1.0f, -1.0f }, { 0.5f, 0.5f, 0.5f } }
    };

    // Cube indices
    uint16_t indices[] = {
        0, 1, 2, 2, 3, 0, // Front
        5, 4, 7, 7, 6, 5, // Back
        4, 0, 3, 3, 7, 4, // Left
        1, 5, 6, 6, 2, 1, // Right
        3, 2, 6, 6, 7, 3, // Top
        4, 5, 1, 1, 0, 4 // Bottom
    };

    // Create vertex buffer
    GfxBufferDescriptor vertexBufferDesc = {
        .label = "Cube Vertices",
        .size = sizeof(vertices),
        .usage = static_cast<GfxBufferUsage>(GFX_BUFFER_USAGE_VERTEX | GFX_BUFFER_USAGE_COPY_DST)
    };

    if (gfxDeviceCreateBuffer(device, &vertexBufferDesc, &vertexBuffer) != GFX_RESULT_SUCCESS) {
        std::cerr << "Failed to create vertex buffer\n";
        return false;
    }

    // Create index buffer
    GfxBufferDescriptor indexBufferDesc = {
        .label = "Cube Indices",
        .size = sizeof(indices),
        .usage = static_cast<GfxBufferUsage>(GFX_BUFFER_USAGE_INDEX | GFX_BUFFER_USAGE_COPY_DST)
    };

    if (gfxDeviceCreateBuffer(device, &indexBufferDesc, &indexBuffer) != GFX_RESULT_SUCCESS) {
        std::cerr << "Failed to create index buffer\n";
        return false;
    }

    // Upload data
    gfxQueueWriteBuffer(queue, vertexBuffer, 0, vertices, sizeof(vertices));
    gfxQueueWriteBuffer(queue, indexBuffer, 0, indices, sizeof(indices));

    return true;
}

bool CubeApp::createUniformBuffer()
{
    GfxDeviceLimits limits;
    if (gfxDeviceGetLimits(device, &limits) != GFX_RESULT_SUCCESS) {
        std::cerr << "Failed to get device limits" << std::endl;
        return false;
    }

    size_t uniformSize = sizeof(UniformData);
    uniformAlignedSize = gfxAlignUp(uniformSize, limits.minUniformBufferOffsetAlignment);
    size_t totalBufferSize = uniformAlignedSize * MAX_FRAMES_IN_FLIGHT * CUBE_COUNT;

    GfxBufferDescriptor uniformBufferDesc = {
        .label = "Shared Transform Uniforms",
        .size = totalBufferSize,
        .usage = static_cast<GfxBufferUsage>(GFX_BUFFER_USAGE_UNIFORM | GFX_BUFFER_USAGE_COPY_DST)
    };

    if (gfxDeviceCreateBuffer(device, &uniformBufferDesc, &sharedUniformBuffer) != GFX_RESULT_SUCCESS) {
        std::cerr << "Failed to create shared uniform buffer\n";
        return false;
    }

    return true;
}

bool CubeApp::createBindGroup()
{
    // Create bind group layout
    GfxBindGroupLayoutEntry uniformLayoutEntry = {
        .binding = 0,
        .visibility = GFX_SHADER_STAGE_VERTEX,
        .type = GFX_BINDING_TYPE_BUFFER,
        .buffer = {
            .hasDynamicOffset = false,
            .minBindingSize = sizeof(UniformData) }
    };

    GfxBindGroupLayoutDescriptor uniformLayoutDesc = {
        .label = "Uniform Bind Group Layout",
        .entries = &uniformLayoutEntry,
        .entryCount = 1
    };

    if (gfxDeviceCreateBindGroupLayout(device, &uniformLayoutDesc, &uniformBindGroupLayout) != GFX_RESULT_SUCCESS) {
        std::cerr << "Failed to create uniform bind group layout\n";
        return false;
    }

    // Create bind groups
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        for (size_t cubeIdx = 0; cubeIdx < CUBE_COUNT; ++cubeIdx) {
            std::string labelStr = std::format("Uniform Bind Group Frame {} Cube {}", i, cubeIdx);
            const char* label = labelStr.c_str();

            GfxBindGroupEntry uniformEntry = {
                .binding = 0,
                .type = GFX_BIND_GROUP_ENTRY_TYPE_BUFFER,
                .resource = {
                    .buffer = {
                        .buffer = sharedUniformBuffer,
                        .offset = (i * CUBE_COUNT + cubeIdx) * uniformAlignedSize,
                        .size = sizeof(UniformData) } }
            };

            GfxBindGroupDescriptor uniformBindGroupDesc = {
                .label = label,
                .layout = uniformBindGroupLayout,
                .entries = &uniformEntry,
                .entryCount = 1
            };

            if (gfxDeviceCreateBindGroup(device, &uniformBindGroupDesc, &uniformBindGroups[i][cubeIdx]) != GFX_RESULT_SUCCESS) {
                std::cerr << std::format("Failed to create uniform bind group {} cube {}\n", i, cubeIdx);
                return false;
            }
        }
    }

    return true;
}

bool CubeApp::createShaders()
{
    size_t vertexShaderSize, fragmentShaderSize;
    void* vertexShaderCode = nullptr;
    void* fragmentShaderCode = nullptr;

    GfxShaderSourceType sourceType;
    if (adapterInfo.backend == GFX_BACKEND_WEBGPU) {
        sourceType = GFX_SHADER_SOURCE_WGSL;
        std::cout << "Loading WGSL shaders...\n";
        vertexShaderCode = loadTextFile("shaders/cube.vert.wgsl", &vertexShaderSize);
        fragmentShaderCode = loadTextFile("shaders/cube.frag.wgsl", &fragmentShaderSize);
        if (!vertexShaderCode || !fragmentShaderCode) {
            std::cerr << "Failed to load WGSL shaders\n";
            return false;
        }
    } else {
        sourceType = GFX_SHADER_SOURCE_SPIRV;
        std::cout << "Loading SPIR-V shaders...\n";
        vertexShaderCode = loadBinaryFile("cube.vert.spv", &vertexShaderSize);
        fragmentShaderCode = loadBinaryFile("cube.frag.spv", &fragmentShaderSize);
        if (!vertexShaderCode || !fragmentShaderCode) {
            std::cerr << "Failed to load SPIR-V shaders\n";
            return false;
        }
    }

    // Create vertex shader
    GfxShaderDescriptor vertexShaderDesc = {
        .label = "Cube Vertex Shader",
        .sourceType = sourceType,
        .code = vertexShaderCode,
        .codeSize = vertexShaderSize,
        .entryPoint = "main"
    };

    if (gfxDeviceCreateShader(device, &vertexShaderDesc, &vertexShader) != GFX_RESULT_SUCCESS) {
        std::cerr << "Failed to create vertex shader\n";
        free(vertexShaderCode);
        free(fragmentShaderCode);
        return false;
    }

    // Create fragment shader
    GfxShaderDescriptor fragmentShaderDesc = {
        .label = "Cube Fragment Shader",
        .sourceType = sourceType,
        .code = fragmentShaderCode,
        .codeSize = fragmentShaderSize,
        .entryPoint = "main"
    };

    if (gfxDeviceCreateShader(device, &fragmentShaderDesc, &fragmentShader) != GFX_RESULT_SUCCESS) {
        std::cerr << "Failed to create fragment shader\n";
        free(vertexShaderCode);
        free(fragmentShaderCode);
        return false;
    }

    free(vertexShaderCode);
    free(fragmentShaderCode);
    return true;
}

bool CubeApp::createRenderPipeline()
{
    // Define vertex attributes
    GfxVertexAttribute attributes[] = {
        { .format = GFX_TEXTURE_FORMAT_R32G32B32_FLOAT,
            .offset = offsetof(Vertex, position),
            .shaderLocation = 0 },
        { .format = GFX_TEXTURE_FORMAT_R32G32B32_FLOAT,
            .offset = offsetof(Vertex, color),
            .shaderLocation = 1 }
    };

    GfxVertexBufferLayout vertexBufferLayout = {
        .arrayStride = sizeof(Vertex),
        .attributes = attributes,
        .attributeCount = 2,
        .stepModeInstance = false
    };

    GfxVertexState vertexState = {
        .module = vertexShader,
        .entryPoint = "main",
        .buffers = &vertexBufferLayout,
        .bufferCount = 1
    };

    GfxColorTargetState colorTarget = {
        .format = swapchainInfo.format,
        .blend = nullptr,
        .writeMask = GFX_COLOR_WRITE_MASK_ALL
    };

    GfxFragmentState fragmentState = {
        .module = fragmentShader,
        .entryPoint = "main",
        .targets = &colorTarget,
        .targetCount = 1
    };

    GfxPrimitiveState primitiveState = {
        .topology = GFX_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        .stripIndexFormat = nullptr,
        .frontFace = GFX_FRONT_FACE_COUNTER_CLOCKWISE,
        .cullMode = GFX_CULL_MODE_BACK,
        .polygonMode = GFX_POLYGON_MODE_FILL
    };

    GfxDepthStencilState depthStencilState = {
        .format = DEPTH_FORMAT,
        .depthWriteEnabled = true,
        .depthCompare = GFX_COMPARE_FUNCTION_LESS,
        .stencilFront = {
            .compare = GFX_COMPARE_FUNCTION_ALWAYS,
            .failOp = GFX_STENCIL_OPERATION_KEEP,
            .depthFailOp = GFX_STENCIL_OPERATION_KEEP,
            .passOp = GFX_STENCIL_OPERATION_KEEP },
        .stencilBack = { .compare = GFX_COMPARE_FUNCTION_ALWAYS, .failOp = GFX_STENCIL_OPERATION_KEEP, .depthFailOp = GFX_STENCIL_OPERATION_KEEP, .passOp = GFX_STENCIL_OPERATION_KEEP },
        .stencilReadMask = 0xFF,
        .stencilWriteMask = 0xFF,
        .depthBias = 0,
        .depthBiasSlopeScale = 0.0f,
        .depthBiasClamp = 0.0f
    };

    GfxBindGroupLayout bindGroupLayouts[] = { uniformBindGroupLayout };

    GfxRenderPipelineDescriptor pipelineDesc = {};
    pipelineDesc.label = "Cube Render Pipeline";
    pipelineDesc.vertex = &vertexState;
    pipelineDesc.fragment = &fragmentState;
    pipelineDesc.primitive = &primitiveState;
    pipelineDesc.depthStencil = &depthStencilState;
    pipelineDesc.sampleCount = MSAA_SAMPLE_COUNT;
    pipelineDesc.renderPass = renderPass;
    pipelineDesc.bindGroupLayouts = bindGroupLayouts;
    pipelineDesc.bindGroupLayoutCount = 1;

    if (gfxDeviceCreateRenderPipeline(device, &pipelineDesc, &renderPipeline) != GFX_RESULT_SUCCESS) {
        std::cerr << "Failed to create render pipeline\n";
        return false;
    }

    return true;
}

bool CubeApp::createSyncObjects()
{
    GfxSemaphoreDescriptor semaphoreDesc = {
        .label = "Semaphore",
        .type = GFX_SEMAPHORE_TYPE_BINARY,
        .initialValue = 0
    };

    GfxFenceDescriptor fenceDesc = {
        .label = "Fence",
        .signaled = true
    };

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        std::string labelStr;
        
        labelStr = std::format("Image Available Semaphore {}", i);
        semaphoreDesc.label = labelStr.c_str();
        if (gfxDeviceCreateSemaphore(device, &semaphoreDesc, &imageAvailableSemaphores[i]) != GFX_RESULT_SUCCESS) {
            std::cerr << std::format("Failed to create image available semaphore {}\n", i);
            return false;
        }
        
        labelStr = std::format("Clear Finished Semaphore {}", i);
        semaphoreDesc.label = labelStr.c_str();
        if (gfxDeviceCreateSemaphore(device, &semaphoreDesc, &clearFinishedSemaphores[i]) != GFX_RESULT_SUCCESS) {
            std::cerr << std::format("Failed to create clear finished semaphore {}\n", i);
            return false;
        }
        
        labelStr = std::format("Render Finished Semaphore {}", i);
        semaphoreDesc.label = labelStr.c_str();
        if (gfxDeviceCreateSemaphore(device, &semaphoreDesc, &renderFinishedSemaphores[i]) != GFX_RESULT_SUCCESS) {
            std::cerr << std::format("Failed to create render finished semaphore {}\n", i);
            return false;
        }
        
        labelStr = std::format("In Flight Fence {}", i);
        fenceDesc.label = labelStr.c_str();
        if (gfxDeviceCreateFence(device, &fenceDesc, &inFlightFences[i]) != GFX_RESULT_SUCCESS) {
            std::cerr << std::format("Failed to create in flight fence {}\n", i);
            return false;
        }
        
        // Create clear encoder
        labelStr = std::format("Clear Encoder Frame {}", i);
        GfxCommandEncoderDescriptor encoderDesc = { .label = labelStr.c_str() };
        if (gfxDeviceCreateCommandEncoder(device, &encoderDesc, &clearEncoders[i]) != GFX_RESULT_SUCCESS) {
            std::cerr << std::format("Failed to create clear encoder {}\n", i);
            return false;
        }
        
        // Create cube encoders
        for (size_t cubeIdx = 0; cubeIdx < CUBE_COUNT; ++cubeIdx) {
            labelStr = std::format("Command Encoder Frame {} Cube {}", i, cubeIdx);
            GfxCommandEncoderDescriptor encoderDesc = { .label = labelStr.c_str() };
            if (gfxDeviceCreateCommandEncoder(device, &encoderDesc, &cubeEncoders[i][cubeIdx]) != GFX_RESULT_SUCCESS) {
                std::cerr << std::format("Failed to create command encoder {} cube {}\n", i, cubeIdx);
                return false;
            }
        }
        
        // Create resolve encoder
        labelStr = std::format("Resolve Encoder Frame {}", i);
        GfxCommandEncoderDescriptor resolveEncoderDesc = { .label = labelStr.c_str() };
        if (gfxDeviceCreateCommandEncoder(device, &resolveEncoderDesc, &resolveEncoders[i]) != GFX_RESULT_SUCCESS) {
            std::cerr << std::format("Failed to create resolve encoder {}\n", i);
            return false;
        }
    }

    return true;
}

bool CubeApp::createRenderingResources()
{
    std::cout << "[DEBUG] createRenderingResources called" << std::endl;

    if (!createGeometry())
        return false;
    if (!createUniformBuffer())
        return false;
    if (!createBindGroup())
        return false;
    if (!createShaders())
        return false;

    return true;
}

void CubeApp::updateCube(int cubeIndex)
{
    UniformData uniforms = {};

    // Create rotation matrices
    float rotX[16], rotY[16], tempModel[16];
    matrixRotateX(rotX, (rotationAngleX + cubeIndex * 30.0f) * M_PI / 180.0f);
    matrixRotateY(rotY, (rotationAngleY + cubeIndex * 45.0f) * M_PI / 180.0f);
    matrixMultiply(tempModel, rotY, rotX);

    // Position cubes
    float translation[16];
    matrixIdentity(translation);
    translation[12] = -(float)CUBE_COUNT * 0.5f + (cubeIndex - 1) * 1.5f;

    matrixMultiply(uniforms.model, tempModel, translation);

    // Create view matrix
    matrixLookAt(uniforms.view,
        0.0f, 0.0f, 10.0f,
        0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f);

    // Create perspective projection matrix
    float aspect = (float)swapchainInfo.width / (float)swapchainInfo.height;
    matrixPerspective(uniforms.projection,
        45.0f * M_PI / 180.0f,
        aspect,
        0.1f,
        100.0f,
        adapterInfo.backend);

    // Upload uniform data
    size_t offset = (currentFrame * CUBE_COUNT + cubeIndex) * uniformAlignedSize;
    gfxQueueWriteBuffer(queue, sharedUniformBuffer, offset, &uniforms, sizeof(uniforms));
}

void CubeApp::update(float deltaTime)
{
    rotationAngleX += 45.0f * deltaTime;
    rotationAngleY += 30.0f * deltaTime;
    if (rotationAngleX >= 360.0f)
        rotationAngleX -= 360.0f;
    if (rotationAngleY >= 360.0f)
        rotationAngleY -= 360.0f;

    // Update uniforms for each cube
    for (int i = 0; i < CUBE_COUNT; ++i) {
        updateCube(i);
    }
}

void CubeApp::recordClearCommands(uint32_t imageIndex)
{
    GfxCommandEncoder encoder = clearEncoders[currentFrame];
    gfxCommandEncoderBegin(encoder);

    GfxColor clearColor = { 0.1f, 0.2f, 0.3f, 1.0f };

    GfxRenderPassBeginDescriptor beginDesc = {
        .label = "Clear Pass",
        .renderPass = clearRenderPass,
        .framebuffer = framebuffers[imageIndex],
        .colorClearValues = &clearColor,
        .colorClearValueCount = 1,
        .depthClearValue = 1.0f,
        .stencilClearValue = 0
    };

    GfxRenderPassEncoder renderPass;
    if (gfxCommandEncoderBeginRenderPass(encoder, &beginDesc, &renderPass) == GFX_RESULT_SUCCESS) {
        gfxRenderPassEncoderEnd(renderPass);
    }

    gfxCommandEncoderEnd(encoder);
}

void CubeApp::recordCubeCommands(int cubeIndex, uint32_t imageIndex)
{
    GfxCommandEncoder encoder = cubeEncoders[currentFrame][cubeIndex];
    gfxCommandEncoderBegin(encoder);

    GfxColor clearColor = { 0.1f, 0.2f, 0.3f, 1.0f };

    GfxRenderPassBeginDescriptor beginDesc = {
        .label = "Main Render Pass",
        .renderPass = renderPass,
        .framebuffer = framebuffers[imageIndex],
        .colorClearValues = &clearColor,
        .colorClearValueCount = 1,
        .depthClearValue = 1.0f,
        .stencilClearValue = 0
    };

    GfxRenderPassEncoder renderPass;
    if (gfxCommandEncoderBeginRenderPass(encoder, &beginDesc, &renderPass) == GFX_RESULT_SUCCESS) {
        gfxRenderPassEncoderSetPipeline(renderPass, renderPipeline);

        GfxViewport viewport = { 0.0f, 0.0f, (float)swapchainInfo.width, (float)swapchainInfo.height, 0.0f, 1.0f };
        GfxScissorRect scissor = { 0, 0, swapchainInfo.width, swapchainInfo.height };
        gfxRenderPassEncoderSetViewport(renderPass, &viewport);
        gfxRenderPassEncoderSetScissorRect(renderPass, &scissor);

        GfxBufferInfo vertexBufferInfo;
        if (gfxBufferGetInfo(vertexBuffer, &vertexBufferInfo) == GFX_RESULT_SUCCESS) {
            gfxRenderPassEncoderSetVertexBuffer(renderPass, 0, vertexBuffer, 0, vertexBufferInfo.size);
        }

        GfxBufferInfo indexBufferInfo;
        if (gfxBufferGetInfo(indexBuffer, &indexBufferInfo) == GFX_RESULT_SUCCESS) {
            gfxRenderPassEncoderSetIndexBuffer(renderPass, indexBuffer, GFX_INDEX_FORMAT_UINT16, 0, indexBufferInfo.size);
        }

        gfxRenderPassEncoderSetBindGroup(renderPass, 0, uniformBindGroups[currentFrame][cubeIndex], nullptr, 0);
        gfxRenderPassEncoderDrawIndexed(renderPass, 36, 1, 0, 0, 0);

        gfxRenderPassEncoderEnd(renderPass);
    }

    gfxCommandEncoderEnd(encoder);
}

void CubeApp::recordResolveCommands(uint32_t imageIndex)
{
    GfxCommandEncoder encoder = resolveEncoders[currentFrame];
    gfxCommandEncoderBegin(encoder);

    GfxRenderPassBeginDescriptor beginDesc = {
        .label = "Final Resolve Pass",
        .renderPass = resolveRenderPass,
        .framebuffer = framebuffers[imageIndex],
        .colorClearValues = nullptr,
        .colorClearValueCount = 0,
        .depthClearValue = 1.0f,
        .stencilClearValue = 0
    };

    GfxRenderPassEncoder renderPass;
    if (gfxCommandEncoderBeginRenderPass(encoder, &beginDesc, &renderPass) == GFX_RESULT_SUCCESS) {
        gfxRenderPassEncoderEnd(renderPass);
    }

    gfxCommandEncoderEnd(encoder);
}

void CubeApp::render()
{
    gfxFenceWait(inFlightFences[currentFrame], UINT64_MAX);
    gfxFenceReset(inFlightFences[currentFrame]);

    uint32_t imageIndex;
    GfxResult result = gfxSwapchainAcquireNextImage(swapchain, UINT64_MAX,
        imageAvailableSemaphores[currentFrame], nullptr, &imageIndex);

    if (result != GFX_RESULT_SUCCESS) {
        std::cerr << "Failed to acquire swapchain image\n";
        return;
    }

    recordClearCommands(imageIndex);

    if constexpr (USE_THREADING) {
        // Store image index for threads
        currentImageIndex.store(imageIndex);

        // Record cube commands in parallel using ThreadPool
        std::vector<std::future<void>> futures;
        for (int cubeIdx = 0; cubeIdx < CUBE_COUNT; ++cubeIdx) {
            futures.push_back(threadPool->Enqueue([this, cubeIdx, imageIndex]() {
                recordCubeCommands(cubeIdx, imageIndex);
            }));
        }

        // Wait for all threads to finish
        for (auto& future : futures) {
            future.get();
        }

        // Submit clear encoder
        GfxSubmitDescriptor clearSubmit = {
            .commandEncoders = &clearEncoders[currentFrame],
            .commandEncoderCount = 1,
            .waitSemaphores = &imageAvailableSemaphores[currentFrame],
            .waitSemaphoreCount = 1,
            .signalSemaphores = &clearFinishedSemaphores[currentFrame],
            .signalSemaphoreCount = 1,
            .signalFence = nullptr
        };
        gfxQueueSubmit(queue, &clearSubmit);

        // Submit cube encoders
        std::vector<GfxCommandEncoder> cubeEncoderArray(CUBE_COUNT);
        for (int i = 0; i < CUBE_COUNT; ++i) {
            cubeEncoderArray[i] = cubeEncoders[currentFrame][i];
        }

        GfxSubmitDescriptor cubesSubmit = {
            .commandEncoders = cubeEncoderArray.data(),
            .commandEncoderCount = static_cast<uint32_t>(cubeEncoderArray.size()),
            .waitSemaphores = &clearFinishedSemaphores[currentFrame],
            .waitSemaphoreCount = 1,
            .signalSemaphores = nullptr,
            .signalSemaphoreCount = 0,
            .signalFence = nullptr
        };
        gfxQueueSubmit(queue, &cubesSubmit);

        // Submit resolve encoder
        recordResolveCommands(imageIndex);

        GfxSubmitDescriptor resolveSubmit = {
            .commandEncoders = &resolveEncoders[currentFrame],
            .commandEncoderCount = 1,
            .waitSemaphores = nullptr,
            .waitSemaphoreCount = 0,
            .signalSemaphores = &renderFinishedSemaphores[currentFrame],
            .signalSemaphoreCount = 1,
            .signalFence = inFlightFences[currentFrame]
        };
        gfxQueueSubmit(queue, &resolveSubmit);
    } else {
        // Non-threaded path for WebGPU
        GfxCommandEncoder encoder = cubeEncoders[currentFrame][0];
        gfxCommandEncoderBegin(encoder);

        GfxColor clearColor = { 0.1f, 0.2f, 0.3f, 1.0f };
        GfxRenderPassBeginDescriptor beginDesc = {
            .label = "Main Render Pass (All Cubes)",
            .renderPass = clearRenderPass,
            .framebuffer = framebuffers[imageIndex],
            .colorClearValues = &clearColor,
            .colorClearValueCount = 1,
            .depthClearValue = 1.0f,
            .stencilClearValue = 0
        };

        GfxRenderPassEncoder renderPass;
        if (gfxCommandEncoderBeginRenderPass(encoder, &beginDesc, &renderPass) == GFX_RESULT_SUCCESS) {
            gfxRenderPassEncoderSetPipeline(renderPass, renderPipeline);

            GfxViewport viewport = { 0.0f, 0.0f, (float)swapchainInfo.width, (float)swapchainInfo.height, 0.0f, 1.0f };
            GfxScissorRect scissor = { 0, 0, swapchainInfo.width, swapchainInfo.height };
            gfxRenderPassEncoderSetViewport(renderPass, &viewport);
            gfxRenderPassEncoderSetScissorRect(renderPass, &scissor);

            GfxBufferInfo vertexBufferInfo;
            if (gfxBufferGetInfo(vertexBuffer, &vertexBufferInfo) == GFX_RESULT_SUCCESS) {
                gfxRenderPassEncoderSetVertexBuffer(renderPass, 0, vertexBuffer, 0, vertexBufferInfo.size);
            }

            GfxBufferInfo indexBufferInfo;
            if (gfxBufferGetInfo(indexBuffer, &indexBufferInfo) == GFX_RESULT_SUCCESS) {
                gfxRenderPassEncoderSetIndexBuffer(renderPass, indexBuffer, GFX_INDEX_FORMAT_UINT16, 0, indexBufferInfo.size);
            }

            for (int cubeIdx = 0; cubeIdx < CUBE_COUNT; ++cubeIdx) {
                gfxRenderPassEncoderSetBindGroup(renderPass, 0, uniformBindGroups[currentFrame][cubeIdx], nullptr, 0);
                gfxRenderPassEncoderDrawIndexed(renderPass, 36, 1, 0, 0, 0);
            }

            gfxRenderPassEncoderEnd(renderPass);
        }

        gfxCommandEncoderEnd(encoder);

        GfxSubmitDescriptor submitInfo = {
            .commandEncoders = &encoder,
            .commandEncoderCount = 1,
            .waitSemaphores = &imageAvailableSemaphores[currentFrame],
            .waitSemaphoreCount = 1,
            .signalSemaphores = &renderFinishedSemaphores[currentFrame],
            .signalSemaphoreCount = 1,
            .signalFence = inFlightFences[currentFrame]
        };
        gfxQueueSubmit(queue, &submitInfo);
    }

    // Present
    GfxPresentInfo presentInfo = {
        .waitSemaphores = &renderFinishedSemaphores[currentFrame],
        .waitSemaphoreCount = 1
    };
    gfxSwapchainPresent(swapchain, &presentInfo);

    currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

bool CubeApp::mainLoopIteration()
{
    if (!window || glfwWindowShouldClose(window)) {
        return false;
    }

    glfwPollEvents();

    // Handle framebuffer resize
    if (previousWidth != windowWidth || previousHeight != windowHeight) {
        gfxDeviceWaitIdle(device);

        cleanupSizeDependentResources();
        if (!createSizeDependentResources(windowWidth, windowHeight)) {
            std::cerr << "Failed to recreate size-dependent resources after resize" << std::endl;
            return false;
        }

        previousWidth = windowWidth;
        previousHeight = windowHeight;

        std::cout << "Window resized: " << windowWidth << "x" << windowHeight << std::endl;
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

bool CubeApp::initialize()
{
    if (!initWindow())
        return false;
    if (!initializeGraphics())
        return false;
    if (!createSizeDependentResources(windowWidth, windowHeight))
        return false;
    if (!createRenderingResources())
        return false;
    if (!createSyncObjects())
        return false;
    if (!createRenderPipeline())
        return false;

    // Initialize thread pool if using threading
    if constexpr (USE_THREADING) {
        threadPool = std::make_unique<ThreadPool>(CUBE_COUNT);
        std::cout << "Created ThreadPool with " << CUBE_COUNT << " worker threads for parallel command recording" << std::endl;
    }

    previousWidth = windowWidth;
    previousHeight = windowHeight;
    lastTime = getCurrentTime();

    std::cout << "Application initialized successfully!" << std::endl;
    if constexpr (USE_THREADING) {
        std::cout << "Running with ThreadPool (" << CUBE_COUNT << " threads) for parallel command recording" << std::endl;
    } else {
        std::cout << "Running in single-threaded mode" << std::endl;
    }
    std::cout << "Press ESC to exit" << std::endl
              << std::endl;

    return true;
}

void CubeApp::run()
{
    while (mainLoopIteration()) {
        // Continue running
    }

    std::cout << "\nCleaning up resources..." << std::endl;
}

// Matrix math utility functions
void matrixIdentity(float* matrix)
{
    std::memset(matrix, 0, 16 * sizeof(float));
    matrix[0] = matrix[5] = matrix[10] = matrix[15] = 1.0f;
}

void matrixMultiply(float* result, const float* a, const float* b)
{
    float temp[16];
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            temp[i * 4 + j] = 0;
            for (int k = 0; k < 4; ++k) {
                temp[i * 4 + j] += a[i * 4 + k] * b[k * 4 + j];
            }
        }
    }
    std::memcpy(result, temp, sizeof(float) * 16);
}

void matrixRotateX(float* matrix, float angle)
{
    float c = std::cos(angle);
    float s = std::sin(angle);

    matrixIdentity(matrix);
    matrix[5] = c;
    matrix[6] = -s;
    matrix[9] = s;
    matrix[10] = c;
}

void matrixRotateY(float* matrix, float angle)
{
    float c = std::cos(angle);
    float s = std::sin(angle);

    matrixIdentity(matrix);
    matrix[0] = c;
    matrix[2] = s;
    matrix[8] = -s;
    matrix[10] = c;
}

void matrixRotateZ(float* matrix, float angle)
{
    float c = std::cos(angle);
    float s = std::sin(angle);

    matrixIdentity(matrix);
    matrix[0] = c;
    matrix[1] = -s;
    matrix[4] = s;
    matrix[5] = c;
}

void matrixPerspective(float* matrix, float fov, float aspect, float near, float far, GfxBackend backend)
{
    std::memset(matrix, 0, 16 * sizeof(float));

    float f = 1.0f / std::tan(fov / 2.0f);

    matrix[0] = f / aspect;
    if (backend == GFX_BACKEND_VULKAN) {
        matrix[5] = -f; // Invert Y for Vulkan
    } else {
        matrix[5] = f;
    }
    matrix[10] = (far + near) / (near - far);
    matrix[11] = -1.0f;
    matrix[14] = (2.0f * far * near) / (near - far);
}

void matrixLookAt(float* matrix, float eyeX, float eyeY, float eyeZ,
    float centerX, float centerY, float centerZ,
    float upX, float upY, float upZ)
{
    // Calculate forward vector
    float fx = centerX - eyeX;
    float fy = centerY - eyeY;
    float fz = centerZ - eyeZ;

    // Normalize forward vector
    if (!vectorNormalize(&fx, &fy, &fz)) {
        matrixIdentity(matrix);
        return;
    }

    // Calculate right vector (forward cross up)
    float rx = fy * upZ - fz * upY;
    float ry = fz * upX - fx * upZ;
    float rz = fx * upY - fy * upX;

    // Normalize right vector
    if (!vectorNormalize(&rx, &ry, &rz)) {
        matrixIdentity(matrix);
        return;
    }

    // Calculate up vector (right cross forward)
    float ux = ry * fz - rz * fy;
    float uy = rz * fx - rx * fz;
    float uz = rx * fy - ry * fx;

    // Build view matrix
    matrix[0] = rx;
    matrix[1] = ux;
    matrix[2] = -fx;
    matrix[3] = 0.0f;

    matrix[4] = ry;
    matrix[5] = uy;
    matrix[6] = -fy;
    matrix[7] = 0.0f;

    matrix[8] = rz;
    matrix[9] = uz;
    matrix[10] = -fz;
    matrix[11] = 0.0f;

    matrix[12] = -(rx * eyeX + ry * eyeY + rz * eyeZ);
    matrix[13] = -(ux * eyeX + uy * eyeY + uz * eyeZ);
    matrix[14] = fx * eyeX + fy * eyeY + fz * eyeZ;
    matrix[15] = 1.0f;
}

bool vectorNormalize(float* x, float* y, float* z)
{
    constexpr float epsilon = 1e-6f;
    float len = std::sqrt((*x) * (*x) + (*y) * (*y) + (*z) * (*z));

    if (len < epsilon) {
        return false;
    }

    *x /= len;
    *y /= len;
    *z /= len;
    return true;
}

// Helper function to load binary files (SPIR-V shaders)
static void* loadBinaryFile(const char* filepath, size_t* outSize)
{
    std::ifstream file(filepath, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        std::cerr << "Failed to open file: " << filepath << std::endl;
        return nullptr;
    }

    std::streamsize fileSize = file.tellg();
    if (fileSize <= 0) {
        std::cerr << "Invalid file size for: " << filepath << std::endl;
        return nullptr;
    }

    file.seekg(0, std::ios::beg);

    void* buffer = malloc(fileSize);
    if (!buffer) {
        std::cerr << "Failed to allocate memory for file: " << filepath << std::endl;
        return nullptr;
    }

    if (!file.read(static_cast<char*>(buffer), fileSize)) {
        std::cerr << "Failed to read complete file: " << filepath << std::endl;
        free(buffer);
        return nullptr;
    }

    *outSize = static_cast<size_t>(fileSize);
    return buffer;
}

static void* loadTextFile(const char* filepath, size_t* outSize)
{
    std::ifstream file(filepath, std::ios::ate);
    if (!file.is_open()) {
        std::cerr << "Failed to open file: " << filepath << std::endl;
        return nullptr;
    }

    std::streamsize fileSize = file.tellg();
    if (fileSize <= 0) {
        std::cerr << "Invalid file size for: " << filepath << std::endl;
        return nullptr;
    }

    file.seekg(0, std::ios::beg);

    char* buffer = static_cast<char*>(malloc(fileSize + 1));
    if (!buffer) {
        std::cerr << "Failed to allocate memory for file: " << filepath << std::endl;
        return nullptr;
    }

    if (!file.read(buffer, fileSize)) {
        std::cerr << "Failed to read complete file: " << filepath << std::endl;
        free(buffer);
        return nullptr;
    }

    buffer[fileSize] = '\0';
    *outSize = static_cast<size_t>(fileSize + 1);
    return buffer;
}

static float getCurrentTime()
{
#if defined(__EMSCRIPTEN__)
    return (float)emscripten_get_now() / 1000.0f;
#else
    return (float)glfwGetTime();
#endif
}

int main()
{
    std::cout << "=== Threaded Cube Example (C++ ThreadPool with C API) ===" << std::endl
              << std::endl;

    CubeApp app;

    if (!app.initialize()) {
        std::cerr << "Failed to initialize application" << std::endl;
        return -1;
    }

    app.run();

    std::cout << "Example completed successfully!" << std::endl;
    return 0;
}
