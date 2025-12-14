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

// Window dimensions
#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600

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
    GfxBuffer uniformBuffer;
    GfxBindGroupLayout uniformBindGroupLayout;
    GfxBindGroup uniformBindGroup;
    GfxShader vertexShader;
    GfxShader fragmentShader;
    GfxRenderPipeline renderPipeline;

    // Depth buffer
    GfxTexture depthTexture;
    GfxTextureView depthTextureView;

    // Animation state
    float rotationAngleX;
    float rotationAngleY;
    double lastTime;

    bool shouldClose;
} CubeApp;

// Function declarations
static bool initWindow(CubeApp* app);
static bool initializeGraphics(CubeApp* app);
static bool createRenderingResources(CubeApp* app);
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
    if (app && app->swapchain) {
        gfxSwapchainResize(app->swapchain, (uint32_t)width, (uint32_t)height);
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
    handle.display = glfwGetX11Display();
    handle.window = (void*)(uintptr_t)glfwGetX11Window(window);
    handle.xcb_connection = NULL;
    handle.xcb_window = 0;
    handle.isWayland = false;

    printf("[DEBUG] Using X11/Xlib for window surface\n");
    printf("[DEBUG] Display: %p\n", handle.display);
    printf("[DEBUG] Window: %lu\n", (unsigned long)(uintptr_t)handle.window);

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
    for (uint32_t i = 0; i < glfwExtensionCount; i++) {
        printf("[DEBUG]   - %s\n", glfwExtensions[i]);
    }

    // Create graphics instance
    GfxInstanceDescriptor instanceDesc = {
        .backend = GFX_BACKEND_AUTO,
        .enableValidation = true,
        .applicationName = "Cube Example (C)",
        .applicationVersion = 1,
        .requiredExtensions = glfwExtensions,
        .requiredExtensionCount = glfwExtensionCount
    };

    if (gfxCreateInstance(&instanceDesc, &app->instance) != GFX_RESULT_SUCCESS) {
        fprintf(stderr, "Failed to create graphics instance\n");
        return false;
    }

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

    app->queue = gfxDeviceGetQueue(app->device);

    // Create surface
    GfxPlatformWindowHandle windowHandle = getPlatformWindowHandle(app->window);

    GfxSurfaceDescriptor surfaceDesc = {
        .label = "Main Surface",
        .windowHandle = windowHandle,
        .width = WINDOW_WIDTH,
        .height = WINDOW_HEIGHT
    };

    if (gfxDeviceCreateSurface(app->device, &surfaceDesc, &app->surface) != GFX_RESULT_SUCCESS) {
        fprintf(stderr, "Failed to create surface\n");
        return false;
    }

    // Create swapchain
    GfxSwapchainDescriptor swapchainDesc = {
        .label = "Main Swapchain",
        .width = WINDOW_WIDTH,
        .height = WINDOW_HEIGHT,
        .format = GFX_TEXTURE_FORMAT_B8G8R8A8_UNORM,
        .usage = GFX_TEXTURE_USAGE_RENDER_ATTACHMENT,
        .presentMode = GFX_PRESENT_MODE_FIFO,
        .bufferCount = 2
    };

    if (gfxDeviceCreateSwapchain(app->device, app->surface, &swapchainDesc, &app->swapchain) != GFX_RESULT_SUCCESS) {
        fprintf(stderr, "Failed to create swapchain\n");
        return false;
    }

    // Create depth texture
    GfxTextureDescriptor depthTextureDesc = {
        .label = "Depth Buffer",
        .size = {
            .width = WINDOW_WIDTH,
            .height = WINDOW_HEIGHT,
            .depth = 1 },
        .mipLevelCount = 1,
        .sampleCount = 1,
        .format = GFX_TEXTURE_FORMAT_DEPTH32_FLOAT,
        .usage = GFX_TEXTURE_USAGE_RENDER_ATTACHMENT
    };

    if (gfxDeviceCreateTexture(app->device, &depthTextureDesc, &app->depthTexture) != GFX_RESULT_SUCCESS) {
        fprintf(stderr, "Failed to create depth texture\n");
        return false;
    }

    // Create depth texture view
    GfxTextureViewDescriptor depthViewDesc = {
        .label = "Depth Buffer View",
        .format = GFX_TEXTURE_FORMAT_DEPTH32_FLOAT,
        .baseMipLevel = 0,
        .mipLevelCount = 1,
        .baseArrayLayer = 0,
        .arrayLayerCount = 1
    };

    if (gfxTextureCreateView(app->depthTexture, &depthViewDesc, &app->depthTextureView) != GFX_RESULT_SUCCESS) {
        fprintf(stderr, "Failed to create depth texture view\n");
        return false;
    }

    return true;
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

    // Create uniform buffer
    GfxBufferDescriptor uniformBufferDesc = {
        .label = "Transform Uniforms",
        .size = sizeof(UniformData),
        .usage = GFX_BUFFER_USAGE_UNIFORM | GFX_BUFFER_USAGE_COPY_DST,
        .mappedAtCreation = false
    };

    if (gfxDeviceCreateBuffer(app->device, &uniformBufferDesc, &app->uniformBuffer) != GFX_RESULT_SUCCESS) {
        fprintf(stderr, "Failed to create uniform buffer\n");
        return false;
    }

    // Upload vertex and index data
    gfxQueueWriteBuffer(app->queue, app->vertexBuffer, 0, vertices, sizeof(vertices));
    gfxQueueWriteBuffer(app->queue, app->indexBuffer, 0, indices, sizeof(indices));

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

    // Create bind group
    GfxBindGroupEntry uniformEntry = {
        .binding = 0,
        .type = GFX_BIND_GROUP_ENTRY_TYPE_BUFFER,
        .resource = {
            .buffer = {
                .buffer = app->uniformBuffer,
                .offset = 0,
                .size = sizeof(UniformData) } }
    };

    GfxBindGroupDescriptor uniformBindGroupDesc = {
        .label = "Uniform Bind Group",
        .layout = app->uniformBindGroupLayout,
        .entries = &uniformEntry,
        .entryCount = 1
    };

    if (gfxDeviceCreateBindGroup(app->device, &uniformBindGroupDesc, &app->uniformBindGroup) != GFX_RESULT_SUCCESS) {
        fprintf(stderr, "Failed to create uniform bind group\n");
        return false;
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
    GfxColorTargetState colorTarget = {
        .format = GFX_TEXTURE_FORMAT_B8G8R8A8_UNORM,
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
        .frontFaceCounterClockwise = false, // Clockwise = front-facing
        .cullBackFace = true, // Enable back-face culling
        .unclippedDepth = false
    };

    // Depth/stencil state - enable depth testing
    GfxDepthStencilState depthStencilState = {
        .format = GFX_TEXTURE_FORMAT_DEPTH32_FLOAT,
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
        .sampleCount = 1,
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
    float aspect = (float)WINDOW_WIDTH / (float)WINDOW_HEIGHT;
    matrixPerspective(uniforms.projection,
        45.0f * M_PI / 180.0f, // 45 degree FOV
        aspect,
        0.1f, // near plane
        100.0f); // far plane

    // Upload uniform data
    gfxQueueWriteBuffer(app->queue, app->uniformBuffer, 0, &uniforms, sizeof(uniforms));
}

void render(CubeApp* app)
{
    // Get current swapchain texture
    GfxTextureView backbuffer = gfxSwapchainGetCurrentTextureView(app->swapchain);
    if (!backbuffer) {
        fprintf(stderr, "Failed to get swapchain texture view\n");
        return;
    }

    // Create command encoder
    GfxCommandEncoder encoder;
    if (gfxDeviceCreateCommandEncoder(app->device, "Frame Commands", &encoder) != GFX_RESULT_SUCCESS) {
        fprintf(stderr, "Failed to create command encoder\n");
        return;
    }

    // Begin render pass with dark blue clear color
    GfxColor clearColor = { 0.1f, 0.2f, 0.3f, 1.0f };
    GfxRenderPassEncoder renderPass;
    if (gfxCommandEncoderBeginRenderPass(
        encoder,
        &backbuffer,
        1,
        &clearColor,
        app->depthTextureView,
        1.0f,
        0,
        &renderPass) == GFX_RESULT_SUCCESS) {
        
        // Set pipeline
        gfxRenderPassEncoderSetPipeline(renderPass, app->renderPipeline);

        // Set bind group
        gfxRenderPassEncoderSetBindGroup(renderPass, 0, app->uniformBindGroup);

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

    // Submit commands
    gfxQueueSubmit(app->queue, encoder);

    // Present
    gfxSwapchainPresent(app->swapchain);

    // Cleanup
    gfxCommandEncoderDestroy(encoder);
}

void cleanup(CubeApp* app)
{
    // Wait for device to finish
    if (app->device) {
        gfxDeviceWaitIdle(app->device);
    }

    // Destroy graphics resources
    if (app->renderPipeline)
        gfxRenderPipelineDestroy(app->renderPipeline);
    if (app->fragmentShader)
        gfxShaderDestroy(app->fragmentShader);
    if (app->vertexShader)
        gfxShaderDestroy(app->vertexShader);
    if (app->uniformBindGroup)
        gfxBindGroupDestroy(app->uniformBindGroup);
    if (app->uniformBindGroupLayout)
        gfxBindGroupLayoutDestroy(app->uniformBindGroupLayout);
    if (app->uniformBuffer)
        gfxBufferDestroy(app->uniformBuffer);
    if (app->indexBuffer)
        gfxBufferDestroy(app->indexBuffer);
    if (app->vertexBuffer)
        gfxBufferDestroy(app->vertexBuffer);
    if (app->depthTextureView)
        gfxTextureViewDestroy(app->depthTextureView);
    if (app->depthTexture)
        gfxTextureDestroy(app->depthTexture);
    if (app->swapchain)
        gfxSwapchainDestroy(app->swapchain);
    if (app->surface)
        gfxSurfaceDestroy(app->surface);
    if (app->device)
        gfxDeviceDestroy(app->device);
    if (app->adapter)
        gfxAdapterDestroy(app->adapter);
    if (app->instance)
        gfxInstanceDestroy(app->instance);

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
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
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

    if (!initWindow(&app)) {
        cleanup(&app);
        return -1;
    }

    if (!initializeGraphics(&app)) {
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

    // Main loop
    while (!glfwWindowShouldClose(app.window)) {
        glfwPollEvents();

        // Update uniforms
        updateUniforms(&app);

        // Render frame
        render(&app);
    }

    printf("\nCleaning up resources...\n");
    cleanup(&app);

    printf("Example completed successfully!\n");
    return 0;
}