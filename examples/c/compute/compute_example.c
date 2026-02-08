#include <gfx/gfx.h>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#if defined(__EMSCRIPTEN__)
#include <emscripten/emscripten.h>
#else
#ifdef _WIN32
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
#define COMPUTE_TEXTURE_WIDTH 800
#define COMPUTE_TEXTURE_HEIGHT 600
#define COLOR_FORMAT GFX_TEXTURE_FORMAT_B8G8R8A8_UNORM_SRGB

#if defined(__EMSCRIPTEN__)
#define GFX_BACKEND_API GFX_BACKEND_WEBGPU
#else
// here we can choose between VULKAN, WEBGPU
#define GFX_BACKEND_API GFX_BACKEND_VULKAN
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

typedef struct {
    float time;
} ComputeUniformData;

typedef struct {
    float postProcessStrength;
} RenderUniformData;

typedef struct {
    GLFWwindow* window;

    GfxInstance instance;
    GfxAdapter adapter;
    GfxAdapterInfo adapterInfo; // Cached adapter info
    GfxDevice device;
    GfxQueue queue;
    GfxSurface surface;
    GfxSwapchain swapchain;
    GfxSwapchainInfo swapchainInfo;

    // Compute resources
    GfxTexture computeTexture;
    GfxTextureView computeTextureView;
    GfxShader computeShader;
    GfxComputePipeline computePipeline;
    GfxBindGroupLayout computeBindGroupLayout;
    GfxBindGroup* computeBindGroup; // Dynamic
    GfxBuffer* computeUniformBuffer; // Dynamic

    // Render resources (fullscreen quad)
    GfxShader vertexShader;
    GfxShader fragmentShader;
    GfxRenderPass renderPass;
    GfxRenderPipeline renderPipeline;
    GfxBindGroupLayout renderBindGroupLayout;
    GfxSampler sampler;
    GfxBindGroup* renderBindGroup; // Dynamic
    GfxBuffer* renderUniformBuffer; // Dynamic
    GfxFramebuffer* framebuffers; // Dynamic

    uint32_t windowWidth;
    uint32_t windowHeight;
    uint32_t framesInFlightCount; // Dynamic based on surface capabilities

    // Per-frame synchronization (dynamic arrays)
    GfxSemaphore* imageAvailableSemaphores;
    GfxSemaphore* renderFinishedSemaphores;
    GfxFence* inFlightFences;
    GfxCommandEncoder* commandEncoders;

    uint32_t currentFrame;
    uint32_t previousWidth;
    uint32_t previousHeight;
    float elapsedTime;

    // FPS tracking
    uint32_t fpsFrameCount;
    float fpsTimeAccumulator;
    float fpsFrameTimeMin;
    float fpsFrameTimeMax;
} ComputeApp;

static void framebufferResizeCallback(GLFWwindow* window, int width, int height)
{
    ComputeApp* app = (ComputeApp*)glfwGetWindowUserPointer(window);
    app->windowWidth = (uint32_t)width;
    app->windowHeight = (uint32_t)height;
}

static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    (void)scancode;
    (void)mods;
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    }
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

// Helper function to load text files (WGSL shaders)
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

static bool initWindow(ComputeApp* app)
{
    if (!glfwInit()) {
        fprintf(stderr, "Failed to initialize GLFW\n");
        return false;
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

    app->window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Compute & Postprocess Example (C)", NULL, NULL);
    if (!app->window) {
        fprintf(stderr, "Failed to create GLFW window\n");
        glfwTerminate();
        return false;
    }

    app->windowWidth = WINDOW_WIDTH;
    app->windowHeight = WINDOW_HEIGHT;

    glfwSetWindowUserPointer(app->window, app);
    glfwSetFramebufferSizeCallback(app->window, framebufferResizeCallback);
    glfwSetKeyCallback(app->window, keyCallback);

    return true;
}

static GfxPlatformWindowHandle getPlatformWindowHandle(GLFWwindow* window)
{
    GfxPlatformWindowHandle handle = { 0 };
#if defined(__EMSCRIPTEN__)
    handle = gfxPlatformWindowHandleFromEmscripten("#canvas");
#elif defined(_WIN32)
    handle = gfxPlatformWindowHandleFromWin32(glfwGetWin32Window(window), GetModuleHandle(NULL));
#elif defined(__linux__)
    // handle = gfxPlatformWindowHandleFromXlib(glfwGetX11Display(), glfwGetX11Window(window));
    handle = gfxPlatformWindowHandleFromWayland(glfwGetWaylandDisplay(), glfwGetWaylandWindow(window));
#elif defined(__APPLE__)
    handle = gfxPlatformWindowHandleFromMetal(glfwGetMetalLayer(window));
#endif
    return handle;
}

static bool createSwapchain(ComputeApp* app, uint32_t width, uint32_t height)
{
    // Query surface capabilities to determine frame count
    GfxSurfaceInfo surfaceInfo;
    if (gfxSurfaceGetInfo(app->surface, &surfaceInfo) != GFX_RESULT_SUCCESS) {
        fprintf(stderr, "Failed to get surface info\n");
        return false;
    }

    printf("Surface Info:\n");
    printf("  Image Count: min %u, max %u\n", surfaceInfo.minImageCount, surfaceInfo.maxImageCount);
    printf("  Extent: min (%u, %u), max (%u, %u)\n",
        surfaceInfo.minExtent.width, surfaceInfo.minExtent.height,
        surfaceInfo.maxExtent.width, surfaceInfo.maxExtent.height);

    // Calculate frames in flight based on surface capabilities (clamp to [2, 4])
    app->framesInFlightCount = surfaceInfo.minImageCount;
    if (app->framesInFlightCount < 2) {
        app->framesInFlightCount = 2;
    }
    if (app->framesInFlightCount > 4) {
        app->framesInFlightCount = 4;
    }
    printf("Frames in flight: %u\n", app->framesInFlightCount);

    // Allocate dynamic arrays
    app->computeBindGroup = (GfxBindGroup*)calloc(app->framesInFlightCount, sizeof(GfxBindGroup));
    app->computeUniformBuffer = (GfxBuffer*)calloc(app->framesInFlightCount, sizeof(GfxBuffer));
    app->renderBindGroup = (GfxBindGroup*)calloc(app->framesInFlightCount, sizeof(GfxBindGroup));
    app->renderUniformBuffer = (GfxBuffer*)calloc(app->framesInFlightCount, sizeof(GfxBuffer));
    app->framebuffers = (GfxFramebuffer*)calloc(app->framesInFlightCount, sizeof(GfxFramebuffer));
    app->imageAvailableSemaphores = (GfxSemaphore*)calloc(app->framesInFlightCount, sizeof(GfxSemaphore));
    app->renderFinishedSemaphores = (GfxSemaphore*)calloc(app->framesInFlightCount, sizeof(GfxSemaphore));
    app->inFlightFences = (GfxFence*)calloc(app->framesInFlightCount, sizeof(GfxFence));
    app->commandEncoders = (GfxCommandEncoder*)calloc(app->framesInFlightCount, sizeof(GfxCommandEncoder));

    if (!app->computeBindGroup || !app->computeUniformBuffer || !app->renderBindGroup || !app->renderUniformBuffer || !app->framebuffers || !app->imageAvailableSemaphores || !app->renderFinishedSemaphores || !app->inFlightFences || !app->commandEncoders) {
        fprintf(stderr, "Failed to allocate dynamic arrays\n");
        return false;
    }

    GfxSwapchainDescriptor swapchainDesc = {};
    swapchainDesc.sType = GFX_STRUCTURE_TYPE_SWAPCHAIN_DESCRIPTOR;
    swapchainDesc.pNext = NULL;
    swapchainDesc.surface = app->surface;
    swapchainDesc.extent.width = width;
    swapchainDesc.extent.height = height;
    swapchainDesc.format = COLOR_FORMAT;
    swapchainDesc.usage = GFX_TEXTURE_USAGE_RENDER_ATTACHMENT;
    swapchainDesc.presentMode = GFX_PRESENT_MODE_FIFO;
    swapchainDesc.imageCount = app->framesInFlightCount;

    if (gfxDeviceCreateSwapchain(app->device, &swapchainDesc, &app->swapchain) != GFX_RESULT_SUCCESS) {
        fprintf(stderr, "Failed to create swapchain\n");
        return false;
    }

    gfxSwapchainGetInfo(app->swapchain, &app->swapchainInfo);
    return true;
}

static bool initGraphics(ComputeApp* app)
{
    // Set up logging callback
    gfxSetLogCallback(logCallback, NULL);

    printf("Loading graphics backend...\n");
    if (gfxLoadBackend(GFX_BACKEND_API) != GFX_RESULT_SUCCESS) {
        fprintf(stderr, "Failed to load any graphics backend\n");
        return false;
    }

    // Create graphics instance
    const char* instanceExtensions[] = { GFX_INSTANCE_EXTENSION_SURFACE, GFX_INSTANCE_EXTENSION_DEBUG };
    GfxInstanceDescriptor instanceDesc = {};
    instanceDesc.sType = GFX_STRUCTURE_TYPE_INSTANCE_DESCRIPTOR;
    instanceDesc.pNext = NULL;
    instanceDesc.backend = GFX_BACKEND_API;
    instanceDesc.applicationName = "Compute Example (C)";
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
        fprintf(stderr, "Failed to request adapter\n");
        return false;
    }

    // Query and store adapter info
    gfxAdapterGetInfo(app->adapter, &app->adapterInfo);
    printf("Using adapter: %s\n", app->adapterInfo.name);
    printf("  Backend: %s\n", app->adapterInfo.backend == GFX_BACKEND_VULKAN ? "Vulkan" : "WebGPU");

    // Create device
    const char* deviceExtensions[] = { GFX_DEVICE_EXTENSION_SWAPCHAIN };
    GfxDeviceDescriptor deviceDesc = {};
    deviceDesc.sType = GFX_STRUCTURE_TYPE_DEVICE_DESCRIPTOR;
    deviceDesc.pNext = NULL;
    deviceDesc.label = NULL;
    deviceDesc.queueRequests = NULL;
    deviceDesc.queueRequestCount = 0;
    deviceDesc.enabledExtensions = deviceExtensions;
    deviceDesc.enabledExtensionCount = 1;

    if (gfxAdapterCreateDevice(app->adapter, &deviceDesc, &app->device) != GFX_RESULT_SUCCESS) {
        fprintf(stderr, "Failed to create device\n");
        return false;
    }

    if (gfxDeviceGetQueue(app->device, &app->queue) != GFX_RESULT_SUCCESS) {
        fprintf(stderr, "Failed to get device queue\n");
        return false;
    }

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

    return true;
}

static bool createComputeTexture(ComputeApp* app)
{
    GfxTextureDescriptor textureDesc = {};
    textureDesc.sType = GFX_STRUCTURE_TYPE_TEXTURE_DESCRIPTOR;
    textureDesc.pNext = NULL;
    textureDesc.type = GFX_TEXTURE_TYPE_2D;
    textureDesc.size = (GfxExtent3D){ COMPUTE_TEXTURE_WIDTH, COMPUTE_TEXTURE_HEIGHT, 1 };
    textureDesc.format = GFX_TEXTURE_FORMAT_R8G8B8A8_UNORM;
    textureDesc.usage = GFX_TEXTURE_USAGE_STORAGE_BINDING | GFX_TEXTURE_USAGE_TEXTURE_BINDING;
    textureDesc.arrayLayerCount = 1;
    textureDesc.mipLevelCount = 1;
    textureDesc.sampleCount = GFX_SAMPLE_COUNT_1;

    if (gfxDeviceCreateTexture(app->device, &textureDesc, &app->computeTexture) != GFX_RESULT_SUCCESS) {
        fprintf(stderr, "Failed to create compute texture\n");
        return false;
    }

    printf("Created compute texture: %dx%d\n", COMPUTE_TEXTURE_WIDTH, COMPUTE_TEXTURE_HEIGHT);

    GfxTextureViewDescriptor viewDesc = {};
    viewDesc.sType = GFX_STRUCTURE_TYPE_TEXTURE_VIEW_DESCRIPTOR;
    viewDesc.pNext = NULL;
    viewDesc.format = GFX_TEXTURE_FORMAT_R8G8B8A8_UNORM;
    viewDesc.viewType = GFX_TEXTURE_VIEW_TYPE_2D;
    viewDesc.baseMipLevel = 0;
    viewDesc.mipLevelCount = 1;
    viewDesc.baseArrayLayer = 0;
    viewDesc.arrayLayerCount = 1;

    if (gfxTextureCreateView(app->computeTexture, &viewDesc, &app->computeTextureView) != GFX_RESULT_SUCCESS) {
        fprintf(stderr, "Failed to create compute texture view\n");
        return false;
    }

    return true;
}

static bool createComputeShaders(ComputeApp* app)
{
    GfxShaderSourceType sourceType;
    void* computeCode = NULL;
    size_t computeSize = 0;
    bool formatSupported = false;

    // Query shader format support and use the first supported format
    // Try SPIR-V first (generally better performance)
    if (gfxDeviceSupportsShaderFormat(app->device, GFX_SHADER_SOURCE_SPIRV, &formatSupported) == GFX_RESULT_SUCCESS && formatSupported) {
        sourceType = GFX_SHADER_SOURCE_SPIRV;
        printf("Loading SPIR-V shader: generate.comp.spv\n");
        computeCode = loadBinaryFile("generate.comp.spv", &computeSize);
        if (!computeCode) {
            fprintf(stderr, "Failed to load SPIR-V compute shader\n");
            return false;
        }
    }
    // Fall back to WGSL
    else if (gfxDeviceSupportsShaderFormat(app->device, GFX_SHADER_SOURCE_WGSL, &formatSupported) == GFX_RESULT_SUCCESS && formatSupported) {
        sourceType = GFX_SHADER_SOURCE_WGSL;
        printf("Loading WGSL shader: shaders/generate.comp.wgsl\n");
        computeCode = loadTextFile("shaders/generate.comp.wgsl", &computeSize);
        if (!computeCode) {
            fprintf(stderr, "Failed to load WGSL compute shader\n");
            return false;
        }
    } else {
        fprintf(stderr, "Error: No supported shader format found (neither SPIR-V nor WGSL)\n");
        return false;
    }

    GfxShaderDescriptor computeShaderDesc = {};
    computeShaderDesc.sType = GFX_STRUCTURE_TYPE_SHADER_DESCRIPTOR;
    computeShaderDesc.pNext = NULL;
    computeShaderDesc.sourceType = sourceType;
    computeShaderDesc.code = computeCode;
    computeShaderDesc.codeSize = computeSize;
    computeShaderDesc.entryPoint = "main";

    if (gfxDeviceCreateShader(app->device, &computeShaderDesc, &app->computeShader) != GFX_RESULT_SUCCESS) {
        fprintf(stderr, "Failed to create compute shader\n");
        free(computeCode);
        return false;
    }
    free(computeCode);
    return true;
}

static bool createComputeUniformBuffers(ComputeApp* app)
{
    GfxBufferDescriptor computeUniformBufferDesc = {};
    computeUniformBufferDesc.sType = GFX_STRUCTURE_TYPE_BUFFER_DESCRIPTOR;
    computeUniformBufferDesc.pNext = NULL;
    computeUniformBufferDesc.size = sizeof(ComputeUniformData);
    computeUniformBufferDesc.usage = GFX_BUFFER_USAGE_UNIFORM | GFX_BUFFER_USAGE_COPY_DST;
    computeUniformBufferDesc.memoryProperties = GFX_MEMORY_PROPERTY_HOST_VISIBLE | GFX_MEMORY_PROPERTY_HOST_COHERENT;

    for (uint32_t i = 0; i < app->framesInFlightCount; ++i) {
        if (gfxDeviceCreateBuffer(app->device, &computeUniformBufferDesc, &app->computeUniformBuffer[i]) != GFX_RESULT_SUCCESS) {
            fprintf(stderr, "Failed to create compute uniform buffer %u\n", i);
            return false;
        }
    }

    return true;
}

static bool createComputeBindGroup(ComputeApp* app)
{
    GfxBindGroupLayoutEntry computeLayoutEntries[2] = {
        {
            .binding = 0,
            .visibility = GFX_SHADER_STAGE_COMPUTE,
            .type = GFX_BINDING_TYPE_STORAGE_TEXTURE,
            .storageTexture = {
                .format = GFX_TEXTURE_FORMAT_R8G8B8A8_UNORM,
                .viewDimension = GFX_TEXTURE_VIEW_TYPE_2D,
                .writeOnly = true,
            },
        },
        {
            .binding = 1,
            .visibility = GFX_SHADER_STAGE_COMPUTE,
            .type = GFX_BINDING_TYPE_BUFFER,
            .buffer = {
                .hasDynamicOffset = false,
                .minBindingSize = sizeof(ComputeUniformData),
            },
        }
    };

    GfxBindGroupLayoutDescriptor computeLayoutDesc = {};
    computeLayoutDesc.sType = GFX_STRUCTURE_TYPE_BIND_GROUP_LAYOUT_DESCRIPTOR;
    computeLayoutDesc.pNext = NULL;
    computeLayoutDesc.entryCount = 2;
    computeLayoutDesc.entries = computeLayoutEntries;

    if (gfxDeviceCreateBindGroupLayout(app->device, &computeLayoutDesc, &app->computeBindGroupLayout) != GFX_RESULT_SUCCESS) {
        fprintf(stderr, "Failed to create compute bind group layout\n");
        return false;
    }

    // Create compute bind groups (one per frame in flight)
    for (uint32_t i = 0; i < app->framesInFlightCount; ++i) {
        GfxBindGroupEntry computeEntries[2] = {
            {
                .binding = 0,
                .type = GFX_BIND_GROUP_ENTRY_TYPE_TEXTURE_VIEW,
                .resource = {
                    .textureView = app->computeTextureView,
                },
            },
            {
                .binding = 1,
                .type = GFX_BIND_GROUP_ENTRY_TYPE_BUFFER,
                .resource = {
                    .buffer = {
                        .buffer = app->computeUniformBuffer[i],
                        .offset = 0,
                        .size = sizeof(ComputeUniformData),
                    },
                },
            },
        };

        GfxBindGroupDescriptor computeBindGroupDesc = {};
        computeBindGroupDesc.sType = GFX_STRUCTURE_TYPE_BIND_GROUP_DESCRIPTOR;
        computeBindGroupDesc.pNext = NULL;
        computeBindGroupDesc.layout = app->computeBindGroupLayout;
        computeBindGroupDesc.entryCount = 2;
        computeBindGroupDesc.entries = computeEntries;

        if (gfxDeviceCreateBindGroup(app->device, &computeBindGroupDesc, &app->computeBindGroup[i]) != GFX_RESULT_SUCCESS) {
            fprintf(stderr, "Failed to create compute bind group %u\n", i);
            return false;
        }
    }
    return true;
}

static bool createComputePipeline(ComputeApp* app)
{
    GfxComputePipelineDescriptor computePipelineDesc = {};
    computePipelineDesc.sType = GFX_STRUCTURE_TYPE_COMPUTE_PIPELINE_DESCRIPTOR;
    computePipelineDesc.pNext = NULL;
    computePipelineDesc.compute = app->computeShader;
    computePipelineDesc.entryPoint = "main";
    computePipelineDesc.bindGroupLayouts = &app->computeBindGroupLayout;
    computePipelineDesc.bindGroupLayoutCount = 1;

    if (gfxDeviceCreateComputePipeline(app->device, &computePipelineDesc, &app->computePipeline) != GFX_RESULT_SUCCESS) {
        fprintf(stderr, "Failed to create compute pipeline\n");
        return false;
    }
    return true;
}

static bool transitionComputeTexture(ComputeApp* app)
{
    // Transition compute texture to SHADER_READ_ONLY layout initially
    // This way we don't need special handling for the first frame
    GfxCommandEncoder initEncoder = NULL;
    GfxCommandEncoderDescriptor initEncoderDesc = {};
    initEncoderDesc.sType = GFX_STRUCTURE_TYPE_COMMAND_ENCODER_DESCRIPTOR;
    initEncoderDesc.pNext = NULL;
    initEncoderDesc.label = "Init Layout Transition";

    if (gfxDeviceCreateCommandEncoder(app->device, &initEncoderDesc, &initEncoder) != GFX_RESULT_SUCCESS) {
        fprintf(stderr, "Failed to create command encoder for initial layout transition\n");
        return false;
    }

    if (gfxCommandEncoderBegin(initEncoder) != GFX_RESULT_SUCCESS) {
        fprintf(stderr, "Failed to begin initialization command encoder\n");
        gfxCommandEncoderDestroy(initEncoder);
        return false;
    }

    GfxTextureBarrier initBarrier = {
        .texture = app->computeTexture,
        .oldLayout = GFX_TEXTURE_LAYOUT_UNDEFINED,
        .newLayout = GFX_TEXTURE_LAYOUT_SHADER_READ_ONLY,
        .srcStageMask = GFX_PIPELINE_STAGE_TOP_OF_PIPE,
        .dstStageMask = GFX_PIPELINE_STAGE_FRAGMENT_SHADER,
        .srcAccessMask = 0,
        .dstAccessMask = GFX_ACCESS_SHADER_READ,
        .baseMipLevel = 0,
        .mipLevelCount = 1,
        .baseArrayLayer = 0,
        .arrayLayerCount = 1
    };
    GfxPipelineBarrierDescriptor barrierDesc = {};
    barrierDesc.sType = GFX_STRUCTURE_TYPE_PIPELINE_BARRIER_DESCRIPTOR;
    barrierDesc.pNext = NULL;
    barrierDesc.memoryBarriers = NULL;
    barrierDesc.memoryBarrierCount = 0;
    barrierDesc.bufferBarriers = NULL;
    barrierDesc.bufferBarrierCount = 0;
    barrierDesc.textureBarriers = &initBarrier;
    barrierDesc.textureBarrierCount = 1;

    if (gfxCommandEncoderPipelineBarrier(initEncoder, &barrierDesc) != GFX_RESULT_SUCCESS) {
        fprintf(stderr, "Failed to record initialization barrier\n");
        gfxCommandEncoderDestroy(initEncoder);
        return false;
    }

    if (gfxCommandEncoderEnd(initEncoder) != GFX_RESULT_SUCCESS) {
        fprintf(stderr, "Failed to end initialization command encoder\n");
        gfxCommandEncoderDestroy(initEncoder);
        return false;
    }

    GfxSubmitDescriptor submitDescriptor = {};
    submitDescriptor.sType = GFX_STRUCTURE_TYPE_SUBMIT_DESCRIPTOR;
    submitDescriptor.pNext = NULL;
    submitDescriptor.commandEncoderCount = 1;
    submitDescriptor.commandEncoders = &initEncoder;

    if (gfxQueueSubmit(app->queue, &submitDescriptor) != GFX_RESULT_SUCCESS) {
        fprintf(stderr, "Failed to submit initialization commands\n");
        gfxCommandEncoderDestroy(initEncoder);
        return false;
    }
    gfxDeviceWaitIdle(app->device);

    gfxCommandEncoderDestroy(initEncoder);
    return true;
}

static bool createComputeResources(ComputeApp* app)
{
    if (!createComputeTexture(app)) {
        return false;
    }

    if (!createComputeShaders(app)) {
        return false;
    }

    if (!createComputeUniformBuffers(app)) {
        return false;
    }

    if (!createComputeBindGroup(app)) {
        return false;
    }

    if (!createComputePipeline(app)) {
        return false;
    }

    if (!transitionComputeTexture(app)) {
        return false;
    }

    printf("Compute resources created successfully\n");
    return true;
}

static bool createRenderPass(ComputeApp* app)
{
    // Define color attachment target
    GfxRenderPassColorAttachmentTarget colorTarget = {
        .format = app->swapchainInfo.format,
        .sampleCount = GFX_SAMPLE_COUNT_1,
        .ops = {
            .loadOp = GFX_LOAD_OP_CLEAR,
            .storeOp = GFX_STORE_OP_STORE },
        .finalLayout = GFX_TEXTURE_LAYOUT_PRESENT_SRC
    };

    GfxRenderPassColorAttachment colorAttachment = {
        .target = colorTarget,
        .resolveTarget = NULL
    };

    GfxRenderPassDescriptor renderPassDesc = {};
    renderPassDesc.sType = GFX_STRUCTURE_TYPE_RENDER_PASS_DESCRIPTOR;
    renderPassDesc.pNext = NULL;
    renderPassDesc.label = "Fullscreen Render Pass";
    renderPassDesc.colorAttachments = &colorAttachment;
    renderPassDesc.colorAttachmentCount = 1;
    renderPassDesc.depthStencilAttachment = NULL;

    if (gfxDeviceCreateRenderPass(app->device, &renderPassDesc, &app->renderPass) != GFX_RESULT_SUCCESS) {
        fprintf(stderr, "Failed to create render pass\n");
        return false;
    }

    return true;
}

static bool createFramebuffers(ComputeApp* app)
{
    for (uint32_t i = 0; i < app->framesInFlightCount; ++i) {
        GfxTextureView backbuffer = NULL;
        GfxResult result = gfxSwapchainGetTextureView(app->swapchain, i, &backbuffer);
        if (result != GFX_RESULT_SUCCESS || !backbuffer) {
            fprintf(stderr, "[ERROR] Failed to get swapchain image view %d\n", i);
            return false;
        }

        GfxFramebufferAttachment fbColorAttachment = {
            .view = backbuffer,
            .resolveTarget = NULL
        };

        GfxFramebufferAttachment fbDepthAttachment = {
            .view = NULL,
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
        fbDesc.extent.width = app->swapchainInfo.extent.width;
        fbDesc.extent.height = app->swapchainInfo.extent.height;

        if (gfxDeviceCreateFramebuffer(app->device, &fbDesc, &app->framebuffers[i]) != GFX_RESULT_SUCCESS) {
            fprintf(stderr, "Failed to create framebuffer %u\n", i);
            return false;
        }
    }

    return true;
}

static bool createRenderShaders(ComputeApp* app)
{
    GfxShaderSourceType vertexSourceType;
    void* vertexCode = NULL;
    size_t vertexSize = 0;
    bool formatSupported = false;

    // Query shader format support and use the first supported format
    // Try SPIR-V first
    if (gfxDeviceSupportsShaderFormat(app->device, GFX_SHADER_SOURCE_SPIRV, &formatSupported) == GFX_RESULT_SUCCESS && formatSupported) {
        vertexSourceType = GFX_SHADER_SOURCE_SPIRV;
        vertexCode = loadBinaryFile("fullscreen.vert.spv", &vertexSize);
        if (!vertexCode) {
            fprintf(stderr, "Failed to load SPIR-V vertex shader\n");
            return false;
        }
    }
    // Fall back to WGSL
    else if (gfxDeviceSupportsShaderFormat(app->device, GFX_SHADER_SOURCE_WGSL, &formatSupported) == GFX_RESULT_SUCCESS && formatSupported) {
        vertexSourceType = GFX_SHADER_SOURCE_WGSL;
        vertexCode = loadTextFile("shaders/fullscreen.vert.wgsl", &vertexSize);
        if (!vertexCode) {
            fprintf(stderr, "Failed to load WGSL vertex shader\n");
            return false;
        }
    } else {
        fprintf(stderr, "Error: No supported shader format found\n");
        return false;
    }

    GfxShaderDescriptor vertexShaderDesc = {};
    vertexShaderDesc.sType = GFX_STRUCTURE_TYPE_SHADER_DESCRIPTOR;
    vertexShaderDesc.pNext = NULL;
    vertexShaderDesc.sourceType = vertexSourceType;
    vertexShaderDesc.code = vertexCode;
    vertexShaderDesc.codeSize = vertexSize;
    vertexShaderDesc.entryPoint = "main";

    if (gfxDeviceCreateShader(app->device, &vertexShaderDesc, &app->vertexShader) != GFX_RESULT_SUCCESS) {
        fprintf(stderr, "Failed to create vertex shader\n");
        free(vertexCode);
        return false;
    }
    free(vertexCode);

    GfxShaderSourceType fragmentSourceType;
    void* fragmentCode = NULL;
    size_t fragmentSize = 0;

    // Use same format as vertex shader (already queried above)
    if (vertexSourceType == GFX_SHADER_SOURCE_SPIRV) {
        fragmentSourceType = GFX_SHADER_SOURCE_SPIRV;
        fragmentCode = loadBinaryFile("postprocess.frag.spv", &fragmentSize);
        if (!fragmentCode) {
            fprintf(stderr, "Failed to load SPIR-V fragment shader\n");
            return false;
        }
    } else {
        fragmentSourceType = GFX_SHADER_SOURCE_WGSL;
        fragmentCode = loadTextFile("shaders/postprocess.frag.wgsl", &fragmentSize);
        if (!fragmentCode) {
            fprintf(stderr, "Failed to load WGSL fragment shader\n");
            return false;
        }
    }

    GfxShaderDescriptor fragmentShaderDesc = {};
    fragmentShaderDesc.sType = GFX_STRUCTURE_TYPE_SHADER_DESCRIPTOR;
    fragmentShaderDesc.pNext = NULL;
    fragmentShaderDesc.sourceType = fragmentSourceType;
    fragmentShaderDesc.code = fragmentCode;
    fragmentShaderDesc.codeSize = fragmentSize;
    fragmentShaderDesc.entryPoint = "main";

    if (gfxDeviceCreateShader(app->device, &fragmentShaderDesc, &app->fragmentShader) != GFX_RESULT_SUCCESS) {
        fprintf(stderr, "Failed to create fragment shader\n");
        free(fragmentCode);
        return false;
    }
    free(fragmentCode);
    return true;
}

static bool createSampler(ComputeApp* app)
{
    GfxSamplerDescriptor samplerDesc = {};
    samplerDesc.sType = GFX_STRUCTURE_TYPE_SAMPLER_DESCRIPTOR;
    samplerDesc.pNext = NULL;
    samplerDesc.magFilter = GFX_FILTER_MODE_LINEAR;
    samplerDesc.minFilter = GFX_FILTER_MODE_LINEAR;
    samplerDesc.addressModeU = GFX_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerDesc.addressModeV = GFX_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerDesc.maxAnisotropy = 1;

    if (gfxDeviceCreateSampler(app->device, &samplerDesc, &app->sampler) != GFX_RESULT_SUCCESS) {
        fprintf(stderr, "Failed to create sampler\n");
        return false;
    }

    return true;
}

static bool createRenderUniformBuffers(ComputeApp* app)
{
    // Create render uniform buffers (one per frame in flight)
    GfxBufferDescriptor renderUniformBufferDesc = {};
    renderUniformBufferDesc.sType = GFX_STRUCTURE_TYPE_BUFFER_DESCRIPTOR;
    renderUniformBufferDesc.pNext = NULL;
    renderUniformBufferDesc.size = sizeof(RenderUniformData);
    renderUniformBufferDesc.usage = GFX_BUFFER_USAGE_UNIFORM | GFX_BUFFER_USAGE_COPY_DST;
    renderUniformBufferDesc.memoryProperties = GFX_MEMORY_PROPERTY_HOST_VISIBLE | GFX_MEMORY_PROPERTY_HOST_COHERENT;

    for (uint32_t i = 0; i < app->framesInFlightCount; ++i) {
        if (gfxDeviceCreateBuffer(app->device, &renderUniformBufferDesc, &app->renderUniformBuffer[i]) != GFX_RESULT_SUCCESS) {
            fprintf(stderr, "Failed to create render uniform buffer %u\n", i);
            return false;
        }
    }

    return true;
}

static bool createRenderBindGroups(ComputeApp* app)
{
    // Create render bind group layout
    GfxBindGroupLayoutEntry renderLayoutEntries[3] = {
        {
            .binding = 0,
            .visibility = GFX_SHADER_STAGE_FRAGMENT,
            .type = GFX_BINDING_TYPE_SAMPLER,
            .sampler = {
                .comparison = false,
            },
        },
        {
            .binding = 1,
            .visibility = GFX_SHADER_STAGE_FRAGMENT,
            .type = GFX_BINDING_TYPE_TEXTURE,
            .texture = {
                .sampleType = GFX_TEXTURE_SAMPLE_TYPE_FLOAT,
                .viewDimension = GFX_TEXTURE_VIEW_TYPE_2D,
                .multisampled = false,
            },
        },
        {
            .binding = 2,
            .visibility = GFX_SHADER_STAGE_FRAGMENT,
            .type = GFX_BINDING_TYPE_BUFFER,
            .buffer = {
                .hasDynamicOffset = false,
                .minBindingSize = sizeof(RenderUniformData),
            },
        },
    };

    GfxBindGroupLayoutDescriptor renderLayoutDesc = {};
    renderLayoutDesc.sType = GFX_STRUCTURE_TYPE_BIND_GROUP_LAYOUT_DESCRIPTOR;
    renderLayoutDesc.pNext = NULL;
    renderLayoutDesc.entryCount = 3;
    renderLayoutDesc.entries = renderLayoutEntries;

    if (gfxDeviceCreateBindGroupLayout(app->device, &renderLayoutDesc, &app->renderBindGroupLayout) != GFX_RESULT_SUCCESS) {
        fprintf(stderr, "Failed to create render bind group layout\n");
        return false;
    }

    // Create render bind groups (one per frame in flight)
    for (uint32_t i = 0; i < app->framesInFlightCount; ++i) {
        GfxBindGroupEntry renderEntries[3] = {
            {
                .binding = 0,
                .type = GFX_BIND_GROUP_ENTRY_TYPE_SAMPLER,
                .resource = {
                    .sampler = app->sampler,
                },
            },
            {
                .binding = 1,
                .type = GFX_BIND_GROUP_ENTRY_TYPE_TEXTURE_VIEW,
                .resource = {
                    .textureView = app->computeTextureView,
                },
            },
            {
                .binding = 2,
                .type = GFX_BIND_GROUP_ENTRY_TYPE_BUFFER,
                .resource = {
                    .buffer = {
                        .buffer = app->renderUniformBuffer[i],
                        .offset = 0,
                        .size = sizeof(RenderUniformData),
                    },
                },
            }
        };

        GfxBindGroupDescriptor renderBindGroupDesc = {};
        renderBindGroupDesc.sType = GFX_STRUCTURE_TYPE_BIND_GROUP_DESCRIPTOR;
        renderBindGroupDesc.pNext = NULL;
        renderBindGroupDesc.layout = app->renderBindGroupLayout;
        renderBindGroupDesc.entryCount = 3;
        renderBindGroupDesc.entries = renderEntries;

        if (gfxDeviceCreateBindGroup(app->device, &renderBindGroupDesc, &app->renderBindGroup[i]) != GFX_RESULT_SUCCESS) {
            fprintf(stderr, "Failed to create render bind group %u\n", i);
            return false;
        }
    }

    return true;
}

static bool createRenderPipeline(ComputeApp* app)
{
    GfxVertexState vertexState = {
        .module = app->vertexShader,
        .entryPoint = "main",
        .bufferCount = 0
    };

    GfxColorTargetState colorTarget = {
        .format = app->swapchainInfo.format,
        .writeMask = GFX_COLOR_WRITE_MASK_ALL
    };

    GfxFragmentState fragmentState = {
        .module = app->fragmentShader,
        .entryPoint = "main",
        .targetCount = 1,
        .targets = &colorTarget
    };

    GfxPrimitiveState primitiveState = {
        .topology = GFX_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        .frontFace = GFX_FRONT_FACE_COUNTER_CLOCKWISE,
        .cullMode = GFX_CULL_MODE_NONE,
        .polygonMode = GFX_POLYGON_MODE_FILL
    };

    GfxRenderPipelineDescriptor pipelineDesc = {};
    pipelineDesc.sType = GFX_STRUCTURE_TYPE_RENDER_PIPELINE_DESCRIPTOR;
    pipelineDesc.pNext = NULL;
    pipelineDesc.renderPass = app->renderPass;
    pipelineDesc.vertex = &vertexState;
    pipelineDesc.fragment = &fragmentState;
    pipelineDesc.primitive = &primitiveState;
    pipelineDesc.depthStencil = NULL;
    pipelineDesc.sampleCount = GFX_SAMPLE_COUNT_1;
    pipelineDesc.bindGroupLayoutCount = 1;
    pipelineDesc.bindGroupLayouts = &app->renderBindGroupLayout;

    if (gfxDeviceCreateRenderPipeline(app->device, &pipelineDesc, &app->renderPipeline) != GFX_RESULT_SUCCESS) {
        fprintf(stderr, "Failed to create render pipeline\n");
        return false;
    }

    return true;
}

static bool createRenderResources(ComputeApp* app)
{
    if (!createRenderShaders(app)) {
        return false;
    }

    if (!createSampler(app)) {
        return false;
    }

    if (!createRenderUniformBuffers(app)) {
        return false;
    }

    if (!createRenderBindGroups(app)) {
        return false;
    }

    if (!createRenderPipeline(app)) {
        return false;
    }

    printf("Render resources created successfully\n");
    return true;
}
static bool createSyncObjects(ComputeApp* app)
{
    GfxSemaphoreDescriptor semaphoreDesc = {};
    semaphoreDesc.sType = GFX_STRUCTURE_TYPE_SEMAPHORE_DESCRIPTOR;
    semaphoreDesc.pNext = NULL;
    semaphoreDesc.type = GFX_SEMAPHORE_TYPE_BINARY;

    GfxFenceDescriptor fenceDesc = {};
    fenceDesc.sType = GFX_STRUCTURE_TYPE_FENCE_DESCRIPTOR;
    fenceDesc.pNext = NULL;
    fenceDesc.signaled = true;

    for (uint32_t i = 0; i < app->framesInFlightCount; ++i) {
        if (gfxDeviceCreateSemaphore(app->device, &semaphoreDesc, &app->imageAvailableSemaphores[i]) != GFX_RESULT_SUCCESS) {
            fprintf(stderr, "Failed to create image available semaphore\n");
            return false;
        }

        if (gfxDeviceCreateSemaphore(app->device, &semaphoreDesc, &app->renderFinishedSemaphores[i]) != GFX_RESULT_SUCCESS) {
            fprintf(stderr, "Failed to create render finished semaphore\n");
            return false;
        }

        if (gfxDeviceCreateFence(app->device, &fenceDesc, &app->inFlightFences[i]) != GFX_RESULT_SUCCESS) {
            fprintf(stderr, "Failed to create fence\n");
            return false;
        }

        // Create command encoder for this frame
        char label[64];
        snprintf(label, sizeof(label), "Command Encoder %u", i);
        GfxCommandEncoderDescriptor encoderDesc = {};
        encoderDesc.sType = GFX_STRUCTURE_TYPE_COMMAND_ENCODER_DESCRIPTOR;
        encoderDesc.pNext = NULL;
        encoderDesc.label = label;

        if (gfxDeviceCreateCommandEncoder(app->device, &encoderDesc, &app->commandEncoders[i]) != GFX_RESULT_SUCCESS) {
            fprintf(stderr, "Failed to create command encoder %u\n", i);
            return false;
        }
    }

    return true;
}

static void update(ComputeApp* app, float deltaTime)
{
    app->elapsedTime += deltaTime;
}

static void render(ComputeApp* app)
{
    uint32_t frameIndex = app->currentFrame;

    // Wait for previous frame
    gfxFenceWait(app->inFlightFences[frameIndex], GFX_TIMEOUT_INFINITE);
    gfxFenceReset(app->inFlightFences[frameIndex]);

    // Acquire swapchain image
    uint32_t imageIndex = 0;
    GfxResult result = gfxSwapchainAcquireNextImage(
        app->swapchain,
        UINT64_MAX,
        app->imageAvailableSemaphores[frameIndex],
        NULL,
        &imageIndex);

    if (result != GFX_RESULT_SUCCESS) {
        fprintf(stderr, "Failed to acquire swapchain image\n");
        return;
    }

    // Update compute uniforms for current frame
    ComputeUniformData computeUniforms = {
        .time = app->elapsedTime
    };

    gfxQueueWriteBuffer(app->queue, app->computeUniformBuffer[frameIndex], 0, &computeUniforms, sizeof(computeUniforms));

    // Update render uniforms for current frame
    RenderUniformData renderUniforms = {
        .postProcessStrength = 0.5f + 0.5f * sinf(app->elapsedTime * 0.5f) // Animate strength
    };
    gfxQueueWriteBuffer(app->queue, app->renderUniformBuffer[frameIndex], 0, &renderUniforms, sizeof(renderUniforms));

    // Begin command encoder for reuse
    GfxCommandEncoder encoder = app->commandEncoders[frameIndex];
    gfxCommandEncoderBegin(encoder);

    // Transition compute texture to GENERAL layout for compute shader write
    GfxTextureBarrier readToWriteBarrier = {
        .texture = app->computeTexture,
        .oldLayout = GFX_TEXTURE_LAYOUT_SHADER_READ_ONLY,
        .newLayout = GFX_TEXTURE_LAYOUT_GENERAL,
        .srcStageMask = GFX_PIPELINE_STAGE_FRAGMENT_SHADER,
        .dstStageMask = GFX_PIPELINE_STAGE_COMPUTE_SHADER,
        .srcAccessMask = GFX_ACCESS_SHADER_READ,
        .dstAccessMask = GFX_ACCESS_SHADER_WRITE,
        .baseMipLevel = 0,
        .mipLevelCount = 1,
        .baseArrayLayer = 0,
        .arrayLayerCount = 1
    };
    GfxPipelineBarrierDescriptor readToWriteBarrierDesc = {};
    readToWriteBarrierDesc.sType = GFX_STRUCTURE_TYPE_PIPELINE_BARRIER_DESCRIPTOR;
    readToWriteBarrierDesc.pNext = NULL;
    readToWriteBarrierDesc.memoryBarriers = NULL;
    readToWriteBarrierDesc.memoryBarrierCount = 0;
    readToWriteBarrierDesc.bufferBarriers = NULL;
    readToWriteBarrierDesc.bufferBarrierCount = 0;
    readToWriteBarrierDesc.textureBarriers = &readToWriteBarrier;
    readToWriteBarrierDesc.textureBarrierCount = 1;

    gfxCommandEncoderPipelineBarrier(encoder, &readToWriteBarrierDesc);

    // --- COMPUTE PASS: Generate pattern ---
    GfxComputePassBeginDescriptor computePassDesc = {
        .label = "Generate Pattern"
    };
    GfxComputePassEncoder computePass = NULL;
    if (gfxCommandEncoderBeginComputePass(encoder, &computePassDesc, &computePass) != GFX_RESULT_SUCCESS) {
        fprintf(stderr, "Failed to begin compute pass\n");
        return;
    }

    gfxComputePassEncoderSetPipeline(computePass, app->computePipeline);
    gfxComputePassEncoderSetBindGroup(computePass, 0, app->computeBindGroup[frameIndex], NULL, 0);

    // Dispatch compute (16x16 local size, so divide by 16)
    // Uses fixed compute texture resolution, sampler will upscale/downscale to window
    uint32_t workGroupsX = (COMPUTE_TEXTURE_WIDTH + 15) / 16;
    uint32_t workGroupsY = (COMPUTE_TEXTURE_HEIGHT + 15) / 16;
    gfxComputePassEncoderDispatch(computePass, workGroupsX, workGroupsY, 1);

    if (gfxComputePassEncoderEnd(computePass) != GFX_RESULT_SUCCESS) {
        fprintf(stderr, "Failed to end compute pass\n");
        return;
    }

    // Transition compute texture for shader read
    GfxTextureBarrier computeToReadBarrier = {
        .texture = app->computeTexture,
        .oldLayout = GFX_TEXTURE_LAYOUT_GENERAL,
        .newLayout = GFX_TEXTURE_LAYOUT_SHADER_READ_ONLY,
        .srcStageMask = GFX_PIPELINE_STAGE_COMPUTE_SHADER,
        .dstStageMask = GFX_PIPELINE_STAGE_FRAGMENT_SHADER,
        .srcAccessMask = GFX_ACCESS_SHADER_WRITE,
        .dstAccessMask = GFX_ACCESS_SHADER_READ,
        .baseMipLevel = 0,
        .mipLevelCount = 1,
        .baseArrayLayer = 0,
        .arrayLayerCount = 1
    };
    GfxPipelineBarrierDescriptor computeToReadBarrierDesc = {};
    computeToReadBarrierDesc.sType = GFX_STRUCTURE_TYPE_PIPELINE_BARRIER_DESCRIPTOR;
    computeToReadBarrierDesc.pNext = NULL;
    computeToReadBarrierDesc.memoryBarriers = NULL;
    computeToReadBarrierDesc.memoryBarrierCount = 0;
    computeToReadBarrierDesc.bufferBarriers = NULL;
    computeToReadBarrierDesc.bufferBarrierCount = 0;
    computeToReadBarrierDesc.textureBarriers = &computeToReadBarrier;
    computeToReadBarrierDesc.textureBarrierCount = 1;

    gfxCommandEncoderPipelineBarrier(encoder, &computeToReadBarrierDesc);

    // --- RENDER PASS: Post-process and display ---
    GfxColor clearColor = { 0.0f, 0.0f, 0.0f, 1.0f };

    GfxRenderPassBeginDescriptor renderPassBeginDesc = {
        .label = "Fullscreen Render Pass",
        .renderPass = app->renderPass,
        .framebuffer = app->framebuffers[imageIndex],
        .colorClearValues = &clearColor,
        .colorClearValueCount = 1,
        .depthClearValue = 0.0f,
        .stencilClearValue = 0
    };

    GfxRenderPassEncoder renderPass = NULL;
    if (gfxCommandEncoderBeginRenderPass(encoder, &renderPassBeginDesc, &renderPass) != GFX_RESULT_SUCCESS) {
        fprintf(stderr, "Failed to begin render pass\n");
        return;
    }

    gfxRenderPassEncoderSetPipeline(renderPass, app->renderPipeline);
    gfxRenderPassEncoderSetBindGroup(renderPass, 0, app->renderBindGroup[frameIndex], NULL, 0);

    GfxViewport viewport = {
        .x = 0.0f,
        .y = 0.0f,
        .width = (float)app->windowWidth,
        .height = (float)app->windowHeight,
        .minDepth = 0.0f,
        .maxDepth = 1.0f
    };
    gfxRenderPassEncoderSetViewport(renderPass, &viewport);

    GfxScissorRect scissor = {
        .origin = { 0, 0 },
        .extent = { app->windowWidth, app->windowHeight }
    };
    gfxRenderPassEncoderSetScissorRect(renderPass, &scissor);

    // Draw fullscreen quad (6 vertices, no buffers needed)
    gfxRenderPassEncoderDraw(renderPass, 6, 1, 0, 0);

    if (gfxRenderPassEncoderEnd(renderPass) != GFX_RESULT_SUCCESS) {
        fprintf(stderr, "Failed to end render pass\n");
        return;
    }

    if (gfxCommandEncoderEnd(encoder) != GFX_RESULT_SUCCESS) {
        fprintf(stderr, "Failed to end command encoder\n");
        return;
    }

    // Submit
    GfxSubmitDescriptor submitDescriptor = {};
    submitDescriptor.sType = GFX_STRUCTURE_TYPE_SUBMIT_DESCRIPTOR;
    submitDescriptor.pNext = NULL;
    submitDescriptor.commandEncoderCount = 1;
    submitDescriptor.commandEncoders = &encoder;
    submitDescriptor.waitSemaphoreCount = 1;
    submitDescriptor.waitSemaphores = &app->imageAvailableSemaphores[frameIndex];
    submitDescriptor.signalSemaphoreCount = 1;
    submitDescriptor.signalSemaphores = &app->renderFinishedSemaphores[frameIndex];
    submitDescriptor.signalFence = app->inFlightFences[frameIndex];

    if (gfxQueueSubmit(app->queue, &submitDescriptor) != GFX_RESULT_SUCCESS) {
        fprintf(stderr, "Failed to submit command buffer\n");
        return;
    }

    // Present
    GfxPresentDescriptor presentDescriptor = {};
    presentDescriptor.sType = GFX_STRUCTURE_TYPE_PRESENT_DESCRIPTOR;
    presentDescriptor.pNext = NULL;
    presentDescriptor.waitSemaphoreCount = 1;
    presentDescriptor.waitSemaphores = &app->renderFinishedSemaphores[frameIndex];

    result = gfxSwapchainPresent(app->swapchain, &presentDescriptor);
    if (result != GFX_RESULT_SUCCESS) {
        fprintf(stderr, "Failed to present swapchain image\n");
        return;
    }

    app->currentFrame = (app->currentFrame + 1) % app->framesInFlightCount;
}

static void cleanup(ComputeApp* app)
{
    if (app->device) {
        gfxDeviceWaitIdle(app->device);
    }

    // Sync objects
    if (app->commandEncoders) {
        for (uint32_t i = 0; i < app->framesInFlightCount; ++i) {
            if (app->commandEncoders[i]) {
                gfxCommandEncoderDestroy(app->commandEncoders[i]);
            }
        }
        free(app->commandEncoders);
        app->commandEncoders = NULL;
    }
    if (app->imageAvailableSemaphores) {
        for (uint32_t i = 0; i < app->framesInFlightCount; ++i) {
            if (app->imageAvailableSemaphores[i]) {
                gfxSemaphoreDestroy(app->imageAvailableSemaphores[i]);
            }
        }
        free(app->imageAvailableSemaphores);
        app->imageAvailableSemaphores = NULL;
    }
    if (app->renderFinishedSemaphores) {
        for (uint32_t i = 0; i < app->framesInFlightCount; ++i) {
            if (app->renderFinishedSemaphores[i]) {
                gfxSemaphoreDestroy(app->renderFinishedSemaphores[i]);
            }
        }
        free(app->renderFinishedSemaphores);
        app->renderFinishedSemaphores = NULL;
    }
    if (app->inFlightFences) {
        for (uint32_t i = 0; i < app->framesInFlightCount; ++i) {
            if (app->inFlightFences[i]) {
                gfxFenceDestroy(app->inFlightFences[i]);
            }
        }
        free(app->inFlightFences);
        app->inFlightFences = NULL;
    }

    // Render resources
    if (app->renderPipeline) {
        gfxRenderPipelineDestroy(app->renderPipeline);
    }
    if (app->framebuffers) {
        for (uint32_t i = 0; i < app->framesInFlightCount; ++i) {
            if (app->framebuffers[i]) {
                gfxFramebufferDestroy(app->framebuffers[i]);
            }
        }
        free(app->framebuffers);
        app->framebuffers = NULL;
    }
    if (app->renderBindGroup) {
        for (uint32_t i = 0; i < app->framesInFlightCount; ++i) {
            if (app->renderBindGroup[i]) {
                gfxBindGroupDestroy(app->renderBindGroup[i]);
            }
        }
        free(app->renderBindGroup);
        app->renderBindGroup = NULL;
    }
    if (app->renderUniformBuffer) {
        for (uint32_t i = 0; i < app->framesInFlightCount; ++i) {
            if (app->renderUniformBuffer[i]) {
                gfxBufferDestroy(app->renderUniformBuffer[i]);
            }
        }
        free(app->renderUniformBuffer);
        app->renderUniformBuffer = NULL;
    }
    if (app->renderPass) {
        gfxRenderPassDestroy(app->renderPass);
    }
    if (app->renderBindGroupLayout) {
        gfxBindGroupLayoutDestroy(app->renderBindGroupLayout);
    }
    if (app->sampler) {
        gfxSamplerDestroy(app->sampler);
    }
    if (app->fragmentShader) {
        gfxShaderDestroy(app->fragmentShader);
    }
    if (app->vertexShader) {
        gfxShaderDestroy(app->vertexShader);
    }

    // Compute resources
    if (app->computePipeline) {
        gfxComputePipelineDestroy(app->computePipeline);
    }
    if (app->computeBindGroup) {
        for (uint32_t i = 0; i < app->framesInFlightCount; ++i) {
            if (app->computeBindGroup[i]) {
                gfxBindGroupDestroy(app->computeBindGroup[i]);
            }
        }
        free(app->computeBindGroup);
        app->computeBindGroup = NULL;
    }
    if (app->computeUniformBuffer) {
        for (uint32_t i = 0; i < app->framesInFlightCount; ++i) {
            if (app->computeUniformBuffer[i]) {
                gfxBufferDestroy(app->computeUniformBuffer[i]);
            }
        }
        free(app->computeUniformBuffer);
        app->computeUniformBuffer = NULL;
    }
    if (app->computeBindGroupLayout) {
        gfxBindGroupLayoutDestroy(app->computeBindGroupLayout);
    }
    if (app->computeShader) {
        gfxShaderDestroy(app->computeShader);
    }
    if (app->computeTextureView) {
        gfxTextureViewDestroy(app->computeTextureView);
    }
    if (app->computeTexture) {
        gfxTextureDestroy(app->computeTexture);
    }

    // Core resources
    if (app->swapchain) {
        gfxSwapchainDestroy(app->swapchain);
    }
    if (app->surface) {
        gfxSurfaceDestroy(app->surface);
    }
    if (app->device) {
        gfxDeviceDestroy(app->device);
    }
    if (app->instance) {
        gfxInstanceDestroy(app->instance);
    }

    // Unload the backend API after destroying all instances
    gfxUnloadBackend(GFX_BACKEND_API);

    if (app->window) {
        glfwDestroyWindow(app->window);
        glfwTerminate();
    }
}

// Helper function to cleanup size-dependent resources
static void cleanupSizeDependentResources(ComputeApp* app)
{
    if (app->framebuffers) {
        for (uint32_t i = 0; i < app->framesInFlightCount; ++i) {
            if (app->framebuffers[i]) {
                gfxFramebufferDestroy(app->framebuffers[i]);
                app->framebuffers[i] = NULL;
            }
        }
    }

    if (app->renderPass) {
        gfxRenderPassDestroy(app->renderPass);
        app->renderPass = NULL;
    }

    if (app->swapchain) {
        gfxSwapchainDestroy(app->swapchain);
        app->swapchain = NULL;
    }
}

// Helper function to recreate size-dependent resources
static bool createSizeDependentResources(ComputeApp* app, uint32_t width, uint32_t height)
{
    // Create swapchain with new dimensions
    // Compute texture stays at fixed resolution and is sampled with linear filtering
    if (!createSwapchain(app, width, height)) {
        return false;
    }

    if (!createRenderPass(app)) {
        return false;
    }

    // Recreate framebuffers with new swapchain images
    if (!createFramebuffers(app)) {
        return false;
    }

    return true;
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
static bool mainLoopIteration(ComputeApp* app)
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
    float deltaTime = currentTime - app->elapsedTime;

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
    ComputeApp* app = (ComputeApp*)userData;
    if (!mainLoopIteration(app)) {
        emscripten_cancel_main_loop();
        cleanup(app);
    }
}
#endif

int main(void)
{
    printf("=== Compute & Postprocess Example (C) ===\n\n");

    ComputeApp app = { 0 };
    app.currentFrame = 0;
    app.windowWidth = WINDOW_WIDTH;
    app.windowHeight = WINDOW_HEIGHT;

    if (!initWindow(&app)) {
        return 1;
    }

    if (!initGraphics(&app)) {
        cleanup(&app);
        return 1;
    }

    if (!createSizeDependentResources(&app, app.windowWidth, app.windowHeight)) {
        cleanup(&app);
        return 1;
    }

    if (!createComputeResources(&app)) {
        cleanup(&app);
        return 1;
    }

    if (!createRenderResources(&app)) {
        cleanup(&app);
        return 1;
    }

    if (!createSyncObjects(&app)) {
        cleanup(&app);
        return 1;
    }

    printf("\nStarting render loop...\n");
    printf("Press ESC to exit\n\n");

    // Initialize loop state
    app.previousWidth = app.windowWidth;
    app.previousHeight = app.windowHeight;
    app.elapsedTime = 0.0f;

    // Initialize FPS tracking
    app.fpsFrameCount = 0;
    app.fpsTimeAccumulator = 0.0f;
    app.fpsFrameTimeMin = FLT_MAX;
    app.fpsFrameTimeMax = 0.0f;

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
