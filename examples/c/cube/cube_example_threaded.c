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
#include <pthread.h>
#endif

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
#define MAX_FRAMES_IN_FLIGHT 3
#define CUBE_COUNT 12
#define MSAA_SAMPLE_COUNT GFX_SAMPLE_COUNT_4
#define COLOR_FORMAT GFX_TEXTURE_FORMAT_B8G8R8A8_UNORM_SRGB
#define DEPTH_FORMAT GFX_TEXTURE_FORMAT_DEPTH32_FLOAT

#if defined(__EMSCRIPTEN__)
#define GFX_BACKEND_API GFX_BACKEND_WEBGPU
#define USE_THREADING 0
#else
// Use Vulkan for better threading support
#define GFX_BACKEND_API GFX_BACKEND_VULKAN
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
    GfxSwapchain swapchain;
    GfxSwapchainInfo swapchainInfo;

    GfxBuffer vertexBuffer;
    GfxBuffer indexBuffer;
    GfxShader vertexShader;
    GfxShader fragmentShader;
    GfxRenderPass clearRenderPass; // For clear pass (UNDEFINED->COLOR_ATTACHMENT)
    GfxRenderPass renderPass; // For cube passes (LOAD from COLOR_ATTACHMENT)
    GfxRenderPass resolveRenderPass; // For final resolve pass (LOAD + resolve to swapchain)
    GfxRenderPipeline renderPipeline;
    GfxBindGroupLayout uniformBindGroupLayout;

    // Depth buffer
    GfxTexture depthTexture;
    GfxTextureView depthTextureView;

    // MSAA color buffer
    GfxTexture msaaColorTexture;
    GfxTextureView msaaColorTextureView;

    // Framebuffers (one per swapchain image)
    GfxFramebuffer framebuffers[MAX_FRAMES_IN_FLIGHT];

    uint32_t windowWidth;
    uint32_t windowHeight;

    // Per-frame resources (for frames in flight)
    GfxBuffer sharedUniformBuffer; // Single buffer for all frames
    size_t uniformAlignedSize; // Aligned size per frame
    GfxBindGroup uniformBindGroups[MAX_FRAMES_IN_FLIGHT][CUBE_COUNT]; // 3 cubes per frame

    // Clear encoder - submitted first to clear the framebuffer
    GfxCommandEncoder clearEncoders[MAX_FRAMES_IN_FLIGHT];

    // One command encoder per cube per frame - recorded in parallel
    GfxCommandEncoder cubeEncoders[MAX_FRAMES_IN_FLIGHT][CUBE_COUNT];

    // Final resolve encoder - resolves MSAA to swapchain after all cubes
    GfxCommandEncoder resolveEncoders[MAX_FRAMES_IN_FLIGHT];

    // Per-frame synchronization
    GfxSemaphore imageAvailableSemaphores[MAX_FRAMES_IN_FLIGHT];
    GfxSemaphore clearFinishedSemaphores[MAX_FRAMES_IN_FLIGHT]; // Signals when clear is done
    GfxSemaphore renderFinishedSemaphores[MAX_FRAMES_IN_FLIGHT];
    GfxFence inFlightFences[MAX_FRAMES_IN_FLIGHT];
    uint32_t currentFrame;

    // Animation state
    float rotationAngleX;
    float rotationAngleY;

    // Loop state
    uint32_t previousWidth;
    uint32_t previousHeight;
    float lastTime;

#if USE_THREADING
    // Threading infrastructure
    pthread_t cubeThreads[CUBE_COUNT];
    CubeThreadData threadData[CUBE_COUNT];
    pthread_barrier_t recordBarrier;
    volatile bool threadsRunning;
    volatile uint32_t currentImageIndex;
#endif
} CubeApp;

// Function declarations
static bool initWindow(CubeApp* app);
static bool initializeGraphics(CubeApp* app);
static bool createSyncObjects(CubeApp* app);
static bool createSwapchain(CubeApp* app, uint32_t width, uint32_t height);
static bool createTextures(CubeApp* app, uint32_t width, uint32_t height);
static bool createFrameBuffers(CubeApp* app, uint32_t width, uint32_t height);
static bool createRenderPass(CubeApp* app);
static bool createSizeDependentResources(CubeApp* app, uint32_t width, uint32_t height);
static void cleanupSizeDependentResources(CubeApp* app);
static bool createRenderingResources(CubeApp* app);
static void cleanupRenderingResources(CubeApp* app);
static bool createRenderPipeline(CubeApp* app);
static void updateCube(CubeApp* app, int cubeIndex);
static void update(CubeApp* app, float deltaTime);
static void render(CubeApp* app);
static void cleanup(CubeApp* app);
static GfxPlatformWindowHandle getPlatformWindowHandle(GLFWwindow* window);
static void* loadBinaryFile(const char* filepath, size_t* outSize);
static void* loadTextFile(const char* filepath, size_t* outSize);
static void recordCubeCommands(CubeApp* app, int cubeIndex, uint32_t imageIndex);

#if USE_THREADING
static bool initThreading(CubeApp* app);
static void cleanupThreading(CubeApp* app);
static void* cubeRecordThread(void* arg);
#endif

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

bool initWindow(CubeApp* app)
{
    glfwSetErrorCallback(errorCallback);

    if (!glfwInit()) {
        fprintf(stderr, "Failed to initialize GLFW\n");
        return false;
    }

    // Don't create OpenGL context
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

#if USE_THREADING
    app->window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT,
        "Threaded Cube Example - Parallel Command Recording",
        NULL, NULL);
#else
    app->window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT,
        "Cube Example - Unified Graphics API",
        NULL, NULL);
#endif

    if (!app->window) {
        fprintf(stderr, "Failed to create GLFW window\n");
        glfwTerminate();
        return false;
    }

    app->windowWidth = WINDOW_WIDTH;
    app->windowHeight = WINDOW_HEIGHT;

    glfwSetWindowUserPointer(app->window, app);
    glfwSetFramebufferSizeCallback(app->window, framebufferSizeCallback);
    glfwSetKeyCallback(app->window, keyCallback);

    return true;
}

GfxPlatformWindowHandle getPlatformWindowHandle(GLFWwindow* window)
{
    GfxPlatformWindowHandle handle = { 0 };
#if defined(__EMSCRIPTEN__)
    handle = gfxPlatformWindowHandleFromEmscripten("#canvas");

#elif defined(_WIN32)
    handle = gfxPlatformWindowHandleFromWin32(glfwGetWin32Window(window), GetModuleHandle(NULL));

#elif defined(__linux__)
    // Force using Xlib instead of XCB to avoid driver hang
    handle = gfxPlatformWindowHandleFromXlib(glfwGetX11Display(), glfwGetX11Window(window));

#elif defined(__APPLE__)
    handle = gfxPlatformWindowHandleFromMetal(glfwGetMetalLayer(window));
#endif

    return handle;
}

bool initializeGraphics(CubeApp* app)
{
    // Set up logging callback
    gfxSetLogCallback(logCallback, NULL);

    // Load the graphics backend BEFORE creating an instance
    // This is now decoupled - you load the backend API once at startup
    printf("Loading graphics backend...\n");
    if (gfxLoadBackend(GFX_BACKEND_API) != GFX_RESULT_SUCCESS) {
        fprintf(stderr, "Failed to load any graphics backend\n");
        return false;
    }
    printf("Graphics backend loaded successfully!\n");

    // Create graphics instance
    const char* instanceExtensions[] = { GFX_INSTANCE_EXTENSION_SURFACE, GFX_INSTANCE_EXTENSION_DEBUG };
    GfxInstanceDescriptor instanceDesc = {
        .backend = GFX_BACKEND_API,
        .applicationName = "Cube Example (C)",
        .applicationVersion = 1,
        .enabledExtensions = instanceExtensions,
        .enabledExtensionCount = 2
    };

    if (gfxCreateInstance(&instanceDesc, &app->instance) != GFX_RESULT_SUCCESS) {
        fprintf(stderr, "Failed to create graphics instance\n");
        return false;
    }

    // Get adapter
    GfxAdapterDescriptor adapterDesc = {
        .adapterIndex = UINT32_MAX, // Use preference-based selection
        .preference = GFX_ADAPTER_PREFERENCE_HIGH_PERFORMANCE
    };

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
    GfxDeviceDescriptor deviceDesc = {
        .label = "Main Device",
        .enabledExtensions = deviceExtensions,
        .enabledExtensionCount = 1
    };

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
    GfxSurfaceDescriptor surfaceDesc = {
        .label = "Main Surface",
        .windowHandle = windowHandle
    };

    if (gfxDeviceCreateSurface(app->device, &surfaceDesc, &app->surface) != GFX_RESULT_SUCCESS) {
        fprintf(stderr, "Failed to create surface\n");
        return false;
    }

    return true;
}

static bool createSwapchain(CubeApp* app, uint32_t width, uint32_t height)
{
    GfxSwapchainDescriptor swapchainDesc = {
        .label = "Main Swapchain",
        .width = (uint32_t)width,
        .height = (uint32_t)height,
        .format = COLOR_FORMAT,
        .usage = GFX_TEXTURE_USAGE_RENDER_ATTACHMENT,
        .presentMode = GFX_PRESENT_MODE_IMMEDIATE,
        .imageCount = MAX_FRAMES_IN_FLIGHT
    };

    if (gfxDeviceCreateSwapchain(app->device, app->surface, &swapchainDesc, &app->swapchain) != GFX_RESULT_SUCCESS) {
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

static bool createTextures(CubeApp* app, uint32_t width, uint32_t height)
{
    // Create depth texture (MSAA must match color attachment)
    GfxTextureDescriptor depthTextureDesc = {
        .label = "Depth Buffer",
        .type = GFX_TEXTURE_TYPE_2D,
        .size = {
            .width = width,
            .height = height,
            .depth = 1 },
        .arrayLayerCount = 1,
        .mipLevelCount = 1,
        .sampleCount = MSAA_SAMPLE_COUNT,
        .format = DEPTH_FORMAT,
        .usage = GFX_TEXTURE_USAGE_RENDER_ATTACHMENT
    };

    if (gfxDeviceCreateTexture(app->device, &depthTextureDesc, &app->depthTexture) != GFX_RESULT_SUCCESS) {
        fprintf(stderr, "Failed to create depth texture\n");
        return false;
    }

    // Create depth texture view
    GfxTextureViewDescriptor depthViewDesc = {
        .label = "Depth Buffer View",
        .viewType = GFX_TEXTURE_VIEW_TYPE_2D,
        .format = DEPTH_FORMAT,
        .baseMipLevel = 0,
        .mipLevelCount = 1,
        .baseArrayLayer = 0,
        .arrayLayerCount = 1
    };

    if (gfxTextureCreateView(app->depthTexture, &depthViewDesc, &app->depthTextureView) != GFX_RESULT_SUCCESS) {
        fprintf(stderr, "Failed to create depth texture view\n");
        return false;
    }

    // Create MSAA color texture (is unused if MSAA_SAMPLE_COUNT == 1)
    GfxTextureDescriptor msaaColorTextureDesc = {
        .label = "MSAA Color Buffer",
        .type = GFX_TEXTURE_TYPE_2D,
        .size = {
            .width = width,
            .height = height,
            .depth = 1 },
        .arrayLayerCount = 1,
        .mipLevelCount = 1,
        .sampleCount = MSAA_SAMPLE_COUNT,
        .format = app->swapchainInfo.format,
        .usage = GFX_TEXTURE_USAGE_RENDER_ATTACHMENT
    };

    if (gfxDeviceCreateTexture(app->device, &msaaColorTextureDesc, &app->msaaColorTexture) != GFX_RESULT_SUCCESS) {
        fprintf(stderr, "Failed to create MSAA color texture\n");
        return false;
    }

    // Create MSAA color texture view (is unused if MSAA_SAMPLE_COUNT == 1)
    GfxTextureViewDescriptor msaaColorViewDesc = {
        .label = "MSAA Color Buffer View",
        .viewType = GFX_TEXTURE_VIEW_TYPE_2D,
        .format = app->swapchainInfo.format,
        .baseMipLevel = 0,
        .mipLevelCount = 1,
        .baseArrayLayer = 0,
        .arrayLayerCount = 1
    };

    if (gfxTextureCreateView(app->msaaColorTexture, &msaaColorViewDesc, &app->msaaColorTextureView) != GFX_RESULT_SUCCESS) {
        fprintf(stderr, "Failed to create MSAA color texture view\n");
        return false;
    }

    return true;
}

static bool createFrameBuffers(CubeApp* app, uint32_t width, uint32_t height)
{
    // Create framebuffers (one per swapchain image)
    for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        GfxTextureView backbuffer = NULL;
        GfxResult result = gfxSwapchainGetTextureView(app->swapchain, i, &backbuffer);
        if (result != GFX_RESULT_SUCCESS || !backbuffer) {
            fprintf(stderr, "[ERROR] Failed to get swapchain image view %d\n", i);
            return false;
        }

        // Bundle color view with resolve target
        GfxFramebufferAttachment fbColorAttachment = {
            .view = (MSAA_SAMPLE_COUNT > GFX_SAMPLE_COUNT_1) ? app->msaaColorTextureView : backbuffer,
            .resolveTarget = (MSAA_SAMPLE_COUNT > GFX_SAMPLE_COUNT_1) ? backbuffer : NULL
        };

        // Depth/stencil attachment (no resolve)
        GfxFramebufferAttachment fbDepthAttachment = {
            .view = app->depthTextureView,
            .resolveTarget = NULL
        };

        char label[64];
        snprintf(label, sizeof(label), "Framebuffer %u", i);

        GfxFramebufferDescriptor fbDesc = {
            .label = label,
            .renderPass = app->resolveRenderPass, // Use resolve pass as template (has resolve target)
            .colorAttachments = &fbColorAttachment,
            .colorAttachmentCount = 1,
            .depthStencilAttachment = fbDepthAttachment,
            .width = width,
            .height = height
        };

        if (gfxDeviceCreateFramebuffer(app->device, &fbDesc, &app->framebuffers[i]) != GFX_RESULT_SUCCESS) {
            fprintf(stderr, "Failed to create framebuffer %u\n", i);
            return false;
        }
    }

    return true;
}

bool createSizeDependentResources(CubeApp* app, uint32_t width, uint32_t height)
{
    if (!createSwapchain(app, width, height)) {
        return false;
    }

    // Use actual swapchain dimensions (may differ from requested window size)
    uint32_t swapchainWidth = app->swapchainInfo.width;
    uint32_t swapchainHeight = app->swapchainInfo.height;

    if (!createTextures(app, swapchainWidth, swapchainHeight)) {
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

bool createSyncObjects(CubeApp* app)
{
    // Create semaphores and fences for each frame in flight
    GfxSemaphoreDescriptor semaphoreDesc = {
        .label = "Semaphore",
        .type = GFX_SEMAPHORE_TYPE_BINARY,
        .initialValue = 0
    };

    GfxFenceDescriptor fenceDesc = {
        .label = "Fence",
        .signaled = true // Start signaled so first frame doesn't wait
    };

    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        char label[64];

        snprintf(label, sizeof(label), "Image Available Semaphore %d", i);
        semaphoreDesc.label = label;
        if (gfxDeviceCreateSemaphore(app->device, &semaphoreDesc, &app->imageAvailableSemaphores[i]) != GFX_RESULT_SUCCESS) {
            fprintf(stderr, "Failed to create image available semaphore %d\n", i);
            return false;
        }

        snprintf(label, sizeof(label), "Clear Finished Semaphore %d", i);
        semaphoreDesc.label = label;
        if (gfxDeviceCreateSemaphore(app->device, &semaphoreDesc, &app->clearFinishedSemaphores[i]) != GFX_RESULT_SUCCESS) {
            fprintf(stderr, "Failed to create clear finished semaphore %d\n", i);
            return false;
        }

        snprintf(label, sizeof(label), "Render Finished Semaphore %d", i);
        semaphoreDesc.label = label;
        if (gfxDeviceCreateSemaphore(app->device, &semaphoreDesc, &app->renderFinishedSemaphores[i]) != GFX_RESULT_SUCCESS) {
            fprintf(stderr, "Failed to create render finished semaphore %d\n", i);
            return false;
        }

        snprintf(label, sizeof(label), "In Flight Fence %d", i);
        fenceDesc.label = label;
        if (gfxDeviceCreateFence(app->device, &fenceDesc, &app->inFlightFences[i]) != GFX_RESULT_SUCCESS) {
            fprintf(stderr, "Failed to create in flight fence %d\n", i);
            return false;
        }

        // Create clear encoder
        snprintf(label, sizeof(label), "Clear Encoder Frame %d", i);
        GfxCommandEncoderDescriptor encoderDesc = { .label = label };
        if (gfxDeviceCreateCommandEncoder(app->device, &encoderDesc, &app->clearEncoders[i]) != GFX_RESULT_SUCCESS) {
            fprintf(stderr, "Failed to create clear encoder %d\n", i);
            return false;
        }

        // Create command encoders - one per cube for parallel recording
        for (int cubeIdx = 0; cubeIdx < CUBE_COUNT; ++cubeIdx) {
            snprintf(label, sizeof(label), "Command Encoder Frame %d Cube %d", i, cubeIdx);
            GfxCommandEncoderDescriptor encoderDesc = { .label = label };
            if (gfxDeviceCreateCommandEncoder(app->device, &encoderDesc, &app->cubeEncoders[i][cubeIdx]) != GFX_RESULT_SUCCESS) {
                fprintf(stderr, "Failed to create command encoder %d cube %d\n", i, cubeIdx);
                return false;
            }
        }

        // Create resolve encoder
        snprintf(label, sizeof(label), "Resolve Encoder Frame %d", i);
        GfxCommandEncoderDescriptor resolveEncoderDesc = { .label = label };
        if (gfxDeviceCreateCommandEncoder(app->device, &resolveEncoderDesc, &app->resolveEncoders[i]) != GFX_RESULT_SUCCESS) {
            fprintf(stderr, "Failed to create resolve encoder %d\n", i);
            return false;
        }
    }

    app->currentFrame = 0;

    return true;
}

void cleanupSizeDependentResources(CubeApp* app)
{
    // Clean up framebuffers
    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        if (app->framebuffers[i]) {
            gfxFramebufferDestroy(app->framebuffers[i]);
            app->framebuffers[i] = NULL;
        }
    }

    // Clean up render passes (depend on framebuffers)
    if (app->resolveRenderPass) {
        gfxRenderPassDestroy(app->resolveRenderPass);
        app->resolveRenderPass = NULL;
    }
    if (app->clearRenderPass) {
        gfxRenderPassDestroy(app->clearRenderPass);
        app->clearRenderPass = NULL;
    }
    if (app->renderPass) {
        gfxRenderPassDestroy(app->renderPass);
        app->renderPass = NULL;
    }

    // Clean up size-dependent resources
    if (app->msaaColorTextureView) {
        gfxTextureViewDestroy(app->msaaColorTextureView);
        app->msaaColorTextureView = NULL;
    }
    if (app->msaaColorTexture) {
        gfxTextureDestroy(app->msaaColorTexture);
        app->msaaColorTexture = NULL;
    }
    if (app->depthTextureView) {
        gfxTextureViewDestroy(app->depthTextureView);
        app->depthTextureView = NULL;
    }
    if (app->depthTexture) {
        gfxTextureDestroy(app->depthTexture);
        app->depthTexture = NULL;
    }

    if (app->swapchain) {
        gfxSwapchainDestroy(app->swapchain);
        app->swapchain = NULL;
    }
}

void cleanupRenderingResources(CubeApp* app)
{
    // Clean up size-independent rendering resources
    // Note: renderPass moved to cleanupSizeDependentResources
    if (app->renderPipeline) {
        gfxRenderPipelineDestroy(app->renderPipeline);
        app->renderPipeline = NULL;
    }
    if (app->fragmentShader) {
        gfxShaderDestroy(app->fragmentShader);
        app->fragmentShader = NULL;
    }
    if (app->vertexShader) {
        gfxShaderDestroy(app->vertexShader);
        app->vertexShader = NULL;
    }
    if (app->uniformBindGroupLayout) {
        gfxBindGroupLayoutDestroy(app->uniformBindGroupLayout);
        app->uniformBindGroupLayout = NULL;
    }
    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        for (int cubeIdx = 0; cubeIdx < CUBE_COUNT; ++cubeIdx) {
            if (app->uniformBindGroups[i][cubeIdx]) {
                gfxBindGroupDestroy(app->uniformBindGroups[i][cubeIdx]);
                app->uniformBindGroups[i][cubeIdx] = NULL;
            }
        }
    }
    if (app->sharedUniformBuffer) {
        gfxBufferDestroy(app->sharedUniformBuffer);
        app->sharedUniformBuffer = NULL;
    }
    if (app->indexBuffer) {
        gfxBufferDestroy(app->indexBuffer);
        app->indexBuffer = NULL;
    }
    if (app->vertexBuffer) {
        gfxBufferDestroy(app->vertexBuffer);
        app->vertexBuffer = NULL;
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
    GfxBufferDescriptor vertexBufferDesc = {
        .label = "Cube Vertices",
        .size = sizeof(vertices),
        .usage = GFX_BUFFER_USAGE_VERTEX | GFX_BUFFER_USAGE_COPY_DST
    };

    if (gfxDeviceCreateBuffer(app->device, &vertexBufferDesc, &app->vertexBuffer) != GFX_RESULT_SUCCESS) {
        fprintf(stderr, "Failed to create vertex buffer\n");
        return false;
    }

    // Create index buffer
    GfxBufferDescriptor indexBufferDesc = {
        .label = "Cube Indices",
        .size = sizeof(indices),
        .usage = GFX_BUFFER_USAGE_INDEX | GFX_BUFFER_USAGE_COPY_DST
    };

    if (gfxDeviceCreateBuffer(app->device, &indexBufferDesc, &app->indexBuffer) != GFX_RESULT_SUCCESS) {
        fprintf(stderr, "Failed to create index buffer\n");
        return false;
    }

    // Upload vertex and index data
    gfxQueueWriteBuffer(app->queue, app->vertexBuffer, 0, vertices, sizeof(vertices));
    gfxQueueWriteBuffer(app->queue, app->indexBuffer, 0, indices, sizeof(indices));

    return true;
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
    size_t totalBufferSize = app->uniformAlignedSize * MAX_FRAMES_IN_FLIGHT * CUBE_COUNT;

    GfxBufferDescriptor uniformBufferDesc = {
        .label = "Shared Transform Uniforms",
        .size = totalBufferSize,
        .usage = GFX_BUFFER_USAGE_UNIFORM | GFX_BUFFER_USAGE_COPY_DST
    };

    if (gfxDeviceCreateBuffer(app->device, &uniformBufferDesc, &app->sharedUniformBuffer) != GFX_RESULT_SUCCESS) {
        fprintf(stderr, "Failed to create shared uniform buffer\n");
        return false;
    }

    return true;
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

    GfxBindGroupLayoutDescriptor uniformLayoutDesc = {
        .label = "Uniform Bind Group Layout",
        .entries = &uniformLayoutEntry,
        .entryCount = 1
    };

    if (gfxDeviceCreateBindGroupLayout(app->device, &uniformLayoutDesc, &app->uniformBindGroupLayout) != GFX_RESULT_SUCCESS) {
        fprintf(stderr, "Failed to create uniform bind group layout\n");
        return false;
    }

    // Create bind groups (CUBE_COUNT cubes per frame in flight) using offsets into shared buffer
    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        for (int cubeIdx = 0; cubeIdx < CUBE_COUNT; ++cubeIdx) {
            char label[64];
            snprintf(label, sizeof(label), "Uniform Bind Group Frame %d Cube %d", i, cubeIdx);

            GfxBindGroupEntry uniformEntry = {
                .binding = 0,
                .type = GFX_BIND_GROUP_ENTRY_TYPE_BUFFER,
                .resource = {
                    .buffer = {
                        .buffer = app->sharedUniformBuffer,
                        .offset = (i * CUBE_COUNT + cubeIdx) * app->uniformAlignedSize,
                        .size = sizeof(UniformData) } }
            };

            GfxBindGroupDescriptor uniformBindGroupDesc = {
                .label = label,
                .layout = app->uniformBindGroupLayout,
                .entries = &uniformEntry,
                .entryCount = 1
            };

            if (gfxDeviceCreateBindGroup(app->device, &uniformBindGroupDesc, &app->uniformBindGroups[i][cubeIdx]) != GFX_RESULT_SUCCESS) {
                fprintf(stderr, "Failed to create uniform bind group %d cube %d\n", i, cubeIdx);
                return false;
            }
        }
    }

    return true;
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
        vertexShaderCode = loadBinaryFile("cube.vert.spv", &vertexShaderSize);
        fragmentShaderCode = loadBinaryFile("cube.frag.spv", &fragmentShaderSize);
        if (!vertexShaderCode || !fragmentShaderCode) {
            fprintf(stderr, "Failed to load SPIR-V shaders\n");
            return false;
        }
        printf("Successfully loaded SPIR-V shaders (vertex: %zu bytes, fragment: %zu bytes)\n",
            vertexShaderSize, fragmentShaderSize);
    }

    // Create vertex shader
    GfxShaderDescriptor vertexShaderDesc = {
        .label = "Cube Vertex Shader",
        .sourceType = sourceType,
        .code = vertexShaderCode,
        .codeSize = vertexShaderSize,
        .entryPoint = "main"
    };

    if (gfxDeviceCreateShader(app->device, &vertexShaderDesc, &app->vertexShader) != GFX_RESULT_SUCCESS) {
        fprintf(stderr, "Failed to create vertex shader\n");
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

static bool createRenderPass(CubeApp* app)
{
    // Create render pass (persistent, reusable across frames)
    // Define color attachment target - for cube passes that LOAD content
    GfxRenderPassColorAttachmentTarget colorTarget = {
        .format = app->swapchainInfo.format,
        .sampleCount = MSAA_SAMPLE_COUNT,
        .ops = {
            .loadOp = GFX_LOAD_OP_LOAD,
            .storeOp = GFX_STORE_OP_STORE }, // STORE to preserve MSAA content across passes
        .finalLayout = GFX_TEXTURE_LAYOUT_COLOR_ATTACHMENT
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
        .resolveTarget = &dummyResolveTarget // Include for compatibility, but DONT_CARE storeOp means no actual resolve
    };

    // Define depth/stencil attachment target
    GfxRenderPassDepthStencilAttachmentTarget depthTarget = {
        .format = DEPTH_FORMAT,
        .sampleCount = MSAA_SAMPLE_COUNT,
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
        .sampleCount = MSAA_SAMPLE_COUNT,
        .ops = {
            .loadOp = GFX_LOAD_OP_CLEAR,
            .storeOp = GFX_STORE_OP_STORE }, // STORE so it can be loaded by subsequent passes
        .finalLayout = GFX_TEXTURE_LAYOUT_COLOR_ATTACHMENT
    };

    GfxRenderPassColorAttachment clearColorAttachment = {
        .target = clearColorTarget,
        .resolveTarget = &dummyResolveTarget // Include for framebuffer compatibility
    };

    GfxRenderPassDescriptor clearPassDesc = {
        .label = "Clear Render Pass",
        .colorAttachments = &clearColorAttachment,
        .colorAttachmentCount = 1,
        .depthStencilAttachment = &depthAttachment
    };

    if (gfxDeviceCreateRenderPass(app->device, &clearPassDesc, &app->clearRenderPass) != GFX_RESULT_SUCCESS) {
        fprintf(stderr, "Failed to create clear render pass\n");
        return false;
    }

    // Create main render pass (loadOp=LOAD)

    GfxRenderPassDescriptor renderPassDesc = {
        .label = "Cube Render Pass (LOAD)",
        .colorAttachments = &colorAttachment,
        .colorAttachmentCount = 1,
        .depthStencilAttachment = &depthAttachment
    };

    if (gfxDeviceCreateRenderPass(app->device, &renderPassDesc, &app->renderPass) != GFX_RESULT_SUCCESS) {
        fprintf(stderr, "Failed to create render pass\n");
        return false;
    }

    // Create final resolve pass (loadOp=LOAD + resolve to swapchain)
    GfxRenderPassColorAttachment resolveColorAttachment = {
        .target = colorTarget, // Load from MSAA
        .resolveTarget = &resolveTarget // Resolve to swapchain
    };

    // Depth attachment for resolve pass - just LOAD (no clearing needed)
    GfxRenderPassDepthStencilAttachmentTarget resolveDepthTarget = {
        .format = DEPTH_FORMAT,
        .sampleCount = MSAA_SAMPLE_COUNT,
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

    GfxRenderPassDescriptor resolvePassDesc = {
        .label = "Resolve Render Pass",
        .colorAttachments = &resolveColorAttachment,
        .colorAttachmentCount = 1,
        .depthStencilAttachment = &resolveDepthAttachment // Include depth for framebuffer compatibility
    };

    if (gfxDeviceCreateRenderPass(app->device, &resolvePassDesc, &app->resolveRenderPass) != GFX_RESULT_SUCCESS) {
        fprintf(stderr, "Failed to create resolve render pass\n");
        return false;
    }

    return true;
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

    // Note: createRenderPass moved to createSizeDependentResources
    // since it needs swapchainInfo and must be created before framebuffers

    // Initialize animation state
    app->rotationAngleX = 0.0f;
    app->rotationAngleY = 0.0f;

    return true;
}

bool createRenderPipeline(CubeApp* app)
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

    // Define vertex buffer layout
    GfxVertexBufferLayout vertexBufferLayout = {
        .arrayStride = sizeof(Vertex),
        .attributes = attributes,
        .attributeCount = 2,
        .stepModeInstance = false
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

    GfxRenderPipelineDescriptor pipelineDesc = {
        .label = "Cube Render Pipeline",
        .vertex = &vertexState,
        .fragment = &fragmentState,
        .primitive = &primitiveState,
        .depthStencil = &depthStencilState,
        .sampleCount = MSAA_SAMPLE_COUNT,
        .renderPass = app->renderPass,
        .bindGroupLayouts = bindGroupLayouts,
        .bindGroupLayoutCount = 1
    };

    if (gfxDeviceCreateRenderPipeline(app->device, &pipelineDesc, &app->renderPipeline) != GFX_RESULT_SUCCESS) {
        fprintf(stderr, "Failed to create render pipeline\n");
        return false;
    }

    return true;
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
    float aspect = (float)app->swapchainInfo.width / (float)app->swapchainInfo.height;
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

#if USE_THREADING
// Threading support functions
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

bool initThreading(CubeApp* app)
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

void cleanupThreading(CubeApp* app)
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
#endif

// Record clear commands - just begin render pass with CLEAR and end
static void recordClearCommands(CubeApp* app, uint32_t imageIndex)
{
    GfxCommandEncoder encoder = app->clearEncoders[app->currentFrame];
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
    GfxCommandEncoder encoder = app->resolveEncoders[app->currentFrame];
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

// Record commands for a single cube - called by worker threads OR main thread
static void recordCubeCommands(CubeApp* app, int cubeIndex, uint32_t imageIndex)
{
    GfxCommandEncoder encoder = app->cubeEncoders[app->currentFrame][cubeIndex];
    gfxCommandEncoderBegin(encoder);

    // Begin render pass using pre-created render pass and framebuffer
    // Only the first cube (index 0) clears the screen, others load existing content
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
        GfxViewport viewport = { 0.0f, 0.0f, (float)app->swapchainInfo.width, (float)app->swapchainInfo.height, 0.0f, 1.0f };
        GfxScissorRect scissor = { 0, 0, app->swapchainInfo.width, app->swapchainInfo.height };
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
        gfxRenderPassEncoderSetBindGroup(renderPass, 0, app->uniformBindGroups[app->currentFrame][cubeIndex], NULL, 0);

        // Draw indexed (36 indices for the cube)
        gfxRenderPassEncoderDrawIndexed(renderPass, 36, 1, 0, 0, 0);

        // End render pass
        gfxRenderPassEncoderEnd(renderPass);
    }

    // Finish command encoding
    gfxCommandEncoderEnd(encoder);
}

void render(CubeApp* app)
{
    // Wait for previous frame to finish
    gfxFenceWait(app->inFlightFences[app->currentFrame], UINT64_MAX);
    gfxFenceReset(app->inFlightFences[app->currentFrame]);

    // Acquire next swapchain image
    uint32_t imageIndex;
    GfxResult result = gfxSwapchainAcquireNextImage(app->swapchain, UINT64_MAX,
        app->imageAvailableSemaphores[app->currentFrame], NULL, &imageIndex);

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
    GfxSubmitDescriptor clearSubmit = { 0 };
    clearSubmit.commandEncoders = &app->clearEncoders[app->currentFrame];
    clearSubmit.commandEncoderCount = 1;
    clearSubmit.waitSemaphores = &app->imageAvailableSemaphores[app->currentFrame];
    clearSubmit.waitSemaphoreCount = 1;
    clearSubmit.signalSemaphores = &app->clearFinishedSemaphores[app->currentFrame];
    clearSubmit.signalSemaphoreCount = 1;
    clearSubmit.signalFence = NULL; // Don't signal fence yet

    gfxQueueSubmit(app->queue, &clearSubmit);

    // Submit cube encoders (wait on clearFinished, signal renderFinished)
    GfxCommandEncoder cubeEncoderArray[CUBE_COUNT];
    for (int i = 0; i < CUBE_COUNT; i++) {
        cubeEncoderArray[i] = app->cubeEncoders[app->currentFrame][i];
    }

    GfxSubmitDescriptor cubesSubmit = { 0 };
    cubesSubmit.commandEncoders = cubeEncoderArray;
    cubesSubmit.commandEncoderCount = CUBE_COUNT;
    cubesSubmit.waitSemaphores = &app->clearFinishedSemaphores[app->currentFrame];
    cubesSubmit.waitSemaphoreCount = 1;
    cubesSubmit.signalSemaphores = NULL; // Don't signal yet, resolve pass will signal
    cubesSubmit.signalSemaphoreCount = 0;
    cubesSubmit.signalFence = NULL; // Don't signal fence yet

    gfxQueueSubmit(app->queue, &cubesSubmit);

    // Record and submit final resolve pass
    recordResolveCommands(app, imageIndex);

    GfxSubmitDescriptor resolveSubmit = { 0 };
    resolveSubmit.commandEncoders = &app->resolveEncoders[app->currentFrame];
    resolveSubmit.commandEncoderCount = 1;
    resolveSubmit.waitSemaphores = NULL; // No wait needed, depends on cube submits via queue ordering
    resolveSubmit.waitSemaphoreCount = 0;
    resolveSubmit.signalSemaphores = &app->renderFinishedSemaphores[app->currentFrame];
    resolveSubmit.signalSemaphoreCount = 1;
    resolveSubmit.signalFence = app->inFlightFences[app->currentFrame];

    gfxQueueSubmit(app->queue, &resolveSubmit);
#else
    // Non-threaded path for WebGPU - record all cubes in ONE render pass
    GfxCommandEncoder encoder = app->cubeEncoders[app->currentFrame][0];
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
        GfxViewport viewport = { 0.0f, 0.0f, (float)app->swapchainInfo.width, (float)app->swapchainInfo.height, 0.0f, 1.0f };
        GfxScissorRect scissor = { 0, 0, app->swapchainInfo.width, app->swapchainInfo.height };
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
            gfxRenderPassEncoderSetBindGroup(renderPass, 0, app->uniformBindGroups[app->currentFrame][cubeIdx], NULL, 0);
            gfxRenderPassEncoderDrawIndexed(renderPass, 36, 1, 0, 0, 0);
        }

        gfxRenderPassEncoderEnd(renderPass);
    }

    gfxCommandEncoderEnd(encoder);

    // Submit single command buffer for WebGPU
    GfxSubmitDescriptor submitInfo = { 0 };
    submitInfo.commandEncoders = &encoder;
    submitInfo.commandEncoderCount = 1;
    submitInfo.waitSemaphores = &app->imageAvailableSemaphores[app->currentFrame];
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.signalSemaphores = &app->renderFinishedSemaphores[app->currentFrame];
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.signalFence = app->inFlightFences[app->currentFrame];

    gfxQueueSubmit(app->queue, &submitInfo);
#endif

    // Present with synchronization
    GfxPresentInfo presentInfo = { 0 };
    presentInfo.waitSemaphores = &app->renderFinishedSemaphores[app->currentFrame];
    presentInfo.waitSemaphoreCount = 1;
    gfxSwapchainPresent(app->swapchain, &presentInfo);

    // Move to next frame
    app->currentFrame = (app->currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void cleanup(CubeApp* app)
{
#if USE_THREADING
    // Clean up threading infrastructure first
    cleanupThreading(app);
#endif

    // Wait for device to finish
    if (app->device) {
        gfxDeviceWaitIdle(app->device);
    }

    // Clean up size-dependent resources
    cleanupSizeDependentResources(app);

    // Clean up rendering resources
    cleanupRenderingResources(app);

    // Destroy per-frame resources
    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        // Destroy synchronization objects
        if (app->imageAvailableSemaphores[i]) {
            gfxSemaphoreDestroy(app->imageAvailableSemaphores[i]);
        }
        if (app->clearFinishedSemaphores[i]) {
            gfxSemaphoreDestroy(app->clearFinishedSemaphores[i]);
        }
        if (app->renderFinishedSemaphores[i]) {
            gfxSemaphoreDestroy(app->renderFinishedSemaphores[i]);
        }
        if (app->inFlightFences[i]) {
            gfxFenceDestroy(app->inFlightFences[i]);
        }

        // Destroy clear encoder
        if (app->clearEncoders[i]) {
            gfxCommandEncoderDestroy(app->clearEncoders[i]);
        }

        // Destroy per-frame command encoders
        for (int cubeIdx = 0; cubeIdx < CUBE_COUNT; ++cubeIdx) {
            if (app->cubeEncoders[i][cubeIdx]) {
                gfxCommandEncoderDestroy(app->cubeEncoders[i][cubeIdx]);
            }
        }

        // Destroy resolve encoder
        if (app->resolveEncoders[i]) {
            gfxCommandEncoderDestroy(app->resolveEncoders[i]);
        }

        for (int cubeIdx = 0; cubeIdx < CUBE_COUNT; ++cubeIdx) {
            if (app->uniformBindGroups[i][cubeIdx]) {
                gfxBindGroupDestroy(app->uniformBindGroups[i][cubeIdx]);
            }
        }
    }

    // Destroy shared uniform buffer
    if (app->sharedUniformBuffer) {
        gfxBufferDestroy(app->sharedUniformBuffer);
    }

    // Destroy graphics resources
    if (app->renderPipeline) {
        gfxRenderPipelineDestroy(app->renderPipeline);
    }
    if (app->fragmentShader) {
        gfxShaderDestroy(app->fragmentShader);
    }
    if (app->vertexShader) {
        gfxShaderDestroy(app->vertexShader);
    }
    if (app->uniformBindGroupLayout) {
        gfxBindGroupLayoutDestroy(app->uniformBindGroupLayout);
    }
    if (app->indexBuffer) {
        gfxBufferDestroy(app->indexBuffer);
    }
    if (app->vertexBuffer) {
        gfxBufferDestroy(app->vertexBuffer);
    }
    if (app->msaaColorTextureView) {
        gfxTextureViewDestroy(app->msaaColorTextureView);
    }
    if (app->msaaColorTexture) {
        gfxTextureDestroy(app->msaaColorTexture);
    }
    if (app->depthTextureView) {
        gfxTextureViewDestroy(app->depthTextureView);
    }
    if (app->depthTexture) {
        gfxTextureDestroy(app->depthTexture);
    }
    if (app->swapchain) {
        gfxSwapchainDestroy(app->swapchain);
    }
    if (app->surface) {
        gfxSurfaceDestroy(app->surface);
    }
    if (app->device) {
        gfxDeviceDestroy(app->device);
    }
    if (app->adapter) {
        gfxAdapterDestroy(app->adapter);
    }
    if (app->instance) {
        gfxInstanceDestroy(app->instance);
    }

    // Unload the backend API after destroying all instances
    printf("Unloading graphics backend...\n");
    gfxUnloadBackend(GFX_BACKEND_API);

    // Destroy window
    if (app->window) {
        glfwDestroyWindow(app->window);
    }
    glfwTerminate();
}

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

static float getCurrentTime(void)
{
#if defined(__EMSCRIPTEN__)
    return (float)emscripten_get_now() / 1000.0f;
#else
    return (float)glfwGetTime();
#endif
}

// Returns false if loop should exit
static bool mainLoopIteration(CubeApp* app)
{
    if (!app || glfwWindowShouldClose(app->window)) {
        return false;
    }

    glfwPollEvents();

    // Handle framebuffer resize
    if (app->previousWidth != app->windowWidth || app->previousHeight != app->windowHeight) {
        // Wait for all in-flight frames to complete
        gfxDeviceWaitIdle(app->device);

        // Recreate only size-dependent resources (including swapchain)
        gfxDeviceWaitIdle(app->device); // Wait for GPU to finish before destroying resources
        cleanupSizeDependentResources(app);
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

int main(void)
{
    printf("=== Threaded Cube Example with Parallel Command Recording (C) ===\n\n");

    CubeApp app = { 0 }; // Initialize all members to NULL/0
    app.currentFrame = 0;
    app.windowWidth = WINDOW_WIDTH;
    app.windowHeight = WINDOW_HEIGHT;

    if (!initWindow(&app)) {
        cleanup(&app);
        return -1;
    }

    if (!initializeGraphics(&app)) {
        cleanup(&app);
        return -1;
    }

    if (!createSizeDependentResources(&app, app.windowWidth, app.windowHeight)) {
        cleanup(&app);
        return -1;
    }

    if (!createRenderingResources(&app)) {
        cleanup(&app);
        return -1;
    }

    if (!createSyncObjects(&app)) {
        cleanup(&app);
        return -1;
    }

    if (!createRenderPipeline(&app)) {
        cleanup(&app);
        return -1;
    }

#if USE_THREADING
    if (!initThreading(&app)) {
        cleanup(&app);
        return -1;
    }
#endif

    printf("Application initialized successfully!\n");
#if USE_THREADING
    printf("Running with %d worker threads for parallel command recording\n", CUBE_COUNT);
#else
    printf("Running in single-threaded mode\n");
#endif
    printf("Press ESC to exit\n\n");

    // Initialize loop state
    app.previousWidth = app.windowWidth;
    app.previousHeight = app.windowHeight;
    app.lastTime = getCurrentTime();

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