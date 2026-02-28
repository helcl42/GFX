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
#define GLFW_EXPOSE_NATIVE_WAYLAND
#elif defined(__APPLE__)
#define GLFW_EXPOSE_NATIVE_COCOA
#endif
#include <GLFW/glfw3native.h>
#endif

#include <algorithm>
#include <atomic>
#include <cfloat>
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
constexpr uint32_t CUBE_COUNT = 12;
constexpr GfxFormat COLOR_FORMAT = GFX_FORMAT_B8G8R8A8_UNORM_SRGB;
constexpr GfxFormat DEPTH_FORMAT = GFX_FORMAT_DEPTH32_FLOAT;

#if defined(__EMSCRIPTEN__)
constexpr bool USE_THREADING = false;
#else
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

// Log callback function
static void logCallback(GfxLogLevel level, const char* message, void* userData)
{
    (void)userData;
    const char* levelStr = "UNKNOWN";
    switch (level) {
    case GFX_LOG_LEVEL_ERROR:
        levelStr = "ERROR";
        break;
    case GFX_LOG_LEVEL_WARNING:
        levelStr = "WARNING";
        break;
    case GFX_LOG_LEVEL_INFO:
        levelStr = "INFO";
        break;
    case GFX_LOG_LEVEL_DEBUG:
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
    float position[3];
    float color[3];
};

// Uniform buffer structure for transformations
struct UniformData {
    std::array<std::array<float, 4>, 4> model; // Model matrix
    std::array<std::array<float, 4>, 4> view; // View matrix
    std::array<std::array<float, 4>, 4> projection; // Projection matrix
};

// Settings structure for command-line arguments
struct Settings {
    GfxBackend backend;
    GfxSampleCount msaaSampleCount;
    bool vsync;
};

// Per-frame resources for threaded rendering
struct PerFrameResources {
    // Synchronization
    GfxSemaphore imageAvailableSemaphore = nullptr;
    GfxSemaphore clearFinishedSemaphore = nullptr;
    GfxFence inFlightFence = nullptr;

    // Command encoders
    GfxCommandEncoder clearEncoder = nullptr;
    GfxCommandEncoder resolveEncoder = nullptr;
    GfxCommandEncoder transitionEncoder = nullptr; // For COLOR_ATTACHMENT->PRESENT_SRC (MSAA=1)
    std::vector<GfxCommandEncoder> cubeEncoders; // One per cube

    // Bind groups
    std::vector<GfxBindGroup> uniformBindGroups; // One per cube
};

// Utility namespace for file loading and other helpers
namespace util {
std::vector<uint8_t> loadBinaryFile(const char* filepath);
std::string loadTextFile(const char* filepath);
} // namespace util

// Math namespace for matrix and vector operations
namespace math {
void matrixIdentity(std::array<std::array<float, 4>, 4>& matrix);
void matrixPerspective(std::array<std::array<float, 4>, 4>& matrix, float fov, float aspect, float nearPlane, float farPlane, GfxBackend backend);
void matrixLookAt(std::array<std::array<float, 4>, 4>& matrix, float eyeX, float eyeY, float eyeZ, float centerX, float centerY, float centerZ, float upX, float upY, float upZ);
void matrixRotateX(std::array<std::array<float, 4>, 4>& matrix, float angle);
void matrixRotateY(std::array<std::array<float, 4>, 4>& matrix, float angle);
void matrixMultiply(std::array<std::array<float, 4>, 4>& result, const std::array<std::array<float, 4>, 4>& a, const std::array<std::array<float, 4>, 4>& b);
bool vectorNormalize(float& x, float& y, float& z);
} // namespace math

// Main application class
class CubeApp {
public:
    explicit CubeApp(const Settings& settings);
    ~CubeApp() = default;

    bool init();
    void run();
    void cleanup();

private:
    bool createWindow(uint32_t width, uint32_t height);
    void destroyWindow();
    bool createGraphics();
    void destroyGraphics();
    bool createSizeDependentResources(uint32_t width, uint32_t height);
    void destroySizeDependentResources();
    bool createSwapchain(uint32_t width, uint32_t height);
    void destroySwapchain();
    bool createTextures(uint32_t width, uint32_t height);
    void destroyTextures();
    bool createRenderPass();
    void destroyRenderPass();
    bool createFramebuffers(uint32_t width, uint32_t height);
    void destroyFramebuffers();
    bool createGeometry();
    void destroyGeometry();
    bool createUniformBuffer();
    void destroyUniformBuffer();
    bool createShaders();
    void destroyShaders();
    bool createRenderingResources();
    void destroyRenderingResources();
    bool createRenderPipeline();
    void destroyRenderPipeline();
    bool createPerFrameResources();
    void destroyPerFrameResources();

    // Main loop
    void updateCube(int cubeIndex);
    void update(float deltaTime);
    void recordClearCommands(uint32_t imageIndex);
    void recordCubeCommands(int cubeIndex, uint32_t imageIndex);
    void recordResolveCommands(uint32_t imageIndex);
    void recordLayoutTransition(uint32_t imageIndex);
    void render();
    float getCurrentTime();
    bool mainLoopIteration();
#if defined(__EMSCRIPTEN__)
    static void emscriptenMainLoop(void* userData);
#endif

    // Platform-specific
    GfxPlatformWindowHandle getPlatformWindowHandle();

    // Window callbacks
    static void errorCallback(int error, const char* description);
    static void framebufferSizeCallback(GLFWwindow* window, int width, int height);
    static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);

private:
    Settings settings;

    // Public for GLFW callbacks
    GLFWwindow* window = nullptr;
    uint32_t windowWidth = WINDOW_WIDTH;
    uint32_t windowHeight = WINDOW_HEIGHT;

    // Graphics resources
    GfxInstance instance = nullptr;
    GfxAdapter adapter = nullptr;
    GfxAdapterInfo adapterInfo = {};
    GfxDevice device = nullptr;
    GfxQueue queue = nullptr;
    GfxSurface surface = nullptr;
    GfxSwapchain swapchain = nullptr;
    GfxSwapchainInfo swapchainInfo = {};
    GfxSurfaceInfo surfaceInfo = {};
    uint32_t framesInFlight = 3; // Dynamic: set based on surface capabilities

    GfxBuffer vertexBuffer = nullptr;
    GfxBuffer indexBuffer = nullptr;
    GfxShader vertexShader = nullptr;
    GfxShader fragmentShader = nullptr;
    GfxRenderPass clearRenderPass = nullptr;
    GfxRenderPass renderPass = nullptr;
    GfxRenderPass transitionRenderPass = nullptr; // For layout transition (MSAA=1)
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

    // Per-frame resources
    std::vector<PerFrameResources> frameResources;
    uint32_t currentFrame = 0;

    // Per-swapchain-image semaphores
    std::vector<GfxSemaphore> renderFinishedSemaphores;

    // Animation state
    float rotationAngleX = 0.0f;
    float rotationAngleY = 0.0f;

    // Loop state
    uint32_t previousWidth = WINDOW_WIDTH;
    uint32_t previousHeight = WINDOW_HEIGHT;
    float lastTime = 0.0f;

    // FPS tracking
    uint32_t fpsFrameCount = 0;
    float fpsTimeAccumulator = 0.0f;
    float fpsFrameTimeMin = FLT_MAX;
    float fpsFrameTimeMax = 0.0f;

    // Threading
    std::unique_ptr<ThreadPool> threadPool;
    std::atomic<uint32_t> currentImageIndex{ 0 };
};

CubeApp::CubeApp(const Settings& settings)
    : settings(settings)
{
}

bool CubeApp::init()
{
    // 1. Create window
    if (!createWindow(windowWidth, windowHeight)) {
        return false;
    }

    // 2. Create graphics context
    if (!createGraphics()) {
        return false;
    }

    // 3. Create size-dependent resources
    if (!createSizeDependentResources(windowWidth, windowHeight)) {
        return false;
    }

    // 4. Create rendering resources (geometry, uniform buffer, shaders, pipeline)
    if (!createRenderingResources()) {
        return false;
    }

    // 5. Create per-frame resources (sync objects, encoders, bind groups)
    if (!createPerFrameResources()) {
        return false;
    }

    // Initialize thread pool if using threading
    if constexpr (USE_THREADING) {
        threadPool = std::make_unique<ThreadPool>(CUBE_COUNT);
        std::cout << "Created ThreadPool with " << CUBE_COUNT << " worker threads for parallel command recording" << std::endl;
    }

    previousWidth = windowWidth;
    previousHeight = windowHeight;
    lastTime = getCurrentTime();

    // Initialize FPS tracking
    fpsFrameCount = 0;
    fpsTimeAccumulator = 0.0f;
    fpsFrameTimeMin = FLT_MAX;
    fpsFrameTimeMax = 0.0f;

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
#if defined(__EMSCRIPTEN__)
    // Note: emscripten_set_main_loop_arg returns immediately and never blocks
    // Cleanup happens in emscriptenMainLoop when the loop exits
    // Execution continues in the browser event loop
    emscripten_set_main_loop_arg(CubeApp::emscriptenMainLoop, this, 0, 1);
#else
    while (mainLoopIteration()) {
        // Continue running
    }
#endif
}

void CubeApp::cleanup()
{
    // Wait for device to finish
    if (device) {
        gfxDeviceWaitIdle(device);
    }

    // Destroy threadPool before other resources
    threadPool.reset();

    // Destroy resources in reverse order of creation
    // 5. Destroy per-frame resources
    destroyPerFrameResources();

    // 4. Destroy size-dependent resources
    destroySizeDependentResources();

    // 3. Destroy rendering resources
    destroyRenderingResources();

    // 2. Destroy graphics resources
    destroyGraphics();

    // 1. Destroy window
    destroyWindow();
}

bool CubeApp::createWindow(uint32_t width, uint32_t height)
{
    glfwSetErrorCallback(errorCallback);

    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW\n";
        return false;
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

    std::string backendName = (settings.backend == GFX_BACKEND_VULKAN) ? "Vulkan" : "WebGPU";
    std::string threadingInfo = USE_THREADING ? " (Threaded) - Parallel Command Recording" : "";
    std::string title = "Cube Example (C++ ThreadPool) - " + backendName + threadingInfo;

    window = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);
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

void CubeApp::destroyWindow()
{
    if (window) {
        glfwDestroyWindow(window);
        window = nullptr;
    }
    glfwTerminate();
}

bool CubeApp::createGraphics()
{
    // Set up logging callback
    gfxSetLogCallback(logCallback, nullptr);

    auto result = gfxLoadBackend(settings.backend);
    if (result != GFX_RESULT_SUCCESS) {
        std::cerr << "Failed to load graphics backend: " << gfxResultToString(result) << std::endl;
        return false;
    }

    std::cout << "Loading graphics backend...\n";
    if (gfxLoadBackend(settings.backend) != GFX_RESULT_SUCCESS) {
        std::cerr << "Failed to load graphics backend\n";
        return false;
    }
    std::cout << "Graphics backend loaded successfully!\n";

    // Create instance
    const char* instanceExtensions[] = { GFX_INSTANCE_EXTENSION_SURFACE, GFX_INSTANCE_EXTENSION_DEBUG };
    GfxInstanceDescriptor instanceDesc = {
        .backend = settings.backend,
        .applicationName = "Cube Example (C++ ThreadPool)",
        .applicationVersion = 1,
        .enabledExtensions = instanceExtensions,
        .enabledExtensionCount = 2
    };

    if (gfxCreateInstance(&instanceDesc, &instance) != GFX_RESULT_SUCCESS) {
        std::cerr << "Failed to create graphics instance\n";
        return false;
    }

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
    const char* deviceExtensions[] = { GFX_DEVICE_EXTENSION_SWAPCHAIN };
    GfxDeviceDescriptor deviceDesc = {
        .label = "Main Device",
        .enabledExtensions = deviceExtensions,
        .enabledExtensionCount = 1
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

    // Query surface capabilities to determine frames in flight
    if (gfxSurfaceGetInfo(surface, &surfaceInfo) != GFX_RESULT_SUCCESS) {
        std::cerr << "Failed to get surface info\n";
        return false;
    }

    std::cout << "Surface Info:\n";
    std::cout << std::format("  Image Count: min {}, max {}\n",
        surfaceInfo.minImageCount, surfaceInfo.maxImageCount);
    std::cout << std::format("  Extent: min ({}x{}), max ({}x{})\n",
        surfaceInfo.minExtent.width, surfaceInfo.minExtent.height,
        surfaceInfo.maxExtent.width, surfaceInfo.maxExtent.height);

    // Calculate frames in flight based on surface capabilities
    // Use min image count, but clamp to reasonable values (2-4 is typical)
    framesInFlight = surfaceInfo.minImageCount;
    if (framesInFlight < 2) {
        framesInFlight = 2;
    }
    if (framesInFlight > 4) {
        framesInFlight = 4;
    }
    std::cout << std::format("Frames in flight: {}\n", framesInFlight);

    return true;
}

void CubeApp::destroyGraphics()
{
    if (surface) {
        gfxSurfaceDestroy(surface);
        surface = nullptr;
    }
    if (queue) {
        queue = nullptr; // Queue doesn't need explicit destruction
    }
    if (device) {
        gfxDeviceDestroy(device);
        device = nullptr;
    }
    if (adapter) {
        adapter = nullptr; // Adapter doesn't need explicit destruction
    }
    if (instance) {
        gfxInstanceDestroy(instance);
        instance = nullptr;
    }

    std::cout << "Unloading graphics backend..." << std::endl;
    gfxUnloadBackend(settings.backend);
}

bool CubeApp::createSwapchain(uint32_t width, uint32_t height)
{
    GfxSwapchainDescriptor swapchainDesc = {
        .label = "Main Swapchain",
        .surface = surface,
        .extent = { width, height },
        .format = COLOR_FORMAT,
        .usage = GFX_TEXTURE_USAGE_RENDER_ATTACHMENT,
        .presentMode = settings.vsync ? GFX_PRESENT_MODE_FIFO : GFX_PRESENT_MODE_IMMEDIATE,
        .imageCount = framesInFlight
    };

    if (gfxDeviceCreateSwapchain(device, &swapchainDesc, &swapchain) != GFX_RESULT_SUCCESS) {
        std::cerr << "Failed to create swapchain" << std::endl;
        return false;
    }

    if (gfxSwapchainGetInfo(swapchain, &swapchainInfo) != GFX_RESULT_SUCCESS) {
        std::cerr << "Failed to get swapchain info" << std::endl;
        return false;
    }

    std::cout << "Swapchain created: " << swapchainInfo.extent.width << "x" << swapchainInfo.extent.height
              << ", format: " << swapchainInfo.format << std::endl;

    // Create render finished semaphores (one per swapchain image)
    renderFinishedSemaphores.resize(swapchainInfo.imageCount, nullptr);
    GfxSemaphoreDescriptor semaphoreDesc = {
        .sType = GFX_STRUCTURE_TYPE_SEMAPHORE_DESCRIPTOR,
        .pNext = nullptr,
        .type = GFX_SEMAPHORE_TYPE_BINARY
    };

    for (uint32_t i = 0; i < swapchainInfo.imageCount; ++i) {
        std::string labelStr = std::format("Render Finished Semaphore Image {}", i);
        semaphoreDesc.label = labelStr.c_str();
        if (gfxDeviceCreateSemaphore(device, &semaphoreDesc, &renderFinishedSemaphores[i]) != GFX_RESULT_SUCCESS) {
            std::cerr << std::format("Failed to create render finished semaphore {}\n", i);
            return false;
        }
    }

    return true;
}

void CubeApp::destroySwapchain()
{
    // Clean up render finished semaphores
    for (auto& semaphore : renderFinishedSemaphores) {
        if (semaphore) {
            gfxSemaphoreDestroy(semaphore);
            semaphore = nullptr;
        }
    }
    renderFinishedSemaphores.clear();

    if (swapchain) {
        gfxSwapchainDestroy(swapchain);
        swapchain = nullptr;
    }
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
        .sampleCount = settings.msaaSampleCount,
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
        .sampleCount = settings.msaaSampleCount,
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

void CubeApp::destroyTextures()
{
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
}

bool CubeApp::createFramebuffers(uint32_t width, uint32_t height)
{
    // Resize framebuffers vector to match swapchain image count
    framebuffers.resize(swapchainInfo.imageCount, nullptr);

    for (uint32_t i = 0; i < swapchainInfo.imageCount; ++i) {
        GfxTextureView backbuffer = nullptr;
        if (gfxSwapchainGetTextureView(swapchain, i, &backbuffer) != GFX_RESULT_SUCCESS || !backbuffer) {
            std::cerr << std::format("Failed to get swapchain image view {}\n", i);
            return false;
        }

        GfxFramebufferAttachment fbColorAttachment = {
            .view = (settings.msaaSampleCount > GFX_SAMPLE_COUNT_1) ? msaaColorTextureView : backbuffer,
            .resolveTarget = (settings.msaaSampleCount > GFX_SAMPLE_COUNT_1) ? backbuffer : nullptr
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
            .extent = { width, height }
        };

        if (gfxDeviceCreateFramebuffer(device, &fbDesc, &framebuffers[i]) != GFX_RESULT_SUCCESS) {
            std::cerr << std::format("Failed to create framebuffer {}\n", i);
            return false;
        }
    }

    return true;
}

void CubeApp::destroyFramebuffers()
{
    for (auto& fb : framebuffers) {
        if (fb) {
            gfxFramebufferDestroy(fb);
            fb = nullptr;
        }
    }
}

bool CubeApp::createRenderPass()
{
    // Clear pass target
    GfxRenderPassColorAttachmentTarget clearColorTarget = {
        .format = swapchainInfo.format,
        .sampleCount = settings.msaaSampleCount,
        .ops = { .loadOp = GFX_LOAD_OP_CLEAR, .storeOp = GFX_STORE_OP_STORE },
        .finalLayout = GFX_TEXTURE_LAYOUT_COLOR_ATTACHMENT // Always COLOR_ATTACHMENT so subsequent passes can LOAD
    };

    // Load pass target
    GfxRenderPassColorAttachmentTarget colorTarget = {
        .format = swapchainInfo.format,
        .sampleCount = settings.msaaSampleCount,
        .ops = { .loadOp = GFX_LOAD_OP_LOAD, .storeOp = GFX_STORE_OP_STORE },
        .finalLayout = GFX_TEXTURE_LAYOUT_COLOR_ATTACHMENT // Keep in COLOR_ATTACHMENT (renderPassFinal handles PRESENT_SRC)
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
        .sampleCount = settings.msaaSampleCount,
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
        .resolveTarget = (settings.msaaSampleCount > GFX_SAMPLE_COUNT_1) ? &dummyResolveTarget : nullptr
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
        .resolveTarget = (settings.msaaSampleCount > GFX_SAMPLE_COUNT_1) ? &dummyResolveTarget : nullptr
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

    // Transition render pass (MSAA=1: COLOR_ATTACHMENT->PRESENT_SRC)
    if (settings.msaaSampleCount == GFX_SAMPLE_COUNT_1) {
        GfxRenderPassColorAttachmentTarget transitionColorTarget = {
            .format = swapchainInfo.format,
            .sampleCount = settings.msaaSampleCount,
            .ops = { .loadOp = GFX_LOAD_OP_LOAD, .storeOp = GFX_STORE_OP_STORE },
            .finalLayout = GFX_TEXTURE_LAYOUT_PRESENT_SRC
        };

        GfxRenderPassColorAttachment transitionColorAttachment = {
            .target = transitionColorTarget,
            .resolveTarget = nullptr
        };

        // Depth attachment for framebuffer compatibility (not actually used)
        GfxRenderPassDepthStencilAttachmentTarget transitionDepthTarget = {
            .format = DEPTH_FORMAT,
            .sampleCount = settings.msaaSampleCount,
            .depthOps = { .loadOp = GFX_LOAD_OP_DONT_CARE, .storeOp = GFX_STORE_OP_DONT_CARE },
            .stencilOps = { .loadOp = GFX_LOAD_OP_DONT_CARE, .storeOp = GFX_STORE_OP_DONT_CARE },
            .finalLayout = GFX_TEXTURE_LAYOUT_DEPTH_STENCIL_ATTACHMENT
        };

        GfxRenderPassDepthStencilAttachment transitionDepthAttachment = {
            .target = transitionDepthTarget,
            .resolveTarget = nullptr
        };

        GfxRenderPassDescriptor transitionPassDesc = {
            .label = "Layout Transition Pass",
            .colorAttachments = &transitionColorAttachment,
            .colorAttachmentCount = 1,
            .depthStencilAttachment = &transitionDepthAttachment
        };

        if (gfxDeviceCreateRenderPass(device, &transitionPassDesc, &transitionRenderPass) != GFX_RESULT_SUCCESS) {
            std::cerr << "Failed to create transition render pass\n";
            return false;
        }
    }

    // Resolve render pass
    GfxRenderPassColorAttachment resolveColorAttachment = {
        .target = colorTarget,
        .resolveTarget = (settings.msaaSampleCount > GFX_SAMPLE_COUNT_1) ? &resolveTarget : nullptr
    };

    GfxRenderPassDepthStencilAttachmentTarget resolveDepthTarget = {
        .format = DEPTH_FORMAT,
        .sampleCount = this->settings.msaaSampleCount,
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

void CubeApp::destroyRenderPass()
{
    if (resolveRenderPass) {
        gfxRenderPassDestroy(resolveRenderPass);
        resolveRenderPass = nullptr;
    }
    if (transitionRenderPass) {
        gfxRenderPassDestroy(transitionRenderPass);
        transitionRenderPass = nullptr;
    }
    if (clearRenderPass) {
        gfxRenderPassDestroy(clearRenderPass);
        clearRenderPass = nullptr;
    }
    if (renderPass) {
        gfxRenderPassDestroy(renderPass);
        renderPass = nullptr;
    }
}

bool CubeApp::createSizeDependentResources(uint32_t width, uint32_t height)
{
    if (!createSwapchain(width, height)) {
        return false;
    }

    uint32_t swapchainWidth = swapchainInfo.extent.width;
    uint32_t swapchainHeight = swapchainInfo.extent.height;

    if (!createTextures(swapchainWidth, swapchainHeight)) {
        return false;
    }
    if (!createRenderPass()) {
        return false;
    }
    if (!createFramebuffers(swapchainWidth, swapchainHeight)) {
        return false;
    }

    return true;
}

void CubeApp::destroySizeDependentResources()
{
    destroyFramebuffers();
    destroyRenderPass();
    destroyTextures();
    destroySwapchain();
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
        .usage = GFX_FLAGS(GFX_BUFFER_USAGE_VERTEX | GFX_BUFFER_USAGE_COPY_DST),
        .memoryProperties = GFX_MEMORY_PROPERTY_DEVICE_LOCAL
    };

    if (gfxDeviceCreateBuffer(device, &vertexBufferDesc, &vertexBuffer) != GFX_RESULT_SUCCESS) {
        std::cerr << "Failed to create vertex buffer\n";
        return false;
    }

    // Create index buffer
    GfxBufferDescriptor indexBufferDesc = {
        .label = "Cube Indices",
        .size = sizeof(indices),
        .usage = GFX_FLAGS(GFX_BUFFER_USAGE_INDEX | GFX_BUFFER_USAGE_COPY_DST),
        .memoryProperties = GFX_MEMORY_PROPERTY_DEVICE_LOCAL
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

void CubeApp::destroyGeometry()
{
    if (indexBuffer) {
        gfxBufferDestroy(indexBuffer);
        indexBuffer = nullptr;
    }
    if (vertexBuffer) {
        gfxBufferDestroy(vertexBuffer);
        vertexBuffer = nullptr;
    }
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
    size_t totalBufferSize = uniformAlignedSize * framesInFlight * CUBE_COUNT;

    GfxBufferDescriptor uniformBufferDesc = {
        .label = "Shared Transform Uniforms",
        .size = totalBufferSize,
        .usage = GFX_FLAGS(GFX_BUFFER_USAGE_UNIFORM | GFX_BUFFER_USAGE_COPY_DST),
        .memoryProperties = GFX_FLAGS(GFX_MEMORY_PROPERTY_HOST_VISIBLE | GFX_MEMORY_PROPERTY_HOST_COHERENT)
    };

    if (gfxDeviceCreateBuffer(device, &uniformBufferDesc, &sharedUniformBuffer) != GFX_RESULT_SUCCESS) {
        std::cerr << "Failed to create shared uniform buffer\n";
        return false;
    }

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

    return true;
}

void CubeApp::destroyUniformBuffer()
{
    if (uniformBindGroupLayout) {
        gfxBindGroupLayoutDestroy(uniformBindGroupLayout);
        uniformBindGroupLayout = nullptr;
    }

    if (sharedUniformBuffer) {
        gfxBufferDestroy(sharedUniformBuffer);
        sharedUniformBuffer = nullptr;
    }
}

bool CubeApp::createShaders()
{
    GfxShaderSourceType sourceType;
    if (adapterInfo.backend == GFX_BACKEND_WEBGPU) {
        sourceType = GFX_SHADER_SOURCE_WGSL;
        std::cout << "Loading WGSL shaders...\n";
        auto vertexShaderData = util::loadTextFile("shaders/cube.vert.wgsl");
        auto fragmentShaderData = util::loadTextFile("shaders/cube.frag.wgsl");
        if (vertexShaderData.empty() || fragmentShaderData.empty()) {
            std::cerr << "Failed to load WGSL shaders\n";
            return false;
        }

        // Create vertex shader
        GfxShaderDescriptor vertexShaderDesc = {
            .label = "Cube Vertex Shader",
            .sourceType = sourceType,
            .code = vertexShaderData.data(),
            .codeSize = vertexShaderData.size(),
            .entryPoint = "main"
        };

        if (gfxDeviceCreateShader(device, &vertexShaderDesc, &vertexShader) != GFX_RESULT_SUCCESS) {
            std::cerr << "Failed to create vertex shader\n";
            return false;
        }

        // Create fragment shader
        GfxShaderDescriptor fragmentShaderDesc = {
            .label = "Cube Fragment Shader",
            .sourceType = sourceType,
            .code = fragmentShaderData.data(),
            .codeSize = fragmentShaderData.size(),
            .entryPoint = "main"
        };

        if (gfxDeviceCreateShader(device, &fragmentShaderDesc, &fragmentShader) != GFX_RESULT_SUCCESS) {
            std::cerr << "Failed to create fragment shader\n";
            return false;
        }
    } else {
        sourceType = GFX_SHADER_SOURCE_SPIRV;
        std::cout << "Loading SPIR-V shaders...\n";
        auto vertexShaderData = util::loadBinaryFile("shaders/cube.vert.spv");
        auto fragmentShaderData = util::loadBinaryFile("shaders/cube.frag.spv");
        if (vertexShaderData.empty() || fragmentShaderData.empty()) {
            std::cerr << "Failed to load SPIR-V shaders\n";
            return false;
        }

        // Create vertex shader
        GfxShaderDescriptor vertexShaderDesc = {
            .label = "Cube Vertex Shader",
            .sourceType = sourceType,
            .code = vertexShaderData.data(),
            .codeSize = vertexShaderData.size(),
            .entryPoint = "main"
        };

        if (gfxDeviceCreateShader(device, &vertexShaderDesc, &vertexShader) != GFX_RESULT_SUCCESS) {
            std::cerr << "Failed to create vertex shader\n";
            return false;
        }

        // Create fragment shader
        GfxShaderDescriptor fragmentShaderDesc = {
            .label = "Cube Fragment Shader",
            .sourceType = sourceType,
            .code = fragmentShaderData.data(),
            .codeSize = fragmentShaderData.size(),
            .entryPoint = "main"
        };

        if (gfxDeviceCreateShader(device, &fragmentShaderDesc, &fragmentShader) != GFX_RESULT_SUCCESS) {
            std::cerr << "Failed to create fragment shader\n";
            return false;
        }
    }

    return true;
}

void CubeApp::destroyShaders()
{
    if (fragmentShader) {
        gfxShaderDestroy(fragmentShader);
        fragmentShader = nullptr;
    }
    if (vertexShader) {
        gfxShaderDestroy(vertexShader);
        vertexShader = nullptr;
    }
}

bool CubeApp::createRenderingResources()
{
    std::cout << "[DEBUG] createRenderingResources called" << std::endl;

    // 1. Create geometry (vertex/index buffers)
    if (!createGeometry()) {
        return false;
    }

    // 2. Create uniform buffer and layout
    if (!createUniformBuffer()) {
        return false;
    }

    // 3. Create shaders
    if (!createShaders()) {
        return false;
    }

    // 4. Create render pipeline
    if (!createRenderPipeline()) {
        return false;
    }

    return true;
}

void CubeApp::destroyRenderingResources()
{
    // Destroy pipeline
    destroyRenderPipeline();

    // Destroy shaders
    destroyShaders();

    // Destroy uniform buffer
    destroyUniformBuffer();

    // Destroy geometry
    destroyGeometry();
}

bool CubeApp::createRenderPipeline()
{
    // Define vertex attributes
    GfxVertexAttribute attributes[] = {
        { .format = GFX_FORMAT_R32G32B32_FLOAT,
            .offset = offsetof(Vertex, position),
            .shaderLocation = 0 },
        { .format = GFX_FORMAT_R32G32B32_FLOAT,
            .offset = offsetof(Vertex, color),
            .shaderLocation = 1 }
    };

    GfxVertexBufferLayout vertexBufferLayout = {
        .arrayStride = sizeof(Vertex),
        .attributes = attributes,
        .attributeCount = 2,
        .stepMode = GFX_VERTEX_STEP_MODE_VERTEX
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
        .stripIndexFormat = GFX_INDEX_FORMAT_UNDEFINED,
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
    pipelineDesc.sampleCount = this->settings.msaaSampleCount;
    pipelineDesc.renderPass = renderPass;
    pipelineDesc.bindGroupLayouts = bindGroupLayouts;
    pipelineDesc.bindGroupLayoutCount = 1;

    if (gfxDeviceCreateRenderPipeline(device, &pipelineDesc, &renderPipeline) != GFX_RESULT_SUCCESS) {
        std::cerr << "Failed to create render pipeline\n";
        return false;
    }

    return true;
}

void CubeApp::destroyRenderPipeline()
{
    if (renderPipeline) {
        gfxRenderPipelineDestroy(renderPipeline);
        renderPipeline = nullptr;
    }
}

bool CubeApp::createPerFrameResources()
{
    frameResources.resize(framesInFlight);

    GfxSemaphoreDescriptor semaphoreDesc = {
        .label = "Semaphore",
        .type = GFX_SEMAPHORE_TYPE_BINARY,
        .initialValue = 0
    };

    GfxFenceDescriptor fenceDesc = {
        .label = "Fence",
        .signaled = true
    };

    for (size_t i = 0; i < framesInFlight; ++i) {
        auto& frame = frameResources[i];
        std::string labelStr;

        // Create semaphores
        labelStr = std::format("Image Available Semaphore {}", i);
        semaphoreDesc.label = labelStr.c_str();
        if (gfxDeviceCreateSemaphore(device, &semaphoreDesc, &frame.imageAvailableSemaphore) != GFX_RESULT_SUCCESS) {
            std::cerr << std::format("Failed to create image available semaphore {}\n", i);
            return false;
        }

        labelStr = std::format("Clear Finished Semaphore {}", i);
        semaphoreDesc.label = labelStr.c_str();
        if (gfxDeviceCreateSemaphore(device, &semaphoreDesc, &frame.clearFinishedSemaphore) != GFX_RESULT_SUCCESS) {
            std::cerr << std::format("Failed to create clear finished semaphore {}\n", i);
            return false;
        }

        // Create fence
        labelStr = std::format("In Flight Fence {}", i);
        fenceDesc.label = labelStr.c_str();
        if (gfxDeviceCreateFence(device, &fenceDesc, &frame.inFlightFence) != GFX_RESULT_SUCCESS) {
            std::cerr << std::format("Failed to create in flight fence {}\n", i);
            return false;
        }

        // Create clear encoder
        labelStr = std::format("Clear Encoder Frame {}", i);
        GfxCommandEncoderDescriptor encoderDesc = { .label = labelStr.c_str() };
        if (gfxDeviceCreateCommandEncoder(device, &encoderDesc, &frame.clearEncoder) != GFX_RESULT_SUCCESS) {
            std::cerr << std::format("Failed to create clear encoder {}\n", i);
            return false;
        }

        // Create cube encoders
        frame.cubeEncoders.resize(CUBE_COUNT);
        for (size_t cubeIdx = 0; cubeIdx < CUBE_COUNT; ++cubeIdx) {
            labelStr = std::format("Command Encoder Frame {} Cube {}", i, cubeIdx);
            GfxCommandEncoderDescriptor encoderDesc = { .label = labelStr.c_str() };
            if (gfxDeviceCreateCommandEncoder(device, &encoderDesc, &frame.cubeEncoders[cubeIdx]) != GFX_RESULT_SUCCESS) {
                std::cerr << std::format("Failed to create command encoder {} cube {}\n", i, cubeIdx);
                return false;
            }
        }

        // Create resolve encoder
        labelStr = std::format("Resolve Encoder Frame {}", i);
        GfxCommandEncoderDescriptor resolveEncoderDesc = { .label = labelStr.c_str() };
        if (gfxDeviceCreateCommandEncoder(device, &resolveEncoderDesc, &frame.resolveEncoder) != GFX_RESULT_SUCCESS) {
            std::cerr << std::format("Failed to create resolve encoder {}\n", i);
            return false;
        }

        // Create transition encoder
        labelStr = std::format("Transition Encoder Frame {}", i);
        GfxCommandEncoderDescriptor transitionEncoderDesc = { .label = labelStr.c_str() };
        if (gfxDeviceCreateCommandEncoder(device, &transitionEncoderDesc, &frame.transitionEncoder) != GFX_RESULT_SUCCESS) {
            std::cerr << std::format("Failed to create transition encoder {}\n", i);
            return false;
        }

        // Create bind groups
        frame.uniformBindGroups.resize(CUBE_COUNT);
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

            if (gfxDeviceCreateBindGroup(device, &uniformBindGroupDesc, &frame.uniformBindGroups[cubeIdx]) != GFX_RESULT_SUCCESS) {
                std::cerr << std::format("Failed to create uniform bind group {} cube {}\n", i, cubeIdx);
                return false;
            }
        }
    }

    return true;
}

void CubeApp::destroyPerFrameResources()
{
    // Wait for device idle
    if (device) {
        gfxDeviceWaitIdle(device);
    }

    // Destroy per-frame resources
    for (auto& frame : frameResources) {
        // Destroy bind groups
        for (auto& bindGroup : frame.uniformBindGroups) {
            if (bindGroup) {
                gfxBindGroupDestroy(bindGroup);
                bindGroup = nullptr;
            }
        }

        // Destroy cube encoders
        for (auto& encoder : frame.cubeEncoders) {
            if (encoder) {
                gfxCommandEncoderDestroy(encoder);
                encoder = nullptr;
            }
        }

        // Destroy clear and resolve encoders
        if (frame.clearEncoder) {
            gfxCommandEncoderDestroy(frame.clearEncoder);
            frame.clearEncoder = nullptr;
        }
        if (frame.resolveEncoder) {
            gfxCommandEncoderDestroy(frame.resolveEncoder);
            frame.resolveEncoder = nullptr;
        }
        if (frame.transitionEncoder) {
            gfxCommandEncoderDestroy(frame.transitionEncoder);
            frame.transitionEncoder = nullptr;
        }

        // Destroy synchronization objects
        if (frame.imageAvailableSemaphore) {
            gfxSemaphoreDestroy(frame.imageAvailableSemaphore);
            frame.imageAvailableSemaphore = nullptr;
        }
        if (frame.clearFinishedSemaphore) {
            gfxSemaphoreDestroy(frame.clearFinishedSemaphore);
            frame.clearFinishedSemaphore = nullptr;
        }
        if (frame.inFlightFence) {
            gfxFenceDestroy(frame.inFlightFence);
            frame.inFlightFence = nullptr;
        }
    }

    frameResources.clear();
}

void CubeApp::updateCube(int cubeIndex)
{
    UniformData uniforms{};

    // Create rotation matrices (combine X and Y rotations)
    // Each cube rotates slightly differently
    std::array<std::array<float, 4>, 4> rotX, rotY, tempModel, translation;
    math::matrixIdentity(tempModel);
    math::matrixRotateX(rotX, (rotationAngleX + cubeIndex * 30.0f) * M_PI / 180.0f);
    math::matrixRotateY(rotY, (rotationAngleY + cubeIndex * 45.0f) * M_PI / 180.0f);
    math::matrixMultiply(tempModel, rotY, rotX);

    // Position cubes side by side: left (-3, 0, 0), center (0, 0, 0), right (3, 0, 0)
    math::matrixIdentity(translation);
    translation[3][0] = -(float)CUBE_COUNT * 0.5f + (cubeIndex - 1) * 1.5f; // x offset

    // Apply translation after rotation
    math::matrixMultiply(uniforms.model, tempModel, translation);

    // Create view matrix (camera positioned at 0, 0, 10 looking at origin)
    math::matrixLookAt(uniforms.view,
        0.0f, 0.0f, 10.0f, // eye position - pulled back to see all 3 cubes
        0.0f, 0.0f, 0.0f, // look at point
        0.0f, 1.0f, 0.0f); // up vector

    // Create projection matrix
    float aspect = (float)swapchainInfo.extent.width / (float)swapchainInfo.extent.height;
    math::matrixPerspective(uniforms.projection, 45.0f * M_PI / 180.0f, aspect, 0.1f, 100.0f, adapterInfo.backend);

    // Upload uniform data
    size_t offset = (currentFrame * CUBE_COUNT + cubeIndex) * uniformAlignedSize;
    gfxQueueWriteBuffer(queue, sharedUniformBuffer, offset, &uniforms, sizeof(uniforms));
}

void CubeApp::update(float deltaTime)
{
    rotationAngleX += 45.0f * deltaTime;
    rotationAngleY += 30.0f * deltaTime;
    if (rotationAngleX >= 360.0f) {
        rotationAngleX -= 360.0f;
    }
    if (rotationAngleY >= 360.0f) {
        rotationAngleY -= 360.0f;
    }

    // Update uniforms for each cube
    for (int i = 0; i < CUBE_COUNT; ++i) {
        updateCube(i);
    }
}

void CubeApp::recordClearCommands(uint32_t imageIndex)
{
    auto& frame = frameResources[currentFrame];
    gfxCommandEncoderBegin(frame.clearEncoder);

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
    if (gfxCommandEncoderBeginRenderPass(frame.clearEncoder, &beginDesc, &renderPass) == GFX_RESULT_SUCCESS) {
        gfxRenderPassEncoderEnd(renderPass);
    }

    gfxCommandEncoderEnd(frame.clearEncoder);
}

void CubeApp::recordCubeCommands(int cubeIndex, uint32_t imageIndex)
{
    auto& frame = frameResources[currentFrame];
    GfxCommandEncoder encoder = frame.cubeEncoders[cubeIndex];
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

        GfxViewport viewport = { 0.0f, 0.0f, (float)swapchainInfo.extent.width, (float)swapchainInfo.extent.height, 0.0f, 1.0f };
        GfxScissorRect scissor = { 0, 0, swapchainInfo.extent.width, swapchainInfo.extent.height };
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

        gfxRenderPassEncoderSetBindGroup(renderPass, 0, frame.uniformBindGroups[cubeIndex], nullptr, 0);
        gfxRenderPassEncoderDrawIndexed(renderPass, 36, 1, 0, 0, 0);

        gfxRenderPassEncoderEnd(renderPass);
    }

    gfxCommandEncoderEnd(encoder);
}

void CubeApp::recordResolveCommands(uint32_t imageIndex)
{
    auto& frame = frameResources[currentFrame];
    gfxCommandEncoderBegin(frame.resolveEncoder);

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
    if (gfxCommandEncoderBeginRenderPass(frame.resolveEncoder, &beginDesc, &renderPass) == GFX_RESULT_SUCCESS) {
        gfxRenderPassEncoderEnd(renderPass);
    }

    gfxCommandEncoderEnd(frame.resolveEncoder);
}

void CubeApp::recordLayoutTransition(uint32_t imageIndex)
{
    auto& frame = frameResources[currentFrame];
    gfxCommandEncoderBegin(frame.transitionEncoder);

    // Use an empty render pass to transition layout via initialLayout/finalLayout
    GfxRenderPassBeginDescriptor beginDesc = {
        .label = "Layout Transition Pass",
        .renderPass = transitionRenderPass,
        .framebuffer = framebuffers[imageIndex],
        .colorClearValues = nullptr,
        .colorClearValueCount = 0,
        .depthClearValue = 1.0f,
        .stencilClearValue = 0
    };

    GfxRenderPassEncoder renderPass;
    if (gfxCommandEncoderBeginRenderPass(frame.transitionEncoder, &beginDesc, &renderPass) == GFX_RESULT_SUCCESS) {
        // Empty pass - just transitions layout
        gfxRenderPassEncoderEnd(renderPass);
    }

    gfxCommandEncoderEnd(frame.transitionEncoder);
}

void CubeApp::render()
{
    auto& frame = frameResources[currentFrame];

    gfxFenceWait(frame.inFlightFence, GFX_TIMEOUT_INFINITE);
    gfxFenceReset(frame.inFlightFence);

    uint32_t imageIndex;
    GfxResult result = gfxSwapchainAcquireNextImage(swapchain, GFX_TIMEOUT_INFINITE,
        frame.imageAvailableSemaphore, nullptr, &imageIndex);

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
            .commandEncoders = &frame.clearEncoder,
            .commandEncoderCount = 1,
            .waitSemaphores = &frame.imageAvailableSemaphore,
            .waitSemaphoreCount = 1,
            .signalSemaphores = &frame.clearFinishedSemaphore,
            .signalSemaphoreCount = 1,
            .signalFence = nullptr
        };
        gfxQueueSubmit(queue, &clearSubmit);

        // Submit cube encoders
        std::vector<GfxCommandEncoder> cubeEncoderArray(CUBE_COUNT);
        for (int i = 0; i < CUBE_COUNT; ++i) {
            cubeEncoderArray[i] = frame.cubeEncoders[i];
        }

        // When MSAA > 1, we need a resolve pass after cube rendering
        // When MSAA == 1, we render directly to swapchain so no resolve needed
        if (settings.msaaSampleCount > GFX_SAMPLE_COUNT_1) {
            GfxSubmitDescriptor cubesSubmit = {
                .commandEncoders = cubeEncoderArray.data(),
                .commandEncoderCount = static_cast<uint32_t>(cubeEncoderArray.size()),
                .waitSemaphores = &frame.clearFinishedSemaphore,
                .waitSemaphoreCount = 1,
                .signalSemaphores = nullptr,
                .signalSemaphoreCount = 0,
                .signalFence = nullptr
            };
            gfxQueueSubmit(queue, &cubesSubmit);

            // Submit resolve encoder
            recordResolveCommands(imageIndex);

            GfxSubmitDescriptor resolveSubmit = {
                .commandEncoders = &frame.resolveEncoder,
                .commandEncoderCount = 1,
                .waitSemaphores = nullptr,
                .waitSemaphoreCount = 0,
                .signalSemaphores = &renderFinishedSemaphores[imageIndex],
                .signalSemaphoreCount = 1,
                .signalFence = frame.inFlightFence
            };
            gfxQueueSubmit(queue, &resolveSubmit);
        } else {
            // No MSAA: submit cube rendering, then layout transition
            GfxSubmitDescriptor cubesSubmit = {
                .commandEncoders = cubeEncoderArray.data(),
                .commandEncoderCount = static_cast<uint32_t>(cubeEncoderArray.size()),
                .waitSemaphores = &frame.clearFinishedSemaphore,
                .waitSemaphoreCount = 1,
                .signalSemaphores = nullptr,
                .signalSemaphoreCount = 0,
                .signalFence = nullptr
            };
            gfxQueueSubmit(queue, &cubesSubmit);

            // Submit layout transition
            recordLayoutTransition(imageIndex);

            GfxSubmitDescriptor transitionSubmit = {
                .commandEncoders = &frame.transitionEncoder,
                .commandEncoderCount = 1,
                .waitSemaphores = nullptr,
                .waitSemaphoreCount = 0,
                .signalSemaphores = &renderFinishedSemaphores[imageIndex],
                .signalSemaphoreCount = 1,
                .signalFence = frame.inFlightFence
            };
            gfxQueueSubmit(queue, &transitionSubmit);
        }
    } else {
        // Non-threaded path for WebGPU
        GfxCommandEncoder encoder = frame.cubeEncoders[0];
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

            GfxViewport viewport = { 0.0f, 0.0f, (float)swapchainInfo.extent.width, (float)swapchainInfo.extent.height, 0.0f, 1.0f };
            GfxScissorRect scissor = { 0, 0, swapchainInfo.extent.width, swapchainInfo.extent.height };
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
                gfxRenderPassEncoderSetBindGroup(renderPass, 0, frame.uniformBindGroups[cubeIdx], nullptr, 0);
                gfxRenderPassEncoderDrawIndexed(renderPass, 36, 1, 0, 0, 0);
            }

            gfxRenderPassEncoderEnd(renderPass);
        }

        gfxCommandEncoderEnd(encoder);

        GfxSubmitDescriptor submitDescriptor = {
            .commandEncoders = &encoder,
            .commandEncoderCount = 1,
            .waitSemaphores = &frame.imageAvailableSemaphore,
            .waitSemaphoreCount = 1,
            .signalSemaphores = &renderFinishedSemaphores[imageIndex],
            .signalSemaphoreCount = 1,
            .signalFence = frame.inFlightFence
        };
        gfxQueueSubmit(queue, &submitDescriptor);
    }

    // Present
    GfxPresentDescriptor presentDescriptor = {};
    presentDescriptor.sType = GFX_STRUCTURE_TYPE_PRESENT_DESCRIPTOR;
    presentDescriptor.pNext = NULL;
    presentDescriptor.waitSemaphores = &renderFinishedSemaphores[imageIndex];
    presentDescriptor.waitSemaphoreCount = 1;
    gfxSwapchainPresent(swapchain, &presentDescriptor);

    currentFrame = (currentFrame + 1) % framesInFlight;
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
    if (!window || glfwWindowShouldClose(window)) {
        return false;
    }

    glfwPollEvents();

    // Handle framebuffer resize
    if (previousWidth != windowWidth || previousHeight != windowHeight) {
        gfxDeviceWaitIdle(device);

        destroySizeDependentResources();
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

GfxPlatformWindowHandle CubeApp::getPlatformWindowHandle()
{
    GfxPlatformWindowHandle handle = {}; // Use empty initializer for C++
#if defined(__EMSCRIPTEN__)
    handle = gfxPlatformWindowHandleFromEmscripten("#canvas");
#elif defined(_WIN32)
    handle = gfxPlatformWindowHandleFromWin32(GetModuleHandle(NULL), glfwGetWin32Window(window));
#elif defined(__linux__)
    // handle = gfxPlatformWindowHandleFromXlib(glfwGetX11Display(), glfwGetX11Window(window));
    handle = gfxPlatformWindowHandleFromWayland(glfwGetWaylandDisplay(), glfwGetWaylandWindow(window));
#elif defined(__APPLE__)
    handle = gfxPlatformWindowHandleFromMetal(glfwGetCocoaWindow(window));
#endif
    return handle;
}

void CubeApp::errorCallback(int error, const char* description)
{
    std::cerr << "GLFW Error " << error << ": " << description << std::endl;
}

void CubeApp::framebufferSizeCallback(GLFWwindow* window, int width, int height)
{
    auto* app = static_cast<CubeApp*>(glfwGetWindowUserPointer(window));
    if (app) {
        app->windowWidth = static_cast<uint32_t>(width);
        app->windowHeight = static_cast<uint32_t>(height);
    }
}

void CubeApp::keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    }
}

namespace util {
std::vector<uint8_t> loadBinaryFile(const char* filepath)
{
    std::FILE* file = std::fopen(filepath, "rb");
    if (!file) {
        std::cerr << "Failed to open binary file: " << filepath << std::endl;
        return {};
    }

    std::fseek(file, 0, SEEK_END);
    long fileSize = std::ftell(file);
    std::fseek(file, 0, SEEK_SET);

    if (fileSize <= 0) {
        std::cerr << "Invalid file size for binary file: " << filepath << std::endl;
        std::fclose(file);
        return {};
    }

    std::vector<uint8_t> buffer(fileSize);
    size_t bytesRead = std::fread(buffer.data(), 1, fileSize, file);
    std::fclose(file);

    if (bytesRead != static_cast<size_t>(fileSize)) {
        std::cerr << "Failed to read complete binary file: " << filepath << std::endl;
        return {};
    }

    return buffer;
}

std::string loadTextFile(const char* filepath)
{
    std::FILE* file = std::fopen(filepath, "r");
    if (!file) {
        std::cerr << "Failed to open text file: " << filepath << std::endl;
        return {};
    }

    std::fseek(file, 0, SEEK_END);
    long fileSize = std::ftell(file);
    std::fseek(file, 0, SEEK_SET);

    if (fileSize <= 0) {
        std::cerr << "Invalid file size for text file: " << filepath << std::endl;
        std::fclose(file);
        return {};
    }

    std::string buffer(fileSize, '\0');
    size_t bytesRead = std::fread(&buffer[0], 1, fileSize, file);
    std::fclose(file);

    if (bytesRead != static_cast<size_t>(fileSize)) {
        std::cerr << "Failed to read complete text file: " << filepath << std::endl;
        return {};
    }

    return buffer;
}
} // namespace util

// Math namespace implementation
namespace math {
void matrixIdentity(std::array<std::array<float, 4>, 4>& matrix)
{
    matrix = { { { 1.0f, 0.0f, 0.0f, 0.0f },
        { 0.0f, 1.0f, 0.0f, 0.0f },
        { 0.0f, 0.0f, 1.0f, 0.0f },
        { 0.0f, 0.0f, 0.0f, 1.0f } } };
}

void matrixPerspective(std::array<std::array<float, 4>, 4>& matrix, float fov, float aspect, float nearPlane, float farPlane, GfxBackend backend)
{
    matrix = { { { 0.0f, 0.0f, 0.0f, 0.0f },
        { 0.0f, 0.0f, 0.0f, 0.0f },
        { 0.0f, 0.0f, 0.0f, 0.0f },
        { 0.0f, 0.0f, 0.0f, 0.0f } } };

    float f = 1.0f / std::tan(fov / 2.0f);

    matrix[0][0] = f / aspect;
    if (backend == GFX_BACKEND_VULKAN) {
        matrix[1][1] = -f; // Invert Y for Vulkan
    } else {
        matrix[1][1] = f;
    }
    matrix[2][2] = (farPlane + nearPlane) / (nearPlane - farPlane);
    matrix[2][3] = -1.0f;
    matrix[3][2] = (2.0f * farPlane * nearPlane) / (nearPlane - farPlane);
}

void matrixLookAt(std::array<std::array<float, 4>, 4>& matrix, float eyeX, float eyeY, float eyeZ, float centerX, float centerY, float centerZ, float upX, float upY, float upZ)
{
    // Calculate forward vector
    float fx = centerX - eyeX;
    float fy = centerY - eyeY;
    float fz = centerZ - eyeZ;

    // Normalize forward vector
    if (!vectorNormalize(fx, fy, fz)) {
        matrixIdentity(matrix);
        return;
    }

    // Calculate right vector (forward cross up)
    float rx = fy * upZ - fz * upY;
    float ry = fz * upX - fx * upZ;
    float rz = fx * upY - fy * upX;

    // Normalize right vector
    if (!vectorNormalize(rx, ry, rz)) {
        matrixIdentity(matrix);
        return;
    }

    // Calculate up vector (right cross forward)
    float ux = ry * fz - rz * fy;
    float uy = rz * fx - rx * fz;
    float uz = rx * fy - ry * fx;

    // Build view matrix
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

void matrixRotateY(std::array<std::array<float, 4>, 4>& matrix, float angle)
{
    float c = std::cos(angle);
    float s = std::sin(angle);

    matrixIdentity(matrix);
    matrix[0][0] = c;
    matrix[0][2] = s;
    matrix[2][0] = -s;
    matrix[2][2] = c;
}

void matrixRotateX(std::array<std::array<float, 4>, 4>& matrix, float angle)
{
    float c = std::cos(angle);
    float s = std::sin(angle);

    matrixIdentity(matrix);
    matrix[1][1] = c;
    matrix[1][2] = -s;
    matrix[2][1] = s;
    matrix[2][2] = c;
}

void matrixMultiply(std::array<std::array<float, 4>, 4>& result, const std::array<std::array<float, 4>, 4>& a, const std::array<std::array<float, 4>, 4>& b)
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

bool vectorNormalize(float& x, float& y, float& z)
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
} // namespace math

bool parseArguments(int argc, char** argv, Settings& settings)
{
    // Set defaults
#if defined(__EMSCRIPTEN__)
    settings.backend = GFX_BACKEND_WEBGPU;
#else
    settings.backend = GFX_BACKEND_VULKAN;
#endif
    settings.msaaSampleCount = GFX_SAMPLE_COUNT_4;
    settings.vsync = true;

    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];

        if (arg == "--help" || arg == "-h") {
            std::cout << "Usage: " << argv[0] << " [options]\n";
            std::cout << "Options:\n";
            std::cout << "  --backend [vulkan|webgpu]  Select backend (default: vulkan on native, webgpu on emscripten)\n";
            std::cout << "  --msaa [1|2|4|8|16|32|64]  MSAA sample count (default: 4)\n";
            std::cout << "  --vsync [0|1]              Enable/disable vsync (default: 1)\n";
            std::cout << "  --help, -h                 Show this help message\n";
            return false;
        } else if (arg == "--backend" && i + 1 < argc) {
            std::string backend = argv[++i];
            if (backend == "vulkan") {
                settings.backend = GFX_BACKEND_VULKAN;
            } else if (backend == "webgpu") {
                settings.backend = GFX_BACKEND_WEBGPU;
            } else {
                std::cerr << "Error: Invalid backend '" << backend << "'. Use 'vulkan' or 'webgpu'." << std::endl;
                return false;
            }
        } else if (arg == "--msaa" && i + 1 < argc) {
            int msaa = std::atoi(argv[++i]);
            switch (msaa) {
            case 1:
                settings.msaaSampleCount = GFX_SAMPLE_COUNT_1;
                break;
            case 2:
                settings.msaaSampleCount = GFX_SAMPLE_COUNT_2;
                break;
            case 4:
                settings.msaaSampleCount = GFX_SAMPLE_COUNT_4;
                break;
            case 8:
                settings.msaaSampleCount = GFX_SAMPLE_COUNT_8;
                break;
            case 16:
                settings.msaaSampleCount = GFX_SAMPLE_COUNT_16;
                break;
            case 32:
                settings.msaaSampleCount = GFX_SAMPLE_COUNT_32;
                break;
            case 64:
                settings.msaaSampleCount = GFX_SAMPLE_COUNT_64;
                break;
            default:
                std::cerr << "Error: Invalid MSAA sample count '" << msaa << "'. Use 1, 2, 4, 8, 16, 32, or 64." << std::endl;
                return false;
            }
        } else if (arg == "--vsync" && i + 1 < argc) {
            int vsync = std::atoi(argv[++i]);
            settings.vsync = (vsync != 0);
        } else {
            std::cerr << "Error: Unknown argument '" << arg << "'" << std::endl;
            return false;
        }
    }

    return true;
}

int main(int argc, char** argv)
{
    std::cout << "=== Threaded Cube Example (C++ ThreadPool with C API) ===" << std::endl
              << std::endl;

    Settings settings;
    if (!parseArguments(argc, argv, settings)) {
        return 0;
    }

    CubeApp app(settings);
    if (!app.init()) {
        std::cerr << "Failed to initialize application" << std::endl;
        app.cleanup();
        return -1;
    }
    app.run();
    app.cleanup();

    std::cout << "Example completed successfully!" << std::endl;
    return 0;
}
