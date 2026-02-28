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

#include <float.h>
#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600
#define CUBE_COUNT 12
#define COLOR_FORMAT GFX_FORMAT_B8G8R8A8_UNORM_SRGB
#define DEPTH_FORMAT GFX_FORMAT_DEPTH32_FLOAT

#if defined(__EMSCRIPTEN__) || defined(_WIN32) || defined(__APPLE__) || defined(__ANDROID__)
#define USE_THREADING 0
#else
#include <pthread.h>
#define USE_THREADING 1
#endif

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
    printf("[%s] %s\n", levelStr, message);
}

// Vertex structure for cube
typedef struct {
    float position[3];
    float color[3];
} Vertex;

// Uniform buffer structure for transformations
typedef struct {
    float model[16]; // Model matrix
    float view[16]; // View matrix
    float projection[16]; // Projection matrix
} UniformData;

// Application settings/configuration
typedef struct {
    GfxBackend backend; // Graphics backend selection
    GfxSampleCount msaaSampleCount; // MSAA sample count
    bool vsync; // VSync enabled (FIFO) or disabled (IMMEDIATE)
} Settings;

// Per-frame-in-flight resources
typedef struct {
    GfxCommandEncoder clearEncoder;
    GfxCommandEncoder cubeEncoders[CUBE_COUNT]; // One per cube for parallel recording
    GfxCommandEncoder resolveEncoder;
    GfxCommandEncoder transitionEncoder; // For COLOR_ATTACHMENT->PRESENT_SRC transition (MSAA=1 only)
    GfxSemaphore imageAvailableSemaphore;
    GfxSemaphore clearFinishedSemaphore;
    GfxSemaphore renderFinishedSemaphore;
    GfxFence inFlightFence;
    GfxBindGroup uniformBindGroups[CUBE_COUNT]; // One per cube
} PerFrameResources;

// Forward declarations
typedef struct CubeApp CubeApp;

#if USE_THREADING
// Thread work data - one per cube
typedef struct {
    CubeApp* app;
    int cubeIndex;
    pthread_barrier_t* barrier;
} CubeThreadData;
#endif

typedef struct CubeApp {
    GLFWwindow* window;

    GfxInstance instance;
    GfxAdapter adapter;
    GfxAdapterInfo adapterInfo; // Cached adapter info
    GfxDevice device;
    GfxQueue queue;
    GfxSurface surface;
    GfxSurfaceInfo surfaceInfo; // Surface capabilities
    GfxSwapchain swapchain;
    GfxSwapchainInfo swapchainInfo;

    GfxBuffer vertexBuffer;
    GfxBuffer indexBuffer;
    GfxShader vertexShader;
    GfxShader fragmentShader;
    GfxRenderPass clearRenderPass; // For clear pass (UNDEFINED->COLOR_ATTACHMENT)
    GfxRenderPass renderPass; // For cube passes (LOAD from COLOR_ATTACHMENT)
    GfxRenderPass transitionRenderPass; // For layout transition (COLOR_ATTACHMENT->PRESENT_SRC, MSAA=1 only)
    GfxRenderPass resolveRenderPass; // For final resolve pass (LOAD + resolve to swapchain)
    GfxRenderPipeline renderPipeline;
    GfxBindGroupLayout uniformBindGroupLayout;

    // Depth buffer
    GfxTexture depthTexture;
    GfxTextureView depthTextureView;

    // MSAA color buffer
    GfxTexture msaaColorTexture;
    GfxTextureView msaaColorTextureView;

    // Framebuffers (one per swapchain image) - dynamically allocated
    GfxFramebuffer* framebuffers;
    uint32_t framebufferCount;

    uint32_t windowWidth;
    uint32_t windowHeight;

    // Per-frame-in-flight resources - dynamically allocated
    PerFrameResources* frameResources;
    uint32_t framesInFlight; // Dynamic frame count based on surface capabilities
    uint32_t currentFrame;

    // Shared resources (not per-frame)
    GfxBuffer sharedUniformBuffer; // Single buffer for all frames
    size_t uniformAlignedSize; // Aligned size per frame

    // Animation state
    float rotationAngleX;
    float rotationAngleY;

    // Loop state
    uint32_t previousWidth;
    uint32_t previousHeight;
    float lastTime;

    // FPS tracking
    uint32_t fpsFrameCount;
    float fpsTimeAccumulator;
    float fpsFrameTimeMin;
    float fpsFrameTimeMax;

    // Application settings
    Settings settings;

#if USE_THREADING
    // Threading infrastructure
    pthread_t cubeThreads[CUBE_COUNT];
    CubeThreadData threadData[CUBE_COUNT];
    pthread_barrier_t recordBarrier;
    volatile bool threadsRunning;
    volatile uint32_t currentImageIndex;
#endif
} CubeApp;

// Private function declarations
static bool createWindow(CubeApp* app, uint32_t width, uint32_t height);
static void destroyWindow(CubeApp* app);
static bool createGraphics(CubeApp* app);
static void destroyGraphics(CubeApp* app);
static bool createPerFrameResources(CubeApp* app);
static void destroyPerFrameResources(CubeApp* app);
static bool createSizeDependentResources(CubeApp* app, uint32_t width, uint32_t height);
static void destroySizeDependentResources(CubeApp* app);
static bool createRenderPass(CubeApp* app);
static void destroyRenderPass(CubeApp* app);
static bool createSwapchain(CubeApp* app, uint32_t width, uint32_t height);
static void destroySwapchain(CubeApp* app);
static bool createRenderTargetTextures(CubeApp* app, uint32_t width, uint32_t height);
static void destroyRenderTargetTextures(CubeApp* app);
static bool createFrameBuffers(CubeApp* app, uint32_t width, uint32_t height);
static void destroyFrameBuffers(CubeApp* app);
static bool createGeometry(CubeApp* app);
static void destroyGeometry(CubeApp* app);
static bool createUniformBuffer(CubeApp* app);
static void destroyUniformBuffer(CubeApp* app);
static bool createBindGroup(CubeApp* app);
static void destroyBindGroup(CubeApp* app);
static bool createShaders(CubeApp* app);
static void destroyShaders(CubeApp* app);
static bool createRenderingResources(CubeApp* app);
static void destroyRenderingResources(CubeApp* app);
static bool createRenderPipeline(CubeApp* app);
static void destroyRenderPipeline(CubeApp* app);

static void updateCube(CubeApp* app, int cubeIndex);
static float getCurrentTime(void);
static GfxPlatformWindowHandle getPlatformWindowHandle(GLFWwindow* window);
static void* loadBinaryFile(const char* filepath, size_t* outSize);
static void* loadTextFile(const char* filepath, size_t* outSize);

static void recordCubeCommands(CubeApp* app, int cubeIndex, uint32_t imageIndex);
static void recordClearCommands(CubeApp* app, uint32_t imageIndex);
static void recordResolveCommands(CubeApp* app, uint32_t imageIndex);
static void recordLayoutTransition(CubeApp* app, uint32_t imageIndex);

#if USE_THREADING
static bool createThreading(CubeApp* app);
static void destroyThreading(CubeApp* app);
static void* cubeRecordThread(void* arg);
#endif

// Matrix/Vector math function declarations
static void matrixIdentity(float* matrix);
static void matrixMultiply(float* result, const float* a, const float* b);
static void matrixRotateX(float* matrix, float angle);
static void matrixRotateY(float* matrix, float angle);
static void matrixRotateZ(float* matrix, float angle);
static void matrixPerspective(float* matrix, float fov, float aspect, float near, float far, GfxBackend backend);
static void matrixLookAt(float* matrix, float eyeX, float eyeY, float eyeZ, float centerX, float centerY, float centerZ, float upX, float upY, float upZ);
static bool vectorNormalize(float* x, float* y, float* z);

// The public functions called from main
static bool init(CubeApp* app);
static void cleanup(CubeApp* app);
static void update(CubeApp* app, float deltaTime);
static void render(CubeApp* app);

// GLFW callbacks
static void errorCallback(int error, const char* description)
{
    fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}

static void framebufferSizeCallback(GLFWwindow* window, int width, int height)
{
    CubeApp* app = (CubeApp*)glfwGetWindowUserPointer(window);
    if (app) {
        app->windowWidth = (uint32_t)width;
        app->windowHeight = (uint32_t)height;
    }
}

static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    }
}

static bool createWindow(CubeApp* app, uint32_t width, uint32_t height)
{
    glfwSetErrorCallback(errorCallback);

    if (!glfwInit()) {
        fprintf(stderr, "Failed to initialize GLFW\n");
        return false;
    }

    // Don't create OpenGL context
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

    // Create window title with backend name
    const char* backendName = (app->settings.backend == GFX_BACKEND_VULKAN) ? "Vulkan" : "WebGPU";
    char windowTitle[128];
#if USE_THREADING
    snprintf(windowTitle, sizeof(windowTitle), "Cube Example Threaded - %s", backendName);
#else
    snprintf(windowTitle, sizeof(windowTitle), "Cube Example SingleThreaded - %s", backendName);
#endif

    app->window = glfwCreateWindow(width, height, windowTitle, NULL, NULL);

    if (!app->window) {
        fprintf(stderr, "Failed to create GLFW window\n");
        glfwTerminate();
        return false;
    }

    app->windowWidth = width;
    app->windowHeight = height;

    glfwSetWindowUserPointer(app->window, app);
    glfwSetFramebufferSizeCallback(app->window, framebufferSizeCallback);
    glfwSetKeyCallback(app->window, keyCallback);

    return true;
}

static void destroyWindow(CubeApp* app)
{
    if (app->window) {
        glfwDestroyWindow(app->window);
        app->window = NULL;
    }
    glfwTerminate();
}

static bool createGraphics(CubeApp* app)
{
    // Set up logging callback
    gfxSetLogCallback(logCallback, NULL);

    // Load the graphics backend BEFORE creating an instance
    // This is now decoupled - you load the backend API once at startup
    printf("Loading graphics backend...\n");
    if (gfxLoadBackend(app->settings.backend) != GFX_RESULT_SUCCESS) {
        fprintf(stderr, "Failed to load any graphics backend\n");
        return false;
    }
    printf("Graphics backend loaded successfully!\n");

    // Create graphics instance
    const char* instanceExtensions[] = { GFX_INSTANCE_EXTENSION_SURFACE, GFX_INSTANCE_EXTENSION_DEBUG };
    GfxInstanceDescriptor instanceDesc = {};
    instanceDesc.sType = GFX_STRUCTURE_TYPE_INSTANCE_DESCRIPTOR;
    instanceDesc.pNext = NULL;
    instanceDesc.backend = app->settings.backend;
    instanceDesc.applicationName = "Cube Example (C)";
    instanceDesc.applicationVersion = 1;
    instanceDesc.enabledExtensions = instanceExtensions;
    instanceDesc.enabledExtensionCount = 2;

    if (gfxCreateInstance(&instanceDesc, &app->instance) != GFX_RESULT_SUCCESS) {
        fprintf(stderr, "Failed to create graphics instance\n");
        return false;
    }

    // Get adapter
    GfxAdapterDescriptor adapterDesc = {};
    adapterDesc.sType = GFX_STRUCTURE_TYPE_ADAPTER_DESCRIPTOR;
    adapterDesc.pNext = NULL;
    adapterDesc.adapterIndex = UINT32_MAX;
    adapterDesc.preference = GFX_ADAPTER_PREFERENCE_HIGH_PERFORMANCE;

    if (gfxInstanceRequestAdapter(app->instance, &adapterDesc, &app->adapter) != GFX_RESULT_SUCCESS) {
        fprintf(stderr, "Failed to get graphics adapter\n");
        return false;
    }

    // Query and store adapter info
    gfxAdapterGetInfo(app->adapter, &app->adapterInfo);
    printf("Using adapter: %s\n", app->adapterInfo.name);
    printf("  Vendor ID: 0x%04X, Device ID: 0x%04X\n", app->adapterInfo.vendorID, app->adapterInfo.deviceID);
    printf("  Type: %s\n",
        app->adapterInfo.adapterType == GFX_ADAPTER_TYPE_DISCRETE_GPU ? "Discrete GPU" : app->adapterInfo.adapterType == GFX_ADAPTER_TYPE_INTEGRATED_GPU ? "Integrated GPU"
            : app->adapterInfo.adapterType == GFX_ADAPTER_TYPE_CPU                                                                                       ? "CPU"
                                                                                                                                                         : "Unknown");
    printf("  Backend: %s\n", app->adapterInfo.backend == GFX_BACKEND_VULKAN ? "Vulkan" : "WebGPU");

    // Create device
    const char* deviceExtensions[] = { GFX_DEVICE_EXTENSION_SWAPCHAIN };
    GfxDeviceDescriptor deviceDesc = {};
    deviceDesc.sType = GFX_STRUCTURE_TYPE_DEVICE_DESCRIPTOR;
    deviceDesc.pNext = NULL;
    deviceDesc.label = "Main Device";
    deviceDesc.enabledExtensions = deviceExtensions;
    deviceDesc.enabledExtensionCount = 1;

    if (gfxAdapterCreateDevice(app->adapter, &deviceDesc, &app->device) != GFX_RESULT_SUCCESS) {
        fprintf(stderr, "Failed to create device\n");
        return false;
    }

    // Query and print device limits
    GfxDeviceLimits limits;
    if (gfxDeviceGetLimits(app->device, &limits) != GFX_RESULT_SUCCESS) {
        fprintf(stderr, "Failed to get device limits\n");
        return false;
    }
    printf("Device Limits:\n");
    printf("  Min Uniform Buffer Offset Alignment: %u bytes\n", limits.minUniformBufferOffsetAlignment);
    printf("  Min Storage Buffer Offset Alignment: %u bytes\n", limits.minStorageBufferOffsetAlignment);
    printf("  Max Uniform Buffer Binding Size: %u bytes\n", limits.maxUniformBufferBindingSize);
    printf("  Max Storage Buffer Binding Size: %u bytes\n", limits.maxStorageBufferBindingSize);
    printf("  Max Buffer Size: %llu bytes\n", (unsigned long long)limits.maxBufferSize);
    printf("  Max Texture Dimension 1D: %u\n", limits.maxTextureDimension1D);
    printf("  Max Texture Dimension 2D: %u\n", limits.maxTextureDimension2D);
    printf("  Max Texture Dimension 3D: %u\n", limits.maxTextureDimension3D);
    printf("  Max Texture Array Layers: %u\n", limits.maxTextureArrayLayers);

    if (gfxDeviceGetQueue(app->device, &app->queue) != GFX_RESULT_SUCCESS) {
        fprintf(stderr, "Failed to get device queue\n");
        return false;
    }

    // Create surface
    GfxPlatformWindowHandle windowHandle = getPlatformWindowHandle(app->window);
    GfxSurfaceDescriptor surfaceDesc = {};
    surfaceDesc.sType = GFX_STRUCTURE_TYPE_SURFACE_DESCRIPTOR;
    surfaceDesc.pNext = NULL;
    surfaceDesc.label = "Main Surface";
    surfaceDesc.windowHandle = windowHandle;

    if (gfxDeviceCreateSurface(app->device, &surfaceDesc, &app->surface) != GFX_RESULT_SUCCESS) {
        fprintf(stderr, "Failed to create surface\n");
        return false;
    }

    // Query surface capabilities to determine frames in flight
    if (gfxSurfaceGetInfo(app->surface, &app->surfaceInfo) != GFX_RESULT_SUCCESS) {
        fprintf(stderr, "Failed to get surface info\n");
        return false;
    }

    printf("Surface Info:\n");
    printf("  Min Image Count: %u\n", app->surfaceInfo.minImageCount);
    printf("  Max Image Count: %u\n", app->surfaceInfo.maxImageCount);
    printf("  Extent: min (%u, %u), max (%u, %u)\n",
        app->surfaceInfo.minExtent.width, app->surfaceInfo.minExtent.height,
        app->surfaceInfo.maxExtent.width, app->surfaceInfo.maxExtent.height);

    // Calculate frames in flight based on surface capabilities
    // Clamp to a reasonable range [2-4] to avoid excessive memory usage
    app->framesInFlight = app->surfaceInfo.minImageCount;
    if (app->framesInFlight < 2) {
        app->framesInFlight = 2;
    }
    if (app->framesInFlight > 4) {
        app->framesInFlight = 4;
    }
    printf("Frames in flight: %u\n", app->framesInFlight);

    return true;
}

static void destroyGraphics(CubeApp* app)
{
    if (app->surface) {
        gfxSurfaceDestroy(app->surface);
        app->surface = NULL;
    }
    if (app->device) {
        gfxDeviceDestroy(app->device);
        app->device = NULL;
    }
    if (app->instance) {
        gfxInstanceDestroy(app->instance);
        app->instance = NULL;
    }

    // Unload the backend API after destroying all instances
    printf("Unloading graphics backend...\n");
    gfxUnloadBackend(app->settings.backend);
}

static bool createPerFrameResources(CubeApp* app)
{
    // Allocate per-frame resources dynamically
    app->frameResources = (PerFrameResources*)calloc(app->framesInFlight, sizeof(PerFrameResources));
    if (!app->frameResources) {
        fprintf(stderr, "Failed to allocate frame resources\n");
        return false;
    }

    // Create synchronization objects and command encoders for each frame in flight
    GfxSemaphoreDescriptor semaphoreDesc = {};
    semaphoreDesc.sType = GFX_STRUCTURE_TYPE_SEMAPHORE_DESCRIPTOR;
    semaphoreDesc.pNext = NULL;
    semaphoreDesc.label = "Semaphore";
    semaphoreDesc.type = GFX_SEMAPHORE_TYPE_BINARY;
    semaphoreDesc.initialValue = 0;

    GfxFenceDescriptor fenceDesc = {};
    fenceDesc.sType = GFX_STRUCTURE_TYPE_FENCE_DESCRIPTOR;
    fenceDesc.pNext = NULL;
    fenceDesc.label = "Fence";
    fenceDesc.signaled = true; // Start signaled so first frame doesn't wait

    for (uint32_t i = 0; i < app->framesInFlight; ++i) {
        char label[64];
        PerFrameResources* frame = &app->frameResources[i];

        // Create semaphores
        snprintf(label, sizeof(label), "Image Available Semaphore %d", i);
        semaphoreDesc.label = label;
        if (gfxDeviceCreateSemaphore(app->device, &semaphoreDesc, &frame->imageAvailableSemaphore) != GFX_RESULT_SUCCESS) {
            fprintf(stderr, "Failed to create image available semaphore %d\n", i);
            return false;
        }

        snprintf(label, sizeof(label), "Clear Finished Semaphore %d", i);
        semaphoreDesc.label = label;
        if (gfxDeviceCreateSemaphore(app->device, &semaphoreDesc, &frame->clearFinishedSemaphore) != GFX_RESULT_SUCCESS) {
            fprintf(stderr, "Failed to create clear finished semaphore %d\n", i);
            return false;
        }

        snprintf(label, sizeof(label), "Render Finished Semaphore %d", i);
        semaphoreDesc.label = label;
        if (gfxDeviceCreateSemaphore(app->device, &semaphoreDesc, &frame->renderFinishedSemaphore) != GFX_RESULT_SUCCESS) {
            fprintf(stderr, "Failed to create render finished semaphore %d\n", i);
            return false;
        }

        // Create fence
        snprintf(label, sizeof(label), "In Flight Fence %d", i);
        fenceDesc.label = label;
        if (gfxDeviceCreateFence(app->device, &fenceDesc, &frame->inFlightFence) != GFX_RESULT_SUCCESS) {
            fprintf(stderr, "Failed to create in flight fence %d\n", i);
            return false;
        }

        // Create clear encoder
        snprintf(label, sizeof(label), "Clear Encoder Frame %d", i);
        GfxCommandEncoderDescriptor encoderDesc = {};
        encoderDesc.sType = GFX_STRUCTURE_TYPE_COMMAND_ENCODER_DESCRIPTOR;
        encoderDesc.pNext = NULL;
        encoderDesc.label = label;

        if (gfxDeviceCreateCommandEncoder(app->device, &encoderDesc, &frame->clearEncoder) != GFX_RESULT_SUCCESS) {
            fprintf(stderr, "Failed to create clear encoder %d\n", i);
            return false;
        }

        // Create command encoders - one per cube for parallel recording
        for (int cubeIdx = 0; cubeIdx < CUBE_COUNT; ++cubeIdx) {
            snprintf(label, sizeof(label), "Command Encoder Frame %d Cube %d", i, cubeIdx);
            GfxCommandEncoderDescriptor encoderDesc = {};
            encoderDesc.sType = GFX_STRUCTURE_TYPE_COMMAND_ENCODER_DESCRIPTOR;
            encoderDesc.pNext = NULL;
            encoderDesc.label = label;

            if (gfxDeviceCreateCommandEncoder(app->device, &encoderDesc, &frame->cubeEncoders[cubeIdx]) != GFX_RESULT_SUCCESS) {
                fprintf(stderr, "Failed to create command encoder %d cube %d\n", i, cubeIdx);
                return false;
            }
        }

        // Create resolve encoder
        snprintf(label, sizeof(label), "Resolve Encoder Frame %d", i);
        GfxCommandEncoderDescriptor resolveEncoderDesc = {};
        resolveEncoderDesc.sType = GFX_STRUCTURE_TYPE_COMMAND_ENCODER_DESCRIPTOR;
        resolveEncoderDesc.pNext = NULL;
        resolveEncoderDesc.label = label;

        if (gfxDeviceCreateCommandEncoder(app->device, &resolveEncoderDesc, &frame->resolveEncoder) != GFX_RESULT_SUCCESS) {
            fprintf(stderr, "Failed to create resolve encoder %d\n", i);
            return false;
        }

        // Create transition encoder (for COLOR_ATTACHMENT->PRESENT_SRC when MSAA=1)
        snprintf(label, sizeof(label), "Transition Encoder %d", i);
        GfxCommandEncoderDescriptor transitionEncoderDesc = {};
        transitionEncoderDesc.sType = GFX_STRUCTURE_TYPE_COMMAND_ENCODER_DESCRIPTOR;
        transitionEncoderDesc.pNext = NULL;
        transitionEncoderDesc.label = label;

        if (gfxDeviceCreateCommandEncoder(app->device, &transitionEncoderDesc, &frame->transitionEncoder) != GFX_RESULT_SUCCESS) {
            fprintf(stderr, "Failed to create transition encoder %d\n", i);
            return false;
        }

        // Create bind groups for each cube in this frame
        for (int cubeIdx = 0; cubeIdx < CUBE_COUNT; ++cubeIdx) {
            snprintf(label, sizeof(label), "Uniform Bind Group (Frame %d, Cube %d)", i, cubeIdx);

            size_t offset = (i * CUBE_COUNT + cubeIdx) * app->uniformAlignedSize;

            GfxBindGroupEntry entry = {
                .binding = 0,
                .type = GFX_BIND_GROUP_ENTRY_TYPE_BUFFER,
                .resource = {
                    .buffer = {
                        .buffer = app->sharedUniformBuffer,
                        .offset = offset,
                        .size = sizeof(UniformData) } }
            };

            GfxBindGroupDescriptor bindGroupDesc = {};
            bindGroupDesc.sType = GFX_STRUCTURE_TYPE_BIND_GROUP_DESCRIPTOR;
            bindGroupDesc.pNext = NULL;
            bindGroupDesc.label = label;
            bindGroupDesc.layout = app->uniformBindGroupLayout;
            bindGroupDesc.entries = &entry;
            bindGroupDesc.entryCount = 1;

            if (gfxDeviceCreateBindGroup(app->device, &bindGroupDesc, &frame->uniformBindGroups[cubeIdx]) != GFX_RESULT_SUCCESS) {
                fprintf(stderr, "Failed to create bind group for frame %d, cube %d\n", i, cubeIdx);
                return false;
            }
        }
    }

    app->currentFrame = 0;

    return true;
}

static void destroyPerFrameResources(CubeApp* app)
{
    if (!app->frameResources) {
        return;
    }

    // Wait for all in-flight frames to complete before destroying
    for (uint32_t i = 0; i < app->framesInFlight; ++i) {
        PerFrameResources* frame = &app->frameResources[i];
        if (frame->inFlightFence) {
            gfxFenceWait(frame->inFlightFence, GFX_TIMEOUT_INFINITE);
        }
    }

    // Destroy per-frame resources
    for (uint32_t i = 0; i < app->framesInFlight; ++i) {
        PerFrameResources* frame = &app->frameResources[i];

        // Destroy bind groups
        for (int cubeIdx = 0; cubeIdx < CUBE_COUNT; ++cubeIdx) {
            if (frame->uniformBindGroups[cubeIdx]) {
                gfxBindGroupDestroy(frame->uniformBindGroups[cubeIdx]);
                frame->uniformBindGroups[cubeIdx] = NULL;
            }
        }

        // Destroy synchronization objects
        if (frame->renderFinishedSemaphore) {
            gfxSemaphoreDestroy(frame->renderFinishedSemaphore);
            frame->renderFinishedSemaphore = NULL;
        }
        if (frame->clearFinishedSemaphore) {
            gfxSemaphoreDestroy(frame->clearFinishedSemaphore);
            frame->clearFinishedSemaphore = NULL;
        }
        if (frame->imageAvailableSemaphore) {
            gfxSemaphoreDestroy(frame->imageAvailableSemaphore);
            frame->imageAvailableSemaphore = NULL;
        }
        if (frame->inFlightFence) {
            gfxFenceDestroy(frame->inFlightFence);
            frame->inFlightFence = NULL;
        }

        // Destroy command encoders
        if (frame->resolveEncoder) {
            gfxCommandEncoderDestroy(frame->resolveEncoder);
            frame->resolveEncoder = NULL;
        }
        if (frame->transitionEncoder) {
            gfxCommandEncoderDestroy(frame->transitionEncoder);
            frame->transitionEncoder = NULL;
        }
        for (int cubeIdx = 0; cubeIdx < CUBE_COUNT; ++cubeIdx) {
            if (frame->cubeEncoders[cubeIdx]) {
                gfxCommandEncoderDestroy(frame->cubeEncoders[cubeIdx]);
                frame->cubeEncoders[cubeIdx] = NULL;
            }
        }
        if (frame->clearEncoder) {
            gfxCommandEncoderDestroy(frame->clearEncoder);
            frame->clearEncoder = NULL;
        }
    }

    // Free the dynamically allocated array
    free(app->frameResources);
    app->frameResources = NULL;
}

bool createSizeDependentResources(CubeApp* app, uint32_t width, uint32_t height)
{
    if (!createSwapchain(app, width, height)) {
        return false;
    }

    // Use actual swapchain dimensions (may differ from requested window size)
    uint32_t swapchainWidth = app->swapchainInfo.extent.width;
    uint32_t swapchainHeight = app->swapchainInfo.extent.height;

    if (!createRenderTargetTextures(app, swapchainWidth, swapchainHeight)) {
        return false;
    }

    if (!createRenderPass(app)) {
        return false;
    }

    if (!createFrameBuffers(app, swapchainWidth, swapchainHeight)) {
        return false;
    }

    return true;
}

static void destroySizeDependentResources(CubeApp* app)
{
    // Destroy framebuffers
    destroyFrameBuffers(app);

    // Destroy render passes (depend on swapchain format)
    destroyRenderPass(app);

    // Destroy render target textures (depth and MSAA)
    destroyRenderTargetTextures(app);

    // Destroy swapchain
    destroySwapchain(app);
}

static bool createRenderPass(CubeApp* app)
{
    // Create render pass (persistent, reusable across frames)
    // Define color attachment target - for cube passes that LOAD content
    GfxRenderPassColorAttachmentTarget colorTarget = {
        .format = app->swapchainInfo.format,
        .sampleCount = app->settings.msaaSampleCount,
        .ops = {
            .loadOp = GFX_LOAD_OP_LOAD,
            .storeOp = GFX_STORE_OP_STORE }, // STORE to preserve MSAA content across passes
        .finalLayout = GFX_TEXTURE_LAYOUT_COLOR_ATTACHMENT // Keep in COLOR_ATTACHMENT (renderPassFinal handles PRESENT_SRC)
    };

    // Define resolve target (for MSAA -> non-MSAA resolve)
    GfxRenderPassColorAttachmentTarget resolveTarget = {
        .format = app->swapchainInfo.format,
        .sampleCount = GFX_SAMPLE_COUNT_1,
        .ops = {
            .loadOp = GFX_LOAD_OP_DONT_CARE,
            .storeOp = GFX_STORE_OP_STORE },
        .finalLayout = GFX_TEXTURE_LAYOUT_PRESENT_SRC
    };

    // Resolve target for intermediate passes (don't actually resolve, just keep framebuffer compatible)
    GfxRenderPassColorAttachmentTarget dummyResolveTarget = {
        .format = app->swapchainInfo.format,
        .sampleCount = GFX_SAMPLE_COUNT_1,
        .ops = {
            .loadOp = GFX_LOAD_OP_DONT_CARE,
            .storeOp = GFX_STORE_OP_DONT_CARE }, // Don't care - won't use this in intermediate passes
        .finalLayout = GFX_TEXTURE_LAYOUT_PRESENT_SRC
    };

    // Bundle color attachment - include resolve target for framebuffer compatibility
    GfxRenderPassColorAttachment colorAttachment = {
        .target = colorTarget,
        .resolveTarget = (app->settings.msaaSampleCount > GFX_SAMPLE_COUNT_1) ? &dummyResolveTarget : NULL // Include for compatibility only when MSAA is used
    };

    // Define depth/stencil attachment target
    GfxRenderPassDepthStencilAttachmentTarget depthTarget = {
        .format = DEPTH_FORMAT,
        .sampleCount = app->settings.msaaSampleCount,
        .depthOps = {
            .loadOp = GFX_LOAD_OP_CLEAR,
            .storeOp = GFX_STORE_OP_DONT_CARE },
        .stencilOps = { .loadOp = GFX_LOAD_OP_DONT_CARE, .storeOp = GFX_STORE_OP_DONT_CARE },
        .finalLayout = GFX_TEXTURE_LAYOUT_DEPTH_STENCIL_ATTACHMENT
    };

    GfxRenderPassDepthStencilAttachment depthAttachment = {
        .target = depthTarget,
        .resolveTarget = NULL
    };

    // Create clear render pass (loadOp=CLEAR)
    GfxRenderPassColorAttachmentTarget clearColorTarget = {
        .format = app->swapchainInfo.format,
        .sampleCount = app->settings.msaaSampleCount,
        .ops = {
            .loadOp = GFX_LOAD_OP_CLEAR,
            .storeOp = GFX_STORE_OP_STORE }, // STORE so it can be loaded by subsequent passes
        .finalLayout = GFX_TEXTURE_LAYOUT_COLOR_ATTACHMENT // Always COLOR_ATTACHMENT so cube passes can LOAD it
    };

    GfxRenderPassColorAttachment clearColorAttachment = {
        .target = clearColorTarget,
        .resolveTarget = (app->settings.msaaSampleCount > GFX_SAMPLE_COUNT_1) ? &dummyResolveTarget : NULL // Include for framebuffer compatibility only when MSAA is used
    };

    GfxRenderPassDescriptor clearPassDesc = {};
    clearPassDesc.sType = GFX_STRUCTURE_TYPE_RENDER_PASS_DESCRIPTOR;
    clearPassDesc.pNext = NULL;
    clearPassDesc.label = "Clear Render Pass";
    clearPassDesc.colorAttachments = &clearColorAttachment;
    clearPassDesc.colorAttachmentCount = 1;
    clearPassDesc.depthStencilAttachment = &depthAttachment;

    if (gfxDeviceCreateRenderPass(app->device, &clearPassDesc, &app->clearRenderPass) != GFX_RESULT_SUCCESS) {
        fprintf(stderr, "Failed to create clear render pass\n");
        return false;
    }

    // Create main render pass (loadOp=LOAD)

    GfxRenderPassDescriptor renderPassDesc = {};
    renderPassDesc.sType = GFX_STRUCTURE_TYPE_RENDER_PASS_DESCRIPTOR;
    renderPassDesc.pNext = NULL;
    renderPassDesc.label = "Cube Render Pass (LOAD)";
    renderPassDesc.colorAttachments = &colorAttachment;
    renderPassDesc.colorAttachmentCount = 1;
    renderPassDesc.depthStencilAttachment = &depthAttachment;

    if (gfxDeviceCreateRenderPass(app->device, &renderPassDesc, &app->renderPass) != GFX_RESULT_SUCCESS) {
        fprintf(stderr, "Failed to create render pass\n");
        return false;
    }

    // Create transition render pass (for MSAA=1: COLOR_ATTACHMENT->PRESENT_SRC)
    if (app->settings.msaaSampleCount == GFX_SAMPLE_COUNT_1) {
        GfxRenderPassColorAttachmentTarget transitionColorTarget = {
            .format = app->swapchainInfo.format,
            .sampleCount = app->settings.msaaSampleCount,
            .ops = {
                .loadOp = GFX_LOAD_OP_LOAD, // Load existing content
                .storeOp = GFX_STORE_OP_STORE },
            .finalLayout = GFX_TEXTURE_LAYOUT_PRESENT_SRC
        };

        GfxRenderPassColorAttachment transitionColorAttachment = {
            .target = transitionColorTarget,
            .resolveTarget = NULL
        };

        // Depth attachment for transition pass - just to match framebuffer, not actually used
        GfxRenderPassDepthStencilAttachmentTarget transitionDepthTarget = {
            .format = DEPTH_FORMAT,
            .sampleCount = app->settings.msaaSampleCount,
            .depthOps = {
                .loadOp = GFX_LOAD_OP_DONT_CARE, // Don't care - not using depth
                .storeOp = GFX_STORE_OP_DONT_CARE },
            .stencilOps = { .loadOp = GFX_LOAD_OP_DONT_CARE, .storeOp = GFX_STORE_OP_DONT_CARE },
            .finalLayout = GFX_TEXTURE_LAYOUT_DEPTH_STENCIL_ATTACHMENT
        };

        GfxRenderPassDepthStencilAttachment transitionDepthAttachment = {
            .target = transitionDepthTarget,
            .resolveTarget = NULL
        };

        GfxRenderPassDescriptor transitionPassDesc = {};
        transitionPassDesc.sType = GFX_STRUCTURE_TYPE_RENDER_PASS_DESCRIPTOR;
        transitionPassDesc.pNext = NULL;
        transitionPassDesc.label = "Layout Transition Pass";
        transitionPassDesc.colorAttachments = &transitionColorAttachment;
        transitionPassDesc.colorAttachmentCount = 1;
        transitionPassDesc.depthStencilAttachment = &transitionDepthAttachment; // Include for framebuffer compatibility

        if (gfxDeviceCreateRenderPass(app->device, &transitionPassDesc, &app->transitionRenderPass) != GFX_RESULT_SUCCESS) {
            fprintf(stderr, "Failed to create transition render pass\n");
            return false;
        }
    } else {
        app->transitionRenderPass = NULL;
    }

    // Create final resolve pass (loadOp=LOAD + resolve to swapchain)
    GfxRenderPassColorAttachment resolveColorAttachment = {
        .target = colorTarget, // Load from MSAA
        .resolveTarget = &resolveTarget // Resolve to swapchain
    };

    // Depth attachment for resolve pass - just LOAD (no clearing needed)
    GfxRenderPassDepthStencilAttachmentTarget resolveDepthTarget = {
        .format = DEPTH_FORMAT,
        .sampleCount = app->settings.msaaSampleCount,
        .depthOps = {
            .loadOp = GFX_LOAD_OP_LOAD, // Load existing depth
            .storeOp = GFX_STORE_OP_DONT_CARE }, // Don't need to store, just resolving color
        .stencilOps = { .loadOp = GFX_LOAD_OP_DONT_CARE, .storeOp = GFX_STORE_OP_DONT_CARE },
        .finalLayout = GFX_TEXTURE_LAYOUT_DEPTH_STENCIL_ATTACHMENT
    };

    GfxRenderPassDepthStencilAttachment resolveDepthAttachment = {
        .target = resolveDepthTarget,
        .resolveTarget = NULL
    };

    GfxRenderPassDescriptor resolvePassDesc = {};
    resolvePassDesc.sType = GFX_STRUCTURE_TYPE_RENDER_PASS_DESCRIPTOR;
    resolvePassDesc.pNext = NULL;
    resolvePassDesc.label = "Resolve Render Pass";
    resolvePassDesc.colorAttachments = &resolveColorAttachment;
    resolvePassDesc.colorAttachmentCount = 1;
    resolvePassDesc.depthStencilAttachment = &resolveDepthAttachment; // Include depth for framebuffer compatibility

    if (gfxDeviceCreateRenderPass(app->device, &resolvePassDesc, &app->resolveRenderPass) != GFX_RESULT_SUCCESS) {
        fprintf(stderr, "Failed to create resolve render pass\n");
        return false;
    }

    return true;
}

static void destroyRenderPass(CubeApp* app)
{
    if (app->resolveRenderPass) {
        gfxRenderPassDestroy(app->resolveRenderPass);
        app->resolveRenderPass = NULL;
    }
    if (app->transitionRenderPass) {
        gfxRenderPassDestroy(app->transitionRenderPass);
        app->transitionRenderPass = NULL;
    }
    if (app->clearRenderPass) {
        gfxRenderPassDestroy(app->clearRenderPass);
        app->clearRenderPass = NULL;
    }
    if (app->renderPass) {
        gfxRenderPassDestroy(app->renderPass);
        app->renderPass = NULL;
    }
}

static bool createSwapchain(CubeApp* app, uint32_t width, uint32_t height)
{
    GfxSwapchainDescriptor swapchainDesc = {};
    swapchainDesc.sType = GFX_STRUCTURE_TYPE_SWAPCHAIN_DESCRIPTOR;
    swapchainDesc.pNext = NULL;
    swapchainDesc.label = "Main Swapchain";
    swapchainDesc.surface = app->surface;
    swapchainDesc.extent.width = width;
    swapchainDesc.extent.height = height;
    swapchainDesc.format = COLOR_FORMAT;
    swapchainDesc.usage = GFX_TEXTURE_USAGE_RENDER_ATTACHMENT;
    swapchainDesc.presentMode = app->settings.vsync ? GFX_PRESENT_MODE_FIFO : GFX_PRESENT_MODE_IMMEDIATE;
    swapchainDesc.imageCount = app->framesInFlight;

    if (gfxDeviceCreateSwapchain(app->device, &swapchainDesc, &app->swapchain) != GFX_RESULT_SUCCESS) {
        fprintf(stderr, "Failed to create swapchain\n");
        return false;
    }

    // Query the actual swapchain format (may differ from requested format on web)
    GfxResult result = gfxSwapchainGetInfo(app->swapchain, &app->swapchainInfo);
    if (result != GFX_RESULT_SUCCESS) {
        fprintf(stderr, "[ERROR] Failed to get swapchain info\n");
        return false;
    }
    fprintf(stderr, "[INFO] Requested format: %d, Actual swapchain format: %d\n", COLOR_FORMAT, app->swapchainInfo.format);

    return true;
}

static void destroySwapchain(CubeApp* app)
{
    if (app->swapchain) {
        gfxSwapchainDestroy(app->swapchain);
        app->swapchain = NULL;
    }
}

static bool createRenderTargetTextures(CubeApp* app, uint32_t width, uint32_t height)
{
    // Create depth texture (MSAA must match color attachment)
    GfxTextureDescriptor depthTextureDesc = {};
    depthTextureDesc.sType = GFX_STRUCTURE_TYPE_TEXTURE_DESCRIPTOR;
    depthTextureDesc.pNext = NULL;
    depthTextureDesc.label = "Depth Buffer";
    depthTextureDesc.type = GFX_TEXTURE_TYPE_2D;
    depthTextureDesc.size = (GfxExtent3D){ .width = width, .height = height, .depth = 1 };
    depthTextureDesc.arrayLayerCount = 1;
    depthTextureDesc.mipLevelCount = 1;
    depthTextureDesc.sampleCount = app->settings.msaaSampleCount;
    depthTextureDesc.format = DEPTH_FORMAT;
    depthTextureDesc.usage = GFX_TEXTURE_USAGE_RENDER_ATTACHMENT;

    if (gfxDeviceCreateTexture(app->device, &depthTextureDesc, &app->depthTexture) != GFX_RESULT_SUCCESS) {
        fprintf(stderr, "Failed to create depth texture\n");
        return false;
    }

    // Create depth texture view
    GfxTextureViewDescriptor depthViewDesc = {};
    depthViewDesc.sType = GFX_STRUCTURE_TYPE_TEXTURE_VIEW_DESCRIPTOR;
    depthViewDesc.pNext = NULL;
    depthViewDesc.label = "Depth Buffer View";
    depthViewDesc.viewType = GFX_TEXTURE_VIEW_TYPE_2D;
    depthViewDesc.format = DEPTH_FORMAT;
    depthViewDesc.baseMipLevel = 0;
    depthViewDesc.mipLevelCount = 1;
    depthViewDesc.baseArrayLayer = 0;
    depthViewDesc.arrayLayerCount = 1;

    if (gfxTextureCreateView(app->depthTexture, &depthViewDesc, &app->depthTextureView) != GFX_RESULT_SUCCESS) {
        fprintf(stderr, "Failed to create depth texture view\n");
        return false;
    }

    // Create MSAA color texture (is unused if app->settings.msaaSampleCount == 1)
    GfxTextureDescriptor msaaColorTextureDesc = {};
    msaaColorTextureDesc.sType = GFX_STRUCTURE_TYPE_TEXTURE_DESCRIPTOR;
    msaaColorTextureDesc.pNext = NULL;
    msaaColorTextureDesc.label = "MSAA Color Buffer";
    msaaColorTextureDesc.type = GFX_TEXTURE_TYPE_2D;
    msaaColorTextureDesc.size = (GfxExtent3D){ .width = width, .height = height, .depth = 1 };
    msaaColorTextureDesc.arrayLayerCount = 1;
    msaaColorTextureDesc.mipLevelCount = 1;
    msaaColorTextureDesc.sampleCount = app->settings.msaaSampleCount;
    msaaColorTextureDesc.format = app->swapchainInfo.format;
    msaaColorTextureDesc.usage = GFX_TEXTURE_USAGE_RENDER_ATTACHMENT;

    if (gfxDeviceCreateTexture(app->device, &msaaColorTextureDesc, &app->msaaColorTexture) != GFX_RESULT_SUCCESS) {
        fprintf(stderr, "Failed to create MSAA color texture\n");
        return false;
    }

    // Create MSAA color texture view (is unused if app->settings.msaaSampleCount == 1)
    GfxTextureViewDescriptor msaaColorViewDesc = {};
    msaaColorViewDesc.sType = GFX_STRUCTURE_TYPE_TEXTURE_VIEW_DESCRIPTOR;
    msaaColorViewDesc.pNext = NULL;
    msaaColorViewDesc.label = "MSAA Color Buffer View";
    msaaColorViewDesc.viewType = GFX_TEXTURE_VIEW_TYPE_2D;
    msaaColorViewDesc.format = app->swapchainInfo.format;
    msaaColorViewDesc.baseMipLevel = 0;
    msaaColorViewDesc.mipLevelCount = 1;
    msaaColorViewDesc.baseArrayLayer = 0;
    msaaColorViewDesc.arrayLayerCount = 1;

    if (gfxTextureCreateView(app->msaaColorTexture, &msaaColorViewDesc, &app->msaaColorTextureView) != GFX_RESULT_SUCCESS) {
        fprintf(stderr, "Failed to create MSAA color texture view\n");
        return false;
    }

    return true;
}

static void destroyRenderTargetTextures(CubeApp* app)
{
    // Destroy MSAA color texture
    if (app->msaaColorTextureView) {
        gfxTextureViewDestroy(app->msaaColorTextureView);
        app->msaaColorTextureView = NULL;
    }
    if (app->msaaColorTexture) {
        gfxTextureDestroy(app->msaaColorTexture);
        app->msaaColorTexture = NULL;
    }

    // Destroy depth texture
    if (app->depthTextureView) {
        gfxTextureViewDestroy(app->depthTextureView);
        app->depthTextureView = NULL;
    }
    if (app->depthTexture) {
        gfxTextureDestroy(app->depthTexture);
        app->depthTexture = NULL;
    }
}

static bool createFrameBuffers(CubeApp* app, uint32_t width, uint32_t height)
{
    // Allocate framebuffers array dynamically based on actual swapchain image count
    app->framebufferCount = app->swapchainInfo.imageCount;
    app->framebuffers = (GfxFramebuffer*)calloc(app->framebufferCount, sizeof(GfxFramebuffer));
    if (!app->framebuffers) {
        fprintf(stderr, "Failed to allocate framebuffers\n");
        return false;
    }

    // Create framebuffers (one per swapchain image)
    for (uint32_t i = 0; i < app->framebufferCount; ++i) {
        GfxTextureView backbuffer = NULL;
        GfxResult result = gfxSwapchainGetTextureView(app->swapchain, i, &backbuffer);
        if (result != GFX_RESULT_SUCCESS || !backbuffer) {
            fprintf(stderr, "[ERROR] Failed to get swapchain image view %d\n", i);
            return false;
        }

        // Bundle color view with resolve target
        GfxFramebufferAttachment fbColorAttachment = {
            .view = (app->settings.msaaSampleCount > GFX_SAMPLE_COUNT_1) ? app->msaaColorTextureView : backbuffer,
            .resolveTarget = (app->settings.msaaSampleCount > GFX_SAMPLE_COUNT_1) ? backbuffer : NULL
        };

        // Depth/stencil attachment (no resolve)
        GfxFramebufferAttachment fbDepthAttachment = {
            .view = app->depthTextureView,
            .resolveTarget = NULL
        };

        char label[64];
        snprintf(label, sizeof(label), "Framebuffer %u", i);

        GfxFramebufferDescriptor fbDesc = {};
        fbDesc.sType = GFX_STRUCTURE_TYPE_FRAMEBUFFER_DESCRIPTOR;
        fbDesc.pNext = NULL;
        fbDesc.label = label;
        fbDesc.renderPass = app->resolveRenderPass;
        fbDesc.colorAttachments = &fbColorAttachment;
        fbDesc.colorAttachmentCount = 1;
        fbDesc.depthStencilAttachment = fbDepthAttachment;
        fbDesc.extent.width = width;
        fbDesc.extent.height = height;

        if (gfxDeviceCreateFramebuffer(app->device, &fbDesc, &app->framebuffers[i]) != GFX_RESULT_SUCCESS) {
            fprintf(stderr, "Failed to create framebuffer %u\n", i);
            return false;
        }
    }

    return true;
}

static void destroyFrameBuffers(CubeApp* app)
{
    if (app->framebuffers) {
        for (uint32_t i = 0; i < app->framebufferCount; ++i) {
            if (app->framebuffers[i]) {
                gfxFramebufferDestroy(app->framebuffers[i]);
                app->framebuffers[i] = NULL;
            }
        }
        free(app->framebuffers);
        app->framebuffers = NULL;
        app->framebufferCount = 0;
    }
}

static bool createGeometry(CubeApp* app)
{
    // Create cube vertices (8 vertices for a cube)
    Vertex vertices[] = {
        // Front face
        { { -1.0f, -1.0f, 1.0f }, { 1.0f, 0.0f, 0.0f } }, // 0: Bottom-left
        { { 1.0f, -1.0f, 1.0f }, { 0.0f, 1.0f, 0.0f } }, // 1: Bottom-right
        { { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 1.0f } }, // 2: Top-right
        { { -1.0f, 1.0f, 1.0f }, { 1.0f, 1.0f, 0.0f } }, // 3: Top-left

        // Back face
        { { -1.0f, -1.0f, -1.0f }, { 1.0f, 0.0f, 1.0f } }, // 4: Bottom-left
        { { 1.0f, -1.0f, -1.0f }, { 0.0f, 1.0f, 1.0f } }, // 5: Bottom-right
        { { 1.0f, 1.0f, -1.0f }, { 1.0f, 1.0f, 1.0f } }, // 6: Top-right
        { { -1.0f, 1.0f, -1.0f }, { 0.5f, 0.5f, 0.5f } } // 7: Top-left
    };

    // Create cube indices (36 indices for 12 triangles)
    // All faces wound clockwise when viewed from outside
    uint16_t indices[] = {
        // Front face (Z+) - vertices 0,1,2,3
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
        4, 5, 1, 1, 0, 4
    };

    // Create vertex buffer
    GfxBufferDescriptor vertexBufferDesc = {};
    vertexBufferDesc.sType = GFX_STRUCTURE_TYPE_BUFFER_DESCRIPTOR;
    vertexBufferDesc.pNext = NULL;
    vertexBufferDesc.label = "Cube Vertices";
    vertexBufferDesc.size = sizeof(vertices);
    vertexBufferDesc.usage = GFX_BUFFER_USAGE_VERTEX | GFX_BUFFER_USAGE_COPY_DST;
    vertexBufferDesc.memoryProperties = GFX_MEMORY_PROPERTY_DEVICE_LOCAL;

    if (gfxDeviceCreateBuffer(app->device, &vertexBufferDesc, &app->vertexBuffer) != GFX_RESULT_SUCCESS) {
        fprintf(stderr, "Failed to create vertex buffer\n");
        return false;
    }

    // Create index buffer
    GfxBufferDescriptor indexBufferDesc = {};
    indexBufferDesc.sType = GFX_STRUCTURE_TYPE_BUFFER_DESCRIPTOR;
    indexBufferDesc.pNext = NULL;
    indexBufferDesc.label = "Cube Indices";
    indexBufferDesc.size = sizeof(indices);
    indexBufferDesc.usage = GFX_BUFFER_USAGE_INDEX | GFX_BUFFER_USAGE_COPY_DST;
    indexBufferDesc.memoryProperties = GFX_MEMORY_PROPERTY_DEVICE_LOCAL;

    if (gfxDeviceCreateBuffer(app->device, &indexBufferDesc, &app->indexBuffer) != GFX_RESULT_SUCCESS) {
        fprintf(stderr, "Failed to create index buffer\n");
        return false;
    }

    // Upload vertex and index data
    gfxQueueWriteBuffer(app->queue, app->vertexBuffer, 0, vertices, sizeof(vertices));
    gfxQueueWriteBuffer(app->queue, app->indexBuffer, 0, indices, sizeof(indices));

    return true;
}

static void destroyGeometry(CubeApp* app)
{
    if (app->indexBuffer) {
        gfxBufferDestroy(app->indexBuffer);
        app->indexBuffer = NULL;
    }
    if (app->vertexBuffer) {
        gfxBufferDestroy(app->vertexBuffer);
        app->vertexBuffer = NULL;
    }
}

static bool createUniformBuffer(CubeApp* app)
{
    // Create single large uniform buffer for all frames with proper alignment
    GfxDeviceLimits limits;
    if (gfxDeviceGetLimits(app->device, &limits) != GFX_RESULT_SUCCESS) {
        fprintf(stderr, "Failed to get device limits\n");
        return false;
    }

    size_t uniformSize = sizeof(UniformData);
    app->uniformAlignedSize = gfxAlignUp(uniformSize, limits.minUniformBufferOffsetAlignment);
    // Need space for CUBE_COUNT cubes per frame
    size_t totalBufferSize = app->uniformAlignedSize * app->framesInFlight * CUBE_COUNT;

    GfxBufferDescriptor uniformBufferDesc = {};
    uniformBufferDesc.sType = GFX_STRUCTURE_TYPE_BUFFER_DESCRIPTOR;
    uniformBufferDesc.pNext = NULL;
    uniformBufferDesc.label = "Shared Transform Uniforms";
    uniformBufferDesc.size = totalBufferSize;
    uniformBufferDesc.usage = GFX_BUFFER_USAGE_UNIFORM | GFX_BUFFER_USAGE_COPY_DST;
    uniformBufferDesc.memoryProperties = GFX_MEMORY_PROPERTY_HOST_VISIBLE | GFX_MEMORY_PROPERTY_HOST_COHERENT;

    if (gfxDeviceCreateBuffer(app->device, &uniformBufferDesc, &app->sharedUniformBuffer) != GFX_RESULT_SUCCESS) {
        fprintf(stderr, "Failed to create shared uniform buffer\n");
        return false;
    }

    return true;
}

static void destroyUniformBuffer(CubeApp* app)
{
    if (app->sharedUniformBuffer) {
        gfxBufferDestroy(app->sharedUniformBuffer);
        app->sharedUniformBuffer = NULL;
    }
}

static bool createBindGroup(CubeApp* app)
{
    // Create bind group layout for uniforms
    GfxBindGroupLayoutEntry uniformLayoutEntry = {
        .binding = 0,
        .visibility = GFX_SHADER_STAGE_VERTEX,
        .type = GFX_BINDING_TYPE_BUFFER,
        .buffer = {
            .hasDynamicOffset = false,
            .minBindingSize = sizeof(UniformData) }
    };

    GfxBindGroupLayoutDescriptor uniformLayoutDesc = {};
    uniformLayoutDesc.sType = GFX_STRUCTURE_TYPE_BIND_GROUP_LAYOUT_DESCRIPTOR;
    uniformLayoutDesc.pNext = NULL;
    uniformLayoutDesc.label = "Uniform Bind Group Layout";
    uniformLayoutDesc.entries = &uniformLayoutEntry;
    uniformLayoutDesc.entryCount = 1;

    if (gfxDeviceCreateBindGroupLayout(app->device, &uniformLayoutDesc, &app->uniformBindGroupLayout) != GFX_RESULT_SUCCESS) {
        fprintf(stderr, "Failed to create uniform bind group layout\n");
        return false;
    }

    // Note: Bind groups are created in createPerFrameResources() after uniform buffer is ready

    return true;
}

static void destroyBindGroup(CubeApp* app)
{
    // Note: Bind groups are destroyed in destroyPerFrameResources
    // Only destroy the layout here
    if (app->uniformBindGroupLayout) {
        gfxBindGroupLayoutDestroy(app->uniformBindGroupLayout);
        app->uniformBindGroupLayout = NULL;
    }
}

static bool createShaders(CubeApp* app)
{
    // Load shaders from files (works for both native and web)
    size_t vertexShaderSize, fragmentShaderSize;
    void* vertexShaderCode = NULL;
    void* fragmentShaderCode = NULL;

    GfxShaderSourceType sourceType;
    if (app->adapterInfo.backend == GFX_BACKEND_WEBGPU) {
        sourceType = GFX_SHADER_SOURCE_WGSL;
        // Load WGSL shaders for WebGPU (both native and web)
        printf("Loading WGSL shaders...\n");
        vertexShaderCode = loadTextFile("shaders/cube.vert.wgsl", &vertexShaderSize);
        fragmentShaderCode = loadTextFile("shaders/cube.frag.wgsl", &fragmentShaderSize);
        if (!vertexShaderCode || !fragmentShaderCode) {
            fprintf(stderr, "Failed to load WGSL shaders\n");
            return false;
        }
        printf("Successfully loaded WGSL shaders (vertex: %zu bytes, fragment: %zu bytes)\n",
            vertexShaderSize, fragmentShaderSize);
    } else {
        sourceType = GFX_SHADER_SOURCE_SPIRV;
        // Load SPIR-V shaders for Vulkan
        printf("Loading SPIR-V shaders...\n");
        vertexShaderCode = loadBinaryFile("shaders/cube.vert.spv", &vertexShaderSize);
        fragmentShaderCode = loadBinaryFile("shaders/cube.frag.spv", &fragmentShaderSize);
        if (!vertexShaderCode || !fragmentShaderCode) {
            fprintf(stderr, "Failed to load SPIR-V shaders\n");
            return false;
        }
        printf("Successfully loaded SPIR-V shaders (vertex: %zu bytes, fragment: %zu bytes)\n",
            vertexShaderSize, fragmentShaderSize);
    }

    // Create vertex shader
    GfxShaderDescriptor vertexShaderDesc = {};
    vertexShaderDesc.sType = GFX_STRUCTURE_TYPE_SHADER_DESCRIPTOR;
    vertexShaderDesc.pNext = NULL;
    vertexShaderDesc.label = "Cube Vertex Shader";
    vertexShaderDesc.sourceType = sourceType;
    vertexShaderDesc.code = vertexShaderCode;
    vertexShaderDesc.codeSize = vertexShaderSize;
    vertexShaderDesc.entryPoint = "main";

    if (gfxDeviceCreateShader(app->device, &vertexShaderDesc, &app->vertexShader) != GFX_RESULT_SUCCESS) {
        fprintf(stderr, "Failed to create vertex shader\n");
        free(vertexShaderCode);
        free(fragmentShaderCode);
        return false;
    }

    // Create fragment shader
    GfxShaderDescriptor fragmentShaderDesc = {};
    fragmentShaderDesc.sType = GFX_STRUCTURE_TYPE_SHADER_DESCRIPTOR;
    fragmentShaderDesc.pNext = NULL;
    fragmentShaderDesc.label = "Cube Fragment Shader";
    fragmentShaderDesc.sourceType = sourceType;
    fragmentShaderDesc.code = fragmentShaderCode;
    fragmentShaderDesc.codeSize = fragmentShaderSize;
    fragmentShaderDesc.entryPoint = "main";

    if (gfxDeviceCreateShader(app->device, &fragmentShaderDesc, &app->fragmentShader) != GFX_RESULT_SUCCESS) {
        fprintf(stderr, "Failed to create fragment shader\n");
        free(vertexShaderCode);
        free(fragmentShaderCode);
        return false;
    }

    free(vertexShaderCode);
    free(fragmentShaderCode);
    return true;
}

static void destroyShaders(CubeApp* app)
{
    if (app->fragmentShader) {
        gfxShaderDestroy(app->fragmentShader);
        app->fragmentShader = NULL;
    }
    if (app->vertexShader) {
        gfxShaderDestroy(app->vertexShader);
        app->vertexShader = NULL;
    }
}

bool createRenderingResources(CubeApp* app)
{
    printf("[DEBUG] createRenderingResources called\n");

    if (!createGeometry(app)) {
        return false;
    }

    if (!createUniformBuffer(app)) {
        return false;
    }

    if (!createBindGroup(app)) {
        return false;
    }

    if (!createShaders(app)) {
        return false;
    }
    return true;
}

static void destroyRenderingResources(CubeApp* app)
{
    // Destroy render pipeline (depends on shaders and layouts)
    destroyRenderPipeline(app);

    // Destroy shaders
    destroyShaders(app);

    // Destroy bind groups and layouts
    destroyBindGroup(app);

    // Destroy uniform buffer
    destroyUniformBuffer(app);

    // Destroy geometry buffers
    destroyGeometry(app);
}

bool createRenderPipeline(CubeApp* app)
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

    // Define vertex buffer layout
    GfxVertexBufferLayout vertexBufferLayout = {
        .arrayStride = sizeof(Vertex),
        .attributes = attributes,
        .attributeCount = 2,
        .stepMode = GFX_VERTEX_STEP_MODE_VERTEX
    };

    // Vertex state
    GfxVertexState vertexState = {
        .module = app->vertexShader,
        .entryPoint = "main",
        .buffers = &vertexBufferLayout,
        .bufferCount = 1
    };

    // Color target state
    // Note: Always 1 target even with MSAA - resolve is handled by render pass, not fragment shader
    // layout(location = 0) out vec4 outColor;
    // Use actual swapchain format (may differ from requested format on web)
    GfxColorTargetState colorTarget = {
        .format = app->swapchainInfo.format,
        .blend = NULL,
        .writeMask = GFX_COLOR_WRITE_MASK_ALL
    };

    // Fragment state
    GfxFragmentState fragmentState = {
        .module = app->fragmentShader,
        .entryPoint = "main",
        .targets = &colorTarget,
        .targetCount = 1
    };

    // Primitive state
    GfxPrimitiveState primitiveState = {
        .topology = GFX_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        .stripIndexFormat = GFX_INDEX_FORMAT_UNDEFINED,
        .frontFace = GFX_FRONT_FACE_COUNTER_CLOCKWISE,
        .cullMode = GFX_CULL_MODE_BACK, // Enable back-face culling
        .polygonMode = GFX_POLYGON_MODE_FILL
    };

    // Depth/stencil state - enable depth testing
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

    // Create render pipeline
    // Create array with the bind group layout pointer
    GfxBindGroupLayout bindGroupLayouts[] = { app->uniformBindGroupLayout };

    GfxRenderPipelineDescriptor pipelineDesc = {};
    pipelineDesc.sType = GFX_STRUCTURE_TYPE_RENDER_PIPELINE_DESCRIPTOR;
    pipelineDesc.pNext = NULL;
    pipelineDesc.label = "Cube Render Pipeline";
    pipelineDesc.vertex = &vertexState;
    pipelineDesc.fragment = &fragmentState;
    pipelineDesc.primitive = &primitiveState;
    pipelineDesc.depthStencil = &depthStencilState;
    pipelineDesc.sampleCount = app->settings.msaaSampleCount;
    pipelineDesc.renderPass = app->renderPass;
    pipelineDesc.bindGroupLayouts = bindGroupLayouts;
    pipelineDesc.bindGroupLayoutCount = 1;

    if (gfxDeviceCreateRenderPipeline(app->device, &pipelineDesc, &app->renderPipeline) != GFX_RESULT_SUCCESS) {
        fprintf(stderr, "Failed to create render pipeline\n");
        return false;
    }

    return true;
}

static void destroyRenderPipeline(CubeApp* app)
{
    if (app->renderPipeline) {
        gfxRenderPipelineDestroy(app->renderPipeline);
        app->renderPipeline = NULL;
    }
}

void updateCube(CubeApp* app, int cubeIndex)
{
    UniformData uniforms = { 0 }; // Initialize to zero!

    // Create rotation matrices (combine X and Y rotations)
    // Each cube rotates slightly differently
    float rotX[16], rotY[16], tempModel[16];
    matrixRotateX(rotX, (app->rotationAngleX + cubeIndex * 30.0f) * M_PI / 180.0f);
    matrixRotateY(rotY, (app->rotationAngleY + cubeIndex * 45.0f) * M_PI / 180.0f);
    matrixMultiply(tempModel, rotY, rotX);

    // Position cubes side by side: left (-3, 0, 0), center (0, 0, 0), right (3, 0, 0)
    float translation[16];
    matrixIdentity(translation);
    translation[12] = -(float)CUBE_COUNT * 0.5 + (cubeIndex - 1) * 1.5f; // x offset: -3, 0, 3

    // Apply translation after rotation: model = rotation * translation
    // This rotates in place, then translates to world position
    matrixMultiply(uniforms.model, tempModel, translation);

    // Create view matrix (camera positioned at 0, 0, 10 looking at origin)
    matrixLookAt(uniforms.view,
        0.0f, 0.0f, 10.0f, // eye position - pulled back to see all 3 cubes
        0.0f, 0.0f, 0.0f, // look at point
        0.0f, 1.0f, 0.0f); // up vector

    // Create perspective projection matrix
    float aspect = (float)app->swapchainInfo.extent.width / (float)app->swapchainInfo.extent.height;
    matrixPerspective(uniforms.projection,
        45.0f * M_PI / 180.0f, // 45 degree FOV
        aspect,
        0.1f, // near plane
        100.0f, // far plane
        app->adapterInfo.backend); // Adjust for clip space differences

    // Upload uniform data to buffer at aligned offset
    // Formula: (frame * CUBE_COUNT + cube) * alignedSize
    size_t offset = (app->currentFrame * CUBE_COUNT + cubeIndex) * app->uniformAlignedSize;
    gfxQueueWriteBuffer(app->queue, app->sharedUniformBuffer, offset, &uniforms, sizeof(uniforms));
}

static GfxPlatformWindowHandle getPlatformWindowHandle(GLFWwindow* window)
{
    GfxPlatformWindowHandle handle = { 0 };
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

static float getCurrentTime(void)
{
#if defined(__EMSCRIPTEN__)
    return (float)emscripten_get_now() / 1000.0f;
#else
    return (float)glfwGetTime();
#endif
}

// Helper function to load binary files (SPIR-V shaders)
static void* loadBinaryFile(const char* filepath, size_t* outSize)
{
    FILE* file = fopen(filepath, "rb");
    if (!file) {
        fprintf(stderr, "Failed to open file: %s\n", filepath);
        return NULL;
    }

    // Get file size
    fseek(file, 0, SEEK_END);
    long fileSize = ftell(file);
    fseek(file, 0, SEEK_SET);

    if (fileSize <= 0) {
        fprintf(stderr, "Invalid file size for: %s\n", filepath);
        fclose(file);
        return NULL;
    }

    // Allocate buffer
    void* buffer = malloc(fileSize);
    if (!buffer) {
        fprintf(stderr, "Failed to allocate memory for file: %s\n", filepath);
        fclose(file);
        return NULL;
    }

    // Read file
    size_t bytesRead = fread(buffer, 1, fileSize, file);
    fclose(file);

    if (bytesRead != (size_t)fileSize) {
        fprintf(stderr, "Failed to read complete file: %s\n", filepath);
        free(buffer);
        return NULL;
    }

    *outSize = fileSize;
    return buffer;
}

static void* loadTextFile(const char* filepath, size_t* outSize)
{
    FILE* file = fopen(filepath, "r");
    if (!file) {
        fprintf(stderr, "Failed to open file: %s\n", filepath);
        return NULL;
    }

    // Get file size
    fseek(file, 0, SEEK_END);
    long fileSize = ftell(file);
    fseek(file, 0, SEEK_SET);

    if (fileSize <= 0) {
        fprintf(stderr, "Invalid file size for: %s\n", filepath);
        fclose(file);
        return NULL;
    }

    // Allocate buffer with extra byte for null terminator
    char* buffer = (char*)malloc(fileSize + 1);
    if (!buffer) {
        fprintf(stderr, "Failed to allocate memory for file: %s\n", filepath);
        fclose(file);
        return NULL;
    }

    // Read file
    size_t bytesRead = fread(buffer, 1, fileSize, file);
    fclose(file);

    if (bytesRead != (size_t)fileSize) {
        fprintf(stderr, "Failed to read complete file: %s\n", filepath);
        free(buffer);
        return NULL;
    }

    // Null-terminate for text files
    buffer[fileSize] = '\0';

    // Return size including null terminator for shader code
    *outSize = fileSize + 1;
    return buffer;
}

// Record commands for a single cube - called by worker threads OR main thread
static void recordCubeCommands(CubeApp* app, int cubeIndex, uint32_t imageIndex)
{
    PerFrameResources* frame = &app->frameResources[app->currentFrame];
    GfxCommandEncoder encoder = frame->cubeEncoders[cubeIndex];
    gfxCommandEncoderBegin(encoder);

    // Begin render pass using pre-created render pass and framebuffer
    GfxColor clearColor = { 0.1f, 0.2f, 0.3f, 1.0f };

    GfxRenderPassBeginDescriptor beginDesc = {
        .label = "Main Render Pass",
        .renderPass = app->renderPass,
        .framebuffer = app->framebuffers[imageIndex],
        .colorClearValues = &clearColor,
        .colorClearValueCount = 1,
        .depthClearValue = 1.0f,
        .stencilClearValue = 0
    };

    // Override load ops: first cube clears, others load
    // Note: This requires modifying the render pass dynamically, which isn't ideal
    // For now, we'll clear on first cube only by modifying the clear color alpha to signal

    GfxRenderPassEncoder renderPass;
    if (gfxCommandEncoderBeginRenderPass(encoder, &beginDesc, &renderPass) == GFX_RESULT_SUCCESS) {

        // Set pipeline
        gfxRenderPassEncoderSetPipeline(renderPass, app->renderPipeline);

        // Set viewport and scissor to fill the entire render target
        GfxViewport viewport = { 0.0f, 0.0f, (float)app->swapchainInfo.extent.width, (float)app->swapchainInfo.extent.height, 0.0f, 1.0f };
        GfxScissorRect scissor = { { 0, 0 }, { app->swapchainInfo.extent.width, app->swapchainInfo.extent.height } };
        gfxRenderPassEncoderSetViewport(renderPass, &viewport);
        gfxRenderPassEncoderSetScissorRect(renderPass, &scissor);

        // Set vertex buffer
        GfxBufferInfo vertexBufferInfo;
        if (gfxBufferGetInfo(app->vertexBuffer, &vertexBufferInfo) != GFX_RESULT_SUCCESS) {
            fprintf(stderr, "Failed to get vertex buffer info\n");
            return;
        }
        gfxRenderPassEncoderSetVertexBuffer(renderPass, 0, app->vertexBuffer, 0,
            vertexBufferInfo.size);

        // Set index buffer
        GfxBufferInfo indexBufferInfo;
        if (gfxBufferGetInfo(app->indexBuffer, &indexBufferInfo) != GFX_RESULT_SUCCESS) {
            fprintf(stderr, "Failed to get index buffer info\n");
            return;
        }
        gfxRenderPassEncoderSetIndexBuffer(renderPass, app->indexBuffer,
            GFX_INDEX_FORMAT_UINT16, 0,
            indexBufferInfo.size);

        // Bind this cube's bind group and draw
        gfxRenderPassEncoderSetBindGroup(renderPass, 0, frame->uniformBindGroups[cubeIndex], NULL, 0);

        // Draw indexed (36 indices for the cube)
        gfxRenderPassEncoderDrawIndexed(renderPass, 36, 1, 0, 0, 0);

        // End render pass
        gfxRenderPassEncoderEnd(renderPass);
    }

    // Finish command encoding
    gfxCommandEncoderEnd(encoder);
}

// Record clear commands - just begin render pass with CLEAR and end
static void recordClearCommands(CubeApp* app, uint32_t imageIndex)
{
    PerFrameResources* frame = &app->frameResources[app->currentFrame];
    GfxCommandEncoder encoder = frame->clearEncoder;
    gfxCommandEncoderBegin(encoder);

    GfxColor clearColor = { 0.1f, 0.2f, 0.3f, 1.0f };

    GfxRenderPassBeginDescriptor beginDesc = {
        .label = "Clear Pass",
        .renderPass = app->clearRenderPass, // Use clear render pass
        .framebuffer = app->framebuffers[imageIndex],
        .colorClearValues = &clearColor,
        .colorClearValueCount = 1,
        .depthClearValue = 1.0f,
        .stencilClearValue = 0
    };

    GfxRenderPassEncoder renderPass;
    if (gfxCommandEncoderBeginRenderPass(encoder, &beginDesc, &renderPass) == GFX_RESULT_SUCCESS) {
        // Just clear - don't draw anything
        gfxRenderPassEncoderEnd(renderPass);
    }

    gfxCommandEncoderEnd(encoder);
}

// Record final resolve commands - just begin render pass with LOAD and let it resolve
static void recordResolveCommands(CubeApp* app, uint32_t imageIndex)
{
    PerFrameResources* frame = &app->frameResources[app->currentFrame];
    GfxCommandEncoder encoder = frame->resolveEncoder;
    gfxCommandEncoderBegin(encoder);

    GfxRenderPassBeginDescriptor beginDesc = {
        .label = "Final Resolve Pass",
        .renderPass = app->resolveRenderPass,
        .framebuffer = app->framebuffers[imageIndex],
        .colorClearValues = NULL, // Not used with LOAD
        .colorClearValueCount = 0,
        .depthClearValue = 1.0f,
        .stencilClearValue = 0
    };

    GfxRenderPassEncoder renderPass;
    if (gfxCommandEncoderBeginRenderPass(encoder, &beginDesc, &renderPass) == GFX_RESULT_SUCCESS) {
        // Just begin and end - the resolve happens automatically
        gfxRenderPassEncoderEnd(renderPass);
    }

    gfxCommandEncoderEnd(encoder);
}

// Record a simple layout transition from COLOR_ATTACHMENT to PRESENT_SRC (for MSAA=1 only)
static void recordLayoutTransition(CubeApp* app, uint32_t imageIndex)
{
    PerFrameResources* frame = &app->frameResources[app->currentFrame];
    GfxCommandEncoder encoder = frame->transitionEncoder;
    gfxCommandEncoderBegin(encoder);

    // Use an empty render pass to transition layout via initialLayout/finalLayout
    GfxRenderPassBeginDescriptor beginDesc = {
        .label = "Layout Transition Pass",
        .renderPass = app->transitionRenderPass,
        .framebuffer = app->framebuffers[imageIndex],
        .colorClearValues = NULL,
        .colorClearValueCount = 0,
        .depthClearValue = 1.0f,
        .stencilClearValue = 0
    };

    GfxRenderPassEncoder renderPass;
    if (gfxCommandEncoderBeginRenderPass(encoder, &beginDesc, &renderPass) == GFX_RESULT_SUCCESS) {
        // Empty pass - just begin and end to trigger layout transition
        gfxRenderPassEncoderEnd(renderPass);
    }

    gfxCommandEncoderEnd(encoder);
}

#if USE_THREADING
// Threading support functions
bool createThreading(CubeApp* app)
{
    // Initialize barrier for CUBE_COUNT threads + 1 main thread
    if (pthread_barrier_init(&app->recordBarrier, NULL, CUBE_COUNT + 1) != 0) {
        fprintf(stderr, "Failed to initialize pthread barrier\n");
        return false;
    }

    app->threadsRunning = true;

    // Create worker threads
    for (int i = 0; i < CUBE_COUNT; i++) {
        app->threadData[i].app = app;
        app->threadData[i].cubeIndex = i;
        app->threadData[i].barrier = &app->recordBarrier;

        if (pthread_create(&app->cubeThreads[i], NULL, cubeRecordThread, &app->threadData[i]) != 0) {
            fprintf(stderr, "Failed to create cube thread %d\n", i);
            return false;
        }
    }

    printf("Created %d worker threads for parallel command recording\n", CUBE_COUNT);
    return true;
}

void destroyThreading(CubeApp* app)
{
    if (!app->threadsRunning) {
        return;
    }

    // Signal threads to exit
    app->threadsRunning = false;

    // Wake up threads
    pthread_barrier_wait(&app->recordBarrier);

    // Join threads
    for (int i = 0; i < CUBE_COUNT; i++) {
        pthread_join(app->cubeThreads[i], NULL);
    }

    pthread_barrier_destroy(&app->recordBarrier);
    printf("Cleaned up worker threads\n");
}

static void* cubeRecordThread(void* arg)
{
    CubeThreadData* data = (CubeThreadData*)arg;
    printf("Cube thread %d started\n", data->cubeIndex);

    while (data->app->threadsRunning) {
        // Wait for signal to start recording
        pthread_barrier_wait(data->barrier);

        if (!data->app->threadsRunning) {
            break;
        }

        // Record commands for this cube
        uint32_t imageIndex = data->app->currentImageIndex;
        recordCubeCommands(data->app, data->cubeIndex, imageIndex);

        // Wait for all threads to finish recording
        pthread_barrier_wait(data->barrier);
    }

    printf("Cube thread %d exiting\n", data->cubeIndex);
    return NULL;
}
#endif

// Matrix math utility functions
void matrixIdentity(float* matrix)
{
    memset(matrix, 0, 16 * sizeof(float));
    matrix[0] = matrix[5] = matrix[10] = matrix[15] = 1.0f;
}

void matrixMultiply(float* result, const float* a, const float* b)
{
    float temp[16];
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            temp[i * 4 + j] = 0;
            for (int k = 0; k < 4; k++) {
                temp[i * 4 + j] += a[i * 4 + k] * b[k * 4 + j];
            }
        }
    }
    memcpy(result, temp, sizeof(float) * 16);
}

void matrixRotateX(float* matrix, float angle)
{
    float c = cosf(angle);
    float s = sinf(angle);

    matrixIdentity(matrix);
    matrix[5] = c;
    matrix[6] = -s;
    matrix[9] = s;
    matrix[10] = c;
}

void matrixRotateY(float* matrix, float angle)
{
    float c = cosf(angle);
    float s = sinf(angle);

    matrixIdentity(matrix);
    matrix[0] = c;
    matrix[2] = s;
    matrix[8] = -s;
    matrix[10] = c;
}

void matrixRotateZ(float* matrix, float angle)
{
    float c = cosf(angle);
    float s = sinf(angle);

    matrixIdentity(matrix);
    matrix[0] = c;
    matrix[1] = -s;
    matrix[4] = s;
    matrix[5] = c;
}

void matrixPerspective(float* matrix, float fov, float aspect, float near, float far, GfxBackend backend)
{
    memset(matrix, 0, 16 * sizeof(float));

    float f = 1.0f / tanf(fov / 2.0f);

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

    // Normalize right vector (check if forward and up are parallel)
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

// Normalize a 3D vector in place. Returns false if vector is too small to normalize.
bool vectorNormalize(float* x, float* y, float* z)
{
    const float epsilon = 1e-6f;
    float len = sqrtf((*x) * (*x) + (*y) * (*y) + (*z) * (*z));

    if (len < epsilon) {
        return false;
    }

    *x /= len;
    *y /= len;
    *z /= len;
    return true;
}

static bool init(CubeApp* app)
{
    // Initialize in order of dependencies

    // 1. Create window
    if (!createWindow(app, WINDOW_WIDTH, WINDOW_HEIGHT)) {
        fprintf(stderr, "Failed to create window\n");
        return false;
    }

    // 2. Create graphics context (instance, adapter, device, surface)
    if (!createGraphics(app)) {
        fprintf(stderr, "Failed to create graphics\n");
        return false;
    }

    // 3. Create size-dependent resources (swapchain, framebuffers, render pass)
    if (!createSizeDependentResources(app, app->windowWidth, app->windowHeight)) {
        fprintf(stderr, "Failed to create size-dependent resources\n");
        return false;
    }

    // 4. Create rendering resources (textures, buffers, layouts)
    if (!createRenderingResources(app)) {
        fprintf(stderr, "Failed to create rendering resources\n");
        return false;
    }

    // 5. Create per-frame resources (semaphores, fences, command encoders)
    if (!createPerFrameResources(app)) {
        fprintf(stderr, "Failed to create per-frame resources\n");
        return false;
    }

    // 6. Create render pipeline
    if (!createRenderPipeline(app)) {
        fprintf(stderr, "Failed to create render pipeline\n");
        return false;
    }

#if USE_THREADING
    // 7. Create threading infrastructure
    if (!createThreading(app)) {
        fprintf(stderr, "Failed to create threading\n");
        return false;
    }
#endif

    // Initialize loop state
    app->currentFrame = 0;
    app->previousWidth = app->windowWidth;
    app->previousHeight = app->windowHeight;
    app->lastTime = getCurrentTime();

    // Initialize animation state
    app->rotationAngleX = 0.0f;
    app->rotationAngleY = 0.0f;

    // Initialize FPS tracking
    app->fpsFrameCount = 0;
    app->fpsTimeAccumulator = 0.0f;
    app->fpsFrameTimeMin = FLT_MAX;
    app->fpsFrameTimeMax = 0.0f;

    printf("Application initialized successfully!\n");
#if USE_THREADING
    printf("Running with %d worker threads for parallel command recording\n", CUBE_COUNT);
#else
    printf("Running in single-threaded mode\n");
#endif
    return true;
}

static void cleanup(CubeApp* app)
{
#if USE_THREADING
    // Clean up threading infrastructure first
    destroyThreading(app);
#endif

    // Wait for device to finish all GPU work before cleanup
    if (app->device) {
        gfxDeviceWaitIdle(app->device);
    }

    // Destroy in reverse order of creation for symmetry

    // 6. Destroy render pipeline (depends on render pass and resources)
    destroyRenderPipeline(app);

    // 5. Destroy per-frame resources (depends on uniform buffer and layouts)
    destroyPerFrameResources(app);

    // 4. Destroy rendering resources (textures, buffers, layouts)
    destroyRenderingResources(app);

    // 3. Destroy size-dependent resources (swapchain, framebuffers, render pass)
    destroySizeDependentResources(app);

    // 2. Destroy graphics context (surface, device, instance)
    destroyGraphics(app);

    // 1. Destroy window
    destroyWindow(app);
}

void update(CubeApp* app, float deltaTime)
{
    // Update rotation angles (both X and Y axes)
    app->rotationAngleX += 45.0f * deltaTime; // 45 degrees per second around X
    app->rotationAngleY += 30.0f * deltaTime; // 30 degrees per second around Y
    if (app->rotationAngleX >= 360.0f) {
        app->rotationAngleX -= 360.0f;
    }
    if (app->rotationAngleY >= 360.0f) {
        app->rotationAngleY -= 360.0f;
    }

    // Update uniforms for each cube
    for (int i = 0; i < CUBE_COUNT; ++i) {
        updateCube(app, i);
    }
}

void render(CubeApp* app)
{
    PerFrameResources* frame = &app->frameResources[app->currentFrame];

    // Wait for previous frame to finish
    gfxFenceWait(frame->inFlightFence, GFX_TIMEOUT_INFINITE);
    gfxFenceReset(frame->inFlightFence);

    // Acquire next swapchain image
    uint32_t imageIndex;
    GfxResult result = gfxSwapchainAcquireNextImage(app->swapchain, GFX_TIMEOUT_INFINITE,
        frame->imageAvailableSemaphore, NULL, &imageIndex);

    if (result != GFX_RESULT_SUCCESS) {
        fprintf(stderr, "Failed to acquire swapchain image\n");
        return;
    }

    // Record clear commands (always done by main thread)
    recordClearCommands(app, imageIndex);

#if USE_THREADING
    // Store image index for worker threads
    app->currentImageIndex = imageIndex;

    // Signal worker threads to start recording cube commands
    pthread_barrier_wait(&app->recordBarrier);

    // Wait for all threads to finish recording
    pthread_barrier_wait(&app->recordBarrier);

    // Submit clear encoder first (waits on imageAvailable, signals clearFinished)
    GfxSubmitDescriptor clearSubmit = {};
    clearSubmit.sType = GFX_STRUCTURE_TYPE_SUBMIT_DESCRIPTOR;
    clearSubmit.pNext = NULL;

    clearSubmit.commandEncoders = &frame->clearEncoder;
    clearSubmit.commandEncoderCount = 1;
    clearSubmit.waitSemaphores = &frame->imageAvailableSemaphore;
    clearSubmit.waitSemaphoreCount = 1;
    clearSubmit.signalSemaphores = &frame->clearFinishedSemaphore;
    clearSubmit.signalSemaphoreCount = 1;
    clearSubmit.signalFence = NULL; // Don't signal fence yet

    gfxQueueSubmit(app->queue, &clearSubmit);

    // Submit cube encoders (wait on clearFinished, signal renderFinished)
    GfxCommandEncoder cubeEncoderArray[CUBE_COUNT];
    for (int i = 0; i < CUBE_COUNT; i++) {
        cubeEncoderArray[i] = frame->cubeEncoders[i];
    }

    GfxSubmitDescriptor cubesSubmit = {};
    cubesSubmit.sType = GFX_STRUCTURE_TYPE_SUBMIT_DESCRIPTOR;
    cubesSubmit.pNext = NULL;

    cubesSubmit.commandEncoders = cubeEncoderArray;
    cubesSubmit.commandEncoderCount = CUBE_COUNT;
    cubesSubmit.waitSemaphores = &frame->clearFinishedSemaphore;
    cubesSubmit.waitSemaphoreCount = 1;

    if (app->settings.msaaSampleCount > GFX_SAMPLE_COUNT_1) {
        // When using MSAA, don't signal yet - resolve pass will signal
        cubesSubmit.signalSemaphores = NULL; // Don't signal yet, resolve pass will signal
        cubesSubmit.signalSemaphoreCount = 0;
        cubesSubmit.signalFence = NULL; // Don't signal fence yet
    } else {
        // When not using MSAA, don't signal yet - transition pass will signal
        cubesSubmit.signalSemaphores = NULL;
        cubesSubmit.signalSemaphoreCount = 0;
        cubesSubmit.signalFence = NULL;
    }

    gfxQueueSubmit(app->queue, &cubesSubmit);

    // Record and submit layout transition (only when MSAA == 1)
    if (app->settings.msaaSampleCount == GFX_SAMPLE_COUNT_1) {
        recordLayoutTransition(app, imageIndex);

        GfxSubmitDescriptor transitionSubmit = {};
        transitionSubmit.sType = GFX_STRUCTURE_TYPE_SUBMIT_DESCRIPTOR;
        transitionSubmit.pNext = NULL;

        transitionSubmit.commandEncoders = &frame->transitionEncoder;
        transitionSubmit.commandEncoderCount = 1;
        transitionSubmit.waitSemaphores = NULL; // No wait needed, depends on cube submits via queue ordering
        transitionSubmit.waitSemaphoreCount = 0;
        transitionSubmit.signalSemaphores = &frame->renderFinishedSemaphore;
        transitionSubmit.signalSemaphoreCount = 1;
        transitionSubmit.signalFence = frame->inFlightFence;

        gfxQueueSubmit(app->queue, &transitionSubmit);
    }

    // Record and submit final resolve pass (only when MSAA > 1)
    if (app->settings.msaaSampleCount > GFX_SAMPLE_COUNT_1) {
        recordResolveCommands(app, imageIndex);

        GfxSubmitDescriptor resolveSubmit = {};
        resolveSubmit.sType = GFX_STRUCTURE_TYPE_SUBMIT_DESCRIPTOR;
        resolveSubmit.pNext = NULL;

        resolveSubmit.commandEncoders = &frame->resolveEncoder;
        resolveSubmit.commandEncoderCount = 1;
        resolveSubmit.waitSemaphores = NULL; // No wait needed, depends on cube submits via queue ordering
        resolveSubmit.waitSemaphoreCount = 0;
        resolveSubmit.signalSemaphores = &frame->renderFinishedSemaphore;
        resolveSubmit.signalSemaphoreCount = 1;
        resolveSubmit.signalFence = frame->inFlightFence;

        gfxQueueSubmit(app->queue, &resolveSubmit);
    }
#else
    // Non-threaded path for WebGPU - record all cubes in ONE render pass
    GfxCommandEncoder encoder = frame->cubeEncoders[0];
    gfxCommandEncoderBegin(encoder);

    // Begin render pass with CLEAR (not using separate clear pass for WebGPU)
    GfxColor clearColor = { 0.1f, 0.2f, 0.3f, 1.0f };
    GfxRenderPassBeginDescriptor beginDesc = {
        .label = "Main Render Pass (All Cubes)",
        .renderPass = app->clearRenderPass, // Use clear pass to properly clear
        .framebuffer = app->framebuffers[imageIndex],
        .colorClearValues = &clearColor,
        .colorClearValueCount = 1,
        .depthClearValue = 1.0f,
        .stencilClearValue = 0
    };

    GfxRenderPassEncoder renderPass;
    if (gfxCommandEncoderBeginRenderPass(encoder, &beginDesc, &renderPass) == GFX_RESULT_SUCCESS) {
        // Set pipeline
        gfxRenderPassEncoderSetPipeline(renderPass, app->renderPipeline);

        // Set viewport and scissor
        GfxViewport viewport = { 0.0f, 0.0f, (float)app->swapchainInfo.extent.width, (float)app->swapchainInfo.extent.height, 0.0f, 1.0f };
        GfxScissorRect scissor = { { 0, 0 }, { app->swapchainInfo.extent.width, app->swapchainInfo.extent.height } };
        gfxRenderPassEncoderSetViewport(renderPass, &viewport);
        gfxRenderPassEncoderSetScissorRect(renderPass, &scissor);

        // Set vertex buffer once
        GfxBufferInfo vertexBufferInfo;
        if (gfxBufferGetInfo(app->vertexBuffer, &vertexBufferInfo) == GFX_RESULT_SUCCESS) {
            gfxRenderPassEncoderSetVertexBuffer(renderPass, 0, app->vertexBuffer, 0, vertexBufferInfo.size);
        }

        // Set index buffer once
        GfxBufferInfo indexBufferInfo;
        if (gfxBufferGetInfo(app->indexBuffer, &indexBufferInfo) == GFX_RESULT_SUCCESS) {
            gfxRenderPassEncoderSetIndexBuffer(renderPass, app->indexBuffer, GFX_INDEX_FORMAT_UINT16, 0, indexBufferInfo.size);
        }

        // Draw all 3 cubes in ONE render pass
        for (int cubeIdx = 0; cubeIdx < CUBE_COUNT; cubeIdx++) {
            gfxRenderPassEncoderSetBindGroup(renderPass, 0, frame->uniformBindGroups[cubeIdx], NULL, 0);
            gfxRenderPassEncoderDrawIndexed(renderPass, 36, 1, 0, 0, 0);
        }

        gfxRenderPassEncoderEnd(renderPass);
    }

    gfxCommandEncoderEnd(encoder);

    // Submit single command buffer for WebGPU
    GfxSubmitDescriptor submitDescriptor = {};
    submitDescriptor.sType = GFX_STRUCTURE_TYPE_SUBMIT_DESCRIPTOR;
    submitDescriptor.pNext = NULL;

    submitDescriptor.commandEncoders = &encoder;
    submitDescriptor.commandEncoderCount = 1;
    submitDescriptor.waitSemaphores = &frame->imageAvailableSemaphore;
    submitDescriptor.waitSemaphoreCount = 1;
    submitDescriptor.signalSemaphores = &frame->renderFinishedSemaphore;
    submitDescriptor.signalSemaphoreCount = 1;
    submitDescriptor.signalFence = frame->inFlightFence;

    gfxQueueSubmit(app->queue, &submitDescriptor);
#endif

    // Present with synchronization
    GfxPresentDescriptor presentDescriptor = {};
    presentDescriptor.sType = GFX_STRUCTURE_TYPE_PRESENT_DESCRIPTOR;
    presentDescriptor.pNext = NULL;
    presentDescriptor.waitSemaphores = &frame->renderFinishedSemaphore;
    presentDescriptor.waitSemaphoreCount = 1;
    gfxSwapchainPresent(app->swapchain, &presentDescriptor);

    // Move to next frame
    app->currentFrame = (app->currentFrame + 1) % app->framesInFlight;
}

// Returns false if loop should exit
static bool mainLoopIteration(CubeApp* app)
{
    if (glfwWindowShouldClose(app->window)) {
        return false;
    }

    glfwPollEvents();

    // Handle framebuffer resize
    if (app->previousWidth != app->windowWidth || app->previousHeight != app->windowHeight) {
        // Wait for all in-flight frames to complete
        gfxDeviceWaitIdle(app->device);

        // Recreate only size-dependent resources (including swapchain)
        gfxDeviceWaitIdle(app->device); // Wait for GPU to finish before destroying resources
        destroySizeDependentResources(app);
        if (!createSizeDependentResources(app, app->windowWidth, app->windowHeight)) {
            fprintf(stderr, "Failed to recreate size-dependent resources after resize\n");
            return false;
        }

        app->previousWidth = app->windowWidth;
        app->previousHeight = app->windowHeight;

        printf("Window resized: %dx%d\n", app->windowWidth, app->windowHeight);
        return true; // Skip rendering this frame
    }

    // Calculate delta time
    float currentTime = getCurrentTime();
    float deltaTime = currentTime - app->lastTime;
    app->lastTime = currentTime;

    // Track FPS
    if (deltaTime > 0.0f) {
        app->fpsFrameCount++;
        app->fpsTimeAccumulator += deltaTime;

        if (deltaTime < app->fpsFrameTimeMin) {
            app->fpsFrameTimeMin = deltaTime;
        }
        if (deltaTime > app->fpsFrameTimeMax) {
            app->fpsFrameTimeMax = deltaTime;
        }

        // Log FPS every second
        if (app->fpsTimeAccumulator >= 1.0f) {
            float avgFPS = (float)app->fpsFrameCount / app->fpsTimeAccumulator;
            float avgFrameTime = (app->fpsTimeAccumulator / (float)app->fpsFrameCount) * 1000.0f;
            float minFPS = 1.0f / app->fpsFrameTimeMax;
            float maxFPS = 1.0f / app->fpsFrameTimeMin;
            printf("FPS - Avg: %.1f, Min: %.1f, Max: %.1f | Frame Time - Avg: %.2f ms, Min: %.2f ms, Max: %.2f ms\n",
                avgFPS, minFPS, maxFPS,
                avgFrameTime, app->fpsFrameTimeMin * 1000.0f, app->fpsFrameTimeMax * 1000.0f);

            // Reset for next second
            app->fpsFrameCount = 0;
            app->fpsTimeAccumulator = 0.0f;
            app->fpsFrameTimeMin = FLT_MAX;
            app->fpsFrameTimeMax = 0.0f;
        }
    }

    update(app, deltaTime);
    render(app);

    return true;
}

#if defined(__EMSCRIPTEN__)
static void emscriptenMainLoop(void* userData)
{
    CubeApp* app = (CubeApp*)userData;
    if (!mainLoopIteration(app)) {
        emscripten_cancel_main_loop();
        cleanup(app);
    }
}
#endif

// Parse command line arguments
static bool parseArguments(int argc, char** argv, Settings* settings)
{
#if defined(__EMSCRIPTEN__)
    settings->backend = GFX_BACKEND_WEBGPU;
#else
    settings->backend = GFX_BACKEND_VULKAN;
#endif
    settings->msaaSampleCount = GFX_SAMPLE_COUNT_4;
    settings->vsync = true; // VSync on by default

    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--backend") == 0 && i + 1 < argc) {
            i++;
            if (strcmp(argv[i], "vulkan") == 0) {
                settings->backend = GFX_BACKEND_VULKAN;
            } else if (strcmp(argv[i], "webgpu") == 0) {
                settings->backend = GFX_BACKEND_WEBGPU;
            } else {
                fprintf(stderr, "Unknown backend: %s\\n", argv[i]);
                return false;
            }
        } else if (strcmp(argv[i], "--msaa") == 0 && i + 1 < argc) {
            i++;
            int samples = atoi(argv[i]);
            switch (samples) {
            case 1:
                settings->msaaSampleCount = GFX_SAMPLE_COUNT_1;
                break;
            case 2:
                settings->msaaSampleCount = GFX_SAMPLE_COUNT_2;
                break;
            case 4:
                settings->msaaSampleCount = GFX_SAMPLE_COUNT_4;
                break;
            case 8:
                settings->msaaSampleCount = GFX_SAMPLE_COUNT_8;
                break;
            case 16:
                settings->msaaSampleCount = GFX_SAMPLE_COUNT_16;
                break;
            case 32:
                settings->msaaSampleCount = GFX_SAMPLE_COUNT_32;
                break;
            case 64:
                settings->msaaSampleCount = GFX_SAMPLE_COUNT_64;
                break;
            default:
                fprintf(stderr, "Invalid MSAA sample count: %d\\n", samples);
                fprintf(stderr, "Valid values: 1, 2, 4, 8, 16, 32, 64\\n");
                return false;
            }
        } else if (strcmp(argv[i], "--vsync") == 0 && i + 1 < argc) {
            i++;
            int vsync = atoi(argv[i]);
            if (vsync == 0) {
                settings->vsync = false;
            } else if (vsync == 1) {
                settings->vsync = true;
            } else {
                fprintf(stderr, "Invalid vsync value: %s\\n", argv[i]);
                fprintf(stderr, "Valid values: 0 (off), 1 (on)\\n");
                return false;
            }
        } else if (strcmp(argv[i], "--help") == 0) {
            printf("Usage: %s [options]\\n", argv[0]);
            printf("Options:\\n");
            printf("  --backend [vulkan|webgpu]   Select graphics backend\\n");
            printf("  --msaa [1|2|4|8]            Select MSAA sample count\\n");
            printf("  --vsync [0|1]               VSync: 0=off, 1=on\\n");
            printf("  --help                      Show this help message\\n");
            return false;
        } else {
            fprintf(stderr, "Unknown argument: %s\\n", argv[i]);
            return false;
        }
    }

    return true;
}

int main(int argc, char** argv)
{
    printf("=== Threaded Cube Example with Parallel Command Recording (C) ===\n\n");

    CubeApp app = { 0 }; // Initialize all members to NULL/0

    // Parse command line arguments
    if (!parseArguments(argc, argv, &app.settings)) {
        return 0;
    }

    // Initialize all resources and state
    if (!init(&app)) {
        cleanup(&app);
        return -1;
    }

    printf("Press ESC to exit\n\n");

    // Run main loop (platform-specific)
#if defined(__EMSCRIPTEN__)
    // Note: emscripten_set_main_loop_arg returns immediately and never blocks
    // Cleanup happens in emscriptenMainLoop when the loop exits
    // Execution continues in the browser event loop
    emscripten_set_main_loop_arg(emscriptenMainLoop, &app, 0, 1);
#else
    while (mainLoopIteration(&app)) {
        // Loop continues until mainLoopIteration returns false
    }

    printf("\nCleaning up resources...\n");
    cleanup(&app);
    printf("Example completed successfully!\n");
#endif

    return 0;
}
