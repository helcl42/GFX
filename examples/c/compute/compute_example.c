#include <gfx/gfx.h>

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
#define COMPUTE_TEXTURE_WIDTH 800
#define COMPUTE_TEXTURE_HEIGHT 600
#define MAX_FRAMES_IN_FLIGHT 3
#define COLOR_FORMAT GFX_TEXTURE_FORMAT_B8G8R8A8_UNORM_SRGB

// Debug callback function
static void debugCallback(GfxDebugMessageSeverity severity, GfxDebugMessageType type, const char* message, void* userData)
{
    const char* severityStr = "";
    switch (severity) {
    case GFX_DEBUG_MESSAGE_SEVERITY_VERBOSE:
        return; // Skip verbose
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
    GfxDevice device;
    GfxQueue queue;
    GfxSurface surface;
    GfxSwapchain swapchain;

    // Compute resources
    GfxTexture computeTexture;
    GfxTextureView computeTextureView;
    GfxShader computeShader;
    GfxComputePipeline computePipeline;
    GfxBindGroupLayout computeBindGroupLayout;
    GfxBindGroup computeBindGroup[MAX_FRAMES_IN_FLIGHT];
    GfxBuffer computeUniformBuffer[MAX_FRAMES_IN_FLIGHT];

    // Render resources (fullscreen quad)
    GfxShader vertexShader;
    GfxShader fragmentShader;
    GfxRenderPipeline renderPipeline;
    GfxBindGroupLayout renderBindGroupLayout;
    GfxSampler sampler;
    GfxBindGroup renderBindGroup[MAX_FRAMES_IN_FLIGHT];
    GfxBuffer renderUniformBuffer[MAX_FRAMES_IN_FLIGHT];

    uint32_t windowWidth;
    uint32_t windowHeight;

    // Per-frame synchronization
    GfxSemaphore imageAvailableSemaphores[MAX_FRAMES_IN_FLIGHT];
    GfxSemaphore renderFinishedSemaphores[MAX_FRAMES_IN_FLIGHT];
    GfxFence inFlightFences[MAX_FRAMES_IN_FLIGHT];
    GfxCommandEncoder commandEncoders[MAX_FRAMES_IN_FLIGHT];

    uint32_t currentFrame;
    float elapsedTime;
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

static bool readShaderFile(const char* filename, char** outCode, size_t* outSize)
{
    FILE* file = fopen(filename, "rb");
    if (!file) {
        fprintf(stderr, "Failed to open shader file: %s\n", filename);
        return false;
    }

    fseek(file, 0, SEEK_END);
    size_t size = ftell(file);
    fseek(file, 0, SEEK_SET);

    *outCode = (char*)malloc(size);
    if (!*outCode) {
        fclose(file);
        return false;
    }

    size_t read = fread(*outCode, 1, size, file);
    fclose(file);

    if (read != size) {
        free(*outCode);
        return false;
    }

    *outSize = size;
    return true;
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

static bool initGraphics(ComputeApp* app)
{
    printf("Loading graphics backend...\n");
    if (!gfxLoadBackend(GFX_BACKEND_AUTO)) {
        fprintf(stderr, "Failed to load any graphics backend\n");
        return false;
    }

    // Get required Vulkan extensions from GLFW
    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    // Create graphics instance
    GfxInstanceDescriptor instanceDesc = {
        .backend = GFX_BACKEND_AUTO,
        .enableValidation = true,
        .enabledHeadless = false,
        .applicationName = "Compute Example (C)",
        .applicationVersion = 1,
        .requiredExtensions = glfwExtensions,
        .requiredExtensionCount = glfwExtensionCount
    };

    if (gfxCreateInstance(&instanceDesc, &app->instance) != GFX_RESULT_SUCCESS) {
        fprintf(stderr, "Failed to create graphics instance\n");
        return false;
    }

    // Set debug callback
    gfxInstanceSetDebugCallback(app->instance, debugCallback, NULL);

    // Get adapter
    GfxAdapterDescriptor adapterDesc = {
        .powerPreference = GFX_POWER_PREFERENCE_HIGH_PERFORMANCE,
        .forceFallbackAdapter = false
    };

    if (gfxInstanceRequestAdapter(app->instance, &adapterDesc, &app->adapter) != GFX_RESULT_SUCCESS) {
        fprintf(stderr, "Failed to request adapter\n");
        return false;
    }

    const char* adapterName = gfxAdapterGetName(app->adapter);
    printf("Using adapter: %s\n", adapterName ? adapterName : "Unknown");

    // Create device
    GfxDeviceDescriptor deviceDesc = { 0 };
    if (gfxAdapterCreateDevice(app->adapter, &deviceDesc, &app->device) != GFX_RESULT_SUCCESS) {
        fprintf(stderr, "Failed to create device\n");
        return false;
    }

    app->queue = gfxDeviceGetQueue(app->device);

    // Create surface
#ifdef _WIN32
    HWND hwnd = glfwGetWin32Window(app->window);
    GfxSurfaceDescriptor surfaceDesc = {
        .windowHandle = {
            .windowingSystem = GFX_WINDOWING_SYSTEM_WIN32,
            .win32 = { .hwnd = hwnd },
        },
    };
#elif defined(__APPLE__)
    void* nsWindow = glfwGetCocoaWindow(app->window);
    GfxSurfaceDescriptor surfaceDesc = {
        .windowHandle = {
            .windowingSystem = GFX_WINDOWING_SYSTEM_COCOA,
            .cocoa = {
                .nsWindow = nsWindow,
            },
        }
    };
#else
    Display* display = glfwGetX11Display();
    Window window = glfwGetX11Window(app->window);
    GfxSurfaceDescriptor surfaceDesc = {
        .windowHandle = {
            .windowingSystem = GFX_WINDOWING_SYSTEM_X11,
            .x11 = {
                .display = display,
                .window = (void*)(uintptr_t)window,
            },
        }
    };
#endif

    if (gfxDeviceCreateSurface(app->device, &surfaceDesc, &app->surface) != GFX_RESULT_SUCCESS) {
        fprintf(stderr, "Failed to create surface\n");
        return false;
    }

    // Create swapchain
    GfxSwapchainDescriptor swapchainDesc = {
        .width = app->windowWidth,
        .height = app->windowHeight,
        .format = COLOR_FORMAT,
        .usage = GFX_TEXTURE_USAGE_RENDER_ATTACHMENT,
        .presentMode = GFX_PRESENT_MODE_FIFO,
        .bufferCount = MAX_FRAMES_IN_FLIGHT
    };

    if (gfxDeviceCreateSwapchain(app->device, app->surface, &swapchainDesc, &app->swapchain) != GFX_RESULT_SUCCESS) {
        fprintf(stderr, "Failed to create swapchain\n");
        return false;
    }

    return true;
}

static bool createComputeResources(ComputeApp* app)
{
    // Create compute output texture (storage image)
    GfxTextureDescriptor textureDesc = {
        .type = GFX_TEXTURE_TYPE_2D,
        .size = { COMPUTE_TEXTURE_WIDTH, COMPUTE_TEXTURE_HEIGHT, 1 },
        .format = GFX_TEXTURE_FORMAT_R8G8B8A8_UNORM,
        .usage = GFX_TEXTURE_USAGE_STORAGE_BINDING | GFX_TEXTURE_USAGE_TEXTURE_BINDING,
        .mipLevelCount = 1,
        .sampleCount = GFX_SAMPLE_COUNT_1
    };

    if (gfxDeviceCreateTexture(app->device, &textureDesc, &app->computeTexture) != GFX_RESULT_SUCCESS) {
        fprintf(stderr, "Failed to create compute texture\n");
        return false;
    }

    GfxTextureViewDescriptor viewDesc = {
        .format = GFX_TEXTURE_FORMAT_R8G8B8A8_UNORM,
        .viewType = GFX_TEXTURE_VIEW_TYPE_2D,
        .baseMipLevel = 0,
        .mipLevelCount = 1,
        .baseArrayLayer = 0,
        .arrayLayerCount = 1
    };

    if (gfxTextureCreateView(app->computeTexture, &viewDesc, &app->computeTextureView) != GFX_RESULT_SUCCESS) {
        fprintf(stderr, "Failed to create compute texture view\n");
        return false;
    }

    // Load compute shader
    char* computeCode = NULL;
    size_t computeSize = 0;
    if (!readShaderFile("generate.comp.spv", &computeCode, &computeSize)) {
        return false;
    }

    GfxShaderDescriptor computeShaderDesc = {
        .code = computeCode,
        .codeSize = computeSize,
        .entryPoint = "main"
    };

    if (gfxDeviceCreateShader(app->device, &computeShaderDesc, &app->computeShader) != GFX_RESULT_SUCCESS) {
        fprintf(stderr, "Failed to create compute shader\n");
        free(computeCode);
        return false;
    }
    free(computeCode);

    // Create compute uniform buffers (one per frame in flight)
    GfxBufferDescriptor computeUniformBufferDesc = {
        .size = sizeof(ComputeUniformData),
        .usage = GFX_BUFFER_USAGE_UNIFORM | GFX_BUFFER_USAGE_COPY_DST,
        .mappedAtCreation = false
    };

    for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        if (gfxDeviceCreateBuffer(app->device, &computeUniformBufferDesc, &app->computeUniformBuffer[i]) != GFX_RESULT_SUCCESS) {
            fprintf(stderr, "Failed to create compute uniform buffer %d\n", i);
            return false;
        }
    }

    // Create compute bind group layout
    GfxBindGroupLayoutEntry computeLayoutEntries[2] = {
        {
            .binding = 0,
            .visibility = GFX_SHADER_STAGE_COMPUTE,
            .type = GFX_BINDING_TYPE_STORAGE_TEXTURE,
            .storageTexture = {
                .format = GFX_TEXTURE_FORMAT_R8G8B8A8_UNORM,
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

    GfxBindGroupLayoutDescriptor computeLayoutDesc = {
        .entryCount = 2,
        .entries = computeLayoutEntries
    };

    if (gfxDeviceCreateBindGroupLayout(app->device, &computeLayoutDesc, &app->computeBindGroupLayout) != GFX_RESULT_SUCCESS) {
        fprintf(stderr, "Failed to create compute bind group layout\n");
        return false;
    }

    // Create compute bind groups (one per frame in flight)
    for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
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

        GfxBindGroupDescriptor computeBindGroupDesc = {
            .layout = app->computeBindGroupLayout,
            .entryCount = 2,
            .entries = computeEntries
        };

        if (gfxDeviceCreateBindGroup(app->device, &computeBindGroupDesc, &app->computeBindGroup[i]) != GFX_RESULT_SUCCESS) {
            fprintf(stderr, "Failed to create compute bind group %d\n", i);
            return false;
        }
    }

    // Create compute pipeline
    GfxComputePipelineDescriptor computePipelineDesc = {
        .compute = app->computeShader,
        .entryPoint = "main",
        .bindGroupLayouts = &app->computeBindGroupLayout,
        .bindGroupLayoutCount = 1
    };

    if (gfxDeviceCreateComputePipeline(app->device, &computePipelineDesc, &app->computePipeline) != GFX_RESULT_SUCCESS) {
        fprintf(stderr, "Failed to create compute pipeline\n");
        return false;
    }

    // Transition compute texture to SHADER_READ_ONLY layout initially
    // This way we don't need special handling for the first frame
    GfxCommandEncoder initEncoder = NULL;
    GfxCommandEncoderDescriptor initEncoderDesc = { .label = "Init Layout Transition" };
    if (gfxDeviceCreateCommandEncoder(app->device, &initEncoderDesc, &initEncoder) != GFX_RESULT_SUCCESS) {
        fprintf(stderr, "Failed to create command encoder for initial layout transition\n");
        return false;
    }

    gfxCommandEncoderBegin(initEncoder);

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
    gfxCommandEncoderPipelineBarrier(initEncoder, NULL, 0, NULL, 0, &initBarrier, 1);

    gfxCommandEncoderEnd(initEncoder);

    GfxSubmitInfo submitInfo = {
        .commandEncoderCount = 1,
        .commandEncoders = &initEncoder,
    };
    gfxQueueSubmit(app->queue, &submitInfo);
    gfxDeviceWaitIdle(app->device);

    gfxCommandEncoderDestroy(initEncoder);

    printf("Compute resources created successfully\n");
    return true;
}

static bool createRenderResources(ComputeApp* app)
{
    // Load shaders
    char* vertexCode = NULL;
    size_t vertexSize = 0;
    if (!readShaderFile("fullscreen.vert.spv", &vertexCode, &vertexSize)) {
        return false;
    }

    GfxShaderDescriptor vertexShaderDesc = {
        .code = vertexCode,
        .codeSize = vertexSize,
        .entryPoint = "main"
    };

    if (gfxDeviceCreateShader(app->device, &vertexShaderDesc, &app->vertexShader) != GFX_RESULT_SUCCESS) {
        fprintf(stderr, "Failed to create vertex shader\n");
        free(vertexCode);
        return false;
    }
    free(vertexCode);

    char* fragmentCode = NULL;
    size_t fragmentSize = 0;
    if (!readShaderFile("postprocess.frag.spv", &fragmentCode, &fragmentSize)) {
        return false;
    }

    GfxShaderDescriptor fragmentShaderDesc = {
        .code = fragmentCode,
        .codeSize = fragmentSize,
        .entryPoint = "main"
    };

    if (gfxDeviceCreateShader(app->device, &fragmentShaderDesc, &app->fragmentShader) != GFX_RESULT_SUCCESS) {
        fprintf(stderr, "Failed to create fragment shader\n");
        free(fragmentCode);
        return false;
    }
    free(fragmentCode);

    // Create sampler
    GfxSamplerDescriptor samplerDesc = {
        .magFilter = GFX_FILTER_MODE_LINEAR,
        .minFilter = GFX_FILTER_MODE_LINEAR,
        .addressModeU = GFX_ADDRESS_MODE_CLAMP_TO_EDGE,
        .addressModeV = GFX_ADDRESS_MODE_CLAMP_TO_EDGE
    };

    if (gfxDeviceCreateSampler(app->device, &samplerDesc, &app->sampler) != GFX_RESULT_SUCCESS) {
        fprintf(stderr, "Failed to create sampler\n");
        return false;
    }

    // Create render uniform buffers (one per frame in flight)
    GfxBufferDescriptor renderUniformBufferDesc = {
        .size = sizeof(RenderUniformData),
        .usage = GFX_BUFFER_USAGE_UNIFORM | GFX_BUFFER_USAGE_COPY_DST,
        .mappedAtCreation = false
    };

    for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        if (gfxDeviceCreateBuffer(app->device, &renderUniformBufferDesc, &app->renderUniformBuffer[i]) != GFX_RESULT_SUCCESS) {
            fprintf(stderr, "Failed to create render uniform buffer %d\n", i);
            return false;
        }
    }

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

    GfxBindGroupLayoutDescriptor renderLayoutDesc = {
        .entryCount = 3,
        .entries = renderLayoutEntries
    };

    if (gfxDeviceCreateBindGroupLayout(app->device, &renderLayoutDesc, &app->renderBindGroupLayout) != GFX_RESULT_SUCCESS) {
        fprintf(stderr, "Failed to create render bind group layout\n");
        return false;
    }

    // Create render bind groups (one per frame in flight)
    for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
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

        GfxBindGroupDescriptor renderBindGroupDesc = {
            .layout = app->renderBindGroupLayout,
            .entryCount = 3,
            .entries = renderEntries
        };

        if (gfxDeviceCreateBindGroup(app->device, &renderBindGroupDesc, &app->renderBindGroup[i]) != GFX_RESULT_SUCCESS) {
            fprintf(stderr, "Failed to create render bind group %d\n", i);
            return false;
        }
    }

    // Create render pipeline
    GfxVertexState vertexState = {
        .module = app->vertexShader,
        .entryPoint = "main",
        .bufferCount = 0
    };

    GfxColorTargetState colorTarget = {
        .format = COLOR_FORMAT,
        .writeMask = 0xF
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

    GfxRenderPipelineDescriptor pipelineDesc = {
        .vertex = &vertexState,
        .fragment = &fragmentState,
        .primitive = &primitiveState,
        .depthStencil = NULL,
        .sampleCount = GFX_SAMPLE_COUNT_1,
        .bindGroupLayoutCount = 1,
        .bindGroupLayouts = &app->renderBindGroupLayout
    };

    if (gfxDeviceCreateRenderPipeline(app->device, &pipelineDesc, &app->renderPipeline) != GFX_RESULT_SUCCESS) {
        fprintf(stderr, "Failed to create render pipeline\n");
        return false;
    }

    printf("Render resources created successfully\n");
    return true;
}

static bool createSyncObjects(ComputeApp* app)
{
    GfxSemaphoreDescriptor semaphoreDesc = {
        .type = GFX_SEMAPHORE_TYPE_BINARY
    };

    GfxFenceDescriptor fenceDesc = {
        .signaled = true
    };

    for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
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
        snprintf(label, sizeof(label), "Command Encoder %d", i);
        GfxCommandEncoderDescriptor encoderDesc = { .label = label };
        if (gfxDeviceCreateCommandEncoder(app->device, &encoderDesc, &app->commandEncoders[i]) != GFX_RESULT_SUCCESS) {
            fprintf(stderr, "Failed to create command encoder %d\n", i);
            return false;
        }
    }

    return true;
}

static void update(ComputeApp* app, float deltaTime)
{
    app->elapsedTime += deltaTime;
}

static void drawFrame(ComputeApp* app)
{
    uint32_t frameIndex = app->currentFrame;

    // Wait for previous frame
    gfxFenceWait(app->inFlightFences[frameIndex], UINT64_MAX);
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
    gfxCommandEncoderPipelineBarrier(encoder, NULL, 0, NULL, 0, &readToWriteBarrier, 1);

    // --- COMPUTE PASS: Generate pattern ---
    GfxComputePassDescriptor computePassDesc = {
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
    gfxComputePassEncoderDispatchWorkgroups(computePass, workGroupsX, workGroupsY, 1);

    gfxComputePassEncoderEnd(computePass);

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
    gfxCommandEncoderPipelineBarrier(encoder, NULL, 0, NULL, 0, &computeToReadBarrier, 1);

    // --- RENDER PASS: Post-process and display ---
    GfxTextureView swapchainView = gfxSwapchainGetCurrentTextureView(app->swapchain);

    GfxColorAttachment colorAttachment = {
        .view = swapchainView,
        .resolveView = NULL,
        .loadOp = GFX_LOAD_OP_CLEAR,
        .storeOp = GFX_STORE_OP_STORE,
        .resolveLoadOp = GFX_LOAD_OP_DONT_CARE,  // Unused but must be initialized
        .resolveStoreOp = GFX_STORE_OP_STORE,    // Unused but must be initialized
        .clearColor = { 0.0f, 0.0f, 0.0f, 1.0f },
        .finalLayout = GFX_TEXTURE_LAYOUT_PRESENT_SRC,
        .resolveFinalLayout = GFX_TEXTURE_LAYOUT_UNDEFINED  // Unused but must be initialized
    };

    GfxRenderPassDescriptor renderPassDesc = {
        .label = "Fullscreen Render Pass",
        .colorAttachments = &colorAttachment,
        .colorAttachmentCount = 1,
        .depthStencilAttachment = NULL
    };

    GfxRenderPassEncoder renderPass = NULL;
    if (gfxCommandEncoderBeginRenderPass(encoder, &renderPassDesc, &renderPass) != GFX_RESULT_SUCCESS) {
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
        .x = 0,
        .y = 0,
        .width = app->windowWidth,
        .height = app->windowHeight
    };
    gfxRenderPassEncoderSetScissorRect(renderPass, &scissor);

    // Draw fullscreen quad (6 vertices, no buffers needed)
    gfxRenderPassEncoderDraw(renderPass, 6, 1, 0, 0);

    gfxRenderPassEncoderEnd(renderPass);

    gfxCommandEncoderEnd(encoder);

    // Submit
    GfxSubmitInfo submitInfo = {
        .commandEncoderCount = 1,
        .commandEncoders = &encoder,
        .waitSemaphoreCount = 1,
        .waitSemaphores = &app->imageAvailableSemaphores[frameIndex],
        .signalSemaphoreCount = 1,
        .signalSemaphores = &app->renderFinishedSemaphores[frameIndex],
        .signalFence = app->inFlightFences[frameIndex]
    };

    gfxQueueSubmit(app->queue, &submitInfo);

    // Present
    GfxPresentInfo presentInfo = {
        .waitSemaphoreCount = 1,
        .waitSemaphores = &app->renderFinishedSemaphores[frameIndex]
    };

    result = gfxSwapchainPresent(app->swapchain, &presentInfo);

    app->currentFrame = (app->currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

static void cleanup(ComputeApp* app)
{
    if (app->device) {
        gfxDeviceWaitIdle(app->device);
    }

    // Sync objects
    for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        if (app->commandEncoders[i]) {
            gfxCommandEncoderDestroy(app->commandEncoders[i]);
        }
        if (app->imageAvailableSemaphores[i]) {
            gfxSemaphoreDestroy(app->imageAvailableSemaphores[i]);
        }
        if (app->renderFinishedSemaphores[i]) {
            gfxSemaphoreDestroy(app->renderFinishedSemaphores[i]);
        }
        if (app->inFlightFences[i]) {
            gfxFenceDestroy(app->inFlightFences[i]);
        }
    }

    // Render resources
    if (app->renderPipeline) {
        gfxRenderPipelineDestroy(app->renderPipeline);
    }
    for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        if (app->renderBindGroup[i]) {
            gfxBindGroupDestroy(app->renderBindGroup[i]);
        }
        if (app->renderUniformBuffer[i]) {
            gfxBufferDestroy(app->renderUniformBuffer[i]);
        }
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
    for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        if (app->computeBindGroup[i]) {
            gfxBindGroupDestroy(app->computeBindGroup[i]);
        }
        if (app->computeUniformBuffer[i]) {
            gfxBufferDestroy(app->computeUniformBuffer[i]);
        }
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
    if (app->adapter) {
        gfxAdapterDestroy(app->adapter);
    }
    if (app->instance) {
        gfxInstanceDestroy(app->instance);
    }

    if (app->window) {
        glfwDestroyWindow(app->window);
        glfwTerminate();
    }
}

// Helper function to cleanup size-dependent resources
static void cleanupSizeDependentResources(ComputeApp* app)
{
    if (app->swapchain) {
        gfxSwapchainDestroy(app->swapchain);
        app->swapchain = NULL;
    }
}

// Helper function to recreate size-dependent resources
static bool recreateSizeDependentResources(ComputeApp* app, uint32_t width, uint32_t height)
{
    // Create swapchain with new dimensions
    // Compute texture stays at fixed resolution and is sampled with linear filtering
    GfxSwapchainDescriptor swapchainDesc = {
        .width = width,
        .height = height,
        .format = GFX_TEXTURE_FORMAT_B8G8R8A8_UNORM_SRGB,
        .usage = GFX_TEXTURE_USAGE_RENDER_ATTACHMENT,
        .presentMode = GFX_PRESENT_MODE_FIFO
    };

    if (gfxDeviceCreateSwapchain(app->device, app->surface, &swapchainDesc, &app->swapchain) != GFX_RESULT_SUCCESS) {
        fprintf(stderr, "Failed to create swapchain\n");
        return false;
    }

    return true;
}

int main(void)
{
    printf("=== Compute & Postprocess Example (C) ===\n\n");

    ComputeApp app = { 0 };
    app.currentFrame = 0;

    if (!initWindow(&app)) {
        return 1;
    }

    if (!initGraphics(&app)) {
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

    float lastTime = (float)glfwGetTime();

    uint32_t previousWidth = gfxSwapchainGetWidth(app.swapchain);
    uint32_t previousHeight = gfxSwapchainGetHeight(app.swapchain);

    while (!glfwWindowShouldClose(app.window)) {
        glfwPollEvents();

        // Handle framebuffer resize BEFORE rendering
        if (previousWidth != app.windowWidth || previousHeight != app.windowHeight) {
            // Wait for all in-flight frames to complete
            gfxDeviceWaitIdle(app.device);

            // Recreate size-dependent resources
            cleanupSizeDependentResources(&app);
            if (!recreateSizeDependentResources(&app, app.windowWidth, app.windowHeight)) {
                fprintf(stderr, "Failed to recreate size-dependent resources after resize\n");
                break;
            }

            previousWidth = app.windowWidth;
            previousHeight = app.windowHeight;

            printf("Window resized: %dx%d\n", app.windowWidth, app.windowHeight);
            continue; // Skip rendering this frame
        }

        float currentTime = (float)glfwGetTime();
        float deltaTime = (float)(currentTime - lastTime);
        lastTime = currentTime;

        update(&app, deltaTime);
        drawFrame(&app);
    }

    cleanup(&app);

    printf("Application terminated successfully\n");
    return 0;
}
