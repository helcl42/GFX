#include <gfx/gfx.h>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#if defined(__EMSCRIPTEN__)
#include <emscripten/emscripten.h>
#include <emscripten/html5.h>
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
#define CUBE_COUNT 3
#define MSAA_SAMPLE_COUNT GFX_SAMPLE_COUNT_4
#define COLOR_FORMAT GFX_TEXTURE_FORMAT_B8G8R8A8_UNORM_SRGB
#define DEPTH_FORMAT GFX_TEXTURE_FORMAT_DEPTH32_FLOAT

// Vertex data structure
typedef struct {
    float position[3];
    float color[3];
} Vertex;

// Uniform buffer data
typedef struct {
    float model[16];
    float view[16];
    float projection[16];
} UniformData;

// Application state
typedef struct {
    GLFWwindow* window;
    
    GfxInstance instance;
    GfxAdapter adapter;
    GfxDevice device;
    GfxQueue queue;
    GfxSurface surface;
    GfxSwapchain swapchain;
    GfxTextureFormat swapchainFormat; // Actual format (may differ from requested)
    
    GfxBuffer vertexBuffer;
    GfxBuffer indexBuffer;
    GfxBuffer sharedUniformBuffer;
    size_t uniformAlignedSize;
    
    GfxTexture depthTexture;
    GfxTextureView depthTextureView;
    
    GfxTexture msaaColorTexture;
    GfxTextureView msaaColorTextureView;
    
    uint32_t windowWidth;
    uint32_t windowHeight;
    
    GfxShader vertexShader;
    GfxShader fragmentShader;
    
    GfxBindGroupLayout bindGroupLayout;
    GfxBindGroup bindGroups[MAX_FRAMES_IN_FLIGHT][CUBE_COUNT];
    GfxRenderPipeline pipeline;
    
    GfxCommandEncoder commandEncoders[MAX_FRAMES_IN_FLIGHT];
    
    // Per-frame synchronization (WebGPU backend provides no-op implementations)
    GfxSemaphore imageAvailableSemaphores[MAX_FRAMES_IN_FLIGHT];
    GfxSemaphore renderFinishedSemaphores[MAX_FRAMES_IN_FLIGHT];
    GfxFence inFlightFences[MAX_FRAMES_IN_FLIGHT];
    
    uint32_t currentFrame;
    float rotationAngleX;
    float rotationAngleY;
    bool running;
} CubeApp;

// Cube vertices (position + color)
static const Vertex vertices[] = {
    // Front face (red)
    {{-0.5f, -0.5f,  0.5f}, {1.0f, 0.0f, 0.0f}},
    {{ 0.5f, -0.5f,  0.5f}, {1.0f, 0.0f, 0.0f}},
    {{ 0.5f,  0.5f,  0.5f}, {1.0f, 0.0f, 0.0f}},
    {{-0.5f,  0.5f,  0.5f}, {1.0f, 0.0f, 0.0f}},
    
    // Back face (green)
    {{ 0.5f, -0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},
    {{-0.5f, -0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},
    {{-0.5f,  0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},
    {{ 0.5f,  0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},
    
    // Top face (blue)
    {{-0.5f,  0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}},
    {{ 0.5f,  0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}},
    {{ 0.5f,  0.5f, -0.5f}, {0.0f, 0.0f, 1.0f}},
    {{-0.5f,  0.5f, -0.5f}, {0.0f, 0.0f, 1.0f}},
    
    // Bottom face (yellow)
    {{-0.5f, -0.5f, -0.5f}, {1.0f, 1.0f, 0.0f}},
    {{ 0.5f, -0.5f, -0.5f}, {1.0f, 1.0f, 0.0f}},
    {{ 0.5f, -0.5f,  0.5f}, {1.0f, 1.0f, 0.0f}},
    {{-0.5f, -0.5f,  0.5f}, {1.0f, 1.0f, 0.0f}},
    
    // Right face (magenta)
    {{ 0.5f, -0.5f,  0.5f}, {1.0f, 0.0f, 1.0f}},
    {{ 0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 1.0f}},
    {{ 0.5f,  0.5f, -0.5f}, {1.0f, 0.0f, 1.0f}},
    {{ 0.5f,  0.5f,  0.5f}, {1.0f, 0.0f, 1.0f}},
    
    // Left face (cyan)
    {{-0.5f, -0.5f, -0.5f}, {0.0f, 1.0f, 1.0f}},
    {{-0.5f, -0.5f,  0.5f}, {0.0f, 1.0f, 1.0f}},
    {{-0.5f,  0.5f,  0.5f}, {0.0f, 1.0f, 1.0f}},
    {{-0.5f,  0.5f, -0.5f}, {0.0f, 1.0f, 1.0f}},
};

// Cube indices
static const uint16_t indices[] = {
    0,  1,  2,  0,  2,  3,   // Front
    4,  5,  6,  4,  6,  7,   // Back
    8,  9,  10, 8,  10, 11,  // Top
    12, 13, 14, 12, 14, 15,  // Bottom
    16, 17, 18, 16, 18, 19,  // Right
    20, 21, 22, 20, 22, 23,  // Left
};

// Matrix math helpers (from native example)
static void matrixIdentity(float* matrix) {
    memset(matrix, 0, 16 * sizeof(float));
    matrix[0] = matrix[5] = matrix[10] = matrix[15] = 1.0f;
}

static void matrixMultiply(float* result, const float* a, const float* b) {
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

static void matrixRotateX(float* matrix, float angle) {
    float c = cosf(angle);
    float s = sinf(angle);

    matrixIdentity(matrix);
    matrix[5] = c;
    matrix[6] = -s;
    matrix[9] = s;
    matrix[10] = c;
}

static void matrixRotateY(float* matrix, float angle) {
    float c = cosf(angle);
    float s = sinf(angle);

    matrixIdentity(matrix);
    matrix[0] = c;
    matrix[2] = s;
    matrix[8] = -s;
    matrix[10] = c;
}

static void matrixPerspective(float* matrix, float fov, float aspect, float near, float far, GfxBackend backend) {
    memset(matrix, 0, 16 * sizeof(float));

    float f = 1.0f / tanf(fov / 2.0f);

    matrix[0] = f / aspect;
    if (backend == GFX_BACKEND_WEBGPU) {
        matrix[5] = f;
    } else {
        matrix[5] = -f; // Invert Y for Vulkan
    }
    matrix[10] = (far + near) / (near - far);
    matrix[11] = -1.0f;
    matrix[14] = (2.0f * far * near) / (near - far);
}

static void matrixLookAt(float* matrix, float eyeX, float eyeY, float eyeZ,
    float centerX, float centerY, float centerZ,
    float upX, float upY, float upZ) {
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

// Shader code (WGSL)
static const char* vertexShaderSource = 
    "struct Uniforms {\n"
    "    model: mat4x4<f32>,\n"
    "    view: mat4x4<f32>,\n"
    "    projection: mat4x4<f32>,\n"
    "}\n"
    "@group(0) @binding(0) var<uniform> uniforms: Uniforms;\n"
    "\n"
    "struct VertexInput {\n"
    "    @location(0) position: vec3<f32>,\n"
    "    @location(1) color: vec3<f32>,\n"
    "}\n"
    "\n"
    "struct VertexOutput {\n"
    "    @builtin(position) position: vec4<f32>,\n"
    "    @location(0) color: vec3<f32>,\n"
    "}\n"
    "\n"
    "@vertex\n"
    "fn main(input: VertexInput) -> VertexOutput {\n"
    "    var output: VertexOutput;\n"
    "    let worldPos = uniforms.model * vec4<f32>(input.position, 1.0);\n"
    "    let viewPos = uniforms.view * worldPos;\n"
    "    output.position = uniforms.projection * viewPos;\n"
    "    output.color = input.color;\n"
    "    return output;\n"
    "}\n";

static const char* fragmentShaderSource =
    "struct FragmentInput {\n"
    "    @location(0) color: vec3<f32>,\n"
    "}\n"
    "\n"
    "@fragment\n"
    "fn main(input: FragmentInput) -> @location(0) vec4<f32> {\n"
    "    return vec4<f32>(input.color, 1.0);\n"
    "}\n";

// Function declarations
static void framebufferSizeCallback(GLFWwindow* window, int width, int height);
static bool initWindow(CubeApp* app);
static bool initializeGraphics(CubeApp* app);
static bool createSizeDependentResources(CubeApp* app, uint32_t width, uint32_t height);
static bool createSyncObjects(CubeApp* app);
static bool createRenderingResources(CubeApp* app);
static bool createRenderPipeline(CubeApp* app);
static void cleanupSizeDependentResources(CubeApp* app);
static void cleanupRenderingResources(CubeApp* app);

static void framebufferSizeCallback(GLFWwindow* window, int width, int height) {
    CubeApp* app = (CubeApp*)glfwGetWindowUserPointer(window);
    if (app) {
        app->windowWidth = (uint32_t)width;
        app->windowHeight = (uint32_t)height;
    }
}

static bool initWindow(CubeApp* app) {
    // Initialize GLFW
    if (!glfwInit()) {
        fprintf(stderr, "Failed to initialize GLFW\n");
        return false;
    }
    
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    
    app->window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "WebGPU Cube Example", NULL, NULL);
    if (!app->window) {
        fprintf(stderr, "Failed to create window\n");
        glfwTerminate();
        return false;
    }
    
    app->windowWidth = WINDOW_WIDTH;
    app->windowHeight = WINDOW_HEIGHT;
    
    glfwSetWindowUserPointer(app->window, app);
    glfwSetFramebufferSizeCallback(app->window, framebufferSizeCallback);
    
    return true;
}

static bool initializeGraphics(CubeApp* app) {
    printf("Initializing graphics...\n");
    
    // Load backend
    if (!gfxLoadBackend(GFX_BACKEND_WEBGPU)) {
        fprintf(stderr, "Failed to load WebGPU backend\n");
        return false;
    }
    
    // Create instance
    GfxInstanceDescriptor instanceDesc = {
        .backend = GFX_BACKEND_WEBGPU,
    };
    
    if (gfxCreateInstance(&instanceDesc, &app->instance) != GFX_RESULT_SUCCESS) {
        fprintf(stderr, "Failed to create instance\n");
        return false;
    }
    
    // Request adapter
    GfxAdapterDescriptor adapterDesc = {
        .powerPreference = GFX_POWER_PREFERENCE_HIGH_PERFORMANCE,
    };
    
    if (gfxInstanceRequestAdapter(app->instance, &adapterDesc, &app->adapter) != GFX_RESULT_SUCCESS) {
        fprintf(stderr, "Failed to request adapter\n");
        return false;
    }
    
    printf("Adapter: %s\n", gfxAdapterGetName(app->adapter));
    
    // Create device
    GfxDeviceDescriptor deviceDesc = {
        .label = "Main Device",
    };
    
    if (gfxAdapterCreateDevice(app->adapter, &deviceDesc, &app->device) != GFX_RESULT_SUCCESS) {
        fprintf(stderr, "Failed to create device\n");
        return false;
    }
    
    printf("Device created\n");
    
    app->queue = gfxDeviceGetQueue(app->device);
    
    printf("Queue obtained\n");
    
    // Create surface
#if defined(__EMSCRIPTEN__)
    // For Emscripten, use the canvas selector
    GfxPlatformWindowHandle windowHandle = gfxPlatformWindowHandleMakeEmscripten("#canvas");
#else
#ifdef _WIN32
    HWND hwnd = glfwGetWin32Window(app->window);
    GfxPlatformWindowHandle windowHandle = gfxPlatformWindowHandleMakeWin32(hwnd, GetModuleHandle(NULL));
#elif defined(__linux__)
    Display* display = glfwGetX11Display();
    Window window = glfwGetX11Window(app->window);
    GfxPlatformWindowHandle windowHandle = gfxPlatformWindowHandleMakeX11((void*)(uintptr_t)window, display);
#elif defined(__APPLE__)
    void* nsWindow = glfwGetCocoaWindow(app->window);
    GfxPlatformWindowHandle windowHandle = gfxPlatformWindowHandleMakeCocoa(nsWindow, NULL);
#endif
#endif
    
    GfxSurfaceDescriptor surfaceDesc = {
        .windowHandle = windowHandle,
    };
    
    if (gfxDeviceCreateSurface(app->device, &surfaceDesc, &app->surface) != GFX_RESULT_SUCCESS) {
        fprintf(stderr, "Failed to create surface\n");
        return false;
    }
    
    printf("Surface created\n");
    
    return true;
}

static bool createSizeDependentResources(CubeApp* app, uint32_t width, uint32_t height) {
    // Create swapchain
    GfxSwapchainDescriptor swapchainDesc = {
        .width = width,
        .height = height,
        .format = COLOR_FORMAT,
        .usage = GFX_TEXTURE_USAGE_RENDER_ATTACHMENT,
        .presentMode = GFX_PRESENT_MODE_FIFO,
        .bufferCount = 2,
    };
    
    if (gfxDeviceCreateSwapchain(app->device, app->surface, &swapchainDesc, &app->swapchain) != GFX_RESULT_SUCCESS) {
        fprintf(stderr, "Failed to create swapchain\n");
        return false;
    }
    
    printf("Swapchain created\n");
    
    // Query the actual swapchain format (may differ from requested format on web)
    app->swapchainFormat = gfxSwapchainGetFormat(app->swapchain);
    fprintf(stderr, "[INFO] Requested format: %d, Actual swapchain format: %d\n", COLOR_FORMAT, app->swapchainFormat);
    
    // Create depth texture
    GfxTextureDescriptor depthTextureDesc = {
        .label = "Depth Texture",
        .type = GFX_TEXTURE_TYPE_2D,
        .size = {width, height, 1},
        .format = DEPTH_FORMAT,
        .usage = GFX_TEXTURE_USAGE_RENDER_ATTACHMENT,
        .mipLevelCount = 1,
        .sampleCount = MSAA_SAMPLE_COUNT,
        .arrayLayerCount = 1,
    };
    
    if (gfxDeviceCreateTexture(app->device, &depthTextureDesc, &app->depthTexture) != GFX_RESULT_SUCCESS) {
        fprintf(stderr, "Failed to create depth texture\n");
        return false;
    }
    
    if (gfxTextureCreateView(app->depthTexture, NULL, &app->depthTextureView) != GFX_RESULT_SUCCESS) {
        fprintf(stderr, "Failed to create depth texture view\n");
        return false;
    }
    
    // Create MSAA color texture (is unused if MSAA_SAMPLE_COUNT == 1)
    GfxTextureDescriptor msaaColorTextureDesc = {
        .label = "MSAA Color Buffer",
        .type = GFX_TEXTURE_TYPE_2D,
        .size = {width, height, 1},
        .format = app->swapchainFormat,
        .usage = GFX_TEXTURE_USAGE_RENDER_ATTACHMENT,
        .mipLevelCount = 1,
        .sampleCount = MSAA_SAMPLE_COUNT,
        .arrayLayerCount = 1,
    };
    
    if (gfxDeviceCreateTexture(app->device, &msaaColorTextureDesc, &app->msaaColorTexture) != GFX_RESULT_SUCCESS) {
        fprintf(stderr, "Failed to create MSAA color texture\n");
        return false;
    }
    
    // Create MSAA color texture view (is unused if MSAA_SAMPLE_COUNT == 1)
    GfxTextureViewDescriptor msaaColorViewDesc = {
        .label = "MSAA Color Buffer View",
        .viewType = GFX_TEXTURE_VIEW_TYPE_2D,
        .format = app->swapchainFormat,
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

static bool createSyncObjects(CubeApp* app) {
    // Create synchronization objects (WebGPU backend provides no-op implementations)
    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        char label[64];
        
        snprintf(label, sizeof(label), "Image Available Semaphore %d", i);
        GfxSemaphoreDescriptor semaphoreDesc = { .label = label };
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
        GfxFenceDescriptor fenceDesc = { .label = label, .signaled = true };
        if (gfxDeviceCreateFence(app->device, &fenceDesc, &app->inFlightFences[i]) != GFX_RESULT_SUCCESS) {
            fprintf(stderr, "Failed to create in flight fence %d\n", i);
            return false;
        }
        
        // Create command encoder for this frame
        snprintf(label, sizeof(label), "Command Encoder Frame %d", i);
        GfxCommandEncoderDescriptor encoderDesc = { .label = label };
        if (gfxDeviceCreateCommandEncoder(app->device, &encoderDesc, &app->commandEncoders[i]) != GFX_RESULT_SUCCESS) {
            fprintf(stderr, "Failed to create command encoder %d\n", i);
            return false;
        }
    }
    
    app->currentFrame = 0;
    
    return true;
}

static bool createRenderingResources(CubeApp* app) {
    // Create vertex buffer
    GfxBufferDescriptor vertexBufferDesc = {
        .label = "Vertex Buffer",
        .size = sizeof(vertices),
        .usage = GFX_BUFFER_USAGE_VERTEX | GFX_BUFFER_USAGE_COPY_DST,
        .mappedAtCreation = false,
    };
    
    if (gfxDeviceCreateBuffer(app->device, &vertexBufferDesc, &app->vertexBuffer) != GFX_RESULT_SUCCESS) {
        fprintf(stderr, "Failed to create vertex buffer\n");
        return false;
    }
    
    gfxQueueWriteBuffer(app->queue, app->vertexBuffer, 0, vertices, sizeof(vertices));
    
    // Create index buffer
    GfxBufferDescriptor indexBufferDesc = {
        .label = "Index Buffer",
        .size = sizeof(indices),
        .usage = GFX_BUFFER_USAGE_INDEX | GFX_BUFFER_USAGE_COPY_DST,
        .mappedAtCreation = false,
    };
    
    if (gfxDeviceCreateBuffer(app->device, &indexBufferDesc, &app->indexBuffer) != GFX_RESULT_SUCCESS) {
        fprintf(stderr, "Failed to create index buffer\n");
        return false;
    }
    
    gfxQueueWriteBuffer(app->queue, app->indexBuffer, 0, indices, sizeof(indices));
    
    // Create shared uniform buffer with proper alignment for all frames
    GfxDeviceLimits limits;
    gfxDeviceGetLimits(app->device, &limits);
    
    size_t uniformSize = sizeof(UniformData);
    app->uniformAlignedSize = gfxAlignUp(uniformSize, limits.minUniformBufferOffsetAlignment);
    size_t totalBufferSize = app->uniformAlignedSize * MAX_FRAMES_IN_FLIGHT * CUBE_COUNT;
    
    GfxBufferDescriptor uniformBufferDesc = {
        .label = "Shared Uniform Buffer",
        .size = totalBufferSize,
        .usage = GFX_BUFFER_USAGE_UNIFORM | GFX_BUFFER_USAGE_COPY_DST,
        .mappedAtCreation = false,
    };
    
    if (gfxDeviceCreateBuffer(app->device, &uniformBufferDesc, &app->sharedUniformBuffer) != GFX_RESULT_SUCCESS) {
        fprintf(stderr, "Failed to create shared uniform buffer\n");
        return false;
    }
    
    // Create bind group layout
    GfxBindGroupLayoutEntry bindGroupLayoutEntry = {
        .binding = 0,
        .visibility = GFX_SHADER_STAGE_VERTEX,
        .type = GFX_BINDING_TYPE_BUFFER,
        .buffer = {
            .minBindingSize = sizeof(UniformData),
            .hasDynamicOffset = false,
        },
    };
    
    GfxBindGroupLayoutDescriptor bindGroupLayoutDesc = {
        .label = "Bind Group Layout",
        .entryCount = 1,
        .entries = &bindGroupLayoutEntry,
    };
    
    if (gfxDeviceCreateBindGroupLayout(app->device, &bindGroupLayoutDesc, &app->bindGroupLayout) != GFX_RESULT_SUCCESS) {
        fprintf(stderr, "Failed to create bind group layout\n");
        return false;
    }
    
    // Create per-frame bind groups for each cube
    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        for (int cubeIdx = 0; cubeIdx < CUBE_COUNT; ++cubeIdx) {
            char label[64];
            snprintf(label, sizeof(label), "Bind Group Frame %d Cube %d", i, cubeIdx);
            
            GfxBindGroupEntry bindGroupEntry = {
                .binding = 0,
                .type = GFX_BIND_GROUP_ENTRY_TYPE_BUFFER,
                .resource = {
                    .buffer = {
                        .buffer = app->sharedUniformBuffer,
                        .offset = (i * CUBE_COUNT + cubeIdx) * app->uniformAlignedSize,
                        .size = sizeof(UniformData),
                    },
                },
            };
            
            GfxBindGroupDescriptor bindGroupDesc = {
                .label = label,
                .layout = app->bindGroupLayout,
                .entryCount = 1,
                .entries = &bindGroupEntry,
            };
            
            if (gfxDeviceCreateBindGroup(app->device, &bindGroupDesc, &app->bindGroups[i][cubeIdx]) != GFX_RESULT_SUCCESS) {
                fprintf(stderr, "Failed to create bind group %d cube %d\n", i, cubeIdx);
                return false;
            }
        }
    }
    
    // Create shaders
    GfxShaderDescriptor vertexShaderDesc = {
        .label = "Vertex Shader",
        .sourceType = GFX_SHADER_SOURCE_WGSL,
        .code = vertexShaderSource,
        .codeSize = strlen(vertexShaderSource),
    };
    
    if (gfxDeviceCreateShader(app->device, &vertexShaderDesc, &app->vertexShader) != GFX_RESULT_SUCCESS) {
        fprintf(stderr, "Failed to create vertex shader\n");
        return false;
    }
    
    GfxShaderDescriptor fragmentShaderDesc = {
        .label = "Fragment Shader",
        .sourceType = GFX_SHADER_SOURCE_WGSL,
        .code = fragmentShaderSource,
        .codeSize = strlen(fragmentShaderSource),
    };
    
    if (gfxDeviceCreateShader(app->device, &fragmentShaderDesc, &app->fragmentShader) != GFX_RESULT_SUCCESS) {
        fprintf(stderr, "Failed to create fragment shader\n");
        return false;
    }
    
    return true;
}

static bool createRenderPipeline(CubeApp* app) {
    // Create render pipeline
    GfxVertexAttribute vertexAttributes[] = {
        {
            .format = GFX_TEXTURE_FORMAT_R32G32B32_FLOAT,
            .offset = offsetof(Vertex, position),
            .shaderLocation = 0,
        },
        {
            .format = GFX_TEXTURE_FORMAT_R32G32B32_FLOAT,
            .offset = offsetof(Vertex, color),
            .shaderLocation = 1,
        },
    };
    
    GfxVertexBufferLayout vertexBufferLayout = {
        .arrayStride = sizeof(Vertex),
        .stepModeInstance = false,
        .attributeCount = 2,
        .attributes = vertexAttributes,
    };
    
    GfxVertexState vertexState = {
        .module = app->vertexShader,
        .entryPoint = "main",
        .bufferCount = 1,
        .buffers = &vertexBufferLayout,
    };
    
    GfxColorTargetState colorTargetState = {
        .format = app->swapchainFormat, // Use actual swapchain format
        .blend = NULL,
        .writeMask = 0xF, // Write all channels
    };
    
    GfxFragmentState fragmentState = {
        .module = app->fragmentShader,
        .entryPoint = "main",
        .targetCount = 1,
        .targets = &colorTargetState,
    };
    
    GfxPrimitiveState primitiveState = {
        .topology = GFX_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        .stripIndexFormat = NULL,
        .frontFace = GFX_FRONT_FACE_COUNTER_CLOCKWISE,
        .cullMode = GFX_CULL_MODE_BACK,
    };
    
    GfxDepthStencilState depthStencilState = {
        .format = DEPTH_FORMAT,
        .depthWriteEnabled = true,
        .depthCompare = GFX_COMPARE_FUNCTION_LESS,
        .stencilReadMask = 0,
        .stencilWriteMask = 0,
    };
    
    GfxRenderPipelineDescriptor pipelineDesc = {
        .label = "Render Pipeline",
        .vertex = &vertexState,
        .primitive = &primitiveState,
        .depthStencil = &depthStencilState,
        .sampleCount = MSAA_SAMPLE_COUNT,
        .fragment = &fragmentState,
        .bindGroupLayoutCount = 1,
        .bindGroupLayouts = &app->bindGroupLayout,
    };
    
    if (gfxDeviceCreateRenderPipeline(app->device, &pipelineDesc, &app->pipeline) != GFX_RESULT_SUCCESS) {
        fprintf(stderr, "Failed to create render pipeline\n");
        return false;
    }
    
    printf("Graphics initialized successfully\n");
    return true;
}

static void cleanupSizeDependentResources(CubeApp* app) {
    if (app->msaaColorTextureView) gfxTextureViewDestroy(app->msaaColorTextureView);
    if (app->msaaColorTexture) gfxTextureDestroy(app->msaaColorTexture);
    if (app->depthTextureView) gfxTextureViewDestroy(app->depthTextureView);
    if (app->depthTexture) gfxTextureDestroy(app->depthTexture);
    if (app->swapchain) gfxSwapchainDestroy(app->swapchain);
}

static void cleanupRenderingResources(CubeApp* app) {
    if (app->pipeline) gfxRenderPipelineDestroy(app->pipeline);
    if (app->fragmentShader) gfxShaderDestroy(app->fragmentShader);
    if (app->vertexShader) gfxShaderDestroy(app->vertexShader);
    if (app->bindGroupLayout) gfxBindGroupLayoutDestroy(app->bindGroupLayout);
    
    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        for (int cubeIdx = 0; cubeIdx < CUBE_COUNT; ++cubeIdx) {
            if (app->bindGroups[i][cubeIdx]) gfxBindGroupDestroy(app->bindGroups[i][cubeIdx]);
        }
    }
    
    if (app->sharedUniformBuffer) gfxBufferDestroy(app->sharedUniformBuffer);
    if (app->indexBuffer) gfxBufferDestroy(app->indexBuffer);
    if (app->vertexBuffer) gfxBufferDestroy(app->vertexBuffer);
}

static void updateCube(CubeApp* app, int cubeIndex) {
    UniformData uniforms = {0}; // Initialize to zero!

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
    float aspect = (float)app->windowWidth / (float)app->windowHeight;
    matrixPerspective(uniforms.projection,
        45.0f * M_PI / 180.0f, // 45 degree FOV
        aspect,
        0.1f, // near plane
        100.0f, // far plane
        GFX_BACKEND_WEBGPU); // Always WebGPU for web

    // Upload uniform data to buffer at aligned offset
    // Formula: (frame * CUBE_COUNT + cube) * alignedSize
    size_t offset = (app->currentFrame * CUBE_COUNT + cubeIndex) * app->uniformAlignedSize;
    gfxQueueWriteBuffer(app->queue, app->sharedUniformBuffer, offset, &uniforms, sizeof(uniforms));
}

static void update(CubeApp* app, float deltaTime) {
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

static void renderFrame(CubeApp* app) {
    // Get current frame resources
    uint32_t frameIndex = app->currentFrame;
    GfxCommandEncoder commandEncoder = app->commandEncoders[frameIndex];
    
    // Wait for this frame's fence (WebGPU backend provides no-op implementation)
    gfxFenceWait(app->inFlightFences[frameIndex], UINT64_MAX);
    gfxFenceReset(app->inFlightFences[frameIndex]);
    
    // Get backbuffer
    uint32_t imageIndex = 0;
    GfxResult acquireResult = gfxSwapchainAcquireNextImage(app->swapchain, UINT64_MAX, 
        app->imageAvailableSemaphores[frameIndex], NULL, &imageIndex);
    if (acquireResult != GFX_RESULT_SUCCESS) {
        fprintf(stderr, "Failed to acquire swapchain image\n");
        return;
    }
    
    GfxTextureView backbuffer = gfxSwapchainGetImageView(app->swapchain, imageIndex);
    
    // Begin command encoder for current frame
    gfxCommandEncoderBegin(commandEncoder);
    
    // Setup render pass with MSAA
    GfxColorAttachmentTarget colorTargets[2];
    GfxColorAttachment colorAttachment;
    
    if (MSAA_SAMPLE_COUNT == GFX_SAMPLE_COUNT_1) {
        colorTargets[0] = (GfxColorAttachmentTarget){
            .view = backbuffer,
            .ops = {
                .loadOp = GFX_LOAD_OP_CLEAR,
                .storeOp = GFX_STORE_OP_STORE,
                .clearColor = {0.1f, 0.2f, 0.3f, 1.0f},
            },
            .finalLayout = GFX_TEXTURE_LAYOUT_PRESENT_SRC
        };
        colorAttachment = (GfxColorAttachment){
            .target = colorTargets[0],
            .resolveTarget = NULL
        };
    } else {
        colorTargets[0] = (GfxColorAttachmentTarget){
            .view = app->msaaColorTextureView,
            .ops = {
                .loadOp = GFX_LOAD_OP_CLEAR,
                .storeOp = GFX_STORE_OP_DONT_CARE,
                .clearColor = {0.1f, 0.2f, 0.3f, 1.0f},
            },
            .finalLayout = GFX_TEXTURE_LAYOUT_COLOR_ATTACHMENT
        };
        colorTargets[1] = (GfxColorAttachmentTarget){
            .view = backbuffer,
            .ops = {
                .loadOp = GFX_LOAD_OP_DONT_CARE,
                .storeOp = GFX_STORE_OP_STORE,
                .clearColor = {0.1f, 0.2f, 0.3f, 1.0f},
            },
            .finalLayout = GFX_TEXTURE_LAYOUT_PRESENT_SRC
        };
        colorAttachment = (GfxColorAttachment){
            .target = colorTargets[0],
            .resolveTarget = &colorTargets[1]
        };
    }
    
    GfxDepthAttachmentOps depthOps = {
        .loadOp = GFX_LOAD_OP_CLEAR,
        .storeOp = GFX_STORE_OP_STORE,
        .clearValue = 1.0f,
    };
    
    GfxDepthStencilAttachment depthStencilAttachment = {
        .target = {
            .view = app->depthTextureView,
            .depthOps = &depthOps,
            .stencilOps = NULL,
            .finalLayout = GFX_TEXTURE_LAYOUT_DEPTH_STENCIL_ATTACHMENT,
        },
        .resolveTarget = NULL,
    };
    
    GfxRenderPassDescriptor renderPassDesc = {
        .label = "Main Render Pass",
        .colorAttachmentCount = 1,
        .colorAttachments = &colorAttachment,
        .depthStencilAttachment = &depthStencilAttachment,
    };
    
    GfxRenderPassEncoder renderPass;
    if (gfxCommandEncoderBeginRenderPass(commandEncoder, &renderPassDesc, &renderPass) != GFX_RESULT_SUCCESS) {
        fprintf(stderr, "Failed to begin render pass\n");
        return;
    }
    
    // Draw
    gfxRenderPassEncoderSetPipeline(renderPass, app->pipeline);
    gfxRenderPassEncoderSetVertexBuffer(renderPass, 0, app->vertexBuffer, 0, sizeof(vertices));
    gfxRenderPassEncoderSetIndexBuffer(renderPass, app->indexBuffer, GFX_INDEX_FORMAT_UINT16, 0, sizeof(indices));
    
    GfxViewport viewport = {0.0f, 0.0f, (float)app->windowWidth, (float)app->windowHeight, 0.0f, 1.0f};
    gfxRenderPassEncoderSetViewport(renderPass, &viewport);
    
    GfxScissorRect scissor = {0, 0, app->windowWidth, app->windowHeight};
    gfxRenderPassEncoderSetScissorRect(renderPass, &scissor);
    
    // Draw all 3 cubes
    for (int i = 0; i < CUBE_COUNT; ++i) {
        gfxRenderPassEncoderSetBindGroup(renderPass, 0, app->bindGroups[frameIndex][i], NULL, 0);
        gfxRenderPassEncoderDrawIndexed(renderPass, 36, 1, 0, 0, 0);
    }
    
    gfxRenderPassEncoderEnd(renderPass);
    gfxRenderPassEncoderDestroy(renderPass);
    
    // End command encoder
    gfxCommandEncoderEnd(commandEncoder);
    
    // Submit
    GfxSubmitInfo submitInfo = {
        .commandEncoderCount = 1,
        .commandEncoders = &commandEncoder,
        .waitSemaphoreCount = 1,
        .waitSemaphores = &app->imageAvailableSemaphores[frameIndex],
        .signalSemaphoreCount = 1,
        .signalSemaphores = &app->renderFinishedSemaphores[frameIndex],
        .signalFence = app->inFlightFences[frameIndex],
    };
    
    if (gfxQueueSubmit(app->queue, &submitInfo) != GFX_RESULT_SUCCESS) {
        fprintf(stderr, "Failed to submit queue\n");
        return;
    }
    
    // Present
    GfxPresentInfo presentInfo = {
        .waitSemaphoreCount = 1,
        .waitSemaphores = &app->renderFinishedSemaphores[frameIndex],
    };
    if (gfxSwapchainPresent(app->swapchain, &presentInfo) != GFX_RESULT_SUCCESS) {
        fprintf(stderr, "Failed to present\n");
        return;
    }
    
    // Poll device events
    gfxDevicePoll(app->device);
    
    // Advance to next frame
    app->currentFrame = (app->currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

static void cleanup(CubeApp* app) {
    printf("Cleaning up...\n");
    
    // Destroy per-frame resources
    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        if (app->commandEncoders[i]) gfxCommandEncoderDestroy(app->commandEncoders[i]);
        if (app->imageAvailableSemaphores[i]) gfxSemaphoreDestroy(app->imageAvailableSemaphores[i]);
        if (app->renderFinishedSemaphores[i]) gfxSemaphoreDestroy(app->renderFinishedSemaphores[i]);
        if (app->inFlightFences[i]) gfxFenceDestroy(app->inFlightFences[i]);
    }
    
    cleanupRenderingResources(app);
    cleanupSizeDependentResources(app);
    
    if (app->surface) gfxSurfaceDestroy(app->surface);
    if (app->device) gfxDeviceDestroy(app->device);
    if (app->adapter) gfxAdapterDestroy(app->adapter);
    if (app->instance) gfxInstanceDestroy(app->instance);
    
    gfxUnloadAllBackends();
    
    if (app->window) {
        glfwDestroyWindow(app->window);
        glfwTerminate();
    }
}

#if defined(__EMSCRIPTEN__)
static float g_lastTime = 0.0f;
static CubeApp* g_app = NULL;
static uint32_t g_previousWidth = 0;
static uint32_t g_previousHeight = 0;

static void mainLoop(void) {
    if (!g_app || glfwWindowShouldClose(g_app->window)) {
        if (g_app) {
            emscripten_cancel_main_loop();
            cleanup(g_app);
        }
        return;
    }
    
    glfwPollEvents();
    
    // Handle framebuffer resize
    if (g_previousWidth != g_app->windowWidth || g_previousHeight != g_app->windowHeight) {
        // Wait for all in-flight frames to complete
        gfxDeviceWaitIdle(g_app->device);
        
        // Recreate only size-dependent resources (including swapchain)
        cleanupSizeDependentResources(g_app);
        if (!createSizeDependentResources(g_app, g_app->windowWidth, g_app->windowHeight)) {
            fprintf(stderr, "Failed to recreate size-dependent resources after resize\n");
            emscripten_cancel_main_loop();
            cleanup(g_app);
            return;
        }
        
        g_previousWidth = g_app->windowWidth;
        g_previousHeight = g_app->windowHeight;
        
        printf("Window resized: %dx%d\n", g_app->windowWidth, g_app->windowHeight);
        return; // Skip rendering this frame
    }
    
    // Calculate delta time
    float currentTime = (float)emscripten_get_now() / 1000.0f; // Convert ms to seconds
    float deltaTime = currentTime - g_lastTime;
    g_lastTime = currentTime;
    
    update(g_app, deltaTime);
    renderFrame(g_app);
}
#endif

int main(void) {
    printf("Starting WebGPU Cube Example\n");
    
    CubeApp app = {0};
    
    if (!initWindow(&app)) {
        cleanup(&app);
        return 1;
    }
    
    if (!initializeGraphics(&app)) {
        cleanup(&app);
        return 1;
    }
    
    if (!createSizeDependentResources(&app, WINDOW_WIDTH, WINDOW_HEIGHT)) {
        cleanup(&app);
        return 1;
    }
    
    if (!createSyncObjects(&app)) {
        cleanup(&app);
        return 1;
    }
    
    if (!createRenderingResources(&app)) {
        cleanup(&app);
        return 1;
    }
    
    if (!createRenderPipeline(&app)) {
        cleanup(&app);
        return 1;
    }
    
    app.rotationAngleX = 0.0f;
    app.rotationAngleY = 0.0f;
    
#if defined(__EMSCRIPTEN__)
    g_app = &app;
    g_previousWidth = app.windowWidth;
    g_previousHeight = app.windowHeight;
    g_lastTime = (float)emscripten_get_now() / 1000.0f;
    emscripten_set_main_loop(mainLoop, 0, 1);
#else
    uint32_t previousWidth = app.windowWidth;
    uint32_t previousHeight = app.windowHeight;
    float lastTime = (float)glfwGetTime();
    
    while (!glfwWindowShouldClose(app.window)) {
        glfwPollEvents();
        
        // Handle framebuffer resize BEFORE rendering
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
            
            printf("Window resized: %dx%d\n", app.windowWidth, app.windowHeight);
            continue; // Skip rendering this frame
        }
        
        float currentTime = (float)glfwGetTime();
        float deltaTime = currentTime - lastTime;
        lastTime = currentTime;
        
        update(&app, deltaTime);
        renderFrame(&app);
    }
    
    cleanup(&app);
#endif
    
    return 0;
}
