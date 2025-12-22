#include "GfxApi.h"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#ifdef _WIN32
#define GLFW_EXPOSE_NATIVE_WIN32
#elif defined(__linux__)
#define GLFW_EXPOSE_NATIVE_X11
#elif defined(__APPLE__)
#define GLFW_EXPOSE_NATIVE_COCOA
#endif
#include <GLFW/glfw3native.h>

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
#define MSAA_SAMPLE_COUNT GFX_SAMPLE_COUNT_4
#define COLOR_FORMAT GFX_TEXTURE_FORMAT_B8G8R8A8_UNORM_SRGB
#define DEPTH_FORMAT GFX_TEXTURE_FORMAT_DEPTH32_FLOAT

// Debug callback function
static void debugCallback(GfxDebugMessageSeverity severity, GfxDebugMessageType type, const char* message, void* userData)
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

    printf("[%s|%s] %s\n", severityStr, typeStr, message);
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

typedef struct {
    GLFWwindow* window;

    GfxInstance instance;
    GfxAdapter adapter;
    GfxDevice device;
    GfxQueue queue;
    GfxSurface surface;
    GfxSwapchain swapchain;

    GfxBuffer vertexBuffer;
    GfxBuffer indexBuffer;
    GfxShader vertexShader;
    GfxShader fragmentShader;
    GfxRenderPipeline renderPipeline;
    GfxBindGroupLayout uniformBindGroupLayout;

    // Depth buffer
    GfxTexture depthTexture;
    GfxTextureView depthTextureView;

    // MSAA color buffer
    GfxTexture msaaColorTexture;
    GfxTextureView msaaColorTextureView;

    uint32_t windowWidth;
    uint32_t windowHeight;

    // Per-frame resources (for frames in flight)
    GfxBuffer sharedUniformBuffer; // Single buffer for all frames
    size_t uniformAlignedSize; // Aligned size per frame
    GfxBindGroup uniformBindGroups[MAX_FRAMES_IN_FLIGHT];
    GfxCommandEncoder commandEncoders[MAX_FRAMES_IN_FLIGHT]; // Track for deferred cleanup

    // Per-frame synchronization
    GfxSemaphore imageAvailableSemaphores[MAX_FRAMES_IN_FLIGHT];
    GfxSemaphore renderFinishedSemaphores[MAX_FRAMES_IN_FLIGHT];
    GfxFence inFlightFences[MAX_FRAMES_IN_FLIGHT];
    uint32_t currentFrame;

    // Animation state
    float rotationAngleX;
    float rotationAngleY;
    double lastTime;
} CubeApp;

// Function declarations
static bool initWindow(CubeApp* app);
static bool initializeGraphics(CubeApp* app);
static bool createSyncObjects(CubeApp* app);
static bool createSizeDependentResources(CubeApp* app, uint32_t width, uint32_t height);
static void cleanupSizeDependentResources(CubeApp* app);
static bool createRenderingResources(CubeApp* app);
static void cleanupRenderingResources(CubeApp* app);
static bool createRenderPipeline(CubeApp* app);
static void updateUniforms(CubeApp* app);
static void render(CubeApp* app);
static void cleanup(CubeApp* app);
static GfxPlatformWindowHandle getPlatformWindowHandle(GLFWwindow* window);
static void* loadBinaryFile(const char* filepath, size_t* outSize);

// Matrix math function declarations
void matrixIdentity(float* matrix);
void matrixMultiply(float* result, const float* a, const float* b);
void matrixRotateX(float* matrix, float angle);
void matrixRotateY(float* matrix, float angle);
void matrixRotateZ(float* matrix, float angle);
void matrixPerspective(float* matrix, float fov, float aspect, float near, float far);
void matrixLookAt(float* matrix, float eyeX, float eyeY, float eyeZ,
    float centerX, float centerY, float centerZ,
    float upX, float upY, float upZ);

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

    app->window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT,
        "Cube Example - Unified Graphics API",
        NULL, NULL);

    if (!app->window) {
        fprintf(stderr, "Failed to create GLFW window\n");
        glfwTerminate();
        return false;
    }

    glfwSetWindowUserPointer(app->window, app);
    glfwSetFramebufferSizeCallback(app->window, framebufferSizeCallback);
    glfwSetKeyCallback(app->window, keyCallback);

    return true;
}

GfxPlatformWindowHandle getPlatformWindowHandle(GLFWwindow* window)
{
    GfxPlatformWindowHandle handle = { 0 };

#ifdef _WIN32
    handle.hwnd = glfwGetWin32Window(window);
    handle.hinstance = GetModuleHandle(NULL);

#elif defined(__linux__)
    // Force using Xlib instead of XCB to avoid driver hang
    handle.windowingSystem = GFX_WINDOWING_SYSTEM_X11;
    handle.x11.display = glfwGetX11Display();
    handle.x11.window = (void*)(uintptr_t)glfwGetX11Window(window);

#elif defined(__APPLE__)
    handle.nsWindow = glfwGetCocoaWindow(window);
    // Metal layer will be created automatically by the graphics API
    handle.metalLayer = NULL;
#endif

    return handle;
}

bool initializeGraphics(CubeApp* app)
{
    // Load the graphics backend BEFORE creating an instance
    // This is now decoupled - you load the backend API once at startup
    printf("Loading graphics backend...\n");
    if (!gfxLoadBackend(GFX_BACKEND_AUTO)) {
        fprintf(stderr, "Failed to load any graphics backend\n");
        return false;
    }
    printf("Graphics backend loaded successfully!\n");

    // Get required Vulkan extensions from GLFW
    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    printf("[DEBUG] GLFW requires %u extensions:\n", glfwExtensionCount);
    for (uint32_t i = 0; i < glfwExtensionCount; ++i) {
        printf("[DEBUG]   - %s\n", glfwExtensions[i]);
    }

    // Create graphics instance
    GfxInstanceDescriptor instanceDesc = {
        .backend = GFX_BACKEND_AUTO,
        .enableValidation = true,
        .enabledHeadless = false,
        .applicationName = "Cube Example (C)",
        .applicationVersion = 1,
        .requiredExtensions = glfwExtensions,
        .requiredExtensionCount = glfwExtensionCount
    };

    if (gfxCreateInstance(&instanceDesc, &app->instance) != GFX_RESULT_SUCCESS) {
        fprintf(stderr, "Failed to create graphics instance\n");
        return false;
    }

    // Set debug callback after instance creation
    gfxInstanceSetDebugCallback(app->instance, debugCallback, NULL);

    // Get adapter
    GfxAdapterDescriptor adapterDesc = {
        .powerPreference = GFX_POWER_PREFERENCE_HIGH_PERFORMANCE,
        .forceFallbackAdapter = false
    };

    if (gfxInstanceRequestAdapter(app->instance, &adapterDesc, &app->adapter) != GFX_RESULT_SUCCESS) {
        fprintf(stderr, "Failed to get graphics adapter\n");
        return false;
    }

    printf("Using adapter: %s\n", gfxAdapterGetName(app->adapter));
    printf("Backend: %s\n", gfxAdapterGetBackend(app->adapter) == GFX_BACKEND_VULKAN ? "Vulkan" : "WebGPU");

    // Create device
    GfxDeviceDescriptor deviceDesc = {
        .label = "Main Device",
        .requiredFeatures = NULL,
        .requiredFeatureCount = 0
    };

    if (gfxAdapterCreateDevice(app->adapter, &deviceDesc, &app->device) != GFX_RESULT_SUCCESS) {
        fprintf(stderr, "Failed to create device\n");
        return false;
    }

    // Query and print device limits
    GfxDeviceLimits limits;
    gfxDeviceGetLimits(app->device, &limits);
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

    app->queue = gfxDeviceGetQueue(app->device);

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

bool createSizeDependentResources(CubeApp* app, uint32_t width, uint32_t height)
{
    // Create swapchain
    GfxSwapchainDescriptor swapchainDesc = {
        .label = "Main Swapchain",
        .width = (uint32_t)width,
        .height = (uint32_t)height,
        .format = COLOR_FORMAT,
        .usage = GFX_TEXTURE_USAGE_RENDER_ATTACHMENT,
        .presentMode = GFX_PRESENT_MODE_FIFO,
        .bufferCount = MAX_FRAMES_IN_FLIGHT
    };

    if (gfxDeviceCreateSwapchain(app->device, app->surface, &swapchainDesc, &app->swapchain) != GFX_RESULT_SUCCESS) {
        fprintf(stderr, "Failed to create swapchain\n");
        return false;
    }

    // Create depth texture (MSAA must match color attachment)
    GfxTextureDescriptor depthTextureDesc = {
        .label = "Depth Buffer",
        .type = GFX_TEXTURE_TYPE_2D,
        .size = {
            .width = (uint32_t)width,
            .height = (uint32_t)height,
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
            .width = (uint32_t)width,
            .height = (uint32_t)height,
            .depth = 1 },
        .arrayLayerCount = 1,
        .mipLevelCount = 1,
        .sampleCount = MSAA_SAMPLE_COUNT,
        .format = COLOR_FORMAT,
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
        .format = COLOR_FORMAT,
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

        // Create command encoder for this frame
        snprintf(label, sizeof(label), "Command Encoder %d", i);
        if (gfxDeviceCreateCommandEncoder(app->device, label, &app->commandEncoders[i]) != GFX_RESULT_SUCCESS) {
            fprintf(stderr, "Failed to create command encoder %d\n", i);
            return false;
        }
    }

    app->currentFrame = 0;

    return true;
}

void cleanupSizeDependentResources(CubeApp* app)
{
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
        if (app->uniformBindGroups[i]) {
            gfxBindGroupDestroy(app->uniformBindGroups[i]);
            app->uniformBindGroups[i] = NULL;
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

bool createRenderingResources(CubeApp* app)
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
        .usage = GFX_BUFFER_USAGE_VERTEX | GFX_BUFFER_USAGE_COPY_DST,
        .mappedAtCreation = false
    };

    if (gfxDeviceCreateBuffer(app->device, &vertexBufferDesc, &app->vertexBuffer) != GFX_RESULT_SUCCESS) {
        fprintf(stderr, "Failed to create vertex buffer\n");
        return false;
    }

    // Create index buffer
    GfxBufferDescriptor indexBufferDesc = {
        .label = "Cube Indices",
        .size = sizeof(indices),
        .usage = GFX_BUFFER_USAGE_INDEX | GFX_BUFFER_USAGE_COPY_DST,
        .mappedAtCreation = false
    };

    if (gfxDeviceCreateBuffer(app->device, &indexBufferDesc, &app->indexBuffer) != GFX_RESULT_SUCCESS) {
        fprintf(stderr, "Failed to create index buffer\n");
        return false;
    }

    // Upload vertex and index data
    gfxQueueWriteBuffer(app->queue, app->vertexBuffer, 0, vertices, sizeof(vertices));
    gfxQueueWriteBuffer(app->queue, app->indexBuffer, 0, indices, sizeof(indices));

    // Create single large uniform buffer for all frames with proper alignment
    GfxDeviceLimits limits;
    gfxDeviceGetLimits(app->device, &limits);

    size_t uniformSize = sizeof(UniformData);
    app->uniformAlignedSize = gfxAlignUp(uniformSize, limits.minUniformBufferOffsetAlignment);
    size_t totalBufferSize = app->uniformAlignedSize * MAX_FRAMES_IN_FLIGHT;

    GfxBufferDescriptor uniformBufferDesc = {
        .label = "Shared Transform Uniforms",
        .size = totalBufferSize,
        .usage = GFX_BUFFER_USAGE_UNIFORM | GFX_BUFFER_USAGE_COPY_DST,
        .mappedAtCreation = false
    };

    if (gfxDeviceCreateBuffer(app->device, &uniformBufferDesc, &app->sharedUniformBuffer) != GFX_RESULT_SUCCESS) {
        fprintf(stderr, "Failed to create shared uniform buffer\n");
        return false;
    }

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

    // Create bind groups (one per frame in flight) using offsets into shared buffer
    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        char label[64];
        snprintf(label, sizeof(label), "Uniform Bind Group Frame %d", i);

        GfxBindGroupEntry uniformEntry = {
            .binding = 0,
            .type = GFX_BIND_GROUP_ENTRY_TYPE_BUFFER,
            .resource = {
                .buffer = {
                    .buffer = app->sharedUniformBuffer,
                    .offset = i * app->uniformAlignedSize, // Aligned offset into shared buffer
                    .size = sizeof(UniformData) } }
        };

        GfxBindGroupDescriptor uniformBindGroupDesc = {
            .label = label,
            .layout = app->uniformBindGroupLayout,
            .entries = &uniformEntry,
            .entryCount = 1
        };

        if (gfxDeviceCreateBindGroup(app->device, &uniformBindGroupDesc, &app->uniformBindGroups[i]) != GFX_RESULT_SUCCESS) {
            fprintf(stderr, "Failed to create uniform bind group %d\n", i);
            return false;
        }
    }

    // Load SPIR-V shaders
    size_t vertexShaderSize, fragmentShaderSize;
    void* vertexShaderCode = loadBinaryFile("cube.vert.spv", &vertexShaderSize);
    void* fragmentShaderCode = loadBinaryFile("cube.frag.spv", &fragmentShaderSize);

    if (!vertexShaderCode || !fragmentShaderCode) {
        fprintf(stderr, "Failed to load SPIR-V shaders\n");
        return false;
    }

    // Create vertex shader
    GfxShaderDescriptor vertexShaderDesc = {
        .label = "Cube Vertex Shader",
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

    // Initialize animation state
    app->rotationAngleX = 0.0f;
    app->rotationAngleY = 0.0f;
    app->lastTime = glfwGetTime();

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
    GfxColorTargetState colorTarget = {
        .format = COLOR_FORMAT,
        .blend = NULL,
        .writeMask = 0xF // Write all channels
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
        .stripIndexFormat = NULL,
        .frontFace = GFX_FRONT_FACE_CLOCKWISE,
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
        .bindGroupLayouts = bindGroupLayouts,
        .bindGroupLayoutCount = 1
    };

    if (gfxDeviceCreateRenderPipeline(app->device, &pipelineDesc, &app->renderPipeline) != GFX_RESULT_SUCCESS) {
        fprintf(stderr, "Failed to create render pipeline\n");
        return false;
    }

    return true;
}

void updateUniforms(CubeApp* app)
{
    double currentTime = glfwGetTime();
    float deltaTime = (float)(currentTime - app->lastTime);
    app->lastTime = currentTime;

    // Update rotation angles (both X and Y axes)
    app->rotationAngleX += 45.0f * deltaTime; // 45 degrees per second around X
    app->rotationAngleY += 30.0f * deltaTime; // 30 degrees per second around Y
    if (app->rotationAngleX >= 360.0f) {
        app->rotationAngleX -= 360.0f;
    }
    if (app->rotationAngleY >= 360.0f) {
        app->rotationAngleY -= 360.0f;
    }

    UniformData uniforms = { 0 }; // Initialize to zero!

    // Create rotation matrices (combine X and Y rotations)
    float rotX[16], rotY[16], tempModel[16];
    matrixRotateX(rotX, app->rotationAngleX * M_PI / 180.0f);
    matrixRotateY(rotY, app->rotationAngleY * M_PI / 180.0f);
    matrixMultiply(tempModel, rotY, rotX);
    memcpy(uniforms.model, tempModel, sizeof(float) * 16);

    // Create view matrix (camera positioned at 0, 0, 5 looking at origin)
    matrixLookAt(uniforms.view,
        0.0f, 0.0f, 5.0f, // eye position (match C++)
        0.0f, 0.0f, 0.0f, // look at point
        0.0f, 1.0f, 0.0f); // up vector

    // Create perspective projection matrix
    float aspect = (float)gfxSwapchainGetWidth(app->swapchain) / (float)gfxSwapchainGetHeight(app->swapchain);
    matrixPerspective(uniforms.projection,
        45.0f * M_PI / 180.0f, // 45 degree FOV
        aspect,
        0.1f, // near plane
        100.0f); // far plane

    // Upload uniform data to current frame's offset in shared buffer
    size_t offset = app->currentFrame * app->uniformAlignedSize;
    gfxQueueWriteBuffer(app->queue, app->sharedUniformBuffer, offset, &uniforms, sizeof(uniforms));
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

    // Update animation for this frame
    updateUniforms(app);

    // Get texture view for acquired image (for later blit/present)
    GfxTextureView backbuffer = gfxSwapchainGetImageView(app->swapchain, imageIndex);
    if (!backbuffer) {
        fprintf(stderr, "Failed to get swapchain texture view\n");
        return;
    }

    // Reset command encoder for reuse
    GfxCommandEncoder encoder = app->commandEncoders[app->currentFrame];
    gfxCommandEncoderReset(encoder);

    // Begin render pass with dark blue clear color
    // Pass both MSAA color buffer and swapchain image for resolve
    uint32_t colorAttachmentCount;
    GfxTextureView colorAttachments[2];
    GfxTextureLayout finalLayouts[2];
    if (MSAA_SAMPLE_COUNT == GFX_SAMPLE_COUNT_1) {
        colorAttachments[0] = backbuffer;
        finalLayouts[0] = GFX_TEXTURE_LAYOUT_PRESENT_SRC;
        colorAttachmentCount = 1;
    } else {
        colorAttachments[0] = app->msaaColorTextureView;
        colorAttachments[1] = backbuffer;
        finalLayouts[0] = GFX_TEXTURE_LAYOUT_COLOR_ATTACHMENT;
        finalLayouts[1] = GFX_TEXTURE_LAYOUT_PRESENT_SRC;
        colorAttachmentCount = 2;
    }

    GfxColor clearColor = { 0.1f, 0.2f, 0.3f, 1.0f };
    GfxRenderPassEncoder renderPass;
    if (gfxCommandEncoderBeginRenderPass(
            encoder,
            colorAttachments,
            colorAttachmentCount,
            &clearColor,
            finalLayouts,
            app->depthTextureView,
            1.0f,
            0, GFX_TEXTURE_LAYOUT_DEPTH_STENCIL_ATTACHMENT, &renderPass)
        == GFX_RESULT_SUCCESS) {

        // Set pipeline
        gfxRenderPassEncoderSetPipeline(renderPass, app->renderPipeline);

        // Set viewport and scissor to fill the entire render target
        uint32_t swapWidth = gfxSwapchainGetWidth(app->swapchain);
        uint32_t swapHeight = gfxSwapchainGetHeight(app->swapchain);
        GfxViewport viewport = { 0.0f, 0.0f, (float)swapWidth, (float)swapHeight, 0.0f, 1.0f };
        GfxScissorRect scissor = { 0, 0, swapWidth, swapHeight };
        gfxRenderPassEncoderSetViewport(renderPass, &viewport);
        gfxRenderPassEncoderSetScissorRect(renderPass, &scissor);

        // Set bind group for current frame (no dynamic offsets)
        gfxRenderPassEncoderSetBindGroup(renderPass, 0, app->uniformBindGroups[app->currentFrame], NULL, 0);

        // Set vertex buffer
        gfxRenderPassEncoderSetVertexBuffer(renderPass, 0, app->vertexBuffer, 0,
            gfxBufferGetSize(app->vertexBuffer));

        // Set index buffer
        gfxRenderPassEncoderSetIndexBuffer(renderPass, app->indexBuffer,
            GFX_INDEX_FORMAT_UINT16, 0,
            gfxBufferGetSize(app->indexBuffer));

        // Draw indexed (36 indices for the cube)
        gfxRenderPassEncoderDrawIndexed(renderPass, 36, 1, 0, 0, 0);

        // End render pass
        gfxRenderPassEncoderEnd(renderPass);
        gfxRenderPassEncoderDestroy(renderPass);
    }

    // Finish command encoding
    gfxCommandEncoderFinish(encoder);

    // Submit commands with synchronization
    GfxSubmitInfo submitInfo = { 0 };
    submitInfo.commandEncoders = &encoder;
    submitInfo.commandEncoderCount = 1;
    submitInfo.waitSemaphores = &app->imageAvailableSemaphores[app->currentFrame];
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.signalSemaphores = &app->renderFinishedSemaphores[app->currentFrame];
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.signalFence = app->inFlightFences[app->currentFrame];

    gfxQueueSubmitWithSync(app->queue, &submitInfo);

    // Present with synchronization
    GfxPresentInfo presentInfo = { 0 };
    presentInfo.waitSemaphores = &app->renderFinishedSemaphores[app->currentFrame];
    presentInfo.waitSemaphoreCount = 1;
    gfxSwapchainPresentWithSync(app->swapchain, &presentInfo);

    // Move to next frame
    app->currentFrame = (app->currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void cleanup(CubeApp* app)
{
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
        if (app->renderFinishedSemaphores[i]) {
            gfxSemaphoreDestroy(app->renderFinishedSemaphores[i]);
        }
        if (app->inFlightFences[i]) {
            gfxFenceDestroy(app->inFlightFences[i]);
        }

        // Destroy per-frame resources
        if (app->commandEncoders[i]) {
            gfxCommandEncoderDestroy(app->commandEncoders[i]);
        }
        if (app->uniformBindGroups[i]) {
            gfxBindGroupDestroy(app->uniformBindGroups[i]);
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
    gfxUnloadBackend(GFX_BACKEND_AUTO);

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

void matrixPerspective(float* matrix, float fov, float aspect, float near, float far)
{
    memset(matrix, 0, 16 * sizeof(float));

    float f = 1.0f / tanf(fov / 2.0f);

    matrix[0] = f / aspect;
    matrix[5] = f;
    matrix[10] = (far + near) / (near - far);
    matrix[11] = -1.0f;
    matrix[14] = (2.0f * far * near) / (near - far);
}

void matrixLookAt(float* matrix, float eyeX, float eyeY, float eyeZ,
    float centerX, float centerY, float centerZ,
    float upX, float upY, float upZ)
{
    const float epsilon = 1e-6f;

    // Calculate forward vector
    float fx = centerX - eyeX;
    float fy = centerY - eyeY;
    float fz = centerZ - eyeZ;

    // Normalize forward
    float flen = sqrtf(fx * fx + fy * fy + fz * fz);

    // Check for zero-length forward vector
    if (flen < epsilon) {
        matrixIdentity(matrix);
        return;
    }

    fx /= flen;
    fy /= flen;
    fz /= flen;

    // Calculate right vector (forward cross up)
    float rx = fy * upZ - fz * upY;
    float ry = fz * upX - fx * upZ;
    float rz = fx * upY - fy * upX;

    // Normalize right
    float rlen = sqrtf(rx * rx + ry * ry + rz * rz);

    // Check for zero-length right vector (forward and up are parallel)
    if (rlen < epsilon) {
        matrixIdentity(matrix);
        return;
    }

    rx /= rlen;
    ry /= rlen;
    rz /= rlen;

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

int main()
{
    printf("=== Cube Example with Unified Graphics API (C) ===\n\n");

    CubeApp app = { 0 }; // Initialize all members to NULL/0
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

    if (!createSyncObjects(&app)) {
        cleanup(&app);
        return -1;
    }

    if (!createRenderingResources(&app)) {
        cleanup(&app);
        return -1;
    }

    if (!createRenderPipeline(&app)) {
        cleanup(&app);
        return -1;
    }

    printf("Application initialized successfully!\n");
    printf("Press ESC to exit\n\n");

    uint32_t previousWidth = gfxSwapchainGetWidth(app.swapchain);
    uint32_t previousHeight = gfxSwapchainGetHeight(app.swapchain);

    // Main loop
    while (!glfwWindowShouldClose(app.window)) {
        glfwPollEvents();

        // Handle framebuffer resize BEFORE rendering (prevent acquiring image during resize)
        if (previousWidth != app.windowWidth || previousHeight != app.windowHeight) {
            // Wait for all in-flight frames to complete
            gfxDeviceWaitIdle(app.device);

            // Recreate only size-dependent resources (including swapchain)
            cleanupSizeDependentResources(&app);
            if (!createSizeDependentResources(&app, app.windowWidth, app.windowHeight)) {
                fprintf(stderr, "Failed to recreate size-dependent resources after resize\n");
                break;
            }

            previousWidth = app.windowWidth;
            previousHeight = app.windowHeight;

            printf("Window resized: %dx%d\n", gfxSwapchainGetWidth(app.swapchain), gfxSwapchainGetHeight(app.swapchain));
            continue; // Skip rendering this frame
        }

        // Render frame (no resize check needed inside since we checked above)
        render(&app);
    }

    printf("\nCleaning up resources...\n");
    cleanup(&app);

    printf("Example completed successfully!\n");
    return 0;
}