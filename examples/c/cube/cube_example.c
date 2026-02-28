#include <gfx/gfx.h>

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

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

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
#define CUBE_COUNT 3
#define COLOR_FORMAT GFX_FORMAT_B8G8R8A8_UNORM_SRGB
#define DEPTH_FORMAT GFX_FORMAT_DEPTH32_FLOAT

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
    float texCoord[2];
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
    GfxCommandEncoder commandEncoder;
    GfxSemaphore imageAvailableSemaphore;
    GfxSemaphore renderFinishedSemaphore;
    GfxFence inFlightFence;
    GfxBindGroup uniformBindGroups[CUBE_COUNT]; // One per cube
} PerFrameResources;

typedef struct {
    GLFWwindow* window;

    GfxInstance instance;
    GfxAdapter adapter;
    GfxAdapterInfo adapterInfo; // Cached adapter info
    GfxDevice device;
    GfxQueue queue;
    GfxSurface surface;
    GfxSurfaceInfo surfaceInfo;
    GfxSwapchain swapchain;
    GfxSwapchainInfo swapchainInfo;
    uint32_t framesInFlightCount;

    GfxBuffer vertexBuffer;
    GfxBuffer indexBuffer;
    GfxShader vertexShader;
    GfxShader fragmentShader;
    GfxRenderPass renderPass;
    GfxRenderPipeline renderPipeline;
    GfxBindGroupLayout uniformBindGroupLayout;
    GfxBindGroupLayout textureBindGroupLayout;
    GfxBindGroup textureBindGroup;

    // Depth buffer
    GfxTexture depthTexture;
    GfxTextureView depthTextureView;

    // MSAA color buffer
    GfxTexture msaaColorTexture;
    GfxTextureView msaaColorTextureView;

    // Framebuffers (one per swapchain image)
    GfxFramebuffer* framebuffers;

    uint32_t windowWidth;
    uint32_t windowHeight;

    // Per-frame-in-flight resources
    PerFrameResources* frameResources; // Dynamic: [framesInFlightCount]
    uint32_t currentFrame;

    // Shared resources (not per-frame)
    GfxBuffer sharedUniformBuffer;
    size_t uniformAlignedSize; // Aligned size per frame
    GfxTexture cubeTexture;
    GfxTextureView cubeTextureView;
    GfxSampler cubeSampler;

    // Async texture upload resources
    GfxBuffer textureStagingBuffer;
    GfxCommandEncoder textureUploadEncoder;
    GfxFence textureUploadFence;
    bool textureUploadComplete;

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
static bool loadTexture(CubeApp* app);
static void unloadTexture(CubeApp* app);
static bool createRenderingResources(CubeApp* app);
static void destroyRenderingResources(CubeApp* app);
static bool createRenderPipeline(CubeApp* app);
static void destroyRenderPipeline(CubeApp* app);

static void updateCube(CubeApp* app, int cubeIndex);
static GfxPlatformWindowHandle getPlatformWindowHandle(GLFWwindow* window);
static float getCurrentTime(void);
static void* loadBinaryFile(const char* filepath, size_t* outSize);
static void* loadTextFile(const char* filepath, size_t* outSize);

// Matrix/Vector math function declarations
static void matrixIdentity(float* matrix);
static void matrixMultiply(float* result, const float* a, const float* b);
static void matrixRotateX(float* matrix, float angle);
static void matrixRotateY(float* matrix, float angle);
static void matrixRotateZ(float* matrix, float angle);
static void matrixPerspective(float* matrix, float fov, float aspect, float nearPlane, float farPlane, GfxBackend backend);
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

    const char* backendName = (app->settings.backend == GFX_BACKEND_VULKAN) ? "Vulkan" : "WebGPU";
    char windowTitle[128];
    snprintf(windowTitle, sizeof(windowTitle), "Cube Example - %s", backendName);

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
    gfxSetLogCallback(logCallback, NULL);

    // Load the graphics backend BEFORE creating an instance
    // This is now decoupled - you load the backend API once at startup
    const char* backendName = (app->settings.backend == GFX_BACKEND_VULKAN) ? "Vulkan" : "WebGPU";
    printf("Loading graphics backend (%s)...\n", backendName);
    if (gfxLoadBackend(app->settings.backend) != GFX_RESULT_SUCCESS) {
        fprintf(stderr, "Failed to load graphics backend\n");
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
    adapterDesc.adapterIndex = UINT32_MAX; // Use preference-based selection
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
    const char* deviceExtensions[] = {
        GFX_DEVICE_EXTENSION_SWAPCHAIN,
        GFX_DEVICE_EXTENSION_ANISOTROPIC_FILTERING
    };
    GfxDeviceDescriptor deviceDesc = {};
    deviceDesc.sType = GFX_STRUCTURE_TYPE_DEVICE_DESCRIPTOR;
    deviceDesc.pNext = NULL;
    deviceDesc.label = "Main Device";
    deviceDesc.enabledExtensions = deviceExtensions;
    deviceDesc.enabledExtensionCount = 2;

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

    // Query surface capabilities and info
    if (gfxSurfaceGetInfo(app->surface, &app->surfaceInfo) != GFX_RESULT_SUCCESS) {
        fprintf(stderr, "Failed to get surface info\n");
        return false;
    }
    printf("Surface Info:\n");
    printf("  Image Count: min %u, max %u\n", app->surfaceInfo.minImageCount, app->surfaceInfo.maxImageCount);
    printf("  Extent: min (%u, %u), max (%u, %u)\n",
        app->surfaceInfo.minExtent.width, app->surfaceInfo.minExtent.height,
        app->surfaceInfo.maxExtent.width, app->surfaceInfo.maxExtent.height);

    // Calculate frames in flight based on surface capabilities
    // Use min image count, but clamp to reasonable values (2-3 is typical)
    app->framesInFlightCount = app->surfaceInfo.minImageCount;
    if (app->framesInFlightCount < 2) {
        app->framesInFlightCount = 2;
    }
    if (app->framesInFlightCount > 4) {
        app->framesInFlightCount = 4;
    }
    printf("Frames in flight: %u\n", app->framesInFlightCount);

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
    // Allocate per-frame resources array
    app->frameResources = (PerFrameResources*)calloc(app->framesInFlightCount, sizeof(PerFrameResources));
    if (!app->frameResources) {
        fprintf(stderr, "Failed to allocate per-frame resources array\n");
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

    for (uint32_t i = 0; i < app->framesInFlightCount; ++i) {
        char label[64];
        PerFrameResources* frame = &app->frameResources[i];

        // Create semaphores
        snprintf(label, sizeof(label), "Image Available Semaphore %u", i);
        semaphoreDesc.label = label;
        if (gfxDeviceCreateSemaphore(app->device, &semaphoreDesc, &frame->imageAvailableSemaphore) != GFX_RESULT_SUCCESS) {
            fprintf(stderr, "Failed to create image available semaphore %u\n", i);
            return false;
        }

        snprintf(label, sizeof(label), "Render Finished Semaphore %u", i);
        semaphoreDesc.label = label;
        if (gfxDeviceCreateSemaphore(app->device, &semaphoreDesc, &frame->renderFinishedSemaphore) != GFX_RESULT_SUCCESS) {
            fprintf(stderr, "Failed to create render finished semaphore %u\n", i);
            return false;
        }

        // Create fence
        snprintf(label, sizeof(label), "In Flight Fence %u", i);
        fenceDesc.label = label;
        if (gfxDeviceCreateFence(app->device, &fenceDesc, &frame->inFlightFence) != GFX_RESULT_SUCCESS) {
            fprintf(stderr, "Failed to create in flight fence %u\n", i);
            return false;
        }

        // Create command encoder
        snprintf(label, sizeof(label), "Command Encoder %u", i);
        GfxCommandEncoderDescriptor encoderDesc = {};
        encoderDesc.sType = GFX_STRUCTURE_TYPE_COMMAND_ENCODER_DESCRIPTOR;
        encoderDesc.pNext = NULL;
        encoderDesc.label = label;
        if (gfxDeviceCreateCommandEncoder(app->device, &encoderDesc, &frame->commandEncoder) != GFX_RESULT_SUCCESS) {
            fprintf(stderr, "Failed to create command encoder %u\n", i);
            return false;
        }

        // Create bind groups for each cube in this frame
        for (int cubeIdx = 0; cubeIdx < CUBE_COUNT; ++cubeIdx) {
            snprintf(label, sizeof(label), "Uniform Bind Group (Frame %u, Cube %d)", i, cubeIdx);

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
                fprintf(stderr, "Failed to create bind group for frame %u, cube %d\n", i, cubeIdx);
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
    for (uint32_t i = 0; i < app->framesInFlightCount; ++i) {
        PerFrameResources* frame = &app->frameResources[i];
        if (frame->inFlightFence) {
            gfxFenceWait(frame->inFlightFence, GFX_TIMEOUT_INFINITE);
        }
    }

    // Destroy per-frame resources
    for (uint32_t i = 0; i < app->framesInFlightCount; ++i) {
        PerFrameResources* frame = &app->frameResources[i];

        // Destroy bind groups
        for (int cubeIdx = 0; cubeIdx < CUBE_COUNT; ++cubeIdx) {
            if (frame->uniformBindGroups[cubeIdx]) {
                gfxBindGroupDestroy(frame->uniformBindGroups[cubeIdx]);
                frame->uniformBindGroups[cubeIdx] = NULL;
            }
        }

        // Destroy synchronization objects
        if (frame->imageAvailableSemaphore) {
            gfxSemaphoreDestroy(frame->imageAvailableSemaphore);
            frame->imageAvailableSemaphore = NULL;
        }
        if (frame->renderFinishedSemaphore) {
            gfxSemaphoreDestroy(frame->renderFinishedSemaphore);
            frame->renderFinishedSemaphore = NULL;
        }
        if (frame->inFlightFence) {
            gfxFenceDestroy(frame->inFlightFence);
            frame->inFlightFence = NULL;
        }

        // Destroy command encoder
        if (frame->commandEncoder) {
            gfxCommandEncoderDestroy(frame->commandEncoder);
            frame->commandEncoder = NULL;
        }
    }

    // Free the per-frame resources array
    if (app->frameResources) {
        free(app->frameResources);
        app->frameResources = NULL;
    }
}

static bool createSizeDependentResources(CubeApp* app, uint32_t width, uint32_t height)
{
    if (!createSwapchain(app, width, height)) {
        return false;
    }

    uint32_t actualWidth = app->swapchainInfo.extent.width;
    uint32_t actualHeight = app->swapchainInfo.extent.height;

    if (!createRenderTargetTextures(app, actualWidth, actualHeight)) {
        return false;
    }

    // Create render pass AFTER swapchain so we have the format
    if (!createRenderPass(app)) {
        return false;
    }

    if (!createFrameBuffers(app, actualWidth, actualHeight)) {
        return false;
    }

    return true;
}

static void destroySizeDependentResources(CubeApp* app)
{
    // Destroy framebuffers
    destroyFrameBuffers(app);

    // Destroy render pass (depends on swapchain format)
    destroyRenderPass(app);

    // Destroy render target textures (depth and MSAA)
    destroyRenderTargetTextures(app);

    // Destroy swapchain
    destroySwapchain(app);
}

static bool createRenderPass(CubeApp* app)
{
    // Create render pass (persistent, reusable across frames)
    // Define color attachment target with MSAA
    GfxRenderPassColorAttachmentTarget colorTarget = {
        .format = app->swapchainInfo.format,
        .sampleCount = app->settings.msaaSampleCount,
        .ops = {
            .loadOp = GFX_LOAD_OP_CLEAR,
            .storeOp = (app->settings.msaaSampleCount > GFX_SAMPLE_COUNT_1) ? GFX_STORE_OP_DONT_CARE : GFX_STORE_OP_STORE },
        .finalLayout = (app->settings.msaaSampleCount > GFX_SAMPLE_COUNT_1) ? GFX_TEXTURE_LAYOUT_COLOR_ATTACHMENT : GFX_TEXTURE_LAYOUT_PRESENT_SRC
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

    // Bundle color attachment with optional resolve
    GfxRenderPassColorAttachment colorAttachment = {
        .target = colorTarget,
        .resolveTarget = (app->settings.msaaSampleCount > GFX_SAMPLE_COUNT_1) ? &resolveTarget : NULL
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

    GfxRenderPassDescriptor renderPassDesc = {};
    renderPassDesc.sType = GFX_STRUCTURE_TYPE_RENDER_PASS_DESCRIPTOR;
    renderPassDesc.pNext = NULL;
    renderPassDesc.label = "Main Render Pass";
    renderPassDesc.colorAttachments = &colorAttachment;
    renderPassDesc.colorAttachmentCount = 1;
    renderPassDesc.depthStencilAttachment = &depthAttachment;

    if (gfxDeviceCreateRenderPass(app->device, &renderPassDesc, &app->renderPass) != GFX_RESULT_SUCCESS) {
        fprintf(stderr, "Failed to create render pass\n");
        return false;
    }

    return true;
}

static void destroyRenderPass(CubeApp* app)
{
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
    swapchainDesc.imageCount = app->framesInFlightCount;

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

    // Create MSAA color texture (is unused if MSAA sample count == 1)
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

    // Create MSAA color texture view (is unused if MSAA sample count == 1)
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
    // Allocate framebuffers array
    app->framebuffers = (GfxFramebuffer*)calloc(app->framesInFlightCount, sizeof(GfxFramebuffer));
    if (!app->framebuffers) {
        fprintf(stderr, "Failed to allocate framebuffers array\n");
        return false;
    }

    // Create framebuffers (one per swapchain image)
    for (uint32_t i = 0; i < app->framesInFlightCount; ++i) {
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
        fbDesc.renderPass = app->renderPass;
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
    for (uint32_t i = 0; i < app->framesInFlightCount; ++i) {
        if (app->framebuffers[i]) {
            gfxFramebufferDestroy(app->framebuffers[i]);
            app->framebuffers[i] = NULL;
        }
    }

    // Free the framebuffers array
    if (app->framebuffers) {
        free(app->framebuffers);
        app->framebuffers = NULL;
    }
}

static bool createGeometry(CubeApp* app)
{
    // Create cube vertices (24 vertices - 4 per face for proper UV mapping)
    Vertex vertices[] = {
        // Front face (Z+)
        { { -1.0f, -1.0f, 1.0f }, { 0.0f, 1.0f } }, // 0
        { { 1.0f, -1.0f, 1.0f }, { 1.0f, 1.0f } }, // 1
        { { 1.0f, 1.0f, 1.0f }, { 1.0f, 0.0f } }, // 2
        { { -1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f } }, // 3

        // Back face (Z-)
        { { 1.0f, -1.0f, -1.0f }, { 0.0f, 1.0f } }, // 4
        { { -1.0f, -1.0f, -1.0f }, { 1.0f, 1.0f } }, // 5
        { { -1.0f, 1.0f, -1.0f }, { 1.0f, 0.0f } }, // 6
        { { 1.0f, 1.0f, -1.0f }, { 0.0f, 0.0f } }, // 7

        // Left face (X-)
        { { -1.0f, -1.0f, -1.0f }, { 0.0f, 1.0f } }, // 8
        { { -1.0f, -1.0f, 1.0f }, { 1.0f, 1.0f } }, // 9
        { { -1.0f, 1.0f, 1.0f }, { 1.0f, 0.0f } }, // 10
        { { -1.0f, 1.0f, -1.0f }, { 0.0f, 0.0f } }, // 11

        // Right face (X+)
        { { 1.0f, -1.0f, 1.0f }, { 0.0f, 1.0f } }, // 12
        { { 1.0f, -1.0f, -1.0f }, { 1.0f, 1.0f } }, // 13
        { { 1.0f, 1.0f, -1.0f }, { 1.0f, 0.0f } }, // 14
        { { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f } }, // 15

        // Top face (Y+)
        { { -1.0f, 1.0f, 1.0f }, { 0.0f, 1.0f } }, // 16
        { { 1.0f, 1.0f, 1.0f }, { 1.0f, 1.0f } }, // 17
        { { 1.0f, 1.0f, -1.0f }, { 1.0f, 0.0f } }, // 18
        { { -1.0f, 1.0f, -1.0f }, { 0.0f, 0.0f } }, // 19

        // Bottom face (Y-)
        { { -1.0f, -1.0f, -1.0f }, { 0.0f, 1.0f } }, // 20
        { { 1.0f, -1.0f, -1.0f }, { 1.0f, 1.0f } }, // 21
        { { 1.0f, -1.0f, 1.0f }, { 1.0f, 0.0f } }, // 22
        { { -1.0f, -1.0f, 1.0f }, { 0.0f, 0.0f } } // 23
    };

    // Create cube indices (36 indices for 12 triangles)
    uint16_t indices[] = {
        // Front face (Z+)
        0, 1, 2, 2, 3, 0,
        // Back face (Z-)
        4, 5, 6, 6, 7, 4,
        // Left face (X-)
        8, 9, 10, 10, 11, 8,
        // Right face (X+)
        12, 13, 14, 14, 15, 12,
        // Top face (Y+)
        16, 17, 18, 18, 19, 16,
        // Bottom face (Y-)
        20, 21, 22, 22, 23, 20
    };
    GfxBufferDescriptor vertexBufferDesc = {
        .label = "Cube Vertices",
        .size = sizeof(vertices),
        .usage = GFX_BUFFER_USAGE_VERTEX | GFX_BUFFER_USAGE_COPY_DST,
        .memoryProperties = GFX_MEMORY_PROPERTY_DEVICE_LOCAL
    };

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
    if (gfxQueueWriteBuffer(app->queue, app->vertexBuffer, 0, vertices, sizeof(vertices)) != GFX_RESULT_SUCCESS) {
        fprintf(stderr, "Failed to write vertex data to buffer\n");
        return false;
    }
    if (gfxQueueWriteBuffer(app->queue, app->indexBuffer, 0, indices, sizeof(indices)) != GFX_RESULT_SUCCESS) {
        fprintf(stderr, "Failed to write index data to buffer\n");
        return false;
    }

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
    size_t totalBufferSize = app->uniformAlignedSize * app->framesInFlightCount * CUBE_COUNT;

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

    // Note: Bind groups are created in createPerFrameResources() after all resources are ready

    // Create texture bind group layout
    GfxBindGroupLayoutEntry textureLayoutEntries[] = {
        { .binding = 0,
            .visibility = GFX_SHADER_STAGE_FRAGMENT,
            .type = GFX_BINDING_TYPE_TEXTURE,
            .texture = {
                .sampleType = GFX_TEXTURE_SAMPLE_TYPE_FLOAT,
                .viewDimension = GFX_TEXTURE_VIEW_TYPE_2D } },
        { .binding = 1, .visibility = GFX_SHADER_STAGE_FRAGMENT, .type = GFX_BINDING_TYPE_SAMPLER, .sampler = {
                                                                                                       // No type field needed for sampler binding
                                                                                                   } }
    };

    GfxBindGroupLayoutDescriptor textureLayoutDesc = {
        .label = "Texture Bind Group Layout",
        .entries = textureLayoutEntries,
        .entryCount = 2
    };

    if (gfxDeviceCreateBindGroupLayout(app->device, &textureLayoutDesc, &app->textureBindGroupLayout) != GFX_RESULT_SUCCESS) {
        fprintf(stderr, "Failed to create texture bind group layout\n");
        return false;
    }

    // Create texture bind group
    GfxBindGroupEntry textureEntries[] = {
        { .binding = 0,
            .type = GFX_BIND_GROUP_ENTRY_TYPE_TEXTURE_VIEW,
            .resource = { .textureView = app->cubeTextureView } },
        { .binding = 1,
            .type = GFX_BIND_GROUP_ENTRY_TYPE_SAMPLER,
            .resource = { .sampler = app->cubeSampler } }
    };

    GfxBindGroupDescriptor textureBindGroupDesc = {};
    textureBindGroupDesc.sType = GFX_STRUCTURE_TYPE_BIND_GROUP_DESCRIPTOR;
    textureBindGroupDesc.pNext = NULL;
    textureBindGroupDesc.label = "Texture Bind Group";
    textureBindGroupDesc.layout = app->textureBindGroupLayout;
    textureBindGroupDesc.entries = textureEntries;
    textureBindGroupDesc.entryCount = 2;

    if (gfxDeviceCreateBindGroup(app->device, &textureBindGroupDesc, &app->textureBindGroup) != GFX_RESULT_SUCCESS) {
        fprintf(stderr, "Failed to create texture bind group\n");
        return false;
    }

    return true;
}

static void destroyBindGroup(CubeApp* app)
{
    // Destroy texture bind groups and layouts
    if (app->textureBindGroup) {
        gfxBindGroupDestroy(app->textureBindGroup);
        app->textureBindGroup = NULL;
    }
    if (app->textureBindGroupLayout) {
        gfxBindGroupLayoutDestroy(app->textureBindGroupLayout);
        app->textureBindGroupLayout = NULL;
    }

    // Destroy uniform bind group layout
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

    // Query shader format support and use the first supported format
    GfxShaderSourceType sourceType;
    bool formatSupported = false;

    // Try SPIR-V first (generally better performance on native)
    if (gfxDeviceSupportsShaderFormat(app->device, GFX_SHADER_SOURCE_SPIRV, &formatSupported) == GFX_RESULT_SUCCESS && formatSupported) {
        sourceType = GFX_SHADER_SOURCE_SPIRV;
        printf("Loading SPIR-V shaders...\n");
        vertexShaderCode = loadBinaryFile("shaders/cube_textured.vert.spv", &vertexShaderSize);
        fragmentShaderCode = loadBinaryFile("shaders/cube_textured.frag.spv", &fragmentShaderSize);
        if (vertexShaderCode && fragmentShaderCode) {
            printf("Successfully loaded SPIR-V shaders (vertex: %zu bytes, fragment: %zu bytes)\n",
                vertexShaderSize, fragmentShaderSize);
        } else {
            // SPIR-V files not found, fall back to WGSL
            if (vertexShaderCode)
                free(vertexShaderCode);
            if (fragmentShaderCode)
                free(fragmentShaderCode);
            vertexShaderCode = NULL;
            fragmentShaderCode = NULL;
            formatSupported = false; // Reset to try WGSL
        }
    }

    // Try WGSL if SPIR-V is not available or failed to load
    if (!vertexShaderCode && !fragmentShaderCode && gfxDeviceSupportsShaderFormat(app->device, GFX_SHADER_SOURCE_WGSL, &formatSupported) == GFX_RESULT_SUCCESS && formatSupported) {
        sourceType = GFX_SHADER_SOURCE_WGSL;
        printf("Loading WGSL shaders...\n");
        vertexShaderCode = loadTextFile("shaders/cube_textured.vert.wgsl", &vertexShaderSize);
        fragmentShaderCode = loadTextFile("shaders/cube_textured.frag.wgsl", &fragmentShaderSize);
        if (!vertexShaderCode || !fragmentShaderCode) {
            fprintf(stderr, "Failed to load WGSL shaders\n");
            free(vertexShaderCode);
            free(fragmentShaderCode);
            return false;
        }
        printf("Successfully loaded WGSL shaders (vertex: %zu bytes, fragment: %zu bytes)\n",
            vertexShaderSize, fragmentShaderSize);
    }

    if (!vertexShaderCode || !fragmentShaderCode) {
        fprintf(stderr, "Error: No supported shader format found or failed to load shaders\n");
        return false;
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

static bool loadTexture(CubeApp* app)
{
    const char* texturePath = "textures/vulkan.png";

    // Load image with stb_image
    int width, height, channels;
    stbi_set_flip_vertically_on_load(1);
    unsigned char* pixels = stbi_load(texturePath, &width, &height, &channels, STBI_rgb_alpha);

    if (!pixels) {
        fprintf(stderr, "Failed to load texture: %s\n", texturePath);
        return false;
    }

    // Calculate mip levels (log2(max(width, height)) + 1)
    uint32_t maxDim = width > height ? width : height;
    uint32_t mipLevels = 1;
    while (maxDim > 1) {
        maxDim >>= 1;
        mipLevels++;
    }

    printf("Creating texture with %u mip levels (%dx%d) - ASYNC UPLOAD\n", mipLevels, width, height);

    // Create texture with mipmaps
    GfxTextureDescriptor textureDesc = {};
    textureDesc.sType = GFX_STRUCTURE_TYPE_TEXTURE_DESCRIPTOR;
    textureDesc.pNext = NULL;
    textureDesc.label = "Cube Texture";
    textureDesc.type = GFX_TEXTURE_TYPE_2D;
    textureDesc.size = (GfxExtent3D){ (uint32_t)width, (uint32_t)height, 1 };
    textureDesc.arrayLayerCount = 1;
    textureDesc.mipLevelCount = mipLevels;
    textureDesc.sampleCount = GFX_SAMPLE_COUNT_1;
    textureDesc.format = GFX_FORMAT_R8G8B8A8_UNORM_SRGB;
    textureDesc.usage = GFX_TEXTURE_USAGE_TEXTURE_BINDING | GFX_TEXTURE_USAGE_COPY_SRC | GFX_TEXTURE_USAGE_COPY_DST | GFX_TEXTURE_USAGE_RENDER_ATTACHMENT;

    if (gfxDeviceCreateTexture(app->device, &textureDesc, &app->cubeTexture) != GFX_RESULT_SUCCESS) {
        fprintf(stderr, "Failed to create texture\n");
        stbi_image_free(pixels);
        return false;
    }

    // Create staging buffer for async upload
    uint64_t dataSize = (uint64_t)width * height * 4;
    GfxBufferDescriptor stagingDesc = {};
    stagingDesc.sType = GFX_STRUCTURE_TYPE_BUFFER_DESCRIPTOR;
    stagingDesc.pNext = NULL;
    stagingDesc.label = "Texture Staging Buffer";
    stagingDesc.size = dataSize;
    stagingDesc.usage = GFX_BUFFER_USAGE_MAP_WRITE | GFX_BUFFER_USAGE_COPY_SRC;
    stagingDesc.memoryProperties = GFX_MEMORY_PROPERTY_HOST_VISIBLE | GFX_MEMORY_PROPERTY_HOST_COHERENT;

    if (gfxDeviceCreateBuffer(app->device, &stagingDesc, &app->textureStagingBuffer) != GFX_RESULT_SUCCESS) {
        fprintf(stderr, "Failed to create staging buffer\n");
        stbi_image_free(pixels);
        return false;
    }

    // Map and copy texture data to staging buffer
    void* mappedData;
    if (gfxBufferMap(app->textureStagingBuffer, 0, GFX_WHOLE_SIZE, &mappedData) != GFX_RESULT_SUCCESS) {
        fprintf(stderr, "Failed to map staging buffer\n");
        stbi_image_free(pixels);
        return false;
    }

    memcpy(mappedData, pixels, dataSize);
    gfxBufferUnmap(app->textureStagingBuffer);
    stbi_image_free(pixels);

    // Create command encoder for async upload
    GfxCommandEncoderDescriptor encoderDesc = {};
    encoderDesc.sType = GFX_STRUCTURE_TYPE_COMMAND_ENCODER_DESCRIPTOR;
    encoderDesc.pNext = NULL;
    encoderDesc.label = "Texture Upload Encoder";
    if (gfxDeviceCreateCommandEncoder(app->device, &encoderDesc, &app->textureUploadEncoder) != GFX_RESULT_SUCCESS) {
        fprintf(stderr, "Failed to create command encoder for texture upload\n");
        return false;
    }

    if (gfxCommandEncoderBegin(app->textureUploadEncoder) != GFX_RESULT_SUCCESS) {
        fprintf(stderr, "Failed to begin texture upload command encoder\n");
        return false;
    }

    // Copy from staging buffer to texture
    GfxCopyBufferToTextureDescriptor copyDesc = {
        .source = app->textureStagingBuffer,
        .sourceOffset = 0,
        .destination = app->cubeTexture,
        .origin = { 0, 0, 0 },
        .extent = { (uint32_t)width, (uint32_t)height, 1 },
        .mipLevel = 0,
        .finalLayout = GFX_TEXTURE_LAYOUT_SHADER_READ_ONLY
    };
    if (gfxCommandEncoderCopyBufferToTexture(app->textureUploadEncoder, &copyDesc) != GFX_RESULT_SUCCESS) {
        fprintf(stderr, "Failed to copy buffer to texture\n");
        return false;
    }

    // Generate mipmaps in the same command buffer
    if (gfxCommandEncoderGenerateMipmaps(app->textureUploadEncoder, app->cubeTexture) != GFX_RESULT_SUCCESS) {
        fprintf(stderr, "Failed to generate mipmaps\n");
        return false;
    }

    if (gfxCommandEncoderEnd(app->textureUploadEncoder) != GFX_RESULT_SUCCESS) {
        fprintf(stderr, "Failed to end texture upload command encoder\n");
        return false;
    }

    // Create fence for tracking upload completion
    GfxFenceDescriptor fenceDesc = {};
    fenceDesc.sType = GFX_STRUCTURE_TYPE_FENCE_DESCRIPTOR;
    fenceDesc.pNext = NULL;
    fenceDesc.label = "Texture Upload Fence";
    fenceDesc.signaled = false;
    if (gfxDeviceCreateFence(app->device, &fenceDesc, &app->textureUploadFence) != GFX_RESULT_SUCCESS) {
        fprintf(stderr, "Failed to create upload fence\n");
        gfxCommandEncoderDestroy(app->textureUploadEncoder);
        return false;
    }

    // Submit upload commands with fence
    GfxSubmitDescriptor submitDesc = {
        .commandEncoders = &app->textureUploadEncoder,
        .commandEncoderCount = 1,
        .signalFence = app->textureUploadFence
    };
    if (gfxQueueSubmit(app->queue, &submitDesc) != GFX_RESULT_SUCCESS) {
        fprintf(stderr, "Failed to submit texture upload commands\n");
        return false;
    }

    // Texture upload is now happening asynchronously
    // The render loop will check app->textureUploadFence status
    // and clean up resources when complete
    printf("Texture upload submitted asynchronously...\n");

    // Create texture view with all mip levels
    GfxTextureViewDescriptor viewDesc = {};
    viewDesc.sType = GFX_STRUCTURE_TYPE_TEXTURE_VIEW_DESCRIPTOR;
    viewDesc.pNext = NULL;
    viewDesc.label = "Cube Texture View";
    viewDesc.viewType = GFX_TEXTURE_VIEW_TYPE_2D;
    viewDesc.format = GFX_FORMAT_R8G8B8A8_UNORM_SRGB;
    viewDesc.baseMipLevel = 0;
    viewDesc.mipLevelCount = mipLevels;
    viewDesc.baseArrayLayer = 0;
    viewDesc.arrayLayerCount = 1;

    if (gfxTextureCreateView(app->cubeTexture, &viewDesc, &app->cubeTextureView) != GFX_RESULT_SUCCESS) {
        fprintf(stderr, "Failed to create texture view\n");
        return false;
    }

    // Create sampler with mipmap filtering
    GfxSamplerDescriptor samplerDesc = {};
    samplerDesc.sType = GFX_STRUCTURE_TYPE_SAMPLER_DESCRIPTOR;
    samplerDesc.pNext = NULL;
    samplerDesc.label = "Cube Sampler";
    samplerDesc.magFilter = GFX_FILTER_MODE_LINEAR;
    samplerDesc.minFilter = GFX_FILTER_MODE_LINEAR;
    samplerDesc.mipmapFilter = GFX_FILTER_MODE_LINEAR;
    samplerDesc.addressModeU = GFX_ADDRESS_MODE_REPEAT;
    samplerDesc.addressModeV = GFX_ADDRESS_MODE_REPEAT;
    samplerDesc.addressModeW = GFX_ADDRESS_MODE_REPEAT;
    samplerDesc.maxAnisotropy = 1;

    if (gfxDeviceCreateSampler(app->device, &samplerDesc, &app->cubeSampler) != GFX_RESULT_SUCCESS) {
        fprintf(stderr, "Failed to create sampler\n");
        return false;
    }

    return true;
}

static void unloadTexture(CubeApp* app)
{
    // If async texture upload hasn't completed yet, wait for it
    if (!app->textureUploadComplete && app->textureUploadFence) {
        printf("Waiting for texture upload to complete before cleanup...\n");
        gfxFenceWait(app->textureUploadFence, GFX_TIMEOUT_INFINITE);

        if (app->textureStagingBuffer) {
            gfxBufferDestroy(app->textureStagingBuffer);
            app->textureStagingBuffer = NULL;
        }
        if (app->textureUploadEncoder) {
            gfxCommandEncoderDestroy(app->textureUploadEncoder);
            app->textureUploadEncoder = NULL;
        }
        app->textureUploadComplete = true;
    }

    // Clean up async texture upload fence
    if (app->textureUploadFence) {
        gfxFenceDestroy(app->textureUploadFence);
        app->textureUploadFence = NULL;
    }

    if (app->cubeSampler) {
        gfxSamplerDestroy(app->cubeSampler);
        app->cubeSampler = NULL;
    }
    if (app->cubeTextureView) {
        gfxTextureViewDestroy(app->cubeTextureView);
        app->cubeTextureView = NULL;
    }
    if (app->cubeTexture) {
        gfxTextureDestroy(app->cubeTexture);
        app->cubeTexture = NULL;
    }
}

static bool createRenderingResources(CubeApp* app)
{
    printf("[DEBUG] createRenderingResources called\n");

    if (!createGeometry(app)) {
        return false;
    }

    if (!loadTexture(app)) {
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
    // Destroy render pipeline
    destroyRenderPipeline(app);

    // Destroy shaders
    destroyShaders(app);

    // Unload texture
    unloadTexture(app);

    // Destroy bind groups and layouts
    destroyBindGroup(app);

    // Destroy uniform buffer
    destroyUniformBuffer(app);

    // Destroy geometry buffers
    destroyGeometry(app);
}

static bool createRenderPipeline(CubeApp* app)
{
    // Define vertex attributes
    GfxVertexAttribute attributes[] = {
        { .format = GFX_FORMAT_R32G32B32_FLOAT,
            .offset = offsetof(Vertex, position),
            .shaderLocation = 0 },
        { .format = GFX_FORMAT_R32G32_FLOAT,
            .offset = offsetof(Vertex, texCoord),
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
    GfxBindGroupLayout bindGroupLayouts[] = { app->uniformBindGroupLayout, app->textureBindGroupLayout };

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
    pipelineDesc.bindGroupLayoutCount = 2;

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

static void updateCube(CubeApp* app, int cubeIndex)
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
    translation[12] = (cubeIndex - 1) * 3.0f; // x offset: -3, 0, 3

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
    if (gfxQueueWriteBuffer(app->queue, app->sharedUniformBuffer, offset, &uniforms, sizeof(uniforms)) != GFX_RESULT_SUCCESS) {
        fprintf(stderr, "Failed to write uniform data for cube %u\n", cubeIndex);
    }
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

void matrixPerspective(float* matrix, float fov, float aspect, float nearPlane, float farPlane, GfxBackend backend)
{
    memset(matrix, 0, 16 * sizeof(float));

    float f = 1.0f / tanf(fov / 2.0f);

    matrix[0] = f / aspect;
    if (backend == GFX_BACKEND_VULKAN) {
        matrix[5] = -f; // Invert Y for Vulkan
    } else {
        matrix[5] = f;
    }
    matrix[10] = (farPlane + nearPlane) / (nearPlane - farPlane);
    matrix[11] = -1.0f;
    matrix[14] = (2.0f * farPlane * nearPlane) / (nearPlane - farPlane);
}

void matrixLookAt(float* matrix, float eyeX, float eyeY, float eyeZ, float centerX, float centerY, float centerZ, float upX, float upY, float upZ)
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

    // 5. Create per-frame resources (depends on uniform buffer and layouts)
    if (!createPerFrameResources(app)) {
        fprintf(stderr, "Failed to create per-frame resources\n");
        return false;
    }

    // 6. Create render pipeline (depends on render pass and resources)
    if (!createRenderPipeline(app)) {
        fprintf(stderr, "Failed to create render pipeline\n");
        return false;
    }

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
    return true;
}

static void cleanup(CubeApp* app)
{
    // Wait for device to finish all GPU work before cleanup
    if (app->device) {
        gfxDeviceWaitIdle(app->device);
    }

    // 5. Destroy render pipeline (depends on render pass and resources)
    destroyRenderPipeline(app);

    // 4. Destroy per-frame resources (depends on uniform buffer and layouts)
    destroyPerFrameResources(app);

    // 3. Destroy rendering resources (textures, buffers, layouts)
    destroyRenderingResources(app);

    // 2. Destroy size-dependent resources (swapchain, framebuffers, render pass)
    destroySizeDependentResources(app);

    // 1. Destroy graphics context (surface, device, instance)
    destroyGraphics(app);

    // 0. Destroy window
    destroyWindow(app);
}

static void update(CubeApp* app, float deltaTime)
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

static void render(CubeApp* app)
{
    // Check if async texture upload is complete
    if (!app->textureUploadComplete) {
        bool isReady;
        gfxFenceGetStatus(app->textureUploadFence, &isReady);
        if (isReady) {
            printf("Texture upload completed asynchronously!\n");
            app->textureUploadComplete = true;

            // Clean up resources now that upload is done
            gfxBufferDestroy(app->textureStagingBuffer);
            app->textureStagingBuffer = NULL;

            // Safe to destroy encoder now that GPU is done with it
            gfxCommandEncoderDestroy(app->textureUploadEncoder);
            app->textureUploadEncoder = NULL;
        } else {
            // Texture not ready yet - render clear color only
            printf("Waiting for async texture upload...\n");
        }
    }

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

    GfxCommandEncoder encoder = frame->commandEncoder;
    if (gfxCommandEncoderBegin(encoder) != GFX_RESULT_SUCCESS) {
        fprintf(stderr, "Failed to begin command encoder\n");
        return;
    }

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

    GfxRenderPassEncoder renderPass;
    if (gfxCommandEncoderBeginRenderPass(encoder, &beginDesc, &renderPass) == GFX_RESULT_SUCCESS) {

        // Set pipeline
        gfxRenderPassEncoderSetPipeline(renderPass, app->renderPipeline);

        // Set viewport and scissor to fill the entire render target
        GfxViewport viewport = { 0.0f, 0.0f, (float)app->swapchainInfo.extent.width, (float)app->swapchainInfo.extent.height, 0.0f, 1.0f };
        GfxScissorRect scissor = { { 0, 0 }, { app->swapchainInfo.extent.width, app->swapchainInfo.extent.height } };
        gfxRenderPassEncoderSetViewport(renderPass, &viewport);
        gfxRenderPassEncoderSetScissorRect(renderPass, &scissor);

        // Only draw if texture is loaded
        if (app->textureUploadComplete) {
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

            // Bind texture (shared by all cubes)
            gfxRenderPassEncoderSetBindGroup(renderPass, 1, app->textureBindGroup, NULL, 0);

            // Draw CUBE_COUNT cubes at different positions
            for (int i = 0; i < CUBE_COUNT; ++i) {
                // Bind the specific cube's bind group (no dynamic offsets)
                gfxRenderPassEncoderSetBindGroup(renderPass, 0, frame->uniformBindGroups[i], NULL, 0);

                // Draw indexed (36 indices for the cube)
                gfxRenderPassEncoderDrawIndexed(renderPass, 36, 1, 0, 0, 0);
            }
        }

        // End render pass
        if (gfxRenderPassEncoderEnd(renderPass) != GFX_RESULT_SUCCESS) {
            fprintf(stderr, "Failed to end render pass\n");
            return;
        }
    }

    // Finish command encoding
    if (gfxCommandEncoderEnd(encoder) != GFX_RESULT_SUCCESS) {
        fprintf(stderr, "Failed to end command encoder\n");
        return;
    }

    // Submit commands with synchronization
    GfxSubmitDescriptor submitDescriptor = { 0 };
    submitDescriptor.commandEncoders = &encoder;
    submitDescriptor.commandEncoderCount = 1;
    submitDescriptor.waitSemaphores = &frame->imageAvailableSemaphore;
    submitDescriptor.waitSemaphoreCount = 1;
    submitDescriptor.signalSemaphores = &frame->renderFinishedSemaphore;
    submitDescriptor.signalSemaphoreCount = 1;
    submitDescriptor.signalFence = frame->inFlightFence;

    if (gfxQueueSubmit(app->queue, &submitDescriptor) != GFX_RESULT_SUCCESS) {
        fprintf(stderr, "Failed to submit command buffer to queue\n");
        return;
    }

    // Present with synchronization
    GfxPresentDescriptor presentDescriptor = {};
    presentDescriptor.sType = GFX_STRUCTURE_TYPE_PRESENT_DESCRIPTOR;
    presentDescriptor.pNext = NULL;
    presentDescriptor.waitSemaphores = &frame->renderFinishedSemaphore;
    presentDescriptor.waitSemaphoreCount = 1;
    if (gfxSwapchainPresent(app->swapchain, &presentDescriptor) != GFX_RESULT_SUCCESS) {
        fprintf(stderr, "Failed to present swapchain image\n");
        return;
    }

    // Move to next frame
    app->currentFrame = (app->currentFrame + 1) % app->framesInFlightCount;
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
                fprintf(stderr, "Unknown backend: %s\n", argv[i]);
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
                fprintf(stderr, "Invalid MSAA sample count: %d\n", samples);
                fprintf(stderr, "Valid values: 1, 2, 4, 8, 16, 32, 64\n");
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
                fprintf(stderr, "Invalid vsync value: %s\n", argv[i]);
                fprintf(stderr, "Valid values: 0 (off), 1 (on)\n");
                return false;
            }
        } else if (strcmp(argv[i], "--help") == 0) {
            printf("Usage: %s [options]\n", argv[0]);
            printf("Options:\n");
            printf("  --backend [vulkan|webgpu]   Select graphics backend\n");
            printf("  --msaa [1|2|4|8]            Select MSAA sample count\n");
            printf("  --vsync [0|1]               VSync: 0=off, 1=on\n");
            printf("  --help                      Show this help message\n");
            return false;
        } else {
            fprintf(stderr, "Unknown argument: %s\n", argv[i]);
            return false;
        }
    }

    return true;
}

int main(int argc, char** argv)
{
    printf("=== Cube Example with Unified Graphics API (C) ===\n\n");

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
