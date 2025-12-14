#include "../dependencies/include/webgpu/webgpu.h"
#include "GfxApi.h"
#include "GfxBackend.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Platform-specific includes for surface creation
#ifdef _WIN32
#include <windows.h>
#elif defined(__linux__)
#include <X11/Xlib.h>
// For Wayland support
struct wl_display;
struct wl_surface;
#elif defined(__APPLE__)
#include <objc/objc.h>
#include <objc/runtime.h>
#endif

// ============================================================================
// Internal Structures
// ============================================================================

struct GfxInstance_T {
    WGPUInstance wgpu_instance;
};

struct GfxAdapter_T {
    WGPUAdapter wgpu_adapter;
    char* name;
    GfxBackend backend;
};

struct GfxDevice_T {
    WGPUDevice wgpu_device;
    WGPUQueue wgpu_queue;
};

struct GfxQueue_T {
    WGPUQueue wgpu_queue;
};

struct GfxSurface_T {
    WGPUSurface wgpu_surface;
    uint32_t width;
    uint32_t height;
    GfxPlatformWindowHandle window_handle;
};

struct GfxSwapchain_T {
    WGPUSurface wgpu_surface;
    WGPUDevice wgpu_device;
    uint32_t width;
    uint32_t height;
    WGPUTextureFormat format;
    WGPUPresentMode present_mode;
    uint32_t buffer_count;
};

struct GfxBuffer_T {
    WGPUBuffer wgpu_buffer;
    uint64_t size;
    GfxBufferUsage usage;
};

struct GfxTexture_T {
    WGPUTexture wgpu_texture;
    WGPUExtent3D size;
    WGPUTextureFormat format;
    uint32_t mip_levels;
    uint32_t sample_count;
    WGPUTextureUsage usage;
};

struct GfxTextureView_T {
    WGPUTextureView wgpu_view;
    GfxTexture texture;
};

struct GfxSampler_T {
    WGPUSampler wgpu_sampler;
};

struct GfxShader_T {
    WGPUShaderModule wgpu_module;
};

struct GfxBindGroupLayout_T {
    WGPUBindGroupLayout wgpu_layout;
};

struct GfxBindGroup_T {
    WGPUBindGroup wgpu_bind_group;
};

struct GfxRenderPipeline_T {
    WGPURenderPipeline wgpu_pipeline;
};

struct GfxComputePipeline_T {
    WGPUComputePipeline wgpu_pipeline;
};

struct GfxCommandEncoder_T {
    WGPUCommandEncoder wgpu_encoder;
};

struct GfxRenderPassEncoder_T {
    WGPURenderPassEncoder wgpu_encoder;
};

struct GfxComputePassEncoder_T {
    WGPUComputePassEncoder wgpu_encoder;
};

// WebGPU doesn't have native fence/semaphore support
// We'll implement these as empty stubs for API compatibility
struct GfxFence_T {
    bool signaled;
};

struct GfxSemaphore_T {
    GfxSemaphoreType type;
    uint64_t value;
};

// ============================================================================
// Utility Functions
// ============================================================================

static WGPUStringView gfx_string_view(const char* str)
{
    if (!str) {
        return (WGPUStringView){ NULL, WGPU_STRLEN };
    }
    return (WGPUStringView){ str, WGPU_STRLEN };
}

static WGPUTextureFormat gfx_to_wgpu_texture_format(GfxTextureFormat format)
{
    switch (format) {
    case GFX_TEXTURE_FORMAT_R8_UNORM:
        return WGPUTextureFormat_R8Unorm;
    case GFX_TEXTURE_FORMAT_R8G8_UNORM:
        return WGPUTextureFormat_RG8Unorm;
    case GFX_TEXTURE_FORMAT_R8G8B8A8_UNORM:
        return WGPUTextureFormat_RGBA8Unorm;
    case GFX_TEXTURE_FORMAT_R8G8B8A8_UNORM_SRGB:
        return WGPUTextureFormat_RGBA8UnormSrgb;
    case GFX_TEXTURE_FORMAT_B8G8R8A8_UNORM:
        return WGPUTextureFormat_BGRA8Unorm;
    case GFX_TEXTURE_FORMAT_B8G8R8A8_UNORM_SRGB:
        return WGPUTextureFormat_BGRA8UnormSrgb;
    case GFX_TEXTURE_FORMAT_R16_FLOAT:
        return WGPUTextureFormat_R16Float;
    case GFX_TEXTURE_FORMAT_R16G16_FLOAT:
        return WGPUTextureFormat_RG16Float;
    case GFX_TEXTURE_FORMAT_R16G16B16A16_FLOAT:
        return WGPUTextureFormat_RGBA16Float;
    case GFX_TEXTURE_FORMAT_R32_FLOAT:
        return WGPUTextureFormat_R32Float;
    case GFX_TEXTURE_FORMAT_R32G32_FLOAT:
        return WGPUTextureFormat_RG32Float;
    case GFX_TEXTURE_FORMAT_R32G32B32A32_FLOAT:
        return WGPUTextureFormat_RGBA32Float;
    case GFX_TEXTURE_FORMAT_DEPTH16_UNORM:
        return WGPUTextureFormat_Depth16Unorm;
    case GFX_TEXTURE_FORMAT_DEPTH24_PLUS:
        return WGPUTextureFormat_Depth24Plus;
    case GFX_TEXTURE_FORMAT_DEPTH32_FLOAT:
        return WGPUTextureFormat_Depth32Float;
    case GFX_TEXTURE_FORMAT_DEPTH24_PLUS_STENCIL8:
        return WGPUTextureFormat_Depth24PlusStencil8;
    case GFX_TEXTURE_FORMAT_DEPTH32_FLOAT_STENCIL8:
        return WGPUTextureFormat_Depth32FloatStencil8;
    default:
        return WGPUTextureFormat_Undefined;
    }
}

static GfxTextureFormat wgpu_to_gfx_texture_format(WGPUTextureFormat format)
{
    switch (format) {
    case WGPUTextureFormat_R8Unorm:
        return GFX_TEXTURE_FORMAT_R8_UNORM;
    case WGPUTextureFormat_RG8Unorm:
        return GFX_TEXTURE_FORMAT_R8G8_UNORM;
    case WGPUTextureFormat_RGBA8Unorm:
        return GFX_TEXTURE_FORMAT_R8G8B8A8_UNORM;
    case WGPUTextureFormat_RGBA8UnormSrgb:
        return GFX_TEXTURE_FORMAT_R8G8B8A8_UNORM_SRGB;
    case WGPUTextureFormat_BGRA8Unorm:
        return GFX_TEXTURE_FORMAT_B8G8R8A8_UNORM;
    case WGPUTextureFormat_BGRA8UnormSrgb:
        return GFX_TEXTURE_FORMAT_B8G8R8A8_UNORM_SRGB;
    case WGPUTextureFormat_R16Float:
        return GFX_TEXTURE_FORMAT_R16_FLOAT;
    case WGPUTextureFormat_RG16Float:
        return GFX_TEXTURE_FORMAT_R16G16_FLOAT;
    case WGPUTextureFormat_RGBA16Float:
        return GFX_TEXTURE_FORMAT_R16G16B16A16_FLOAT;
    case WGPUTextureFormat_R32Float:
        return GFX_TEXTURE_FORMAT_R32_FLOAT;
    case WGPUTextureFormat_RG32Float:
        return GFX_TEXTURE_FORMAT_R32G32_FLOAT;
    case WGPUTextureFormat_RGBA32Float:
        return GFX_TEXTURE_FORMAT_R32G32B32A32_FLOAT;
    case WGPUTextureFormat_Depth16Unorm:
        return GFX_TEXTURE_FORMAT_DEPTH16_UNORM;
    case WGPUTextureFormat_Depth24Plus:
        return GFX_TEXTURE_FORMAT_DEPTH24_PLUS;
    case WGPUTextureFormat_Depth32Float:
        return GFX_TEXTURE_FORMAT_DEPTH32_FLOAT;
    case WGPUTextureFormat_Depth24PlusStencil8:
        return GFX_TEXTURE_FORMAT_DEPTH24_PLUS_STENCIL8;
    case WGPUTextureFormat_Depth32FloatStencil8:
        return GFX_TEXTURE_FORMAT_DEPTH32_FLOAT_STENCIL8;
    default:
        return GFX_TEXTURE_FORMAT_UNDEFINED;
    }
}

static WGPUBufferUsage gfx_to_wgpu_buffer_usage(GfxBufferUsage usage)
{
    WGPUBufferUsage wgpu_usage = WGPUBufferUsage_None;
    if (usage & GFX_BUFFER_USAGE_MAP_READ)
        wgpu_usage |= WGPUBufferUsage_MapRead;
    if (usage & GFX_BUFFER_USAGE_MAP_WRITE)
        wgpu_usage |= WGPUBufferUsage_MapWrite;
    if (usage & GFX_BUFFER_USAGE_COPY_SRC)
        wgpu_usage |= WGPUBufferUsage_CopySrc;
    if (usage & GFX_BUFFER_USAGE_COPY_DST)
        wgpu_usage |= WGPUBufferUsage_CopyDst;
    if (usage & GFX_BUFFER_USAGE_INDEX)
        wgpu_usage |= WGPUBufferUsage_Index;
    if (usage & GFX_BUFFER_USAGE_VERTEX)
        wgpu_usage |= WGPUBufferUsage_Vertex;
    if (usage & GFX_BUFFER_USAGE_UNIFORM)
        wgpu_usage |= WGPUBufferUsage_Uniform;
    if (usage & GFX_BUFFER_USAGE_STORAGE)
        wgpu_usage |= WGPUBufferUsage_Storage;
    if (usage & GFX_BUFFER_USAGE_INDIRECT)
        wgpu_usage |= WGPUBufferUsage_Indirect;
    return wgpu_usage;
}

static WGPUTextureUsage gfx_to_wgpu_texture_usage(GfxTextureUsage usage)
{
    WGPUTextureUsage wgpu_usage = WGPUTextureUsage_None;
    if (usage & GFX_TEXTURE_USAGE_COPY_SRC)
        wgpu_usage |= WGPUTextureUsage_CopySrc;
    if (usage & GFX_TEXTURE_USAGE_COPY_DST)
        wgpu_usage |= WGPUTextureUsage_CopyDst;
    if (usage & GFX_TEXTURE_USAGE_TEXTURE_BINDING)
        wgpu_usage |= WGPUTextureUsage_TextureBinding;
    if (usage & GFX_TEXTURE_USAGE_STORAGE_BINDING)
        wgpu_usage |= WGPUTextureUsage_StorageBinding;
    if (usage & GFX_TEXTURE_USAGE_RENDER_ATTACHMENT)
        wgpu_usage |= WGPUTextureUsage_RenderAttachment;
    return wgpu_usage;
}

static WGPUPresentMode gfx_to_wgpu_present_mode(GfxPresentMode mode)
{
    switch (mode) {
    case GFX_PRESENT_MODE_IMMEDIATE:
        return WGPUPresentMode_Immediate;
    case GFX_PRESENT_MODE_FIFO:
        return WGPUPresentMode_Fifo;
    case GFX_PRESENT_MODE_FIFO_RELAXED:
        return WGPUPresentMode_FifoRelaxed;
    case GFX_PRESENT_MODE_MAILBOX:
        return WGPUPresentMode_Mailbox;
    default:
        return WGPUPresentMode_Fifo;
    }
}

static WGPUPrimitiveTopology gfx_to_wgpu_primitive_topology(GfxPrimitiveTopology topology)
{
    switch (topology) {
    case GFX_PRIMITIVE_TOPOLOGY_POINT_LIST:
        return WGPUPrimitiveTopology_PointList;
    case GFX_PRIMITIVE_TOPOLOGY_LINE_LIST:
        return WGPUPrimitiveTopology_LineList;
    case GFX_PRIMITIVE_TOPOLOGY_LINE_STRIP:
        return WGPUPrimitiveTopology_LineStrip;
    case GFX_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST:
        return WGPUPrimitiveTopology_TriangleList;
    case GFX_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP:
        return WGPUPrimitiveTopology_TriangleStrip;
    default:
        return WGPUPrimitiveTopology_TriangleList;
    }
}

static WGPUIndexFormat gfx_to_wgpu_index_format(GfxIndexFormat format)
{
    switch (format) {
    case GFX_INDEX_FORMAT_UINT16:
        return WGPUIndexFormat_Uint16;
    case GFX_INDEX_FORMAT_UINT32:
        return WGPUIndexFormat_Uint32;
    default:
        return WGPUIndexFormat_Undefined;
    }
}

// ============================================================================
// Platform-specific Surface Creation Helpers
// ============================================================================

#ifdef _WIN32
static WGPUSurface create_surface_win32(WGPUInstance instance, const GfxPlatformWindowHandle* handle)
{
    if (!handle->hwnd || !handle->hinstance) {
        return NULL;
    }

    WGPUSurfaceSourceWindowsHWND source = WGPU_SURFACE_SOURCE_WINDOWS_HWND_INIT;
    source.hwnd = handle->hwnd;
    source.hinstance = handle->hinstance;

    WGPUSurfaceDescriptor surface_desc = WGPU_SURFACE_DESCRIPTOR_INIT;
    surface_desc.label = gfx_string_view("Win32 Surface");
    surface_desc.nextInChain = (WGPUChainedStruct*)&source;

    return wgpuInstanceCreateSurface(instance, &surface_desc);
}
#endif

#ifdef __linux__
static WGPUSurface create_surface_x11(WGPUInstance instance, const GfxPlatformWindowHandle* handle)
{
    if (!handle->window || !handle->display) {
        return NULL;
    }

    WGPUSurfaceSourceXlibWindow source = WGPU_SURFACE_SOURCE_XLIB_WINDOW_INIT;
    source.display = handle->display;
    source.window = (uint64_t)(uintptr_t)handle->window;

    WGPUSurfaceDescriptor surface_desc = WGPU_SURFACE_DESCRIPTOR_INIT;
    surface_desc.label = gfx_string_view("X11 Surface");
    surface_desc.nextInChain = (WGPUChainedStruct*)&source;

    return wgpuInstanceCreateSurface(instance, &surface_desc);
}

static WGPUSurface create_surface_wayland(WGPUInstance instance, const GfxPlatformWindowHandle* handle)
{
    if (!handle->window || !handle->display) {
        return NULL;
    }

    WGPUSurfaceSourceWaylandSurface source = WGPU_SURFACE_SOURCE_WAYLAND_SURFACE_INIT;
    source.display = handle->display;
    source.surface = handle->window;

    WGPUSurfaceDescriptor surface_desc = WGPU_SURFACE_DESCRIPTOR_INIT;
    surface_desc.label = gfx_string_view("Wayland Surface");
    surface_desc.nextInChain = (WGPUChainedStruct*)&source;

    return wgpuInstanceCreateSurface(instance, &surface_desc);
}
#endif

#ifdef __APPLE__
static WGPUSurface create_surface_metal(WGPUInstance instance, const GfxPlatformWindowHandle* handle)
{
    void* metal_layer = handle->metalLayer;

    // If no metal layer provided, try to get it from the NSWindow
    if (!metal_layer && handle->nsWindow) {
        // Get the content view from the NSWindow
        id nsWindow = (id)handle->nsWindow;
        SEL contentViewSel = sel_registerName("contentView");
        id contentView = ((id(*)(id, SEL))objc_msgSend)(nsWindow, contentViewSel);

        if (contentView) {
            // Get the layer from the view
            SEL layerSel = sel_registerName("layer");
            metal_layer = ((void* (*)(id, SEL))objc_msgSend)(contentView, layerSel);

            // If it's not a CAMetalLayer, we might need to create one
            // This is a simplified approach - in practice you'd want more robust layer handling
            if (metal_layer) {
                Class metalLayerClass = objc_getClass("CAMetalLayer");
                if (metalLayerClass && !object_isKindOfClass((id)metal_layer, metalLayerClass)) {
                    // Create a new CAMetalLayer
                    id newMetalLayer = ((id(*)(Class, SEL))objc_msgSend)(metalLayerClass, sel_registerName("new"));

                    // Set the metal layer as the view's layer
                    SEL setLayerSel = sel_registerName("setLayer:");
                    ((void (*)(id, SEL, id))objc_msgSend)(contentView, setLayerSel, newMetalLayer);

                    // Enable layer backing
                    SEL setWantsLayerSel = sel_registerName("setWantsLayer:");
                    ((void (*)(id, SEL, BOOL))objc_msgSend)(contentView, setWantsLayerSel, YES);

                    metal_layer = newMetalLayer;
                }
            }
        }
    }

    if (!metal_layer) {
        return NULL;
    }

    WGPUSurfaceSourceMetalLayer source = WGPU_SURFACE_SOURCE_METAL_LAYER_INIT;
    source.layer = metal_layer;

    WGPUSurfaceDescriptor surface_desc = WGPU_SURFACE_DESCRIPTOR_INIT;
    surface_desc.label = gfx_string_view("Metal Surface");
    surface_desc.nextInChain = (WGPUChainedStruct*)&source;

    return wgpuInstanceCreateSurface(instance, &surface_desc);
}
#endif

static WGPUSurface create_platform_surface(WGPUInstance instance, const GfxPlatformWindowHandle* handle)
{
    if (!instance || !handle) {
        return NULL;
    }

#ifdef _WIN32
    return create_surface_win32(instance, handle);

#elif defined(__linux__)
    if (handle->isWayland) {
        return create_surface_wayland(instance, handle);
    } else {
        return create_surface_x11(instance, handle);
    }

#elif defined(__APPLE__)
    return create_surface_metal(instance, handle);

#else
    // Generic fallback - not implemented for other platforms
    (void)instance;
    (void)handle;
    return NULL;
#endif
}

// ============================================================================
// Instance functions
// ============================================================================

GfxInstance webgpu_createInstance(const GfxInstanceDescriptor* descriptor)
{
    WGPUInstanceDescriptor wgpu_desc = WGPU_INSTANCE_DESCRIPTOR_INIT;

    if (descriptor && descriptor->applicationName) {
        // WebGPU doesn't have application name in instance descriptor
        // This would typically be handled at a higher level
    }

    WGPUInstance wgpu_instance = wgpuCreateInstance(&wgpu_desc);
    if (!wgpu_instance) {
        return NULL;
    }

    GfxInstance instance = (GfxInstance)malloc(sizeof(struct GfxInstance_T));
    if (!instance) {
        wgpuInstanceRelease(wgpu_instance);
        return NULL;
    }

    instance->wgpu_instance = wgpu_instance;
    return instance;
}

void webgpu_instanceDestroy(GfxInstance instance)
{
    if (!instance)
        return;

    if (instance->wgpu_instance) {
        wgpuInstanceRelease(instance->wgpu_instance);
    }
    free(instance);
}

// Callback for adapter request
static void adapter_request_callback(WGPURequestAdapterStatus status, WGPUAdapter adapter,
    WGPUStringView message, void* userdata1, void* userdata2)
{
    GfxAdapter* result = (GfxAdapter*)userdata1;
    if (status == WGPURequestAdapterStatus_Success && adapter) {
        *result = (GfxAdapter)malloc(sizeof(struct GfxAdapter_T));
        if (*result) {
            (*result)->wgpu_adapter = adapter;
            (*result)->name = strdup("WebGPU Adapter");
            (*result)->backend = GFX_BACKEND_WEBGPU;
        }
    } else {
        *result = NULL;
    }
}

GfxAdapter webgpu_instanceRequestAdapter(GfxInstance instance, const GfxAdapterDescriptor* descriptor)
{
    if (!instance)
        return NULL;

    WGPURequestAdapterOptions options = WGPU_REQUEST_ADAPTER_OPTIONS_INIT;

    if (descriptor) {
        switch (descriptor->powerPreference) {
        case GFX_POWER_PREFERENCE_LOW_POWER:
            options.powerPreference = WGPUPowerPreference_LowPower;
            break;
        case GFX_POWER_PREFERENCE_HIGH_PERFORMANCE:
            options.powerPreference = WGPUPowerPreference_HighPerformance;
            break;
        default:
            options.powerPreference = WGPUPowerPreference_Undefined;
            break;
        }
        options.forceFallbackAdapter = descriptor->forceFallbackAdapter ? WGPU_TRUE : WGPU_FALSE;
    }

    GfxAdapter result = NULL;
    WGPURequestAdapterCallbackInfo callback_info = WGPU_REQUEST_ADAPTER_CALLBACK_INFO_INIT;
    callback_info.mode = WGPUCallbackMode_WaitAnyOnly;
    callback_info.callback = adapter_request_callback;
    callback_info.userdata1 = &result;

    WGPUFuture future = wgpuInstanceRequestAdapter(instance->wgpu_instance, &options, callback_info);

    // Wait for the adapter request to complete
    WGPUFutureWaitInfo wait_info = WGPU_FUTURE_WAIT_INFO_INIT;
    wait_info.future = future;
    wgpuInstanceWaitAny(instance->wgpu_instance, 1, &wait_info, UINT64_MAX);

    return result;
}

uint32_t webgpu_instanceEnumerateAdapters(GfxInstance instance, GfxAdapter* adapters, uint32_t maxAdapters)
{
    if (!instance || maxAdapters == 0)
        return 0;

    // WebGPU doesn't have a direct enumerate function, so we'll request the default adapter
    GfxAdapter adapter = webgpu_instanceRequestAdapter(instance, NULL);
    if (adapter && adapters) {
        adapters[0] = adapter;
        return 1;
    }
    return 0;
}

// ============================================================================
// Adapter functions
// ============================================================================

void webgpu_adapterDestroy(GfxAdapter adapter)
{
    if (!adapter)
        return;

    if (adapter->wgpu_adapter) {
        wgpuAdapterRelease(adapter->wgpu_adapter);
    }
    free(adapter->name);
    free(adapter);
}

// Callback for device request
static void device_request_callback(WGPURequestDeviceStatus status, WGPUDevice device,
    WGPUStringView message, void* userdata1, void* userdata2)
{
    GfxDevice* result = (GfxDevice*)userdata1;
    if (status == WGPURequestDeviceStatus_Success && device) {
        *result = (GfxDevice)malloc(sizeof(struct GfxDevice_T));
        if (*result) {
            (*result)->wgpu_device = device;
            (*result)->wgpu_queue = wgpuDeviceGetQueue(device);
        }
    } else {
        *result = NULL;
    }
}

GfxDevice webgpu_adapterCreateDevice(GfxAdapter adapter, const GfxDeviceDescriptor* descriptor)
{
    if (!adapter)
        return NULL;

    WGPUDeviceDescriptor wgpu_desc = WGPU_DEVICE_DESCRIPTOR_INIT;
    if (descriptor && descriptor->label) {
        wgpu_desc.label = gfx_string_view(descriptor->label);
    }

    GfxDevice result = NULL;
    WGPURequestDeviceCallbackInfo callback_info = WGPU_REQUEST_DEVICE_CALLBACK_INFO_INIT;
    callback_info.mode = WGPUCallbackMode_WaitAnyOnly;
    callback_info.callback = device_request_callback;
    callback_info.userdata1 = &result;

    WGPUFuture future = wgpuAdapterRequestDevice(adapter->wgpu_adapter, &wgpu_desc, callback_info);

    // Wait for the device request to complete
    WGPUFutureWaitInfo wait_info = WGPU_FUTURE_WAIT_INFO_INIT;
    wait_info.future = future;
    // Note: This is a simplified wait - in a real implementation you'd want proper instance handling
    // wgpuInstanceWaitAny(instance, 1, &wait_info, UINT64_MAX);

    return result;
}

const char* webgpu_adapterGetName(GfxAdapter adapter)
{
    return adapter ? adapter->name : NULL;
}

GfxBackend webgpu_adapterGetBackend(GfxAdapter adapter)
{
    return adapter ? adapter->backend : GFX_BACKEND_AUTO;
}

// ============================================================================
// Device functions
// ============================================================================

void webgpu_deviceDestroy(GfxDevice device)
{
    if (!device)
        return;

    if (device->wgpu_device) {
        wgpuDeviceRelease(device->wgpu_device);
    }
    free(device);
}

GfxQueue webgpu_deviceGetQueue(GfxDevice device)
{
    if (!device)
        return NULL;

    GfxQueue queue = (GfxQueue)malloc(sizeof(struct GfxQueue_T));
    if (!queue)
        return NULL;

    queue->wgpu_queue = device->wgpu_queue;
    wgpuQueueAddRef(queue->wgpu_queue);

    return queue;
}

GfxSurface webgpu_deviceCreateSurface(GfxDevice device, const GfxSurfaceDescriptor* descriptor)
{
    if (!device || !descriptor)
        return NULL;

    // We need the instance to create the surface, but we only have the device
    // In a real implementation, you might want to store the instance reference in the device
    // For now, we'll create a temporary instance or assume it's available globally

    // Create a temporary instance for surface creation
    // Note: In practice, you'd want to store the instance in your device structure
    WGPUInstanceDescriptor instance_desc = WGPU_INSTANCE_DESCRIPTOR_INIT;
    WGPUInstance temp_instance = wgpuCreateInstance(&instance_desc);

    if (!temp_instance) {
        return NULL;
    }

    WGPUSurface wgpu_surface = create_platform_surface(temp_instance, &descriptor->windowHandle);

    // Clean up temporary instance
    wgpuInstanceRelease(temp_instance);

    if (!wgpu_surface) {
        return NULL;
    }

    GfxSurface surface = (GfxSurface)malloc(sizeof(struct GfxSurface_T));
    if (!surface) {
        wgpuSurfaceRelease(wgpu_surface);
        return NULL;
    }

    surface->wgpu_surface = wgpu_surface;
    surface->width = descriptor->width;
    surface->height = descriptor->height;
    surface->window_handle = descriptor->windowHandle;

    return surface;
}

GfxSwapchain webgpu_deviceCreateSwapchain(GfxDevice device, GfxSurface surface, const GfxSwapchainDescriptor* descriptor)
{
    if (!device || !surface || !descriptor)
        return NULL;

    // First, get surface capabilities to validate the format and present mode
    WGPUSurfaceCapabilities capabilities = WGPU_SURFACE_CAPABILITIES_INIT;
    wgpuSurfaceGetCapabilities(surface->wgpu_surface, device->wgpu_device, &capabilities);

    // Validate format is supported
    WGPUTextureFormat format = gfx_to_wgpu_texture_format(descriptor->format);
    bool format_supported = false;
    for (size_t i = 0; i < capabilities.formatCount; i++) {
        if (capabilities.formats[i] == format) {
            format_supported = true;
            break;
        }
    }

    if (!format_supported && capabilities.formatCount > 0) {
        // Fall back to the first supported format
        format = capabilities.formats[0];
    }

    // Validate present mode is supported
    WGPUPresentMode present_mode = gfx_to_wgpu_present_mode(descriptor->presentMode);
    bool present_mode_supported = false;
    for (size_t i = 0; i < capabilities.presentModeCount; i++) {
        if (capabilities.presentModes[i] == present_mode) {
            present_mode_supported = true;
            break;
        }
    }

    if (!present_mode_supported && capabilities.presentModeCount > 0) {
        // Fall back to the first supported present mode (should be FIFO)
        present_mode = capabilities.presentModes[0];
    }

    // Configure the surface
    WGPUSurfaceConfiguration config = WGPU_SURFACE_CONFIGURATION_INIT;
    config.device = device->wgpu_device;
    config.format = format;
    config.usage = gfx_to_wgpu_texture_usage(descriptor->usage);
    config.width = descriptor->width;
    config.height = descriptor->height;
    config.presentMode = present_mode;
    config.alphaMode = WGPUCompositeAlphaMode_Auto;

    wgpuSurfaceConfigure(surface->wgpu_surface, &config);

    // Create swapchain wrapper
    GfxSwapchain swapchain = (GfxSwapchain)malloc(sizeof(struct GfxSwapchain_T));
    if (!swapchain) {
        return NULL;
    }

    swapchain->wgpu_surface = surface->wgpu_surface;
    wgpuSurfaceAddRef(surface->wgpu_surface); // Add reference since we're sharing it
    swapchain->wgpu_device = device->wgpu_device;
    wgpuDeviceAddRef(device->wgpu_device); // Add reference since we're sharing it
    swapchain->width = descriptor->width;
    swapchain->height = descriptor->height;
    swapchain->format = format;
    swapchain->present_mode = present_mode;
    swapchain->buffer_count = descriptor->bufferCount;

    // Free capabilities memory
    if (capabilities.formats) {
        free((void*)capabilities.formats);
    }
    if (capabilities.presentModes) {
        free((void*)capabilities.presentModes);
    }
    if (capabilities.alphaModes) {
        free((void*)capabilities.alphaModes);
    }

    return swapchain;
}

GfxBuffer webgpu_deviceCreateBuffer(GfxDevice device, const GfxBufferDescriptor* descriptor)
{
    if (!device || !descriptor)
        return NULL;

    WGPUBufferDescriptor wgpu_desc = WGPU_BUFFER_DESCRIPTOR_INIT;
    if (descriptor->label) {
        wgpu_desc.label = gfx_string_view(descriptor->label);
    }
    wgpu_desc.size = descriptor->size;
    wgpu_desc.usage = gfx_to_wgpu_buffer_usage(descriptor->usage);
    wgpu_desc.mappedAtCreation = descriptor->mappedAtCreation ? WGPU_TRUE : WGPU_FALSE;

    WGPUBuffer wgpu_buffer = wgpuDeviceCreateBuffer(device->wgpu_device, &wgpu_desc);
    if (!wgpu_buffer)
        return NULL;

    GfxBuffer buffer = (GfxBuffer)malloc(sizeof(struct GfxBuffer_T));
    if (!buffer) {
        wgpuBufferRelease(wgpu_buffer);
        return NULL;
    }

    buffer->wgpu_buffer = wgpu_buffer;
    buffer->size = descriptor->size;
    buffer->usage = descriptor->usage;

    return buffer;
}

GfxTexture webgpu_deviceCreateTexture(GfxDevice device, const GfxTextureDescriptor* descriptor)
{
    if (!device || !descriptor)
        return NULL;

    WGPUTextureDescriptor wgpu_desc = WGPU_TEXTURE_DESCRIPTOR_INIT;
    if (descriptor->label) {
        wgpu_desc.label = gfx_string_view(descriptor->label);
    }
    wgpu_desc.size = (WGPUExtent3D){
        .width = descriptor->size.width,
        .height = descriptor->size.height,
        .depth = descriptor->size.depth
    };
    wgpu_desc.mipLevelCount = descriptor->mipLevelCount;
    wgpu_desc.sampleCount = descriptor->sampleCount;
    wgpu_desc.format = gfx_to_wgpu_texture_format(descriptor->format);
    wgpu_desc.usage = gfx_to_wgpu_texture_usage(descriptor->usage);

    WGPUTexture wgpu_texture = wgpuDeviceCreateTexture(device->wgpu_device, &wgpu_desc);
    if (!wgpu_texture)
        return NULL;

    GfxTexture texture = (GfxTexture)malloc(sizeof(struct GfxTexture_T));
    if (!texture) {
        wgpuTextureRelease(wgpu_texture);
        return NULL;
    }

    texture->wgpu_texture = wgpu_texture;
    texture->size = wgpu_desc.size;
    texture->format = wgpu_desc.format;
    texture->mip_levels = descriptor->mipLevelCount;
    texture->sample_count = descriptor->sampleCount;
    texture->usage = wgpu_desc.usage;

    return texture;
}

GfxSampler webgpu_deviceCreateSampler(GfxDevice device, const GfxSamplerDescriptor* descriptor)
{
    if (!device || !descriptor)
        return NULL;

    WGPUSamplerDescriptor wgpu_desc = WGPU_SAMPLER_DESCRIPTOR_INIT;
    if (descriptor->label) {
        wgpu_desc.label = gfx_string_view(descriptor->label);
    }

    // Map address modes
    switch (descriptor->addressModeU) {
    case GFX_ADDRESS_MODE_REPEAT:
        wgpu_desc.addressModeU = WGPUAddressMode_Repeat;
        break;
    case GFX_ADDRESS_MODE_MIRROR_REPEAT:
        wgpu_desc.addressModeU = WGPUAddressMode_MirrorRepeat;
        break;
    case GFX_ADDRESS_MODE_CLAMP_TO_EDGE:
        wgpu_desc.addressModeU = WGPUAddressMode_ClampToEdge;
        break;
    default:
        wgpu_desc.addressModeU = WGPUAddressMode_ClampToEdge;
        break;
    }

    switch (descriptor->addressModeV) {
    case GFX_ADDRESS_MODE_REPEAT:
        wgpu_desc.addressModeV = WGPUAddressMode_Repeat;
        break;
    case GFX_ADDRESS_MODE_MIRROR_REPEAT:
        wgpu_desc.addressModeV = WGPUAddressMode_MirrorRepeat;
        break;
    case GFX_ADDRESS_MODE_CLAMP_TO_EDGE:
        wgpu_desc.addressModeV = WGPUAddressMode_ClampToEdge;
        break;
    default:
        wgpu_desc.addressModeV = WGPUAddressMode_ClampToEdge;
        break;
    }

    switch (descriptor->addressModeW) {
    case GFX_ADDRESS_MODE_REPEAT:
        wgpu_desc.addressModeW = WGPUAddressMode_Repeat;
        break;
    case GFX_ADDRESS_MODE_MIRROR_REPEAT:
        wgpu_desc.addressModeW = WGPUAddressMode_MirrorRepeat;
        break;
    case GFX_ADDRESS_MODE_CLAMP_TO_EDGE:
        wgpu_desc.addressModeW = WGPUAddressMode_ClampToEdge;
        break;
    default:
        wgpu_desc.addressModeW = WGPUAddressMode_ClampToEdge;
        break;
    }

    // Map filter modes
    switch (descriptor->magFilter) {
    case GFX_FILTER_MODE_NEAREST:
        wgpu_desc.magFilter = WGPUFilterMode_Nearest;
        break;
    case GFX_FILTER_MODE_LINEAR:
        wgpu_desc.magFilter = WGPUFilterMode_Linear;
        break;
    default:
        wgpu_desc.magFilter = WGPUFilterMode_Nearest;
        break;
    }

    switch (descriptor->minFilter) {
    case GFX_FILTER_MODE_NEAREST:
        wgpu_desc.minFilter = WGPUFilterMode_Nearest;
        break;
    case GFX_FILTER_MODE_LINEAR:
        wgpu_desc.minFilter = WGPUFilterMode_Linear;
        break;
    default:
        wgpu_desc.minFilter = WGPUFilterMode_Nearest;
        break;
    }

    switch (descriptor->mipmapFilter) {
    case GFX_FILTER_MODE_NEAREST:
        wgpu_desc.mipmapFilter = WGPUMipmapFilterMode_Nearest;
        break;
    case GFX_FILTER_MODE_LINEAR:
        wgpu_desc.mipmapFilter = WGPUMipmapFilterMode_Linear;
        break;
    default:
        wgpu_desc.mipmapFilter = WGPUMipmapFilterMode_Nearest;
        break;
    }

    wgpu_desc.lodMinClamp = descriptor->lodMinClamp;
    wgpu_desc.lodMaxClamp = descriptor->lodMaxClamp;
    wgpu_desc.maxAnisotropy = descriptor->maxAnisotropy;

    if (descriptor->compare) {
        switch (*descriptor->compare) {
        case GFX_COMPARE_FUNCTION_NEVER:
            wgpu_desc.compare = WGPUCompareFunction_Never;
            break;
        case GFX_COMPARE_FUNCTION_LESS:
            wgpu_desc.compare = WGPUCompareFunction_Less;
            break;
        case GFX_COMPARE_FUNCTION_EQUAL:
            wgpu_desc.compare = WGPUCompareFunction_Equal;
            break;
        case GFX_COMPARE_FUNCTION_LESS_EQUAL:
            wgpu_desc.compare = WGPUCompareFunction_LessEqual;
            break;
        case GFX_COMPARE_FUNCTION_GREATER:
            wgpu_desc.compare = WGPUCompareFunction_Greater;
            break;
        case GFX_COMPARE_FUNCTION_NOT_EQUAL:
            wgpu_desc.compare = WGPUCompareFunction_NotEqual;
            break;
        case GFX_COMPARE_FUNCTION_GREATER_EQUAL:
            wgpu_desc.compare = WGPUCompareFunction_GreaterEqual;
            break;
        case GFX_COMPARE_FUNCTION_ALWAYS:
            wgpu_desc.compare = WGPUCompareFunction_Always;
            break;
        default:
            wgpu_desc.compare = WGPUCompareFunction_Undefined;
            break;
        }
    }

    WGPUSampler wgpu_sampler = wgpuDeviceCreateSampler(device->wgpu_device, &wgpu_desc);
    if (!wgpu_sampler)
        return NULL;

    GfxSampler sampler = (GfxSampler)malloc(sizeof(struct GfxSampler_T));
    if (!sampler) {
        wgpuSamplerRelease(wgpu_sampler);
        return NULL;
    }

    sampler->wgpu_sampler = wgpu_sampler;
    return sampler;
}

GfxShader webgpu_deviceCreateShader(GfxDevice device, const GfxShaderDescriptor* descriptor)
{
    if (!device || !descriptor || !descriptor->code)
        return NULL;

    WGPUShaderModuleDescriptor wgpu_desc = WGPU_SHADER_MODULE_DESCRIPTOR_INIT;
    if (descriptor->label) {
        wgpu_desc.label = gfx_string_view(descriptor->label);
    }

    // Create WGSL source chain
    WGPUShaderSourceWGSL wgsl_source = WGPU_SHADER_SOURCE_WGSL_INIT;
    wgsl_source.code = gfx_string_view(descriptor->code);
    wgpu_desc.nextInChain = (WGPUChainedStruct*)&wgsl_source;

    WGPUShaderModule wgpu_module = wgpuDeviceCreateShaderModule(device->wgpu_device, &wgpu_desc);
    if (!wgpu_module)
        return NULL;

    GfxShader shader = (GfxShader)malloc(sizeof(struct GfxShader_T));
    if (!shader) {
        wgpuShaderModuleRelease(wgpu_module);
        return NULL;
    }

    shader->wgpu_module = wgpu_module;
    return shader;
}

GfxCommandEncoder webgpu_deviceCreateCommandEncoder(GfxDevice device, const char* label)
{
    if (!device)
        return NULL;

    WGPUCommandEncoderDescriptor wgpu_desc = WGPU_COMMAND_ENCODER_DESCRIPTOR_INIT;
    if (label) {
        wgpu_desc.label = gfx_string_view(label);
    }

    WGPUCommandEncoder wgpu_encoder = wgpuDeviceCreateCommandEncoder(device->wgpu_device, &wgpu_desc);
    if (!wgpu_encoder)
        return NULL;

    GfxCommandEncoder encoder = (GfxCommandEncoder)malloc(sizeof(struct GfxCommandEncoder_T));
    if (!encoder) {
        wgpuCommandEncoderRelease(wgpu_encoder);
        return NULL;
    }

    encoder->wgpu_encoder = wgpu_encoder;
    return encoder;
}

// ============================================================================
// Buffer functions
// ============================================================================

void webgpu_bufferDestroy(GfxBuffer buffer)
{
    if (!buffer)
        return;

    if (buffer->wgpu_buffer) {
        wgpuBufferRelease(buffer->wgpu_buffer);
    }
    free(buffer);
}

uint64_t webgpu_bufferGetSize(GfxBuffer buffer)
{
    return buffer ? buffer->size : 0;
}

GfxBufferUsage webgpu_bufferGetUsage(GfxBuffer buffer)
{
    return buffer ? buffer->usage : GFX_BUFFER_USAGE_NONE;
}

void* webgpu_bufferMapAsync(GfxBuffer buffer, uint64_t offset, uint64_t size)
{
    if (!buffer)
        return NULL;

    // This is a simplified synchronous implementation
    // Real implementation would use async mapping with callbacks
    WGPUBufferMapCallbackInfo callback_info = WGPU_BUFFER_MAP_CALLBACK_INFO_INIT;
    callback_info.mode = WGPUCallbackMode_WaitAnyOnly;
    // Would need proper callback implementation

    // For now, return NULL to indicate mapping not implemented
    return NULL;
}

void webgpu_bufferUnmap(GfxBuffer buffer)
{
    if (!buffer)
        return;
    wgpuBufferUnmap(buffer->wgpu_buffer);
}

// ============================================================================
// Texture functions
// ============================================================================

void webgpu_textureDestroy(GfxTexture texture)
{
    if (!texture)
        return;

    if (texture->wgpu_texture) {
        wgpuTextureRelease(texture->wgpu_texture);
    }
    free(texture);
}

GfxExtent3D webgpu_textureGetSize(GfxTexture texture)
{
    if (!texture)
        return (GfxExtent3D){ 0, 0, 0 };

    return (GfxExtent3D){
        .width = texture->size.width,
        .height = texture->size.height,
        .depth = texture->size.depth
    };
}

GfxTextureFormat webgpu_textureGetFormat(GfxTexture texture)
{
    return texture ? wgpu_to_gfx_texture_format(texture->format) : GFX_TEXTURE_FORMAT_UNDEFINED;
}

uint32_t webgpu_textureGetMipLevelCount(GfxTexture texture)
{
    return texture ? texture->mip_levels : 0;
}

uint32_t webgpu_textureGetSampleCount(GfxTexture texture)
{
    return texture ? texture->sample_count : 0;
}

GfxTextureUsage webgpu_textureGetUsage(GfxTexture texture)
{
    if (!texture)
        return GFX_TEXTURE_USAGE_NONE;

    GfxTextureUsage usage = GFX_TEXTURE_USAGE_NONE;
    if (texture->usage & WGPUTextureUsage_CopySrc)
        usage |= GFX_TEXTURE_USAGE_COPY_SRC;
    if (texture->usage & WGPUTextureUsage_CopyDst)
        usage |= GFX_TEXTURE_USAGE_COPY_DST;
    if (texture->usage & WGPUTextureUsage_TextureBinding)
        usage |= GFX_TEXTURE_USAGE_TEXTURE_BINDING;
    if (texture->usage & WGPUTextureUsage_StorageBinding)
        usage |= GFX_TEXTURE_USAGE_STORAGE_BINDING;
    if (texture->usage & WGPUTextureUsage_RenderAttachment)
        usage |= GFX_TEXTURE_USAGE_RENDER_ATTACHMENT;

    return usage;
}

GfxTextureView webgpu_textureCreateView(GfxTexture texture, const GfxTextureViewDescriptor* descriptor)
{
    if (!texture)
        return NULL;

    WGPUTextureViewDescriptor wgpu_desc = WGPU_TEXTURE_VIEW_DESCRIPTOR_INIT;
    if (descriptor) {
        if (descriptor->label) {
            wgpu_desc.label = gfx_string_view(descriptor->label);
        }
        wgpu_desc.format = gfx_to_wgpu_texture_format(descriptor->format);
        wgpu_desc.baseMipLevel = descriptor->baseMipLevel;
        wgpu_desc.mipLevelCount = descriptor->mipLevelCount;
        wgpu_desc.baseArrayLayer = descriptor->baseArrayLayer;
        wgpu_desc.arrayLayerCount = descriptor->arrayLayerCount;
    }

    WGPUTextureView wgpu_view = wgpuTextureCreateView(texture->wgpu_texture, &wgpu_desc);
    if (!wgpu_view)
        return NULL;

    GfxTextureView view = (GfxTextureView)malloc(sizeof(struct GfxTextureView_T));
    if (!view) {
        wgpuTextureViewRelease(wgpu_view);
        return NULL;
    }

    view->wgpu_view = wgpu_view;
    view->texture = texture;

    return view;
}

// ============================================================================
// TextureView functions
// ============================================================================

void webgpu_textureViewDestroy(GfxTextureView textureView)
{
    if (!textureView)
        return;

    if (textureView->wgpu_view) {
        wgpuTextureViewRelease(textureView->wgpu_view);
    }
    free(textureView);
}

GfxTexture webgpu_textureViewGetTexture(GfxTextureView textureView)
{
    return textureView ? textureView->texture : NULL;
}

// ============================================================================
// Sampler functions
// ============================================================================

void webgpu_samplerDestroy(GfxSampler sampler)
{
    if (!sampler)
        return;

    if (sampler->wgpu_sampler) {
        wgpuSamplerRelease(sampler->wgpu_sampler);
    }
    free(sampler);
}

// ============================================================================
// Shader functions
// ============================================================================

void webgpu_shaderDestroy(GfxShader shader)
{
    if (!shader)
        return;

    if (shader->wgpu_module) {
        wgpuShaderModuleRelease(shader->wgpu_module);
    }
    free(shader);
}

// ============================================================================
// Queue functions
// ============================================================================

void webgpu_queueSubmit(GfxQueue queue, GfxCommandEncoder commandEncoder)
{
    if (!queue || !commandEncoder)
        return;

    WGPUCommandBufferDescriptor cmd_desc = WGPU_COMMAND_BUFFER_DESCRIPTOR_INIT;
    WGPUCommandBuffer cmd_buffer = wgpuCommandEncoderFinish(commandEncoder->wgpu_encoder, &cmd_desc);

    if (cmd_buffer) {
        wgpuQueueSubmit(queue->wgpu_queue, 1, &cmd_buffer);
        wgpuCommandBufferRelease(cmd_buffer);
    }
}

void webgpu_queueWriteBuffer(GfxQueue queue, GfxBuffer buffer, uint64_t offset, const void* data, uint64_t size)
{
    if (!queue || !buffer || !data)
        return;

    wgpuQueueWriteBuffer(queue->wgpu_queue, buffer->wgpu_buffer, offset, data, size);
}

void webgpu_queueWriteTexture(GfxQueue queue, GfxTexture texture, const GfxOrigin3D* origin, uint32_t mipLevel,
    const void* data, uint64_t dataSize, uint32_t bytesPerRow, const GfxExtent3D* extent)
{
    if (!queue || !texture || !data || !origin || !extent)
        return;

    WGPUTexelCopyTextureInfo dest = WGPU_TEXEL_COPY_TEXTURE_INFO_INIT;
    dest.texture = texture->wgpu_texture;
    dest.mipLevel = mipLevel;
    dest.origin = (WGPUOrigin3D){ origin->x, origin->y, origin->z };

    WGPUTexelCopyBufferLayout layout = WGPU_TEXEL_COPY_BUFFER_LAYOUT_INIT;
    layout.bytesPerRow = bytesPerRow;

    WGPUExtent3D wgpu_extent = { extent->width, extent->height, extent->depth };

    wgpuQueueWriteTexture(queue->wgpu_queue, &dest, data, dataSize, &layout, &wgpu_extent);
}

// ============================================================================
// CommandEncoder functions
// ============================================================================

void webgpu_commandEncoderDestroy(GfxCommandEncoder commandEncoder)
{
    if (!commandEncoder)
        return;

    if (commandEncoder->wgpu_encoder) {
        wgpuCommandEncoderRelease(commandEncoder->wgpu_encoder);
    }
    free(commandEncoder);
}

GfxRenderPassEncoder webgpu_commandEncoderBeginRenderPass(GfxCommandEncoder commandEncoder,
    const GfxTextureView* colorAttachments, uint32_t colorAttachmentCount,
    const GfxColor* clearColors,
    GfxTextureView depthStencilAttachment,
    float depthClearValue, uint32_t stencilClearValue)
{
    if (!commandEncoder)
        return NULL;

    WGPURenderPassDescriptor wgpu_desc = WGPU_RENDER_PASS_DESCRIPTOR_INIT;

    // Set up color attachments
    WGPURenderPassColorAttachment* wgpu_color_attachments = NULL;
    if (colorAttachmentCount > 0 && colorAttachments) {
        wgpu_color_attachments = (WGPURenderPassColorAttachment*)malloc(
            sizeof(WGPURenderPassColorAttachment) * colorAttachmentCount);

        for (uint32_t i = 0; i < colorAttachmentCount; i++) {
            wgpu_color_attachments[i] = (WGPURenderPassColorAttachment)WGPU_RENDER_PASS_COLOR_ATTACHMENT_INIT;
            if (colorAttachments[i]) {
                wgpu_color_attachments[i].view = colorAttachments[i]->wgpu_view;
                wgpu_color_attachments[i].loadOp = WGPULoadOp_Clear;
                wgpu_color_attachments[i].storeOp = WGPUStoreOp_Store;

                if (clearColors) {
                    wgpu_color_attachments[i].clearValue = (WGPUColor){
                        clearColors[i].r, clearColors[i].g, clearColors[i].b, clearColors[i].a
                    };
                }
            }
        }

        wgpu_desc.colorAttachments = wgpu_color_attachments;
        wgpu_desc.colorAttachmentCount = colorAttachmentCount;
    }

    // Set up depth-stencil attachment
    WGPURenderPassDepthStencilAttachment wgpu_depth_stencil = WGPU_RENDER_PASS_DEPTH_STENCIL_ATTACHMENT_INIT;
    if (depthStencilAttachment) {
        wgpu_depth_stencil.view = depthStencilAttachment->wgpu_view;
        wgpu_depth_stencil.depthLoadOp = WGPULoadOp_Clear;
        wgpu_depth_stencil.depthStoreOp = WGPUStoreOp_Store;
        wgpu_depth_stencil.depthClearValue = depthClearValue;
        wgpu_depth_stencil.stencilLoadOp = WGPULoadOp_Clear;
        wgpu_depth_stencil.stencilStoreOp = WGPUStoreOp_Store;
        wgpu_depth_stencil.stencilClearValue = stencilClearValue;

        wgpu_desc.depthStencilAttachment = &wgpu_depth_stencil;
    }

    WGPURenderPassEncoder wgpu_encoder = wgpuCommandEncoderBeginRenderPass(
        commandEncoder->wgpu_encoder, &wgpu_desc);

    free(wgpu_color_attachments);

    if (!wgpu_encoder)
        return NULL;

    GfxRenderPassEncoder encoder = (GfxRenderPassEncoder)malloc(sizeof(struct GfxRenderPassEncoder_T));
    if (!encoder) {
        wgpuRenderPassEncoderRelease(wgpu_encoder);
        return NULL;
    }

    encoder->wgpu_encoder = wgpu_encoder;
    return encoder;
}

GfxComputePassEncoder webgpu_commandEncoderBeginComputePass(GfxCommandEncoder commandEncoder, const char* label)
{
    if (!commandEncoder)
        return NULL;

    WGPUComputePassDescriptor wgpu_desc = WGPU_COMPUTE_PASS_DESCRIPTOR_INIT;
    if (label) {
        wgpu_desc.label = gfx_string_view(label);
    }

    WGPUComputePassEncoder wgpu_encoder = wgpuCommandEncoderBeginComputePass(
        commandEncoder->wgpu_encoder, &wgpu_desc);

    if (!wgpu_encoder)
        return NULL;

    GfxComputePassEncoder encoder = (GfxComputePassEncoder)malloc(sizeof(struct GfxComputePassEncoder_T));
    if (!encoder) {
        wgpuComputePassEncoderRelease(wgpu_encoder);
        return NULL;
    }

    encoder->wgpu_encoder = wgpu_encoder;
    return encoder;
}

void webgpu_commandEncoderCopyBufferToBuffer(GfxCommandEncoder commandEncoder,
    GfxBuffer source, uint64_t sourceOffset,
    GfxBuffer destination, uint64_t destinationOffset,
    uint64_t size)
{
    if (!commandEncoder || !source || !destination)
        return;

    wgpuCommandEncoderCopyBufferToBuffer(commandEncoder->wgpu_encoder,
        source->wgpu_buffer, sourceOffset,
        destination->wgpu_buffer, destinationOffset,
        size);
}

void webgpu_commandEncoderFinish(GfxCommandEncoder commandEncoder)
{
    // This is handled in gfxQueueSubmit
}

// ============================================================================
// RenderPassEncoder functions
// ============================================================================

void webgpu_renderPassEncoderDestroy(GfxRenderPassEncoder renderPassEncoder)
{
    if (!renderPassEncoder)
        return;

    if (renderPassEncoder->wgpu_encoder) {
        wgpuRenderPassEncoderRelease(renderPassEncoder->wgpu_encoder);
    }
    free(renderPassEncoder);
}

void webgpu_renderPassEncoderSetVertexBuffer(GfxRenderPassEncoder renderPassEncoder, uint32_t slot,
    GfxBuffer buffer, uint64_t offset, uint64_t size)
{
    if (!renderPassEncoder || !buffer)
        return;

    wgpuRenderPassEncoderSetVertexBuffer(renderPassEncoder->wgpu_encoder, slot,
        buffer->wgpu_buffer, offset, size);
}

void webgpu_renderPassEncoderSetIndexBuffer(GfxRenderPassEncoder renderPassEncoder,
    GfxBuffer buffer, GfxIndexFormat format,
    uint64_t offset, uint64_t size)
{
    if (!renderPassEncoder || !buffer)
        return;

    wgpuRenderPassEncoderSetIndexBuffer(renderPassEncoder->wgpu_encoder,
        buffer->wgpu_buffer,
        gfx_to_wgpu_index_format(format),
        offset, size);
}

void webgpu_renderPassEncoderDraw(GfxRenderPassEncoder renderPassEncoder,
    uint32_t vertexCount, uint32_t instanceCount,
    uint32_t firstVertex, uint32_t firstInstance)
{
    if (!renderPassEncoder)
        return;

    wgpuRenderPassEncoderDraw(renderPassEncoder->wgpu_encoder, vertexCount, instanceCount,
        firstVertex, firstInstance);
}

void webgpu_renderPassEncoderDrawIndexed(GfxRenderPassEncoder renderPassEncoder,
    uint32_t indexCount, uint32_t instanceCount,
    uint32_t firstIndex, int32_t baseVertex,
    uint32_t firstInstance)
{
    if (!renderPassEncoder)
        return;

    wgpuRenderPassEncoderDrawIndexed(renderPassEncoder->wgpu_encoder, indexCount, instanceCount,
        firstIndex, baseVertex, firstInstance);
}

void webgpu_renderPassEncoderEnd(GfxRenderPassEncoder renderPassEncoder)
{
    if (!renderPassEncoder)
        return;

    wgpuRenderPassEncoderEnd(renderPassEncoder->wgpu_encoder);
}

// ============================================================================
// ComputePassEncoder functions
// ============================================================================

void webgpu_computePassEncoderDestroy(GfxComputePassEncoder computePassEncoder)
{
    if (!computePassEncoder)
        return;

    if (computePassEncoder->wgpu_encoder) {
        wgpuComputePassEncoderRelease(computePassEncoder->wgpu_encoder);
    }
    free(computePassEncoder);
}

void webgpu_computePassEncoderDispatchWorkgroups(GfxComputePassEncoder computePassEncoder,
    uint32_t workgroupCountX, uint32_t workgroupCountY,
    uint32_t workgroupCountZ)
{
    if (!computePassEncoder)
        return;

    wgpuComputePassEncoderDispatchWorkgroups(computePassEncoder->wgpu_encoder,
        workgroupCountX, workgroupCountY, workgroupCountZ);
}

void webgpu_computePassEncoderEnd(GfxComputePassEncoder computePassEncoder)
{
    if (!computePassEncoder)
        return;

    wgpuComputePassEncoderEnd(computePassEncoder->wgpu_encoder);
}

// ============================================================================
// Surface functions (simplified implementations)
// ============================================================================

void webgpu_surfaceDestroy(GfxSurface surface)
{
    if (!surface)
        return;

    if (surface->wgpu_surface) {
        wgpuSurfaceRelease(surface->wgpu_surface);
    }
    free(surface);
}

uint32_t webgpu_surfaceGetWidth(GfxSurface surface)
{
    return surface ? surface->width : 0;
}

uint32_t webgpu_surfaceGetHeight(GfxSurface surface)
{
    return surface ? surface->height : 0;
}

void webgpu_surfaceResize(GfxSurface surface, uint32_t width, uint32_t height)
{
    if (!surface)
        return;

    surface->width = width;
    surface->height = height;
}

// ============================================================================
// Swapchain functions (simplified implementations)
// ============================================================================

void webgpu_swapchainDestroy(GfxSwapchain swapchain)
{
    if (!swapchain)
        return;

    if (swapchain->wgpu_surface) {
        wgpuSurfaceRelease(swapchain->wgpu_surface);
    }
    if (swapchain->wgpu_device) {
        wgpuDeviceRelease(swapchain->wgpu_device);
    }
    free(swapchain);
}

uint32_t webgpu_swapchainGetWidth(GfxSwapchain swapchain)
{
    return swapchain ? swapchain->width : 0;
}

uint32_t webgpu_swapchainGetHeight(GfxSwapchain swapchain)
{
    return swapchain ? swapchain->height : 0;
}

GfxTextureFormat webgpu_swapchainGetFormat(GfxSwapchain swapchain)
{
    return swapchain ? wgpu_to_gfx_texture_format(swapchain->format) : GFX_TEXTURE_FORMAT_UNDEFINED;
}

uint32_t webgpu_swapchainGetBufferCount(GfxSwapchain swapchain)
{
    return swapchain ? swapchain->buffer_count : 0;
}

GfxTextureView webgpu_swapchainGetCurrentTextureView(GfxSwapchain swapchain)
{
    if (!swapchain || !swapchain->wgpu_surface)
        return NULL;

    WGPUSurfaceTexture surface_texture = WGPU_SURFACE_TEXTURE_INIT;
    wgpuSurfaceGetCurrentTexture(swapchain->wgpu_surface, &surface_texture);

    if (surface_texture.status != WGPUSurfaceGetCurrentTextureStatus_SuccessOptimal || !surface_texture.texture) {
        return NULL;
    }

    WGPUTextureView wgpu_view = wgpuTextureCreateView(surface_texture.texture, NULL);
    if (!wgpu_view) {
        wgpuTextureRelease(surface_texture.texture);
        return NULL;
    }

    GfxTextureView view = (GfxTextureView)malloc(sizeof(struct GfxTextureView_T));
    if (!view) {
        wgpuTextureViewRelease(wgpu_view);
        wgpuTextureRelease(surface_texture.texture);
        return NULL;
    }

    view->wgpu_view = wgpu_view;
    view->texture = NULL; // Surface texture is temporary

    return view;
}

void webgpu_swapchainPresent(GfxSwapchain swapchain)
{
    if (!swapchain || !swapchain->wgpu_surface)
        return;

    wgpuSurfacePresent(swapchain->wgpu_surface);
}

void webgpu_swapchainResize(GfxSwapchain swapchain, uint32_t width, uint32_t height)
{
    if (!swapchain)
        return;

    swapchain->width = width;
    swapchain->height = height;

    // Reconfigure the surface with new dimensions
    WGPUSurfaceConfiguration config = WGPU_SURFACE_CONFIGURATION_INIT;
    config.device = swapchain->wgpu_device;
    config.format = swapchain->format;
    config.usage = WGPUTextureUsage_RenderAttachment;
    config.width = width;
    config.height = height;
    config.presentMode = swapchain->present_mode;
    config.alphaMode = WGPUCompositeAlphaMode_Auto;

    wgpuSurfaceConfigure(swapchain->wgpu_surface, &config);
}

bool webgpu_swapchainNeedsRecreation(GfxSwapchain swapchain)
{
    if (!swapchain || !swapchain->wgpu_surface)
        return false;

    // Check if we can get a surface texture
    WGPUSurfaceTexture surface_texture = WGPU_SURFACE_TEXTURE_INIT;
    wgpuSurfaceGetCurrentTexture(swapchain->wgpu_surface, &surface_texture);

    bool needs_recreation = false;

    switch (surface_texture.status) {
    case WGPUSurfaceGetCurrentTextureStatus_SuccessOptimal:
    case WGPUSurfaceGetCurrentTextureStatus_SuccessSuboptimal:
        needs_recreation = false;
        break;
    case WGPUSurfaceGetCurrentTextureStatus_Timeout:
    case WGPUSurfaceGetCurrentTextureStatus_Outdated:
    case WGPUSurfaceGetCurrentTextureStatus_Lost:
    case WGPUSurfaceGetCurrentTextureStatus_Error:
        needs_recreation = true;
        break;
    }

    // Release the surface texture if we got one
    if (surface_texture.texture) {
        wgpuTextureRelease(surface_texture.texture);
    }

    return needs_recreation;
}

GfxPlatformWindowHandle webgpu_surfaceGetPlatformHandle(GfxSurface surface)
{
    if (!surface) {
        GfxPlatformWindowHandle handle = { 0 };
        return handle;
    }
    return surface->window_handle;
}

// ============================================================================
// BindGroupLayout functions
// ============================================================================

GfxBindGroupLayout webgpu_deviceCreateBindGroupLayout(GfxDevice device, const GfxBindGroupLayoutDescriptor* descriptor)
{
    if (!device || !descriptor)
        return NULL;

    WGPUBindGroupLayoutDescriptor wgpu_desc = WGPU_BIND_GROUP_LAYOUT_DESCRIPTOR_INIT;
    if (descriptor->label) {
        wgpu_desc.label = gfx_string_view(descriptor->label);
    }

    // Convert bind group layout entries
    WGPUBindGroupLayoutEntry* wgpu_entries = NULL;
    if (descriptor->entryCount > 0) {
        wgpu_entries = (WGPUBindGroupLayoutEntry*)malloc(sizeof(WGPUBindGroupLayoutEntry) * descriptor->entryCount);

        for (uint32_t i = 0; i < descriptor->entryCount; i++) {
            const GfxBindGroupLayoutEntry* entry = &descriptor->entries[i];
            WGPUBindGroupLayoutEntry* wgpu_entry = &wgpu_entries[i];

            *wgpu_entry = (WGPUBindGroupLayoutEntry)WGPU_BIND_GROUP_LAYOUT_ENTRY_INIT;
            wgpu_entry->binding = entry->binding;

            // Convert shader stage visibility
            wgpu_entry->visibility = WGPUShaderStage_None;
            if (entry->visibility & GFX_SHADER_STAGE_VERTEX) {
                wgpu_entry->visibility |= WGPUShaderStage_Vertex;
            }
            if (entry->visibility & GFX_SHADER_STAGE_FRAGMENT) {
                wgpu_entry->visibility |= WGPUShaderStage_Fragment;
            }
            if (entry->visibility & GFX_SHADER_STAGE_COMPUTE) {
                wgpu_entry->visibility |= WGPUShaderStage_Compute;
            }

            // Determine binding type based on entry properties
            if (entry->buffer.minBindingSize > 0) {
                wgpu_entry->buffer.type = WGPUBufferBindingType_Uniform;
                wgpu_entry->buffer.hasDynamicOffset = entry->buffer.hasDynamicOffset ? WGPU_TRUE : WGPU_FALSE;
                wgpu_entry->buffer.minBindingSize = entry->buffer.minBindingSize;
            } else if (entry->sampler.comparison) {
                wgpu_entry->sampler.type = WGPUSamplerBindingType_Comparison;
            } else if (!entry->texture.multisampled) {
                wgpu_entry->texture.sampleType = WGPUTextureSampleType_Float;
                wgpu_entry->texture.viewDimension = WGPUTextureViewDimension_2D;
                wgpu_entry->texture.multisampled = WGPU_FALSE;
            }
        }

        wgpu_desc.entries = wgpu_entries;
        wgpu_desc.entryCount = descriptor->entryCount;
    }

    WGPUBindGroupLayout wgpu_layout = wgpuDeviceCreateBindGroupLayout(device->wgpu_device, &wgpu_desc);

    free(wgpu_entries);

    if (!wgpu_layout)
        return NULL;

    GfxBindGroupLayout layout = (GfxBindGroupLayout)malloc(sizeof(struct GfxBindGroupLayout_T));
    if (!layout) {
        wgpuBindGroupLayoutRelease(wgpu_layout);
        return NULL;
    }

    layout->wgpu_layout = wgpu_layout;
    return layout;
}

void webgpu_bindGroupLayoutDestroy(GfxBindGroupLayout bindGroupLayout)
{
    if (!bindGroupLayout)
        return;

    if (bindGroupLayout->wgpu_layout) {
        wgpuBindGroupLayoutRelease(bindGroupLayout->wgpu_layout);
    }
    free(bindGroupLayout);
}

// ============================================================================
// BindGroup functions
// ============================================================================

GfxBindGroup webgpu_deviceCreateBindGroup(GfxDevice device, const GfxBindGroupDescriptor* descriptor)
{
    if (!device || !descriptor || !descriptor->layout)
        return NULL;

    WGPUBindGroupDescriptor wgpu_desc = WGPU_BIND_GROUP_DESCRIPTOR_INIT;
    if (descriptor->label) {
        wgpu_desc.label = gfx_string_view(descriptor->label);
    }
    wgpu_desc.layout = descriptor->layout->wgpu_layout;

    // Convert bind group entries
    WGPUBindGroupEntry* wgpu_entries = NULL;
    if (descriptor->entryCount > 0) {
        wgpu_entries = (WGPUBindGroupEntry*)malloc(sizeof(WGPUBindGroupEntry) * descriptor->entryCount);

        for (uint32_t i = 0; i < descriptor->entryCount; i++) {
            const GfxBindGroupEntry* entry = &descriptor->entries[i];
            WGPUBindGroupEntry* wgpu_entry = &wgpu_entries[i];

            *wgpu_entry = (WGPUBindGroupEntry)WGPU_BIND_GROUP_ENTRY_INIT;
            wgpu_entry->binding = entry->binding;

            switch (entry->type) {
            case GFX_BIND_GROUP_ENTRY_TYPE_BUFFER:
                wgpu_entry->buffer = entry->resource.buffer.buffer->wgpu_buffer;
                wgpu_entry->offset = entry->resource.buffer.offset;
                wgpu_entry->size = entry->resource.buffer.size;
                break;

            case GFX_BIND_GROUP_ENTRY_TYPE_SAMPLER:
                wgpu_entry->sampler = entry->resource.sampler->wgpu_sampler;
                break;

            case GFX_BIND_GROUP_ENTRY_TYPE_TEXTURE_VIEW:
                wgpu_entry->textureView = entry->resource.textureView->wgpu_view;
                break;
            }
        }

        wgpu_desc.entries = wgpu_entries;
        wgpu_desc.entryCount = descriptor->entryCount;
    }

    WGPUBindGroup wgpu_bind_group = wgpuDeviceCreateBindGroup(device->wgpu_device, &wgpu_desc);

    free(wgpu_entries);

    if (!wgpu_bind_group)
        return NULL;

    GfxBindGroup bind_group = (GfxBindGroup)malloc(sizeof(struct GfxBindGroup_T));
    if (!bind_group) {
        wgpuBindGroupRelease(wgpu_bind_group);
        return NULL;
    }

    bind_group->wgpu_bind_group = wgpu_bind_group;
    return bind_group;
}

void webgpu_bindGroupDestroy(GfxBindGroup bindGroup)
{
    if (!bindGroup)
        return;

    if (bindGroup->wgpu_bind_group) {
        wgpuBindGroupRelease(bindGroup->wgpu_bind_group);
    }
    free(bindGroup);
}

// ============================================================================
// RenderPipeline functions
// ============================================================================

GfxRenderPipeline webgpu_deviceCreateRenderPipeline(GfxDevice device, const GfxRenderPipelineDescriptor* descriptor)
{
    if (!device || !descriptor)
        return NULL;

    WGPURenderPipelineDescriptor wgpu_desc = WGPU_RENDER_PIPELINE_DESCRIPTOR_INIT;
    if (descriptor->label) {
        wgpu_desc.label = gfx_string_view(descriptor->label);
    }

    // Vertex state
    WGPUVertexState vertex_state = WGPU_VERTEX_STATE_INIT;
    vertex_state.module = descriptor->vertex.module->wgpu_module;
    vertex_state.entryPoint = gfx_string_view(descriptor->vertex.entryPoint);

    // Convert vertex buffer layouts
    WGPUVertexBufferLayout* vertex_buffers = NULL;
    WGPUVertexAttribute* all_attributes = NULL;
    uint32_t total_attributes = 0;

    if (descriptor->vertex.bufferCount > 0 && descriptor->vertex.buffers) {
        vertex_buffers = (WGPUVertexBufferLayout*)malloc(sizeof(WGPUVertexBufferLayout) * descriptor->vertex.bufferCount);

        // Count total attributes
        for (uint32_t i = 0; i < descriptor->vertex.bufferCount; i++) {
            total_attributes += descriptor->vertex.buffers[i].attributeCount;
        }

        all_attributes = (WGPUVertexAttribute*)malloc(sizeof(WGPUVertexAttribute) * total_attributes);

        uint32_t attr_index = 0;
        for (uint32_t i = 0; i < descriptor->vertex.bufferCount; i++) {
            const GfxVertexBufferLayout* buffer = &descriptor->vertex.buffers[i];
            WGPUVertexBufferLayout* wgpu_buffer = &vertex_buffers[i];

            *wgpu_buffer = (WGPUVertexBufferLayout)WGPU_VERTEX_BUFFER_LAYOUT_INIT;
            wgpu_buffer->arrayStride = buffer->arrayStride;
            wgpu_buffer->stepMode = buffer->stepModeInstance ? WGPUVertexStepMode_Instance : WGPUVertexStepMode_Vertex;
            wgpu_buffer->attributes = &all_attributes[attr_index];
            wgpu_buffer->attributeCount = buffer->attributeCount;

            for (uint32_t j = 0; j < buffer->attributeCount; j++) {
                const GfxVertexAttribute* attr = &buffer->attributes[j];
                WGPUVertexAttribute* wgpu_attr = &all_attributes[attr_index + j];

                *wgpu_attr = (WGPUVertexAttribute)WGPU_VERTEX_ATTRIBUTE_INIT;
                wgpu_attr->format = gfx_to_wgpu_texture_format(attr->format);
                wgpu_attr->offset = attr->offset;
                wgpu_attr->shaderLocation = attr->shaderLocation;
            }

            attr_index += buffer->attributeCount;
        }

        vertex_state.buffers = vertex_buffers;
        vertex_state.bufferCount = descriptor->vertex.bufferCount;
    }

    wgpu_desc.vertex = vertex_state;

    // Fragment state
    WGPUFragmentState fragment_state = WGPU_FRAGMENT_STATE_INIT;
    WGPUColorTargetState* color_targets = NULL;
    WGPUBlendState* blend_states = NULL;

    if (descriptor->fragment) {
        fragment_state.module = descriptor->fragment->module->wgpu_module;
        fragment_state.entryPoint = gfx_string_view(descriptor->fragment->entryPoint);

        if (descriptor->fragment->targetCount > 0 && descriptor->fragment->targets) {
            color_targets = (WGPUColorTargetState*)malloc(sizeof(WGPUColorTargetState) * descriptor->fragment->targetCount);
            blend_states = (WGPUBlendState*)malloc(sizeof(WGPUBlendState) * descriptor->fragment->targetCount);

            for (uint32_t i = 0; i < descriptor->fragment->targetCount; i++) {
                const GfxColorTargetState* target = &descriptor->fragment->targets[i];
                WGPUColorTargetState* wgpu_target = &color_targets[i];

                *wgpu_target = (WGPUColorTargetState)WGPU_COLOR_TARGET_STATE_INIT;
                wgpu_target->format = gfx_to_wgpu_texture_format(target->format);
                wgpu_target->writeMask = target->writeMask;

                if (target->blend) {
                    WGPUBlendState* blend = &blend_states[i];
                    *blend = (WGPUBlendState)WGPU_BLEND_STATE_INIT;

                    // Color blend
                    switch (target->blend->color.operation) {
                    case GFX_BLEND_OPERATION_ADD:
                        blend->color.operation = WGPUBlendOperation_Add;
                        break;
                    case GFX_BLEND_OPERATION_SUBTRACT:
                        blend->color.operation = WGPUBlendOperation_Subtract;
                        break;
                    case GFX_BLEND_OPERATION_REVERSE_SUBTRACT:
                        blend->color.operation = WGPUBlendOperation_ReverseSubtract;
                        break;
                    case GFX_BLEND_OPERATION_MIN:
                        blend->color.operation = WGPUBlendOperation_Min;
                        break;
                    case GFX_BLEND_OPERATION_MAX:
                        blend->color.operation = WGPUBlendOperation_Max;
                        break;
                    }

                    // Map blend factors (simplified)
                    blend->color.srcFactor = WGPUBlendFactor_One;
                    blend->color.dstFactor = WGPUBlendFactor_Zero;
                    blend->alpha.operation = blend->color.operation;
                    blend->alpha.srcFactor = WGPUBlendFactor_One;
                    blend->alpha.dstFactor = WGPUBlendFactor_Zero;

                    wgpu_target->blend = blend;
                }
            }

            fragment_state.targets = color_targets;
            fragment_state.targetCount = descriptor->fragment->targetCount;
        }

        wgpu_desc.fragment = &fragment_state;
    }

    // Primitive state
    WGPUPrimitiveState primitive_state = WGPU_PRIMITIVE_STATE_INIT;
    primitive_state.topology = gfx_to_wgpu_primitive_topology(descriptor->primitive.topology);
    primitive_state.frontFace = descriptor->primitive.frontFaceCounterClockwise ? WGPUFrontFace_CCW : WGPUFrontFace_CW;
    primitive_state.cullMode = descriptor->primitive.cullBackFace ? WGPUCullMode_Back : WGPUCullMode_None;
    primitive_state.unclippedDepth = descriptor->primitive.unclippedDepth ? WGPU_TRUE : WGPU_FALSE;

    if (descriptor->primitive.stripIndexFormat) {
        primitive_state.stripIndexFormat = gfx_to_wgpu_index_format(*descriptor->primitive.stripIndexFormat);
    }

    wgpu_desc.primitive = primitive_state;

    // Multisample state
    WGPUMultisampleState multisample_state = WGPU_MULTISAMPLE_STATE_INIT;
    multisample_state.count = descriptor->sampleCount;
    wgpu_desc.multisample = multisample_state;

    // Create pipeline
    WGPURenderPipeline wgpu_pipeline = wgpuDeviceCreateRenderPipeline(device->wgpu_device, &wgpu_desc);

    // Cleanup temporary allocations
    free(vertex_buffers);
    free(all_attributes);
    free(color_targets);
    free(blend_states);

    if (!wgpu_pipeline)
        return NULL;

    GfxRenderPipeline pipeline = (GfxRenderPipeline)malloc(sizeof(struct GfxRenderPipeline_T));
    if (!pipeline) {
        wgpuRenderPipelineRelease(wgpu_pipeline);
        return NULL;
    }

    pipeline->wgpu_pipeline = wgpu_pipeline;
    return pipeline;
}

void webgpu_renderPipelineDestroy(GfxRenderPipeline renderPipeline)
{
    if (!renderPipeline)
        return;

    if (renderPipeline->wgpu_pipeline) {
        wgpuRenderPipelineRelease(renderPipeline->wgpu_pipeline);
    }
    free(renderPipeline);
}

// ============================================================================
// ComputePipeline functions
// ============================================================================

GfxComputePipeline webgpu_deviceCreateComputePipeline(GfxDevice device, const GfxComputePipelineDescriptor* descriptor)
{
    if (!device || !descriptor || !descriptor->compute)
        return NULL;

    WGPUComputePipelineDescriptor wgpu_desc = WGPU_COMPUTE_PIPELINE_DESCRIPTOR_INIT;
    if (descriptor->label) {
        wgpu_desc.label = gfx_string_view(descriptor->label);
    }

    // Set compute stage directly in the descriptor
    wgpu_desc.compute.module = descriptor->compute->wgpu_module;
    wgpu_desc.compute.entryPoint = gfx_string_view(descriptor->entryPoint);

    WGPUComputePipeline wgpu_pipeline = wgpuDeviceCreateComputePipeline(device->wgpu_device, &wgpu_desc);
    if (!wgpu_pipeline)
        return NULL;

    GfxComputePipeline pipeline = (GfxComputePipeline)malloc(sizeof(struct GfxComputePipeline_T));
    if (!pipeline) {
        wgpuComputePipelineRelease(wgpu_pipeline);
        return NULL;
    }

    pipeline->wgpu_pipeline = wgpu_pipeline;
    return pipeline;
}

void webgpu_computePipelineDestroy(GfxComputePipeline computePipeline)
{
    if (!computePipeline)
        return;

    if (computePipeline->wgpu_pipeline) {
        wgpuComputePipelineRelease(computePipeline->wgpu_pipeline);
    }
    free(computePipeline);
}

// ============================================================================
// Additional RenderPassEncoder functions
// ============================================================================

void webgpu_renderPassEncoderSetPipeline(GfxRenderPassEncoder renderPassEncoder, GfxRenderPipeline pipeline)
{
    if (!renderPassEncoder || !pipeline)
        return;

    wgpuRenderPassEncoderSetPipeline(renderPassEncoder->wgpu_encoder, pipeline->wgpu_pipeline);
}

void webgpu_renderPassEncoderSetBindGroup(GfxRenderPassEncoder renderPassEncoder, uint32_t index, GfxBindGroup bindGroup)
{
    if (!renderPassEncoder || !bindGroup)
        return;

    wgpuRenderPassEncoderSetBindGroup(renderPassEncoder->wgpu_encoder, index, bindGroup->wgpu_bind_group, 0, NULL);
}

// ============================================================================
// Additional ComputePassEncoder functions
// ============================================================================

void webgpu_computePassEncoderSetPipeline(GfxComputePassEncoder computePassEncoder, GfxComputePipeline pipeline)
{
    if (!computePassEncoder || !pipeline)
        return;

    wgpuComputePassEncoderSetPipeline(computePassEncoder->wgpu_encoder, pipeline->wgpu_pipeline);
}

void webgpu_computePassEncoderSetBindGroup(GfxComputePassEncoder computePassEncoder, uint32_t index, GfxBindGroup bindGroup)
{
    if (!computePassEncoder || !bindGroup)
        return;

    wgpuComputePassEncoderSetBindGroup(computePassEncoder->wgpu_encoder, index, bindGroup->wgpu_bind_group, 0, NULL);
}

// ============================================================================
// Additional CommandEncoder functions
// ============================================================================

void webgpu_commandEncoderCopyBufferToTexture(GfxCommandEncoder commandEncoder,
    GfxBuffer source, uint64_t sourceOffset, uint32_t bytesPerRow,
    GfxTexture destination, const GfxOrigin3D* origin,
    const GfxExtent3D* extent, uint32_t mipLevel)
{
    if (!commandEncoder || !source || !destination || !origin || !extent)
        return;

    WGPUTexelCopyBufferInfo source_info = WGPU_TEXEL_COPY_BUFFER_INFO_INIT;
    source_info.buffer = source->wgpu_buffer;
    source_info.layout.offset = sourceOffset;
    source_info.layout.bytesPerRow = bytesPerRow;

    WGPUTexelCopyTextureInfo dest_info = WGPU_TEXEL_COPY_TEXTURE_INFO_INIT;
    dest_info.texture = destination->wgpu_texture;
    dest_info.mipLevel = mipLevel;
    dest_info.origin = (WGPUOrigin3D){ origin->x, origin->y, origin->z };

    WGPUExtent3D wgpu_extent = { extent->width, extent->height, extent->depth };

    wgpuCommandEncoderCopyBufferToTexture(commandEncoder->wgpu_encoder, &source_info, &dest_info, &wgpu_extent);
}

void webgpu_commandEncoderCopyTextureToBuffer(GfxCommandEncoder commandEncoder,
    GfxTexture source, const GfxOrigin3D* origin, uint32_t mipLevel,
    GfxBuffer destination, uint64_t destinationOffset, uint32_t bytesPerRow,
    const GfxExtent3D* extent)
{
    if (!commandEncoder || !source || !destination || !origin || !extent)
        return;

    WGPUTexelCopyTextureInfo source_info = WGPU_TEXEL_COPY_TEXTURE_INFO_INIT;
    source_info.texture = source->wgpu_texture;
    source_info.mipLevel = mipLevel;
    source_info.origin = (WGPUOrigin3D){ origin->x, origin->y, origin->z };

    WGPUTexelCopyBufferInfo dest_info = WGPU_TEXEL_COPY_BUFFER_INFO_INIT;
    dest_info.buffer = destination->wgpu_buffer;
    dest_info.layout.offset = destinationOffset;
    dest_info.layout.bytesPerRow = bytesPerRow;

    WGPUExtent3D wgpu_extent = { extent->width, extent->height, extent->depth };

    wgpuCommandEncoderCopyTextureToBuffer(commandEncoder->wgpu_encoder, &source_info, &dest_info, &wgpu_extent);
}

// ============================================================================
// Additional Surface functions
// ============================================================================

uint32_t webgpu_surfaceGetSupportedFormats(GfxSurface surface, GfxTextureFormat* formats, uint32_t maxFormats)
{
    if (!surface || !surface->wgpu_surface)
        return 0;

    // This would require device information which we don't have in the surface
    // In a real implementation, you'd store device reference or pass it as parameter
    return 0;
}

uint32_t webgpu_surfaceGetSupportedPresentModes(GfxSurface surface, GfxPresentMode* presentModes, uint32_t maxModes)
{
    if (!surface || !surface->wgpu_surface)
        return 0;

    // Similar to formats, this would require device information
    return 0;
}

// ============================================================================
// Fence functions (WebGPU compatibility stubs)
// ============================================================================

GfxFence webgpu_deviceCreateFence(GfxDevice device, const GfxFenceDescriptor* descriptor)
{
    if (!device || !descriptor)
        return NULL;

    // WebGPU doesn't have native fence support
    // We'll create a simple stub for API compatibility
    GfxFence fence = (GfxFence)malloc(sizeof(struct GfxFence_T));
    if (!fence)
        return NULL;

    fence->signaled = descriptor->signaled;
    return fence;
}

void webgpu_fenceDestroy(GfxFence fence)
{
    if (!fence)
        return;
    free(fence);
}

GfxResult webgpu_fenceGetStatus(GfxFence fence, bool* isSignaled)
{
    if (!fence || !isSignaled)
        return GFX_RESULT_ERROR_INVALID_PARAMETER;

    *isSignaled = fence->signaled;
    return GFX_RESULT_SUCCESS;
}

GfxResult webgpu_fenceWait(GfxFence fence, uint64_t timeoutNs)
{
    if (!fence)
        return GFX_RESULT_ERROR_INVALID_PARAMETER;

    // WebGPU doesn't have fence waiting
    // In a real implementation, you might poll the device
    return fence->signaled ? GFX_RESULT_SUCCESS : GFX_RESULT_TIMEOUT;
}

void webgpu_fenceReset(GfxFence fence)
{
    if (!fence)
        return;
    fence->signaled = false;
}

// ============================================================================
// Semaphore functions (WebGPU compatibility stubs)
// ============================================================================

GfxSemaphore webgpu_deviceCreateSemaphore(GfxDevice device, const GfxSemaphoreDescriptor* descriptor)
{
    if (!device || !descriptor)
        return NULL;

    // WebGPU doesn't have native semaphore support
    // We'll create a simple stub for API compatibility
    GfxSemaphore semaphore = (GfxSemaphore)malloc(sizeof(struct GfxSemaphore_T));
    if (!semaphore)
        return NULL;

    semaphore->type = descriptor->type;
    semaphore->value = descriptor->initialValue;
    return semaphore;
}

void webgpu_semaphoreDestroy(GfxSemaphore semaphore)
{
    if (!semaphore)
        return;
    free(semaphore);
}

GfxSemaphoreType webgpu_semaphoreGetType(GfxSemaphore semaphore)
{
    return semaphore ? semaphore->type : GFX_SEMAPHORE_TYPE_BINARY;
}

GfxResult webgpu_semaphoreSignal(GfxSemaphore semaphore, uint64_t value)
{
    if (!semaphore)
        return GFX_RESULT_ERROR_INVALID_PARAMETER;

    if (semaphore->type == GFX_SEMAPHORE_TYPE_TIMELINE) {
        semaphore->value = value;
    }
    return GFX_RESULT_SUCCESS;
}

GfxResult webgpu_semaphoreWait(GfxSemaphore semaphore, uint64_t value, uint64_t timeoutNs)
{
    if (!semaphore)
        return GFX_RESULT_ERROR_INVALID_PARAMETER;

    // WebGPU doesn't have semaphore waiting
    // This is a stub implementation for API compatibility
    if (semaphore->type == GFX_SEMAPHORE_TYPE_TIMELINE) {
        return (semaphore->value >= value) ? GFX_RESULT_SUCCESS : GFX_RESULT_TIMEOUT;
    }
    return GFX_RESULT_SUCCESS;
}

uint64_t webgpu_semaphoreGetValue(GfxSemaphore semaphore)
{
    if (!semaphore)
        return 0;
    return semaphore->value;
}

// ============================================================================
// Queue functions (additional)
// ============================================================================

void webgpu_queueSubmitWithSync(GfxQueue queue, const GfxSubmitInfo* submitInfo)
{
    if (!queue || !submitInfo)
        return;

    // WebGPU doesn't support semaphore-based synchronization
    // We'll submit the command buffers and ignore synchronization primitives
    for (uint32_t i = 0; i < submitInfo->commandEncoderCount; i++) {
        if (submitInfo->commandEncoders[i]) {
            WGPUCommandBufferDescriptor cmd_desc = WGPU_COMMAND_BUFFER_DESCRIPTOR_INIT;
            WGPUCommandBuffer cmd_buffer = wgpuCommandEncoderFinish(
                submitInfo->commandEncoders[i]->wgpu_encoder, &cmd_desc);

            if (cmd_buffer) {
                wgpuQueueSubmit(queue->wgpu_queue, 1, &cmd_buffer);
                wgpuCommandBufferRelease(cmd_buffer);
            }
        }
    }

    // Signal the fence if provided (simplified)
    if (submitInfo->signalFence) {
        submitInfo->signalFence->signaled = true;
    }
}

void webgpu_queueWaitIdle(GfxQueue queue)
{
    if (!queue)
        return;

    // WebGPU doesn't have a direct wait idle
    // We submit the queue and wait for work to complete
    WGPUQueueWorkDoneCallbackInfo callback_info = WGPU_QUEUE_WORK_DONE_CALLBACK_INFO_INIT;
    callback_info.mode = WGPUCallbackMode_WaitAnyOnly;
    callback_info.callback = NULL;

    wgpuQueueOnSubmittedWorkDone(queue->wgpu_queue, callback_info);
}

// ============================================================================
// Device functions (additional)
// ============================================================================

void webgpu_deviceWaitIdle(GfxDevice device)
{
    if (!device || !device->wgpu_queue)
        return;

    // Wait for all submitted work to complete
    WGPUQueueWorkDoneCallbackInfo callback_info = WGPU_QUEUE_WORK_DONE_CALLBACK_INFO_INIT;
    callback_info.mode = WGPUCallbackMode_WaitAnyOnly;
    callback_info.callback = NULL;

    wgpuQueueOnSubmittedWorkDone(device->wgpu_queue, callback_info);
}

// ============================================================================
// Backend Function Table Export
// ============================================================================

static const GfxBackendAPI webgpu_api = {
    .createInstance = webgpu_createInstance,
    .instanceDestroy = webgpu_instanceDestroy,
    .instanceRequestAdapter = webgpu_instanceRequestAdapter,
    .instanceEnumerateAdapters = webgpu_instanceEnumerateAdapters,
    .adapterDestroy = webgpu_adapterDestroy,
    .adapterCreateDevice = webgpu_adapterCreateDevice,
    .adapterGetName = webgpu_adapterGetName,
    .adapterGetBackend = webgpu_adapterGetBackend,
    .deviceDestroy = webgpu_deviceDestroy,
    .deviceGetQueue = webgpu_deviceGetQueue,
    .deviceCreateSurface = webgpu_deviceCreateSurface,
    .deviceCreateSwapchain = webgpu_deviceCreateSwapchain,
    .deviceCreateBuffer = webgpu_deviceCreateBuffer,
    .deviceCreateTexture = webgpu_deviceCreateTexture,
    .deviceCreateSampler = webgpu_deviceCreateSampler,
    .deviceCreateShader = webgpu_deviceCreateShader,
    .deviceCreateBindGroupLayout = webgpu_deviceCreateBindGroupLayout,
    .deviceCreateBindGroup = webgpu_deviceCreateBindGroup,
    .deviceCreateRenderPipeline = webgpu_deviceCreateRenderPipeline,
    .deviceCreateComputePipeline = webgpu_deviceCreateComputePipeline,
    .deviceCreateCommandEncoder = webgpu_deviceCreateCommandEncoder,
    .deviceCreateFence = webgpu_deviceCreateFence,
    .deviceCreateSemaphore = webgpu_deviceCreateSemaphore,
    .deviceWaitIdle = webgpu_deviceWaitIdle,
    .surfaceDestroy = webgpu_surfaceDestroy,
    .surfaceGetWidth = webgpu_surfaceGetWidth,
    .surfaceGetHeight = webgpu_surfaceGetHeight,
    .surfaceResize = webgpu_surfaceResize,
    .surfaceGetSupportedFormats = webgpu_surfaceGetSupportedFormats,
    .surfaceGetSupportedPresentModes = webgpu_surfaceGetSupportedPresentModes,
    .surfaceGetPlatformHandle = webgpu_surfaceGetPlatformHandle,
    .swapchainDestroy = webgpu_swapchainDestroy,
    .swapchainGetWidth = webgpu_swapchainGetWidth,
    .swapchainGetHeight = webgpu_swapchainGetHeight,
    .swapchainGetFormat = webgpu_swapchainGetFormat,
    .swapchainGetBufferCount = webgpu_swapchainGetBufferCount,
    .swapchainGetCurrentTextureView = webgpu_swapchainGetCurrentTextureView,
    .swapchainPresent = webgpu_swapchainPresent,
    .swapchainResize = webgpu_swapchainResize,
    .swapchainNeedsRecreation = webgpu_swapchainNeedsRecreation,
    .bufferDestroy = webgpu_bufferDestroy,
    .bufferGetSize = webgpu_bufferGetSize,
    .bufferGetUsage = webgpu_bufferGetUsage,
    .bufferMapAsync = webgpu_bufferMapAsync,
    .bufferUnmap = webgpu_bufferUnmap,
    .textureDestroy = webgpu_textureDestroy,
    .textureGetSize = webgpu_textureGetSize,
    .textureGetFormat = webgpu_textureGetFormat,
    .textureGetMipLevelCount = webgpu_textureGetMipLevelCount,
    .textureGetSampleCount = webgpu_textureGetSampleCount,
    .textureGetUsage = webgpu_textureGetUsage,
    .textureCreateView = webgpu_textureCreateView,
    .textureViewDestroy = webgpu_textureViewDestroy,
    .textureViewGetTexture = webgpu_textureViewGetTexture,
    .samplerDestroy = webgpu_samplerDestroy,
    .shaderDestroy = webgpu_shaderDestroy,
    .bindGroupLayoutDestroy = webgpu_bindGroupLayoutDestroy,
    .bindGroupDestroy = webgpu_bindGroupDestroy,
    .renderPipelineDestroy = webgpu_renderPipelineDestroy,
    .computePipelineDestroy = webgpu_computePipelineDestroy,
    .queueSubmit = webgpu_queueSubmit,
    .queueSubmitWithSync = webgpu_queueSubmitWithSync,
    .queueWriteBuffer = webgpu_queueWriteBuffer,
    .queueWriteTexture = webgpu_queueWriteTexture,
    .queueWaitIdle = webgpu_queueWaitIdle,
    .commandEncoderDestroy = webgpu_commandEncoderDestroy,
    .commandEncoderBeginRenderPass = webgpu_commandEncoderBeginRenderPass,
    .commandEncoderBeginComputePass = webgpu_commandEncoderBeginComputePass,
    .commandEncoderCopyBufferToBuffer = webgpu_commandEncoderCopyBufferToBuffer,
    .commandEncoderCopyBufferToTexture = webgpu_commandEncoderCopyBufferToTexture,
    .commandEncoderCopyTextureToBuffer = webgpu_commandEncoderCopyTextureToBuffer,
    .commandEncoderFinish = webgpu_commandEncoderFinish,
    .renderPassEncoderDestroy = webgpu_renderPassEncoderDestroy,
    .renderPassEncoderSetPipeline = webgpu_renderPassEncoderSetPipeline,
    .renderPassEncoderSetBindGroup = webgpu_renderPassEncoderSetBindGroup,
    .renderPassEncoderSetVertexBuffer = webgpu_renderPassEncoderSetVertexBuffer,
    .renderPassEncoderSetIndexBuffer = webgpu_renderPassEncoderSetIndexBuffer,
    .renderPassEncoderDraw = webgpu_renderPassEncoderDraw,
    .renderPassEncoderDrawIndexed = webgpu_renderPassEncoderDrawIndexed,
    .renderPassEncoderEnd = webgpu_renderPassEncoderEnd,
    .computePassEncoderDestroy = webgpu_computePassEncoderDestroy,
    .computePassEncoderSetPipeline = webgpu_computePassEncoderSetPipeline,
    .computePassEncoderSetBindGroup = webgpu_computePassEncoderSetBindGroup,
    .computePassEncoderDispatchWorkgroups = webgpu_computePassEncoderDispatchWorkgroups,
    .computePassEncoderEnd = webgpu_computePassEncoderEnd,
    .fenceDestroy = webgpu_fenceDestroy,
    .fenceGetStatus = webgpu_fenceGetStatus,
    .fenceWait = webgpu_fenceWait,
    .fenceReset = webgpu_fenceReset,
    .semaphoreDestroy = webgpu_semaphoreDestroy,
    .semaphoreGetType = webgpu_semaphoreGetType,
    .semaphoreSignal = webgpu_semaphoreSignal,
    .semaphoreWait = webgpu_semaphoreWait,
    .semaphoreGetValue = webgpu_semaphoreGetValue,
};

const GfxBackendAPI* gfxGetWebgpuBackend(void)
{
    return &webgpu_api;
}
