#include "WebGPUBackend.h"

#include "../dependencies/include/webgpu/webgpu.h"

#include <gfx/gfx.h>

#include "IBackend.h"

#include <cassert>
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>
#include <vector>

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
// Utility Functions
// ============================================================================

namespace {

WGPUStringView gfxStringView(const char* str)
{
    if (!str) {
        return (WGPUStringView){ nullptr, WGPU_STRLEN };
    }
    return (WGPUStringView){ str, WGPU_STRLEN };
}

WGPUTextureFormat gfxFormatToWGPUFormat(GfxTextureFormat format)
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

GfxTextureFormat wgpuFormatToGfxFormat(WGPUTextureFormat format)
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

WGPUBufferUsage gfxBufferUsageToWGPU(GfxBufferUsage usage)
{
    WGPUBufferUsage wgpu_usage = WGPUBufferUsage_None;
    if (usage & GFX_BUFFER_USAGE_MAP_READ) {
        wgpu_usage |= WGPUBufferUsage_MapRead;
    }
    if (usage & GFX_BUFFER_USAGE_MAP_WRITE) {
        wgpu_usage |= WGPUBufferUsage_MapWrite;
    }
    if (usage & GFX_BUFFER_USAGE_COPY_SRC) {
        wgpu_usage |= WGPUBufferUsage_CopySrc;
    }
    if (usage & GFX_BUFFER_USAGE_COPY_DST) {
        wgpu_usage |= WGPUBufferUsage_CopyDst;
    }
    if (usage & GFX_BUFFER_USAGE_INDEX) {
        wgpu_usage |= WGPUBufferUsage_Index;
    }
    if (usage & GFX_BUFFER_USAGE_VERTEX) {
        wgpu_usage |= WGPUBufferUsage_Vertex;
    }
    if (usage & GFX_BUFFER_USAGE_UNIFORM) {
        wgpu_usage |= WGPUBufferUsage_Uniform;
    }
    if (usage & GFX_BUFFER_USAGE_STORAGE) {
        wgpu_usage |= WGPUBufferUsage_Storage;
    }
    if (usage & GFX_BUFFER_USAGE_INDIRECT) {
        wgpu_usage |= WGPUBufferUsage_Indirect;
    }
    return wgpu_usage;
}

WGPUTextureUsage gfxTextureUsageToWGPU(GfxTextureUsage usage)
{
    WGPUTextureUsage wgpu_usage = WGPUTextureUsage_None;
    if (usage & GFX_TEXTURE_USAGE_COPY_SRC) {
        wgpu_usage |= WGPUTextureUsage_CopySrc;
    }
    if (usage & GFX_TEXTURE_USAGE_COPY_DST) {
        wgpu_usage |= WGPUTextureUsage_CopyDst;
    }
    if (usage & GFX_TEXTURE_USAGE_TEXTURE_BINDING) {
        wgpu_usage |= WGPUTextureUsage_TextureBinding;
    }
    if (usage & GFX_TEXTURE_USAGE_STORAGE_BINDING) {
        wgpu_usage |= WGPUTextureUsage_StorageBinding;
    }
    if (usage & GFX_TEXTURE_USAGE_RENDER_ATTACHMENT) {
        wgpu_usage |= WGPUTextureUsage_RenderAttachment;
    }
    return wgpu_usage;
}

WGPUPresentMode gfxPresentModeToWGPU(GfxPresentMode mode)
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

WGPUPrimitiveTopology gfxPrimitiveTopologyToWGPU(GfxPrimitiveTopology topology)
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

WGPUIndexFormat gfxIndexFormatToWGPU(GfxIndexFormat format)
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

WGPUBlendOperation gfxBlendOperationToWGPU(GfxBlendOperation operation)
{
    switch (operation) {
    case GFX_BLEND_OPERATION_ADD:
        return WGPUBlendOperation_Add;
    case GFX_BLEND_OPERATION_SUBTRACT:
        return WGPUBlendOperation_Subtract;
    case GFX_BLEND_OPERATION_REVERSE_SUBTRACT:
        return WGPUBlendOperation_ReverseSubtract;
    case GFX_BLEND_OPERATION_MIN:
        return WGPUBlendOperation_Min;
    case GFX_BLEND_OPERATION_MAX:
        return WGPUBlendOperation_Max;
    default:
        return WGPUBlendOperation_Add;
    }
}

WGPUBlendFactor gfxBlendFactorToWGPU(GfxBlendFactor factor)
{
    switch (factor) {
    case GFX_BLEND_FACTOR_ZERO:
        return WGPUBlendFactor_Zero;
    case GFX_BLEND_FACTOR_ONE:
        return WGPUBlendFactor_One;
    case GFX_BLEND_FACTOR_SRC:
        return WGPUBlendFactor_Src;
    case GFX_BLEND_FACTOR_ONE_MINUS_SRC:
        return WGPUBlendFactor_OneMinusSrc;
    case GFX_BLEND_FACTOR_SRC_ALPHA:
        return WGPUBlendFactor_SrcAlpha;
    case GFX_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA:
        return WGPUBlendFactor_OneMinusSrcAlpha;
    case GFX_BLEND_FACTOR_DST:
        return WGPUBlendFactor_Dst;
    case GFX_BLEND_FACTOR_ONE_MINUS_DST:
        return WGPUBlendFactor_OneMinusDst;
    case GFX_BLEND_FACTOR_DST_ALPHA:
        return WGPUBlendFactor_DstAlpha;
    case GFX_BLEND_FACTOR_ONE_MINUS_DST_ALPHA:
        return WGPUBlendFactor_OneMinusDstAlpha;
    case GFX_BLEND_FACTOR_SRC_ALPHA_SATURATED:
        return WGPUBlendFactor_SrcAlphaSaturated;
    case GFX_BLEND_FACTOR_CONSTANT:
        return WGPUBlendFactor_Constant;
    case GFX_BLEND_FACTOR_ONE_MINUS_CONSTANT:
        return WGPUBlendFactor_OneMinusConstant;
    default:
        return WGPUBlendFactor_Zero;
    }
}

WGPUCompareFunction gfxCompareFunctionToWGPU(GfxCompareFunction func)
{
    switch (func) {
    case GFX_COMPARE_FUNCTION_NEVER:
        return WGPUCompareFunction_Never;
    case GFX_COMPARE_FUNCTION_LESS:
        return WGPUCompareFunction_Less;
    case GFX_COMPARE_FUNCTION_EQUAL:
        return WGPUCompareFunction_Equal;
    case GFX_COMPARE_FUNCTION_LESS_EQUAL:
        return WGPUCompareFunction_LessEqual;
    case GFX_COMPARE_FUNCTION_GREATER:
        return WGPUCompareFunction_Greater;
    case GFX_COMPARE_FUNCTION_NOT_EQUAL:
        return WGPUCompareFunction_NotEqual;
    case GFX_COMPARE_FUNCTION_GREATER_EQUAL:
        return WGPUCompareFunction_GreaterEqual;
    case GFX_COMPARE_FUNCTION_ALWAYS:
        return WGPUCompareFunction_Always;
    default:
        return WGPUCompareFunction_Always;
    }
}

WGPUTextureSampleType gfxTextureSampleTypeToWGPU(GfxTextureSampleType sampleType)
{
    switch (sampleType) {
    case GFX_TEXTURE_SAMPLE_TYPE_FLOAT:
        return WGPUTextureSampleType_Float;
    case GFX_TEXTURE_SAMPLE_TYPE_UNFILTERABLE_FLOAT:
        return WGPUTextureSampleType_UnfilterableFloat;
    case GFX_TEXTURE_SAMPLE_TYPE_DEPTH:
        return WGPUTextureSampleType_Depth;
    case GFX_TEXTURE_SAMPLE_TYPE_SINT:
        return WGPUTextureSampleType_Sint;
    case GFX_TEXTURE_SAMPLE_TYPE_UINT:
        return WGPUTextureSampleType_Uint;
    default:
        return WGPUTextureSampleType_Float;
    }
}

} // namespace

// ============================================================================
// Platform-specific Surface Creation Helpers
// ============================================================================

#ifdef _WIN32
static WGPUSurface createSurfaceWin32(WGPUInstance instance, const GfxPlatformWindowHandle* handle)
{
    if (!handle->hwnd || !handle->hinstance) {
        return nullptr;
    }

    WGPUSurfaceSourceWindowsHWND source = WGPU_SURFACE_SOURCE_WINDOWS_HWND_INIT;
    source.hwnd = handle->hwnd;
    source.hinstance = handle->hinstance;

    WGPUSurfaceDescriptor surface_desc = WGPU_SURFACE_DESCRIPTOR_INIT;
    surface_desc.label = gfxStringView("Win32 Surface");
    surface_desc.nextInChain = (WGPUChainedStruct*)&source;

    return wgpuInstanceCreateSurface(instance, &surface_desc);
}
#endif

#ifdef __linux__
static WGPUSurface createSurfaceX11(WGPUInstance instance, const GfxPlatformWindowHandle* handle)
{
    if (!handle->x11.window || !handle->x11.display) {
        return nullptr;
    }

    WGPUSurfaceSourceXlibWindow source = WGPU_SURFACE_SOURCE_XLIB_WINDOW_INIT;
    source.display = handle->x11.display;
    source.window = (uint64_t)(uintptr_t)handle->x11.window;

    WGPUSurfaceDescriptor surface_desc = WGPU_SURFACE_DESCRIPTOR_INIT;
    surface_desc.label = gfxStringView("X11 Surface");
    surface_desc.nextInChain = (WGPUChainedStruct*)&source;

    return wgpuInstanceCreateSurface(instance, &surface_desc);
}

static WGPUSurface createSurfaceWayland(WGPUInstance instance, const GfxPlatformWindowHandle* handle)
{
    if (!handle->wayland.surface || !handle->wayland.display) {
        return nullptr;
    }

    WGPUSurfaceSourceWaylandSurface source = WGPU_SURFACE_SOURCE_WAYLAND_SURFACE_INIT;
    source.display = handle->wayland.display;
    source.surface = handle->wayland.surface;

    WGPUSurfaceDescriptor surface_desc = WGPU_SURFACE_DESCRIPTOR_INIT;
    surface_desc.label = gfxStringView("Wayland Surface");
    surface_desc.nextInChain = (WGPUChainedStruct*)&source;

    return wgpuInstanceCreateSurface(instance, &surface_desc);
}
#endif

#ifdef __APPLE__
static WGPUSurface createSurfaceMetal(WGPUInstance instance, const GfxPlatformWindowHandle* handle)
{
    void* metal_layer = handle->metalLayer;

    // If no metal layer provided, try to get it from the NSWindow
    if (!metal_layer && handle->nsWindow) {
        id nsWindow = (id)handle->nsWindow;
        SEL contentViewSel = sel_registerName("contentView");
        id contentView = ((id (*)(id, SEL))objc_msgSend)(nsWindow, contentViewSel);

        if (contentView) {
            SEL layerSel = sel_registerName("layer");
            metal_layer = ((void* (*)(id, SEL))objc_msgSend)(contentView, layerSel);

            if (metal_layer) {
                Class metalLayerClass = objc_getClass("CAMetalLayer");
                if (metalLayerClass && !object_isKindOfClass((id)metal_layer, metalLayerClass)) {
                    id newMetalLayer = ((id (*)(Class, SEL))objc_msgSend)(metalLayerClass, sel_registerName("new"));
                    SEL setLayerSel = sel_registerName("setLayer:");
                    ((void (*)(id, SEL, id))objc_msgSend)(contentView, setLayerSel, newMetalLayer);
                    SEL setWantsLayerSel = sel_registerName("setWantsLayer:");
                    ((void (*)(id, SEL, BOOL))objc_msgSend)(contentView, setWantsLayerSel, YES);
                    metal_layer = newMetalLayer;
                }
            }
        }
    }

    if (!metal_layer) {
        return nullptr;
    }

    WGPUSurfaceSourceMetalLayer source = WGPU_SURFACE_SOURCE_METAL_LAYER_INIT;
    source.layer = metal_layer;

    WGPUSurfaceDescriptor surface_desc = WGPU_SURFACE_DESCRIPTOR_INIT;
    surface_desc.label = gfxStringView("Metal Surface");
    surface_desc.nextInChain = (WGPUChainedStruct*)&source;

    return wgpuInstanceCreateSurface(instance, &surface_desc);
}
#endif

static WGPUSurface createPlatformSurface(WGPUInstance instance, const GfxPlatformWindowHandle* handle)
{
    if (!instance || !handle) {
        return nullptr;
    }

    switch (handle->windowingSystem) {
    case GFX_WINDOWING_SYSTEM_WIN32:
        return createSurfaceWin32(instance, handle);
    case GFX_WINDOWING_SYSTEM_WAYLAND:
        return createSurfaceWayland(instance, handle);
    case GFX_WINDOWING_SYSTEM_X11:
    case GFX_WINDOWING_SYSTEM_XCB:
        return createSurfaceX11(instance, handle);
    case GFX_WINDOWING_SYSTEM_COCOA:
        return createSurfaceMetal(instance, handle);
    default:
        return nullptr;
    }
}

static WGPUTextureDimension gfxTextureTypeToWGPU(GfxTextureType type)
{
    switch (type) {
    case GFX_TEXTURE_TYPE_1D:
        return WGPUTextureDimension_1D;
    case GFX_TEXTURE_TYPE_2D:
        return WGPUTextureDimension_2D;
    case GFX_TEXTURE_TYPE_CUBE:
        return WGPUTextureDimension_2D; // Cube maps are 2D arrays in WebGPU
    case GFX_TEXTURE_TYPE_3D:
        return WGPUTextureDimension_3D;
    default:
        return WGPUTextureDimension_2D;
    }
}

static WGPUTextureViewDimension gfxTextureViewTypeToWGPU(GfxTextureViewType type)
{
    switch (type) {
    case GFX_TEXTURE_VIEW_TYPE_1D:
        return WGPUTextureViewDimension_1D;
    case GFX_TEXTURE_VIEW_TYPE_2D:
        return WGPUTextureViewDimension_2D;
    case GFX_TEXTURE_VIEW_TYPE_3D:
        return WGPUTextureViewDimension_3D;
    case GFX_TEXTURE_VIEW_TYPE_CUBE:
        return WGPUTextureViewDimension_Cube;
    case GFX_TEXTURE_VIEW_TYPE_1D_ARRAY:
        return WGPUTextureViewDimension_1D; // WebGPU doesn't have 1D arrays
    case GFX_TEXTURE_VIEW_TYPE_2D_ARRAY:
        return WGPUTextureViewDimension_2DArray;
    case GFX_TEXTURE_VIEW_TYPE_CUBE_ARRAY:
        return WGPUTextureViewDimension_CubeArray;
    default:
        return WGPUTextureViewDimension_2D;
    }
}

// ============================================================================
// Internal C++ Classes with RAII
// ============================================================================

namespace gfx::webgpu {

class Instance {
public:
    // Prevent copying
    Instance(const Instance&) = delete;
    Instance& operator=(const Instance&) = delete;

    Instance(const GfxInstanceDescriptor* descriptor)
    {
        WGPUInstanceDescriptor wgpu_desc = WGPU_INSTANCE_DESCRIPTOR_INIT;
        m_instance = wgpuCreateInstance(&wgpu_desc);
    }

    ~Instance()
    {
        if (m_instance) {
            wgpuInstanceRelease(m_instance);
        }
    }

    WGPUInstance handle() const { return m_instance; }

private:
    WGPUInstance m_instance = nullptr;
};

class Adapter {
public:
    // Prevent copying
    Adapter(const Adapter&) = delete;
    Adapter& operator=(const Adapter&) = delete;

    Adapter(WGPUAdapter adapter, Instance* instance)
        : m_adapter(adapter)
        , m_instance(instance)
        , m_name("WebGPU Adapter")
    {
    }

    ~Adapter()
    {
        if (m_adapter) {
            wgpuAdapterRelease(m_adapter);
        }
    }

    WGPUAdapter handle() const { return m_adapter; }
    const char* getName() const { return m_name.c_str(); }
    Instance* getInstance() const { return m_instance; }

private:
    WGPUAdapter m_adapter = nullptr;
    Instance* m_instance = nullptr; // Non-owning
    std::string m_name;
};

class Queue {
public:
    // Prevent copying
    Queue(const Queue&) = delete;
    Queue& operator=(const Queue&) = delete;

    Queue(WGPUQueue queue, Device* device)
        : m_queue(queue)
        , m_device(device)
    {
        if (m_queue) {
            wgpuQueueAddRef(m_queue);
        }
    }

    ~Queue()
    {
        if (m_queue) {
            wgpuQueueRelease(m_queue);
        }
    }

    WGPUQueue handle() const { return m_queue; }
    Device* getDevice() const { return m_device; }

private:
    WGPUQueue m_queue = nullptr;
    Device* m_device = nullptr; // Non-owning pointer to parent device
};

class Device {
public:
    // Prevent copying
    Device(const Device&) = delete;
    Device& operator=(const Device&) = delete;

    Device(Adapter* adapter, WGPUDevice device)
        : m_adapter(adapter)
        , m_device(device)
    {
        if (m_device) {
            WGPUQueue wgpu_queue = wgpuDeviceGetQueue(m_device);
            m_queue = std::make_unique<Queue>(wgpu_queue, this);
        }
    }

    ~Device()
    {
        m_queue.reset();
        if (m_device) {
            wgpuDeviceRelease(m_device);
        }
    }

    WGPUDevice handle() const { return m_device; }
    Queue* getQueue() { return m_queue.get(); }
    Adapter* getAdapter() { return m_adapter; }

private:
    WGPUDevice m_device = nullptr;
    Adapter* m_adapter = nullptr; // Non-owning pointer
    std::unique_ptr<Queue> m_queue;
};

class Buffer {
public:
    // Prevent copying
    Buffer(const Buffer&) = delete;
    Buffer& operator=(const Buffer&) = delete;

    Buffer(WGPUBuffer buffer, uint64_t size, GfxBufferUsage usage, Device* device)
        : m_buffer(buffer)
        , m_size(size)
        , m_usage(usage)
        , m_device(device)
    {
    }

    ~Buffer()
    {
        if (m_buffer) {
            wgpuBufferRelease(m_buffer);
        }
    }

    WGPUBuffer handle() const { return m_buffer; }
    uint64_t getSize() const { return m_size; }
    GfxBufferUsage getUsage() const { return m_usage; }
    Device* getDevice() const { return m_device; }

private:
    WGPUBuffer m_buffer = nullptr;
    uint64_t m_size = 0;
    GfxBufferUsage m_usage = GFX_BUFFER_USAGE_NONE;
    Device* m_device = nullptr; // Non-owning pointer to parent device
};

class Texture {
public:
    // Prevent copying
    Texture(const Texture&) = delete;
    Texture& operator=(const Texture&) = delete;

    Texture(WGPUTexture texture, WGPUExtent3D size, WGPUTextureFormat format,
        uint32_t mipLevels, uint32_t sampleCount, WGPUTextureUsage usage)
        : m_texture(texture)
        , m_size(size)
        , m_format(format)
        , m_mipLevels(mipLevels)
        , m_sampleCount(sampleCount)
        , m_usage(usage)
    {
    }

    ~Texture()
    {
        if (m_texture) {
            wgpuTextureRelease(m_texture);
        }
    }

    WGPUTexture handle() const { return m_texture; }
    WGPUExtent3D getSize() const { return m_size; }
    WGPUTextureFormat getFormat() const { return m_format; }
    uint32_t getMipLevels() const { return m_mipLevels; }
    uint32_t getSampleCount() const { return m_sampleCount; }
    WGPUTextureUsage getUsage() const { return m_usage; }

private:
    WGPUTexture m_texture = nullptr;
    WGPUExtent3D m_size = {};
    WGPUTextureFormat m_format = WGPUTextureFormat_Undefined;
    uint32_t m_mipLevels = 0;
    uint32_t m_sampleCount = 0;
    WGPUTextureUsage m_usage = WGPUTextureUsage_None;
};

class TextureView {
public:
    // Prevent copying
    TextureView(const TextureView&) = delete;
    TextureView& operator=(const TextureView&) = delete;

    TextureView(WGPUTextureView view, Texture* texture = nullptr)
        : m_view(view)
        , m_texture(texture)
    {
    }

    ~TextureView()
    {
        if (m_view) {
            wgpuTextureViewRelease(m_view);
        }
    }

    WGPUTextureView handle() const { return m_view; }
    Texture* getTexture() { return m_texture; }

private:
    WGPUTextureView m_view = nullptr;
    Texture* m_texture = nullptr; // Non-owning
};

class Sampler {
public:
    // Prevent copying
    Sampler(const Sampler&) = delete;
    Sampler& operator=(const Sampler&) = delete;

    Sampler(WGPUSampler sampler)
        : m_sampler(sampler)
    {
    }

    ~Sampler()
    {
        if (m_sampler) {
            wgpuSamplerRelease(m_sampler);
        }
    }

    WGPUSampler handle() const { return m_sampler; }

private:
    WGPUSampler m_sampler = nullptr;
};

class Shader {
public:
    // Prevent copying
    Shader(const Shader&) = delete;
    Shader& operator=(const Shader&) = delete;

    Shader(WGPUShaderModule module)
        : m_module(module)
    {
    }

    ~Shader()
    {
        if (m_module) {
            wgpuShaderModuleRelease(m_module);
        }
    }

    WGPUShaderModule handle() const { return m_module; }

private:
    WGPUShaderModule m_module = nullptr;
};

class BindGroupLayout {
public:
    // Prevent copying
    BindGroupLayout(const BindGroupLayout&) = delete;
    BindGroupLayout& operator=(const BindGroupLayout&) = delete;

    BindGroupLayout(WGPUBindGroupLayout layout)
        : m_layout(layout)
    {
    }

    ~BindGroupLayout()
    {
        if (m_layout) {
            wgpuBindGroupLayoutRelease(m_layout);
        }
    }

    WGPUBindGroupLayout handle() const { return m_layout; }

private:
    WGPUBindGroupLayout m_layout = nullptr;
};

class BindGroup {
public:
    // Prevent copying
    BindGroup(const BindGroup&) = delete;
    BindGroup& operator=(const BindGroup&) = delete;

    BindGroup(WGPUBindGroup bindGroup)
        : m_bindGroup(bindGroup)
    {
    }

    ~BindGroup()
    {
        if (m_bindGroup) {
            wgpuBindGroupRelease(m_bindGroup);
        }
    }

    WGPUBindGroup handle() const { return m_bindGroup; }

private:
    WGPUBindGroup m_bindGroup = nullptr;
};

class RenderPipeline {
public:
    // Prevent copying
    RenderPipeline(const RenderPipeline&) = delete;
    RenderPipeline& operator=(const RenderPipeline&) = delete;

    RenderPipeline(WGPURenderPipeline pipeline)
        : m_pipeline(pipeline)
    {
    }

    ~RenderPipeline()
    {
        if (m_pipeline) {
            wgpuRenderPipelineRelease(m_pipeline);
        }
    }

    WGPURenderPipeline handle() const { return m_pipeline; }

private:
    WGPURenderPipeline m_pipeline = nullptr;
};

class ComputePipeline {
public:
    // Prevent copying
    ComputePipeline(const ComputePipeline&) = delete;
    ComputePipeline& operator=(const ComputePipeline&) = delete;

    ComputePipeline(WGPUComputePipeline pipeline)
        : m_pipeline(pipeline)
    {
    }

    ~ComputePipeline()
    {
        if (m_pipeline) {
            wgpuComputePipelineRelease(m_pipeline);
        }
    }

    WGPUComputePipeline handle() const { return m_pipeline; }

private:
    WGPUComputePipeline m_pipeline = nullptr;
};

class CommandEncoder {
public:
    // Prevent copying
    CommandEncoder(const CommandEncoder&) = delete;
    CommandEncoder& operator=(const CommandEncoder&) = delete;

    CommandEncoder(WGPUCommandEncoder encoder)
        : m_encoder(encoder)
    {
    }

    ~CommandEncoder()
    {
        if (m_encoder) {
            wgpuCommandEncoderRelease(m_encoder);
        }
    }

    WGPUCommandEncoder handle() const { return m_encoder; }

private:
    WGPUCommandEncoder m_encoder = nullptr;
};

class RenderPassEncoder {
public:
    // Prevent copying
    RenderPassEncoder(const RenderPassEncoder&) = delete;
    RenderPassEncoder& operator=(const RenderPassEncoder&) = delete;

    RenderPassEncoder(WGPURenderPassEncoder encoder)
        : m_encoder(encoder)
    {
    }

    ~RenderPassEncoder()
    {
        if (m_encoder) {
            wgpuRenderPassEncoderRelease(m_encoder);
        }
    }

    WGPURenderPassEncoder handle() const { return m_encoder; }

private:
    WGPURenderPassEncoder m_encoder = nullptr;
};

class ComputePassEncoder {
public:
    // Prevent copying
    ComputePassEncoder(const ComputePassEncoder&) = delete;
    ComputePassEncoder& operator=(const ComputePassEncoder&) = delete;

    ComputePassEncoder(WGPUComputePassEncoder encoder)
        : m_encoder(encoder)
    {
    }

    ~ComputePassEncoder()
    {
        if (m_encoder) {
            wgpuComputePassEncoderRelease(m_encoder);
        }
    }

    WGPUComputePassEncoder handle() const { return m_encoder; }

private:
    WGPUComputePassEncoder m_encoder = nullptr;
};

class Surface {
public:
    // Prevent copying
    Surface(const Surface&) = delete;
    Surface& operator=(const Surface&) = delete;

    Surface(WGPUSurface surface, uint32_t width, uint32_t height, const GfxPlatformWindowHandle& windowHandle)
        : m_surface(surface)
        , m_width(width)
        , m_height(height)
        , m_windowHandle(windowHandle)
    {
    }

    ~Surface()
    {
        if (m_surface) {
            wgpuSurfaceRelease(m_surface);
        }
    }

    WGPUSurface handle() const { return m_surface; }
    uint32_t getWidth() const { return m_width; }
    uint32_t getHeight() const { return m_height; }
    void setSize(uint32_t width, uint32_t height)
    {
        m_width = width;
        m_height = height;
    }
    const GfxPlatformWindowHandle& getWindowHandle() const { return m_windowHandle; }

private:
    WGPUSurface m_surface = nullptr;
    uint32_t m_width = 0;
    uint32_t m_height = 0;
    GfxPlatformWindowHandle m_windowHandle = {};
};

class Swapchain {
public:
    // Prevent copying
    Swapchain(const Swapchain&) = delete;
    Swapchain& operator=(const Swapchain&) = delete;

    Swapchain(WGPUSurface surface, WGPUDevice device, uint32_t width, uint32_t height,
        WGPUTextureFormat format, WGPUPresentMode presentMode, uint32_t bufferCount)
        : m_surface(surface)
        , m_device(device)
        , m_width(width)
        , m_height(height)
        , m_format(format)
        , m_presentMode(presentMode)
        , m_bufferCount(bufferCount)
    {
    }

    ~Swapchain()
    {
        // Don't release surface or device - they're not owned
    }

    WGPUSurface surface() const { return m_surface; }
    WGPUDevice device() const { return m_device; }
    uint32_t getWidth() const { return m_width; }
    uint32_t getHeight() const { return m_height; }
    WGPUTextureFormat getFormat() const { return m_format; }
    uint32_t getBufferCount() const { return m_bufferCount; }

    void setSize(uint32_t width, uint32_t height)
    {
        m_width = width;
        m_height = height;
    }

private:
    WGPUSurface m_surface = nullptr; // Non-owning
    WGPUDevice m_device = nullptr; // Non-owning
    uint32_t m_width = 0;
    uint32_t m_height = 0;
    WGPUTextureFormat m_format = WGPUTextureFormat_Undefined;
    WGPUPresentMode m_presentMode = WGPUPresentMode_Fifo;
    uint32_t m_bufferCount = 0;
};

class Fence {
public:
    // Prevent copying
    Fence(const Fence&) = delete;
    Fence& operator=(const Fence&) = delete;

    Fence(bool signaled)
        : m_signaled(signaled)
    {
    }

    ~Fence() = default;

    bool isSignaled() const { return m_signaled; }
    void setSignaled(bool signaled) { m_signaled = signaled; }

private:
    bool m_signaled = false;
};

class Semaphore {
public:
    // Prevent copying
    Semaphore(const Semaphore&) = delete;
    Semaphore& operator=(const Semaphore&) = delete;

    Semaphore(GfxSemaphoreType type, uint64_t value)
        : m_type(type)
        , m_value(value)
    {
    }

    ~Semaphore() = default;

    GfxSemaphoreType getType() const { return m_type; }
    uint64_t getValue() const { return m_value; }
    void setValue(uint64_t value) { m_value = value; }

private:
    GfxSemaphoreType m_type = GFX_SEMAPHORE_TYPE_BINARY;
    uint64_t m_value = 0;
};

} // namespace gfx::webgpu

// ============================================================================
// C API Functions
// ============================================================================

GfxResult webgpu_createInstance(const GfxInstanceDescriptor* descriptor, GfxInstance* outInstance)
{
    if (!outInstance) {
        return GFX_RESULT_ERROR_INVALID_PARAMETER;
    }

    try {
        auto* instance = new gfx::webgpu::Instance(descriptor);
        *outInstance = reinterpret_cast<GfxInstance>(instance);
        return GFX_RESULT_SUCCESS;
    } catch (...) {
        return GFX_RESULT_ERROR_UNKNOWN;
    }
}

void webgpu_instanceDestroy(GfxInstance instance)
{
    if (!instance) {
        return;
    }
    delete reinterpret_cast<gfx::webgpu::Instance*>(instance);
}

void webgpu_instanceSetDebugCallback(GfxInstance instance, GfxDebugCallback callback, void* userData)
{
    // TODO: Implement debug callback using WebGPU error handling
    (void)instance;
    (void)callback;
    (void)userData;
}

GfxResult webgpu_instanceRequestAdapter(GfxInstance instance, const GfxAdapterDescriptor* descriptor,
    GfxAdapter* outAdapter)
{
    if (!instance || !outAdapter) {
        return GFX_RESULT_ERROR_INVALID_PARAMETER;
    }

    auto* inst = reinterpret_cast<gfx::webgpu::Instance*>(instance);

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

    WGPURequestAdapterCallbackInfo callbackInfo = WGPU_REQUEST_ADAPTER_CALLBACK_INFO_INIT;
    callbackInfo.mode = WGPUCallbackMode_WaitAnyOnly;
    callbackInfo.callback = [](WGPURequestAdapterStatus status, WGPUAdapter adapter,
                                WGPUStringView message, void* userdata1, void* userdata2) {
        if (status == WGPURequestAdapterStatus_Success && adapter) {
            struct Context {
                GfxAdapter* outAdapter;
                gfx::webgpu::Instance* instance;
            };
            auto* context = static_cast<Context*>(userdata1);
            auto* adapterObj = new gfx::webgpu::Adapter(adapter, context->instance);
            *context->outAdapter = reinterpret_cast<GfxAdapter>(adapterObj);
        }
    };

    struct {
        GfxAdapter* outAdapter;
        gfx::webgpu::Instance* instance;
    } context = { outAdapter, inst };

    callbackInfo.userdata1 = &context;

    WGPUFuture future = wgpuInstanceRequestAdapter(inst->handle(), &options, callbackInfo);

    WGPUFutureWaitInfo waitInfo = WGPU_FUTURE_WAIT_INFO_INIT;
    waitInfo.future = future;
    wgpuInstanceWaitAny(inst->handle(), 1, &waitInfo, UINT64_MAX);

    return *outAdapter ? GFX_RESULT_SUCCESS : GFX_RESULT_ERROR_UNKNOWN;
}

uint32_t webgpu_instanceEnumerateAdapters(GfxInstance instance, GfxAdapter* adapters, uint32_t maxAdapters)
{
    if (!instance || maxAdapters == 0) {
        return 0;
    }

    GfxAdapter adapter = nullptr;
    if (webgpu_instanceRequestAdapter(instance, nullptr, &adapter) == GFX_RESULT_SUCCESS && adapter) {
        if (adapters) {
            adapters[0] = adapter;
        }
        return 1;
    }
    return 0;
}

void webgpu_adapterDestroy(GfxAdapter adapter)
{
    if (!adapter) {
        return;
    }
    delete reinterpret_cast<gfx::webgpu::Adapter*>(adapter);
}

GfxResult webgpu_adapterCreateDevice(GfxAdapter adapter, const GfxDeviceDescriptor* descriptor,
    GfxDevice* outDevice)
{
    if (!adapter || !outDevice) {
        return GFX_RESULT_ERROR_INVALID_PARAMETER;
    }

    auto* adapterPtr = reinterpret_cast<gfx::webgpu::Adapter*>(adapter);

    WGPUDeviceDescriptor wgpuDesc = WGPU_DEVICE_DESCRIPTOR_INIT;
    if (descriptor && descriptor->label) {
        wgpuDesc.label = gfxStringView(descriptor->label);
    }

    struct DeviceRequestContext {
        GfxDevice* outDevice;
        gfx::webgpu::Adapter* adapter;
    };

    DeviceRequestContext context = { outDevice, adapterPtr };

    WGPURequestDeviceCallbackInfo callbackInfo = WGPU_REQUEST_DEVICE_CALLBACK_INFO_INIT;
    callbackInfo.mode = WGPUCallbackMode_WaitAnyOnly;
    callbackInfo.callback = [](WGPURequestDeviceStatus status, WGPUDevice device,
                                WGPUStringView message, void* userdata1, void* userdata2) {
        if (status == WGPURequestDeviceStatus_Success && device) {
            auto* context = static_cast<DeviceRequestContext*>(userdata1);
            auto* deviceObj = new gfx::webgpu::Device(context->adapter, device);
            *context->outDevice = reinterpret_cast<GfxDevice>(deviceObj);
        }
    };
    callbackInfo.userdata1 = &context;

    WGPUFuture future = wgpuAdapterRequestDevice(adapterPtr->handle(), &wgpuDesc, callbackInfo);

    // Properly wait for the device creation to complete
    if (adapterPtr->getInstance()) {
        WGPUFutureWaitInfo waitInfo = WGPU_FUTURE_WAIT_INFO_INIT;
        waitInfo.future = future;
        wgpuInstanceWaitAny(adapterPtr->getInstance()->handle(), 1, &waitInfo, UINT64_MAX);
    }

    return *outDevice ? GFX_RESULT_SUCCESS : GFX_RESULT_ERROR_UNKNOWN;
}

const char* webgpu_adapterGetName(GfxAdapter adapter)
{
    if (!adapter) {
        return nullptr;
    }
    auto* adapterPtr = reinterpret_cast<gfx::webgpu::Adapter*>(adapter);
    return adapterPtr->getName();
}

GfxBackend webgpu_adapterGetBackend(GfxAdapter adapter)
{
    return adapter ? GFX_BACKEND_WEBGPU : GFX_BACKEND_AUTO;
}

void webgpu_deviceDestroy(GfxDevice device)
{
    if (!device) {
        return;
    }
    delete reinterpret_cast<gfx::webgpu::Device*>(device);
}

GfxQueue webgpu_deviceGetQueue(GfxDevice device)
{
    if (!device) {
        return nullptr;
    }
    auto* devicePtr = reinterpret_cast<gfx::webgpu::Device*>(device);
    return reinterpret_cast<GfxQueue>(devicePtr->getQueue());
}

GfxResult webgpu_deviceCreateSurface(GfxDevice device, const GfxSurfaceDescriptor* descriptor,
    GfxSurface* outSurface)
{
    if (!device || !descriptor || !outSurface) {
        return GFX_RESULT_ERROR_INVALID_PARAMETER;
    }

    auto* devicePtr = reinterpret_cast<gfx::webgpu::Device*>(device);

    // Need instance for surface creation - would need to store it in device
    // For now, create temporary instance
    WGPUInstanceDescriptor instDesc = WGPU_INSTANCE_DESCRIPTOR_INIT;
    WGPUInstance tempInst = wgpuCreateInstance(&instDesc);

    if (!tempInst) {
        return GFX_RESULT_ERROR_UNKNOWN;
    }

    WGPUSurface wgpuSurface = createPlatformSurface(tempInst, &descriptor->windowHandle);
    wgpuInstanceRelease(tempInst);

    if (!wgpuSurface) {
        return GFX_RESULT_ERROR_UNKNOWN;
    }

    auto* surface = new gfx::webgpu::Surface(wgpuSurface, descriptor->windowHandle);
    *outSurface = reinterpret_cast<GfxSurface>(surface);
    return GFX_RESULT_SUCCESS;
}

GfxResult webgpu_deviceCreateSwapchain(GfxDevice device, GfxSurface surface,
    const GfxSwapchainDescriptor* descriptor,
    GfxSwapchain* outSwapchain)
{
    if (!device || !surface || !descriptor || !outSwapchain) {
        return GFX_RESULT_ERROR_INVALID_PARAMETER;
    }

    auto* devicePtr = reinterpret_cast<gfx::webgpu::Device*>(device);
    auto* surfacePtr = reinterpret_cast<gfx::webgpu::Surface*>(surface);

    // Get surface capabilities
    WGPUSurfaceCapabilities capabilities = WGPU_SURFACE_CAPABILITIES_INIT;
    wgpuSurfaceGetCapabilities(surfacePtr->handle(), devicePtr->handle(), &capabilities);

    WGPUTextureFormat format = gfxFormatToWGPUFormat(descriptor->format);
    bool formatSupported = false;
    for (size_t i = 0; i < capabilities.formatCount; ++i) {
        if (capabilities.formats[i] == format) {
            formatSupported = true;
            break;
        }
    }
    if (!formatSupported && capabilities.formatCount > 0) {
        format = capabilities.formats[0];
    }

    WGPUPresentMode presentMode = gfxPresentModeToWGPU(descriptor->presentMode);
    bool presentModeSupported = false;
    for (size_t i = 0; i < capabilities.presentModeCount; ++i) {
        if (capabilities.presentModes[i] == presentMode) {
            presentModeSupported = true;
            break;
        }
    }
    if (!presentModeSupported && capabilities.presentModeCount > 0) {
        presentMode = capabilities.presentModes[0];
    }

    // Configure surface
    WGPUSurfaceConfiguration config = WGPU_SURFACE_CONFIGURATION_INIT;
    config.device = devicePtr->handle();
    config.format = format;
    config.usage = gfxTextureUsageToWGPU(descriptor->usage);
    config.width = descriptor->width;
    config.height = descriptor->height;
    config.presentMode = presentMode;
    config.alphaMode = WGPUCompositeAlphaMode_Auto;

    wgpuSurfaceConfigure(surfacePtr->handle(), &config);

    auto* swapchain = new gfx::webgpu::Swapchain(surfacePtr->handle(), devicePtr->handle(),
        descriptor->width, descriptor->height,
        format, presentMode, descriptor->bufferCount);
    *outSwapchain = reinterpret_cast<GfxSwapchain>(swapchain);

    // Free capabilities
    if (capabilities.formats) {
        free((void*)capabilities.formats);
    }
    if (capabilities.presentModes) {
        free((void*)capabilities.presentModes);
    }
    if (capabilities.alphaModes) {
        free((void*)capabilities.alphaModes);
    }

    return GFX_RESULT_SUCCESS;
}

GfxResult webgpu_deviceCreateBuffer(GfxDevice device, const GfxBufferDescriptor* descriptor,
    GfxBuffer* outBuffer)
{
    if (!device || !descriptor || !outBuffer) {
        return GFX_RESULT_ERROR_INVALID_PARAMETER;
    }

    auto* devicePtr = reinterpret_cast<gfx::webgpu::Device*>(device);

    WGPUBufferDescriptor wgpuDesc = WGPU_BUFFER_DESCRIPTOR_INIT;
    if (descriptor->label) {
        wgpuDesc.label = gfxStringView(descriptor->label);
    }
    wgpuDesc.size = descriptor->size;
    wgpuDesc.usage = gfxBufferUsageToWGPU(descriptor->usage);
    wgpuDesc.mappedAtCreation = descriptor->mappedAtCreation ? WGPU_TRUE : WGPU_FALSE;

    WGPUBuffer wgpuBuffer = wgpuDeviceCreateBuffer(devicePtr->handle(), &wgpuDesc);
    if (!wgpuBuffer) {
        return GFX_RESULT_ERROR_UNKNOWN;
    }

    auto* buffer = new gfx::webgpu::Buffer(wgpuBuffer, descriptor->size, descriptor->usage, devicePtr);
    *outBuffer = reinterpret_cast<GfxBuffer>(buffer);
    return GFX_RESULT_SUCCESS;
}

GfxResult webgpu_deviceCreateTexture(GfxDevice device, const GfxTextureDescriptor* descriptor,
    GfxTexture* outTexture)
{
    if (!device || !descriptor || !outTexture) {
        return GFX_RESULT_ERROR_INVALID_PARAMETER;
    }

    auto* devicePtr = reinterpret_cast<gfx::webgpu::Device*>(device);

    WGPUTextureDescriptor wgpuDesc = WGPU_TEXTURE_DESCRIPTOR_INIT;
    if (descriptor->label) {
        wgpuDesc.label = gfxStringView(descriptor->label);
    }
    wgpuDesc.dimension = gfxTextureTypeToWGPU(descriptor->type);

    // Set size based on texture type
    uint32_t arrayLayers = descriptor->arrayLayerCount > 0 ? descriptor->arrayLayerCount : 1;
    if (descriptor->type == GFX_TEXTURE_TYPE_CUBE) {
        // Cube maps need 6 or 6*N layers
        if (arrayLayers < 6) {
            arrayLayers = 6;
        }
    }

    wgpuDesc.size = { descriptor->size.width, descriptor->size.height,
        descriptor->type == GFX_TEXTURE_TYPE_3D ? descriptor->size.depth : arrayLayers };
    wgpuDesc.mipLevelCount = descriptor->mipLevelCount;
    wgpuDesc.sampleCount = descriptor->sampleCount;
    wgpuDesc.format = gfxFormatToWGPUFormat(descriptor->format);
    wgpuDesc.usage = gfxTextureUsageToWGPU(descriptor->usage);

    WGPUTexture wgpuTexture = wgpuDeviceCreateTexture(devicePtr->handle(), &wgpuDesc);
    if (!wgpuTexture) {
        return GFX_RESULT_ERROR_UNKNOWN;
    }

    auto* texture = new gfx::webgpu::Texture(wgpuTexture, wgpuDesc.size, wgpuDesc.format,
        descriptor->mipLevelCount, descriptor->sampleCount,
        wgpuDesc.usage);
    *outTexture = reinterpret_cast<GfxTexture>(texture);
    return GFX_RESULT_SUCCESS;
}

GfxResult webgpu_deviceCreateSampler(GfxDevice device, const GfxSamplerDescriptor* descriptor,
    GfxSampler* outSampler)
{
    if (!device || !descriptor || !outSampler) {
        return GFX_RESULT_ERROR_INVALID_PARAMETER;
    }

    auto* devicePtr = reinterpret_cast<gfx::webgpu::Device*>(device);

    WGPUSamplerDescriptor wgpuDesc = WGPU_SAMPLER_DESCRIPTOR_INIT;
    if (descriptor->label) {
        wgpuDesc.label = gfxStringView(descriptor->label);
    }

    // Convert address modes
    switch (descriptor->addressModeU) {
    case GFX_ADDRESS_MODE_REPEAT:
        wgpuDesc.addressModeU = WGPUAddressMode_Repeat;
        break;
    case GFX_ADDRESS_MODE_MIRROR_REPEAT:
        wgpuDesc.addressModeU = WGPUAddressMode_MirrorRepeat;
        break;
    case GFX_ADDRESS_MODE_CLAMP_TO_EDGE:
        wgpuDesc.addressModeU = WGPUAddressMode_ClampToEdge;
        break;
    default:
        wgpuDesc.addressModeU = WGPUAddressMode_ClampToEdge;
        break;
    }

    switch (descriptor->addressModeV) {
    case GFX_ADDRESS_MODE_REPEAT:
        wgpuDesc.addressModeV = WGPUAddressMode_Repeat;
        break;
    case GFX_ADDRESS_MODE_MIRROR_REPEAT:
        wgpuDesc.addressModeV = WGPUAddressMode_MirrorRepeat;
        break;
    case GFX_ADDRESS_MODE_CLAMP_TO_EDGE:
        wgpuDesc.addressModeV = WGPUAddressMode_ClampToEdge;
        break;
    default:
        wgpuDesc.addressModeV = WGPUAddressMode_ClampToEdge;
        break;
    }

    switch (descriptor->addressModeW) {
    case GFX_ADDRESS_MODE_REPEAT:
        wgpuDesc.addressModeW = WGPUAddressMode_Repeat;
        break;
    case GFX_ADDRESS_MODE_MIRROR_REPEAT:
        wgpuDesc.addressModeW = WGPUAddressMode_MirrorRepeat;
        break;
    case GFX_ADDRESS_MODE_CLAMP_TO_EDGE:
        wgpuDesc.addressModeW = WGPUAddressMode_ClampToEdge;
        break;
    default:
        wgpuDesc.addressModeW = WGPUAddressMode_ClampToEdge;
        break;
    }

    // Convert filter modes
    wgpuDesc.magFilter = (descriptor->magFilter == GFX_FILTER_MODE_LINEAR) ? WGPUFilterMode_Linear : WGPUFilterMode_Nearest;
    wgpuDesc.minFilter = (descriptor->minFilter == GFX_FILTER_MODE_LINEAR) ? WGPUFilterMode_Linear : WGPUFilterMode_Nearest;
    wgpuDesc.mipmapFilter = (descriptor->mipmapFilter == GFX_FILTER_MODE_LINEAR) ? WGPUMipmapFilterMode_Linear : WGPUMipmapFilterMode_Nearest;

    wgpuDesc.lodMinClamp = descriptor->lodMinClamp;
    wgpuDesc.lodMaxClamp = descriptor->lodMaxClamp;
    wgpuDesc.maxAnisotropy = descriptor->maxAnisotropy;

    if (descriptor->compare) {
        wgpuDesc.compare = gfxCompareFunctionToWGPU(*descriptor->compare);
    }

    WGPUSampler wgpuSampler = wgpuDeviceCreateSampler(devicePtr->handle(), &wgpuDesc);
    if (!wgpuSampler) {
        return GFX_RESULT_ERROR_UNKNOWN;
    }

    auto* sampler = new gfx::webgpu::Sampler(wgpuSampler);
    *outSampler = reinterpret_cast<GfxSampler>(sampler);
    return GFX_RESULT_SUCCESS;
}

GfxResult webgpu_deviceCreateShader(GfxDevice device, const GfxShaderDescriptor* descriptor,
    GfxShader* outShader)
{
    if (!device || !descriptor || !descriptor->code || !outShader) {
        return GFX_RESULT_ERROR_INVALID_PARAMETER;
    }

    auto* devicePtr = reinterpret_cast<gfx::webgpu::Device*>(device);

    WGPUShaderModuleDescriptor wgpuDesc = WGPU_SHADER_MODULE_DESCRIPTOR_INIT;
    if (descriptor->label) {
        wgpuDesc.label = gfxStringView(descriptor->label);
    }

    WGPUShaderSourceWGSL wgslSource = WGPU_SHADER_SOURCE_WGSL_INIT;
    wgslSource.code = gfxStringView(descriptor->code);
    wgpuDesc.nextInChain = (WGPUChainedStruct*)&wgslSource;

    WGPUShaderModule wgpuModule = wgpuDeviceCreateShaderModule(devicePtr->handle(), &wgpuDesc);
    if (!wgpuModule) {
        return GFX_RESULT_ERROR_UNKNOWN;
    }

    auto* shader = new gfx::webgpu::Shader(wgpuModule);
    *outShader = reinterpret_cast<GfxShader>(shader);
    return GFX_RESULT_SUCCESS;
}

GfxResult webgpu_deviceCreateBindGroupLayout(GfxDevice device, const GfxBindGroupLayoutDescriptor* descriptor,
    GfxBindGroupLayout* outLayout)
{
    if (!device || !descriptor || !outLayout) {
        return GFX_RESULT_ERROR_INVALID_PARAMETER;
    }

    auto* devicePtr = reinterpret_cast<gfx::webgpu::Device*>(device);

    WGPUBindGroupLayoutDescriptor wgpuDesc = WGPU_BIND_GROUP_LAYOUT_DESCRIPTOR_INIT;
    if (descriptor->label) {
        wgpuDesc.label = gfxStringView(descriptor->label);
    }

    std::vector<WGPUBindGroupLayoutEntry> entries;
    if (descriptor->entryCount > 0 && descriptor->entries) {
        entries.reserve(descriptor->entryCount);

        for (uint32_t i = 0; i < descriptor->entryCount; ++i) {
            const auto& entry = descriptor->entries[i];
            WGPUBindGroupLayoutEntry wgpuEntry = WGPU_BIND_GROUP_LAYOUT_ENTRY_INIT;

            wgpuEntry.binding = entry.binding;
            wgpuEntry.visibility = WGPUShaderStage_None;
            if (entry.visibility & GFX_SHADER_STAGE_VERTEX) {
                wgpuEntry.visibility |= WGPUShaderStage_Vertex;
            }
            if (entry.visibility & GFX_SHADER_STAGE_FRAGMENT) {
                wgpuEntry.visibility |= WGPUShaderStage_Fragment;
            }
            if (entry.visibility & GFX_SHADER_STAGE_COMPUTE) {
                wgpuEntry.visibility |= WGPUShaderStage_Compute;
            }

            switch (entry.type) {
            case GFX_BINDING_TYPE_BUFFER:
                wgpuEntry.buffer.type = WGPUBufferBindingType_Uniform;
                wgpuEntry.buffer.hasDynamicOffset = entry.buffer.hasDynamicOffset ? WGPU_TRUE : WGPU_FALSE;
                wgpuEntry.buffer.minBindingSize = entry.buffer.minBindingSize;
                break;
            case GFX_BINDING_TYPE_SAMPLER:
                wgpuEntry.sampler.type = entry.sampler.comparison ? WGPUSamplerBindingType_Comparison : WGPUSamplerBindingType_Filtering;
                break;
            case GFX_BINDING_TYPE_TEXTURE:
                wgpuEntry.texture.sampleType = gfxTextureSampleTypeToWGPU(entry.texture.sampleType);
                wgpuEntry.texture.viewDimension = gfxTextureViewTypeToWGPU(entry.texture.viewDimension);
                wgpuEntry.texture.multisampled = entry.texture.multisampled ? WGPU_TRUE : WGPU_FALSE;
                break;
            case GFX_BINDING_TYPE_STORAGE_TEXTURE:
                wgpuEntry.storageTexture.access = entry.storageTexture.writeOnly ? WGPUStorageTextureAccess_WriteOnly : WGPUStorageTextureAccess_ReadOnly;
                wgpuEntry.storageTexture.format = gfxFormatToWGPUFormat(entry.storageTexture.format);
                wgpuEntry.storageTexture.viewDimension = gfxTextureViewTypeToWGPU(entry.storageTexture.viewDimension);
                break;
            }

            entries.push_back(wgpuEntry);
        }

        wgpuDesc.entries = entries.data();
        wgpuDesc.entryCount = static_cast<uint32_t>(entries.size());
    }

    WGPUBindGroupLayout wgpuLayout = wgpuDeviceCreateBindGroupLayout(devicePtr->handle(), &wgpuDesc);
    if (!wgpuLayout) {
        return GFX_RESULT_ERROR_UNKNOWN;
    }

    auto* layout = new gfx::webgpu::BindGroupLayout(wgpuLayout);
    *outLayout = reinterpret_cast<GfxBindGroupLayout>(layout);
    return GFX_RESULT_SUCCESS;
}

GfxResult webgpu_deviceCreateBindGroup(GfxDevice device, const GfxBindGroupDescriptor* descriptor,
    GfxBindGroup* outBindGroup)
{
    if (!device || !descriptor || !descriptor->layout || !outBindGroup) {
        return GFX_RESULT_ERROR_INVALID_PARAMETER;
    }

    auto* devicePtr = reinterpret_cast<gfx::webgpu::Device*>(device);
    auto* layoutPtr = reinterpret_cast<gfx::webgpu::BindGroupLayout*>(descriptor->layout);

    WGPUBindGroupDescriptor wgpuDesc = WGPU_BIND_GROUP_DESCRIPTOR_INIT;
    if (descriptor->label) {
        wgpuDesc.label = gfxStringView(descriptor->label);
    }
    wgpuDesc.layout = layoutPtr->handle();

    std::vector<WGPUBindGroupEntry> entries;
    if (descriptor->entryCount > 0 && descriptor->entries) {
        entries.reserve(descriptor->entryCount);

        for (uint32_t i = 0; i < descriptor->entryCount; ++i) {
            const auto& entry = descriptor->entries[i];
            WGPUBindGroupEntry wgpuEntry = WGPU_BIND_GROUP_ENTRY_INIT;

            wgpuEntry.binding = entry.binding;

            switch (entry.type) {
            case GFX_BIND_GROUP_ENTRY_TYPE_BUFFER: {
                auto* buffer = reinterpret_cast<gfx::webgpu::Buffer*>(entry.resource.buffer.buffer);
                wgpuEntry.buffer = buffer->handle();
                wgpuEntry.offset = entry.resource.buffer.offset;
                wgpuEntry.size = entry.resource.buffer.size;
                break;
            }
            case GFX_BIND_GROUP_ENTRY_TYPE_SAMPLER: {
                auto* sampler = reinterpret_cast<gfx::webgpu::Sampler*>(entry.resource.sampler);
                wgpuEntry.sampler = sampler->handle();
                break;
            }
            case GFX_BIND_GROUP_ENTRY_TYPE_TEXTURE_VIEW: {
                auto* textureView = reinterpret_cast<gfx::webgpu::TextureView*>(entry.resource.textureView);
                wgpuEntry.textureView = textureView->handle();
                break;
            }
            }

            entries.push_back(wgpuEntry);
        }

        wgpuDesc.entries = entries.data();
        wgpuDesc.entryCount = static_cast<uint32_t>(entries.size());
    }

    WGPUBindGroup wgpuBindGroup = wgpuDeviceCreateBindGroup(devicePtr->handle(), &wgpuDesc);
    if (!wgpuBindGroup) {
        return GFX_RESULT_ERROR_UNKNOWN;
    }

    auto* bindGroup = new gfx::webgpu::BindGroup(wgpuBindGroup);
    *outBindGroup = reinterpret_cast<GfxBindGroup>(bindGroup);
    return GFX_RESULT_SUCCESS;
}

GfxResult webgpu_deviceCreateRenderPipeline(GfxDevice device, const GfxRenderPipelineDescriptor* descriptor,
    GfxRenderPipeline* outPipeline)
{
    if (!device || !descriptor || !outPipeline) {
        return GFX_RESULT_ERROR_INVALID_PARAMETER;
    }

    auto* devicePtr = reinterpret_cast<gfx::webgpu::Device*>(device);

    WGPURenderPipelineDescriptor wgpuDesc = WGPU_RENDER_PIPELINE_DESCRIPTOR_INIT;
    if (descriptor->label) {
        wgpuDesc.label = gfxStringView(descriptor->label);
    }

    // Vertex state
    auto* vertexShader = reinterpret_cast<gfx::webgpu::Shader*>(descriptor->vertex.module);
    WGPUVertexState vertexState = WGPU_VERTEX_STATE_INIT;
    vertexState.module = vertexShader->handle();
    vertexState.entryPoint = gfxStringView(descriptor->vertex.entryPoint);

    // Convert vertex buffers
    std::vector<WGPUVertexBufferLayout> vertexBuffers;
    std::vector<std::vector<WGPUVertexAttribute>> allAttributes;

    if (descriptor->vertex.bufferCount > 0) {
        vertexBuffers.reserve(descriptor->vertex.bufferCount);
        allAttributes.reserve(descriptor->vertex.bufferCount);

        for (uint32_t i = 0; i < descriptor->vertex.bufferCount; ++i) {
            const auto& buffer = descriptor->vertex.buffers[i];

            std::vector<WGPUVertexAttribute> attributes;
            attributes.reserve(buffer.attributeCount);

            for (uint32_t j = 0; j < buffer.attributeCount; ++j) {
                const auto& attr = buffer.attributes[j];
                WGPUVertexAttribute wgpuAttr = WGPU_VERTEX_ATTRIBUTE_INIT;
                wgpuAttr.format = gfxFormatToWGPUFormat(attr.format);
                wgpuAttr.offset = attr.offset;
                wgpuAttr.shaderLocation = attr.shaderLocation;
                attributes.push_back(wgpuAttr);
            }

            allAttributes.push_back(std::move(attributes));

            WGPUVertexBufferLayout wgpuBuffer = WGPU_VERTEX_BUFFER_LAYOUT_INIT;
            wgpuBuffer.arrayStride = buffer.arrayStride;
            wgpuBuffer.stepMode = buffer.stepModeInstance ? WGPUVertexStepMode_Instance : WGPUVertexStepMode_Vertex;
            wgpuBuffer.attributes = allAttributes.back().data();
            wgpuBuffer.attributeCount = static_cast<uint32_t>(allAttributes.back().size());
            vertexBuffers.push_back(wgpuBuffer);
        }

        vertexState.buffers = vertexBuffers.data();
        vertexState.bufferCount = static_cast<uint32_t>(vertexBuffers.size());
    }

    wgpuDesc.vertex = vertexState;

    // Fragment state (optional)
    WGPUFragmentState fragmentState = WGPU_FRAGMENT_STATE_INIT;
    std::vector<WGPUColorTargetState> colorTargets;
    std::vector<WGPUBlendState> blendStates;

    if (descriptor->fragment) {
        auto* fragmentShader = reinterpret_cast<gfx::webgpu::Shader*>(descriptor->fragment->module);
        fragmentState.module = fragmentShader->handle();
        fragmentState.entryPoint = gfxStringView(descriptor->fragment->entryPoint);

        if (descriptor->fragment->targetCount > 0) {
            colorTargets.reserve(descriptor->fragment->targetCount);

            for (uint32_t i = 0; i < descriptor->fragment->targetCount; ++i) {
                const auto& target = descriptor->fragment->targets[i];
                WGPUColorTargetState wgpuTarget = WGPU_COLOR_TARGET_STATE_INIT;
                wgpuTarget.format = gfxFormatToWGPUFormat(target.format);
                wgpuTarget.writeMask = target.writeMask;

                if (target.blend) {
                    WGPUBlendState blend = WGPU_BLEND_STATE_INIT;

                    // Color blend
                    blend.color.operation = gfxBlendOperationToWGPU(target.blend->color.operation);
                    blend.color.srcFactor = gfxBlendFactorToWGPU(target.blend->color.srcFactor);
                    blend.color.dstFactor = gfxBlendFactorToWGPU(target.blend->color.dstFactor);

                    // Alpha blend
                    blend.alpha.operation = gfxBlendOperationToWGPU(target.blend->alpha.operation);
                    blend.alpha.srcFactor = gfxBlendFactorToWGPU(target.blend->alpha.srcFactor);
                    blend.alpha.dstFactor = gfxBlendFactorToWGPU(target.blend->alpha.dstFactor);

                    blendStates.push_back(blend);
                    wgpuTarget.blend = &blendStates.back();
                }

                colorTargets.push_back(wgpuTarget);
            }

            fragmentState.targets = colorTargets.data();
            fragmentState.targetCount = static_cast<uint32_t>(colorTargets.size());
        }

        wgpuDesc.fragment = &fragmentState;
    }

    // Primitive state
    WGPUPrimitiveState primitiveState = WGPU_PRIMITIVE_STATE_INIT;
    primitiveState.topology = gfxPrimitiveTopologyToWGPU(descriptor->primitive.topology);
    primitiveState.frontFace = descriptor->primitive.frontFaceCounterClockwise ? WGPUFrontFace_CCW : WGPUFrontFace_CW;
    primitiveState.cullMode = descriptor->primitive.cullBackFace ? WGPUCullMode_Back : WGPUCullMode_None;

    if (descriptor->primitive.stripIndexFormat) {
        primitiveState.stripIndexFormat = gfxIndexFormatToWGPU(*descriptor->primitive.stripIndexFormat);
    }

    wgpuDesc.primitive = primitiveState;

    // Multisample state
    WGPUMultisampleState multisampleState = WGPU_MULTISAMPLE_STATE_INIT;
    multisampleState.count = descriptor->sampleCount;
    wgpuDesc.multisample = multisampleState;

    WGPURenderPipeline wgpuPipeline = wgpuDeviceCreateRenderPipeline(devicePtr->handle(), &wgpuDesc);
    if (!wgpuPipeline) {
        return GFX_RESULT_ERROR_UNKNOWN;
    }

    auto* pipeline = new gfx::webgpu::RenderPipeline(wgpuPipeline);
    *outPipeline = reinterpret_cast<GfxRenderPipeline>(pipeline);
    return GFX_RESULT_SUCCESS;
}

GfxResult webgpu_deviceCreateComputePipeline(GfxDevice device, const GfxComputePipelineDescriptor* descriptor,
    GfxComputePipeline* outPipeline)
{
    if (!device || !descriptor || !descriptor->compute || !outPipeline) {
        return GFX_RESULT_ERROR_INVALID_PARAMETER;
    }

    auto* devicePtr = reinterpret_cast<gfx::webgpu::Device*>(device);
    auto* shader = reinterpret_cast<gfx::webgpu::Shader*>(descriptor->compute);

    WGPUComputePipelineDescriptor wgpuDesc = WGPU_COMPUTE_PIPELINE_DESCRIPTOR_INIT;
    if (descriptor->label) {
        wgpuDesc.label = gfxStringView(descriptor->label);
    }

    wgpuDesc.compute.module = shader->handle();
    wgpuDesc.compute.entryPoint = gfxStringView(descriptor->entryPoint);

    WGPUComputePipeline wgpuPipeline = wgpuDeviceCreateComputePipeline(devicePtr->handle(), &wgpuDesc);
    if (!wgpuPipeline) {
        return GFX_RESULT_ERROR_UNKNOWN;
    }

    auto* pipeline = new gfx::webgpu::ComputePipeline(wgpuPipeline);
    *outPipeline = reinterpret_cast<GfxComputePipeline>(pipeline);
    return GFX_RESULT_SUCCESS;
}

GfxResult webgpu_deviceCreateCommandEncoder(GfxDevice device, const char* label,
    GfxCommandEncoder* outEncoder)
{
    if (!device || !outEncoder) {
        return GFX_RESULT_ERROR_INVALID_PARAMETER;
    }

    auto* devicePtr = reinterpret_cast<gfx::webgpu::Device*>(device);

    WGPUCommandEncoderDescriptor wgpuDesc = WGPU_COMMAND_ENCODER_DESCRIPTOR_INIT;
    if (label) {
        wgpuDesc.label = gfxStringView(label);
    }

    WGPUCommandEncoder wgpuEncoder = wgpuDeviceCreateCommandEncoder(devicePtr->handle(), &wgpuDesc);
    if (!wgpuEncoder) {
        return GFX_RESULT_ERROR_UNKNOWN;
    }

    auto* encoder = new gfx::webgpu::CommandEncoder(wgpuEncoder);
    *outEncoder = reinterpret_cast<GfxCommandEncoder>(encoder);
    return GFX_RESULT_SUCCESS;
}

GfxResult webgpu_deviceCreateFence(GfxDevice device, const GfxFenceDescriptor* descriptor,
    GfxFence* outFence)
{
    if (!device || !descriptor || !outFence) {
        return GFX_RESULT_ERROR_INVALID_PARAMETER;
    }

    auto* fence = new gfx::webgpu::Fence(descriptor->signaled);
    *outFence = reinterpret_cast<GfxFence>(fence);
    return GFX_RESULT_SUCCESS;
}

GfxResult webgpu_deviceCreateSemaphore(GfxDevice device, const GfxSemaphoreDescriptor* descriptor,
    GfxSemaphore* outSemaphore)
{
    if (!device || !descriptor || !outSemaphore) {
        return GFX_RESULT_ERROR_INVALID_PARAMETER;
    }

    auto* semaphore = new gfx::webgpu::Semaphore(descriptor->type, descriptor->initialValue);
    *outSemaphore = reinterpret_cast<GfxSemaphore>(semaphore);
    return GFX_RESULT_SUCCESS;
}

void webgpu_deviceWaitIdle(GfxDevice device)
{
    if (!device) {
        return;
    }
    auto* devicePtr = reinterpret_cast<gfx::webgpu::Device*>(device);

    if (devicePtr->getQueue()) {
        WGPUQueueWorkDoneCallbackInfo callbackInfo = WGPU_QUEUE_WORK_DONE_CALLBACK_INFO_INIT;
        callbackInfo.mode = WGPUCallbackMode_WaitAnyOnly;
        wgpuQueueOnSubmittedWorkDone(devicePtr->getQueue()->handle(), callbackInfo);
    }
}

void webgpu_deviceGetLimits(GfxDevice device, GfxDeviceLimits* outLimits)
{
    if (!device || !outLimits) {
        return;
    }
    auto* devicePtr = reinterpret_cast<gfx::webgpu::Device*>(device);

    WGPUSupportedLimits limits{};
    limits.nextInChain = nullptr;
    wgpuDeviceGetLimits(devicePtr->handle(), &limits);

    outLimits->minUniformBufferOffsetAlignment = limits.limits.minUniformBufferOffsetAlignment;
    outLimits->minStorageBufferOffsetAlignment = limits.limits.minStorageBufferOffsetAlignment;
    outLimits->maxUniformBufferBindingSize = limits.limits.maxUniformBufferBindingSize;
    outLimits->maxStorageBufferBindingSize = limits.limits.maxStorageBufferBindingSize;
    outLimits->maxBufferSize = limits.limits.maxBufferSize;
    outLimits->maxTextureDimension1D = limits.limits.maxTextureDimension1D;
    outLimits->maxTextureDimension2D = limits.limits.maxTextureDimension2D;
    outLimits->maxTextureDimension3D = limits.limits.maxTextureDimension3D;
    outLimits->maxTextureArrayLayers = limits.limits.maxTextureArrayLayers;
}

// Surface functions
void webgpu_surfaceDestroy(GfxSurface surface)
{
    if (!surface) {
        return;
    }
    delete reinterpret_cast<gfx::webgpu::Surface*>(surface);
}

uint32_t webgpu_surfaceGetSupportedFormats(GfxSurface surface, GfxTextureFormat* formats, uint32_t maxFormats)
{
    // WebGPU surface capabilities need device - not available at surface level
    return 0;
}

uint32_t webgpu_surfaceGetSupportedPresentModes(GfxSurface surface, GfxPresentMode* presentModes, uint32_t maxModes)
{
    // WebGPU surface capabilities need device - not available at surface level
    return 0;
}

GfxPlatformWindowHandle webgpu_surfaceGetPlatformHandle(GfxSurface surface)
{
    if (!surface) {
        GfxPlatformWindowHandle handle = {};
        return handle;
    }
    return reinterpret_cast<gfx::webgpu::Surface*>(surface)->getWindowHandle();
}

// Swapchain functions
void webgpu_swapchainDestroy(GfxSwapchain swapchain)
{
    if (!swapchain) {
        return;
    }
    delete reinterpret_cast<gfx::webgpu::Swapchain*>(swapchain);
}

uint32_t webgpu_swapchainGetWidth(GfxSwapchain swapchain)
{
    if (!swapchain) {
        return 0;
    }
    return reinterpret_cast<gfx::webgpu::Swapchain*>(swapchain)->getWidth();
}

uint32_t webgpu_swapchainGetHeight(GfxSwapchain swapchain)
{
    if (!swapchain) {
        return 0;
    }
    return reinterpret_cast<gfx::webgpu::Swapchain*>(swapchain)->getHeight();
}

GfxTextureFormat webgpu_swapchainGetFormat(GfxSwapchain swapchain)
{
    if (!swapchain) {
        return GFX_TEXTURE_FORMAT_UNDEFINED;
    }
    auto* swapchainPtr = reinterpret_cast<gfx::webgpu::Swapchain*>(swapchain);
    return wgpuFormatToGfxFormat(swapchainPtr->getFormat());
}

uint32_t webgpu_swapchainGetBufferCount(GfxSwapchain swapchain)
{
    if (!swapchain) {
        return 0;
    }
    return reinterpret_cast<gfx::webgpu::Swapchain*>(swapchain)->getBufferCount();
}

GfxResult webgpu_swapchainAcquireNextImage(GfxSwapchain swapchain, uint64_t timeoutNs,
    GfxSemaphore imageAvailableSemaphore, GfxFence fence, uint32_t* outImageIndex)
{
    if (!swapchain || !outImageIndex) {
        return GFX_RESULT_ERROR_INVALID_PARAMETER;
    }

    // WebGPU doesn't have explicit acquire semantics with semaphores
    // The surface texture is acquired implicitly when we call wgpuSurfaceGetCurrentTexture
    // For now, we just return image index 0 (WebGPU doesn't expose multiple image indices)
    // The semaphore and fence are noted but WebGPU doesn't support explicit synchronization primitives

    (void)timeoutNs;
    (void)imageAvailableSemaphore; // WebGPU doesn't support explicit semaphore signaling

    auto* swapchainPtr = reinterpret_cast<gfx::webgpu::Swapchain*>(swapchain);

    // Get current texture to check status
    WGPUSurfaceTexture surfaceTexture = WGPU_SURFACE_TEXTURE_INIT;
    wgpuSurfaceGetCurrentTexture(swapchainPtr->surface(), &surfaceTexture);

    GfxResult result = GFX_RESULT_SUCCESS;
    switch (surfaceTexture.status) {
    case WGPUSurfaceGetCurrentTextureStatus_SuccessOptimal:
    case WGPUSurfaceGetCurrentTextureStatus_SuccessSuboptimal:
        *outImageIndex = 0; // WebGPU only exposes current image
        result = GFX_RESULT_SUCCESS;
        break;
    case WGPUSurfaceGetCurrentTextureStatus_Timeout:
        result = GFX_RESULT_TIMEOUT;
        break;
    case WGPUSurfaceGetCurrentTextureStatus_Outdated:
        result = GFX_RESULT_ERROR_OUT_OF_DATE;
        break;
    case WGPUSurfaceGetCurrentTextureStatus_Lost:
        result = GFX_RESULT_ERROR_SURFACE_LOST;
        break;
    default:
        result = GFX_RESULT_ERROR_UNKNOWN;
        break;
    }

    if (surfaceTexture.texture) {
        wgpuTextureRelease(surfaceTexture.texture);
    }

    // Signal fence if provided (even though WebGPU doesn't truly have fences)
    if (fence && result == GFX_RESULT_SUCCESS) {
        auto* fencePtr = reinterpret_cast<gfx::webgpu::Fence*>(fence);
        fencePtr->setSignaled(true);
    }

    return result;
}

GfxTextureView webgpu_swapchainGetImageView(GfxSwapchain swapchain, uint32_t imageIndex)
{
    if (!swapchain) {
        return nullptr;
    }

    // WebGPU doesn't expose multiple swapchain images by index
    // Always return the current texture view regardless of index
    (void)imageIndex;

    return webgpu_swapchainGetCurrentTextureView(swapchain);
}

GfxTextureView webgpu_swapchainGetCurrentTextureView(GfxSwapchain swapchain)
{
    if (!swapchain) {
        return nullptr;
    }

    auto* swapchainPtr = reinterpret_cast<gfx::webgpu::Swapchain*>(swapchain);

    WGPUSurfaceTexture surfaceTexture = WGPU_SURFACE_TEXTURE_INIT;
    wgpuSurfaceGetCurrentTexture(swapchainPtr->surface(), &surfaceTexture);

    if (surfaceTexture.status != WGPUSurfaceGetCurrentTextureStatus_SuccessOptimal || !surfaceTexture.texture) {
        return nullptr;
    }

    WGPUTextureView wgpuView = wgpuTextureCreateView(surfaceTexture.texture, nullptr);
    if (!wgpuView) {
        wgpuTextureRelease(surfaceTexture.texture);
        return nullptr;
    }

    auto* view = new gfx::webgpu::TextureView(wgpuView, nullptr);
    return reinterpret_cast<GfxTextureView>(view);
}

GfxResult webgpu_swapchainPresent(GfxSwapchain swapchain, const GfxPresentInfo* presentInfo)
{
    if (!swapchain) {
        return GFX_RESULT_ERROR_INVALID_PARAMETER;
    }

    // WebGPU doesn't support explicit wait semaphores for present
    // The queue submission already ensures ordering, so we just present
    (void)presentInfo; // Wait semaphores are noted but not used in WebGPU

    auto* swapchainPtr = reinterpret_cast<gfx::webgpu::Swapchain*>(swapchain);
    wgpuSurfacePresent(swapchainPtr->surface());

    // Check if presentation succeeded by checking surface status
    WGPUSurfaceTexture surfaceTexture = WGPU_SURFACE_TEXTURE_INIT;
    wgpuSurfaceGetCurrentTexture(swapchainPtr->surface(), &surfaceTexture);

    bool presentOk = (surfaceTexture.status == WGPUSurfaceGetCurrentTextureStatus_SuccessOptimal || surfaceTexture.status == WGPUSurfaceGetCurrentTextureStatus_SuccessSuboptimal);

    if (surfaceTexture.texture) {
        wgpuTextureRelease(surfaceTexture.texture);
    }

    return presentOk ? GFX_RESULT_SUCCESS : GFX_RESULT_ERROR_UNKNOWN;
}

// Buffer functions
void webgpu_bufferDestroy(GfxBuffer buffer)
{
    if (!buffer) {
        return;
    }
    delete reinterpret_cast<gfx::webgpu::Buffer*>(buffer);
}

uint64_t webgpu_bufferGetSize(GfxBuffer buffer)
{
    if (!buffer) {
        return 0;
    }
    return reinterpret_cast<gfx::webgpu::Buffer*>(buffer)->getSize();
}

GfxBufferUsage webgpu_bufferGetUsage(GfxBuffer buffer)
{
    if (!buffer) {
        return GFX_BUFFER_USAGE_NONE;
    }
    return reinterpret_cast<gfx::webgpu::Buffer*>(buffer)->getUsage();
}

GfxResult webgpu_bufferMap(GfxBuffer buffer, uint64_t offset, uint64_t size, void** mappedPointer)
{
    if (!buffer || !mappedPointer) {
        return GFX_RESULT_ERROR_INVALID_PARAMETER;
    }

    auto* bufferPtr = reinterpret_cast<gfx::webgpu::Buffer*>(buffer);
    
    // If size is 0, map the entire buffer from offset
    uint64_t mapSize = size;
    if (mapSize == 0) {
        mapSize = bufferPtr->getSize() - offset;
    }

    // Determine map mode based on buffer usage
    WGPUMapMode mapMode = WGPUMapMode_None;
    if (bufferPtr->getUsage() & GFX_BUFFER_USAGE_MAP_READ) {
        mapMode |= WGPUMapMode_Read;
    }
    if (bufferPtr->getUsage() & GFX_BUFFER_USAGE_MAP_WRITE) {
        mapMode |= WGPUMapMode_Write;
    }

    if (mapMode == WGPUMapMode_None) {
        return GFX_RESULT_ERROR_INVALID_PARAMETER;
    }

    // Set up async mapping with synchronous wait
    struct MapCallbackData {
        WGPUMapAsyncStatus status = WGPUMapAsyncStatus_Unknown;
        bool completed = false;
    };

    MapCallbackData callbackData;

    WGPUBufferMapCallbackInfo callbackInfo = WGPU_BUFFER_MAP_CALLBACK_INFO_INIT;
    callbackInfo.mode = WGPUCallbackMode_WaitAnyOnly;
    callbackInfo.callback = [](WGPUMapAsyncStatus status, WGPUStringView message, void* userdata1, void* userdata2) {
        auto* data = static_cast<MapCallbackData*>(userdata1);
        data->status = status;
        data->completed = true;
    };
    callbackInfo.userdata1 = &callbackData;

    WGPUFuture future = wgpuBufferMapAsync(bufferPtr->handle(), mapMode, offset, mapSize, callbackInfo);

    // Properly wait for the mapping to complete
    if (bufferPtr->getDevice() && bufferPtr->getDevice()->getAdapter() && bufferPtr->getDevice()->getAdapter()->getInstance()) {
        WGPUFutureWaitInfo waitInfo = WGPU_FUTURE_WAIT_INFO_INIT;
        waitInfo.future = future;
        wgpuInstanceWaitAny(bufferPtr->getDevice()->getAdapter()->getInstance()->handle(), 1, &waitInfo, UINT64_MAX);
    }

    if (!callbackData.completed || callbackData.status != WGPUMapAsyncStatus_Success) {
        return GFX_RESULT_ERROR_UNKNOWN;
    }

    // Get the mapped range
    void* mappedData = wgpuBufferGetMappedRange(bufferPtr->handle(), offset, mapSize);
    if (!mappedData) {
        wgpuBufferUnmap(bufferPtr->handle());
        return GFX_RESULT_ERROR_UNKNOWN;
    }

    *mappedPointer = mappedData;
    return GFX_RESULT_SUCCESS;
}

void webgpu_bufferUnmap(GfxBuffer buffer)
{
    if (!buffer) {
        return;
    }
    auto* bufferPtr = reinterpret_cast<gfx::webgpu::Buffer*>(buffer);
    wgpuBufferUnmap(bufferPtr->handle());
}

// Texture functions
void webgpu_textureDestroy(GfxTexture texture)
{
    if (!texture) {
        return;
    }
    delete reinterpret_cast<gfx::webgpu::Texture*>(texture);
}

GfxExtent3D webgpu_textureGetSize(GfxTexture texture)
{
    if (!texture) {
        return { 0, 0, 0 };
    }
    auto* texturePtr = reinterpret_cast<gfx::webgpu::Texture*>(texture);
    WGPUExtent3D size = texturePtr->getSize();
    return { size.width, size.height, size.depth };
}

GfxTextureFormat webgpu_textureGetFormat(GfxTexture texture)
{
    if (!texture) {
        return GFX_TEXTURE_FORMAT_UNDEFINED;
    }
    auto* texturePtr = reinterpret_cast<gfx::webgpu::Texture*>(texture);
    return wgpuFormatToGfxFormat(texturePtr->getFormat());
}

uint32_t webgpu_textureGetMipLevelCount(GfxTexture texture)
{
    if (!texture) {
        return 0;
    }
    return reinterpret_cast<gfx::webgpu::Texture*>(texture)->getMipLevels();
}

uint32_t webgpu_textureGetSampleCount(GfxTexture texture)
{
    if (!texture) {
        return 0;
    }
    return reinterpret_cast<gfx::webgpu::Texture*>(texture)->getSampleCount();
}

GfxTextureUsage webgpu_textureGetUsage(GfxTexture texture)
{
    if (!texture) {
        return GFX_TEXTURE_USAGE_NONE;
    }

    auto* texturePtr = reinterpret_cast<gfx::webgpu::Texture*>(texture);
    WGPUTextureUsage wgpuUsage = texturePtr->getUsage();

    GfxTextureUsage usage = GFX_TEXTURE_USAGE_NONE;
    if (wgpuUsage & WGPUTextureUsage_CopySrc) {
        usage |= GFX_TEXTURE_USAGE_COPY_SRC;
    }
    if (wgpuUsage & WGPUTextureUsage_CopyDst) {
        usage |= GFX_TEXTURE_USAGE_COPY_DST;
    }
    if (wgpuUsage & WGPUTextureUsage_TextureBinding) {
        usage |= GFX_TEXTURE_USAGE_TEXTURE_BINDING;
    }
    if (wgpuUsage & WGPUTextureUsage_StorageBinding) {
        usage |= GFX_TEXTURE_USAGE_STORAGE_BINDING;
    }
    if (wgpuUsage & WGPUTextureUsage_RenderAttachment) {
        usage |= GFX_TEXTURE_USAGE_RENDER_ATTACHMENT;
    }

    return usage;
}

GfxTextureLayout webgpu_textureGetLayout(GfxTexture texture)
{
    // WebGPU doesn't have explicit layouts, return GENERAL as a reasonable default
    if (!texture) {
        return GFX_TEXTURE_LAYOUT_UNDEFINED;
    }
    return GFX_TEXTURE_LAYOUT_GENERAL;
}

GfxResult webgpu_textureCreateView(GfxTexture texture, const GfxTextureViewDescriptor* descriptor,
    GfxTextureView* outView)
{
    if (!texture || !outView) {
        return GFX_RESULT_ERROR_INVALID_PARAMETER;
    }

    auto* texturePtr = reinterpret_cast<gfx::webgpu::Texture*>(texture);

    WGPUTextureViewDescriptor wgpuDesc = WGPU_TEXTURE_VIEW_DESCRIPTOR_INIT;
    if (descriptor) {
        if (descriptor->label) {
            wgpuDesc.label = gfxStringView(descriptor->label);
        }
        wgpuDesc.dimension = gfxTextureViewTypeToWGPU(descriptor->viewType);
        wgpuDesc.format = gfxFormatToWGPUFormat(descriptor->format);
        wgpuDesc.baseMipLevel = descriptor->baseMipLevel;
        wgpuDesc.mipLevelCount = descriptor->mipLevelCount;
        wgpuDesc.baseArrayLayer = descriptor->baseArrayLayer;
        wgpuDesc.arrayLayerCount = descriptor->arrayLayerCount;
    }

    WGPUTextureView wgpuView = wgpuTextureCreateView(texturePtr->handle(), &wgpuDesc);
    if (!wgpuView) {
        return GFX_RESULT_ERROR_UNKNOWN;
    }

    auto* view = new gfx::webgpu::TextureView(wgpuView, texturePtr);
    *outView = reinterpret_cast<GfxTextureView>(view);
    return GFX_RESULT_SUCCESS;
}

// TextureView functions
void webgpu_textureViewDestroy(GfxTextureView textureView)
{
    if (!textureView) {
        return;
    }
    delete reinterpret_cast<gfx::webgpu::TextureView*>(textureView);
}

// Sampler functions
void webgpu_samplerDestroy(GfxSampler sampler)
{
    if (!sampler) {
        return;
    }
    delete reinterpret_cast<gfx::webgpu::Sampler*>(sampler);
}

// Shader functions
void webgpu_shaderDestroy(GfxShader shader)
{
    if (!shader) {
        return;
    }
    delete reinterpret_cast<gfx::webgpu::Shader*>(shader);
}

// BindGroupLayout functions
void webgpu_bindGroupLayoutDestroy(GfxBindGroupLayout bindGroupLayout)
{
    if (!bindGroupLayout) {
        return;
    }
    delete reinterpret_cast<gfx::webgpu::BindGroupLayout*>(bindGroupLayout);
}

// BindGroup functions
void webgpu_bindGroupDestroy(GfxBindGroup bindGroup)
{
    if (!bindGroup) {
        return;
    }
    delete reinterpret_cast<gfx::webgpu::BindGroup*>(bindGroup);
}

// RenderPipeline functions
void webgpu_renderPipelineDestroy(GfxRenderPipeline renderPipeline)
{
    if (!renderPipeline) {
        return;
    }
    delete reinterpret_cast<gfx::webgpu::RenderPipeline*>(renderPipeline);
}

// ComputePipeline functions
void webgpu_computePipelineDestroy(GfxComputePipeline computePipeline)
{
    if (!computePipeline) {
        return;
    }
    delete reinterpret_cast<gfx::webgpu::ComputePipeline*>(computePipeline);
}

// Queue functions
GfxResult webgpu_queueSubmit(GfxQueue queue, const GfxSubmitInfo* submitInfo)
{
    if (!queue || !submitInfo) {
        return GFX_RESULT_ERROR_INVALID_PARAMETER;
    }

    auto* queuePtr = reinterpret_cast<gfx::webgpu::Queue*>(queue);

    // WebGPU doesn't support semaphore-based sync - just submit command buffers
    for (uint32_t i = 0; i < submitInfo->commandEncoderCount; ++i) {
        if (submitInfo->commandEncoders[i]) {
            auto* encoderPtr = reinterpret_cast<gfx::webgpu::CommandEncoder*>(submitInfo->commandEncoders[i]);

            WGPUCommandBufferDescriptor cmdDesc = WGPU_COMMAND_BUFFER_DESCRIPTOR_INIT;
            WGPUCommandBuffer cmdBuffer = wgpuCommandEncoderFinish(encoderPtr->handle(), &cmdDesc);

            if (cmdBuffer) {
                wgpuQueueSubmit(queuePtr->handle(), 1, &cmdBuffer);
                wgpuCommandBufferRelease(cmdBuffer);
            } else {
                return GFX_RESULT_ERROR_UNKNOWN;
            }
        }
    }

    // Signal fence if provided
    if (submitInfo->signalFence) {
        auto* fencePtr = reinterpret_cast<gfx::webgpu::Fence*>(submitInfo->signalFence);
        fencePtr->setSignaled(true);
    }

    return GFX_RESULT_SUCCESS;
}

void webgpu_queueWriteBuffer(GfxQueue queue, GfxBuffer buffer, uint64_t offset,
    const void* data, uint64_t size)
{
    if (!queue || !buffer || !data) {
        return;
    }

    auto* queuePtr = reinterpret_cast<gfx::webgpu::Queue*>(queue);
    auto* bufferPtr = reinterpret_cast<gfx::webgpu::Buffer*>(buffer);

    wgpuQueueWriteBuffer(queuePtr->handle(), bufferPtr->handle(), offset, data, size);
}

void webgpu_queueWriteTexture(GfxQueue queue, GfxTexture texture, const GfxOrigin3D* origin,
    uint32_t mipLevel, const void* data, uint64_t dataSize,
    uint32_t bytesPerRow, const GfxExtent3D* extent, GfxTextureLayout finalLayout)
{
    if (!queue || !texture || !origin || !extent || !data) {
        return;
    }

    auto* queuePtr = reinterpret_cast<gfx::webgpu::Queue*>(queue);
    auto* texturePtr = reinterpret_cast<gfx::webgpu::Texture*>(texture);

    WGPUTexelCopyTextureInfo dest = WGPU_TEXEL_COPY_TEXTURE_INFO_INIT;
    dest.texture = texturePtr->handle();
    dest.mipLevel = mipLevel;
    dest.origin = { origin->x, origin->y, origin->z };

    WGPUTexelCopyBufferLayout layout = WGPU_TEXEL_COPY_BUFFER_LAYOUT_INIT;
    layout.bytesPerRow = bytesPerRow;

    WGPUExtent3D wgpuExtent = { extent->width, extent->height, extent->depth };

    wgpuQueueWriteTexture(queuePtr->handle(), &dest, data, dataSize, &layout, &wgpuExtent);

    (void)finalLayout; // WebGPU handles layout transitions automatically
}

GfxResult webgpu_queueWaitIdle(GfxQueue queue)
{
    if (!queue) {
        return GFX_RESULT_ERROR_INVALID_PARAMETER;
    }

    auto* queuePtr = reinterpret_cast<gfx::webgpu::Queue*>(queue);

    // Submit empty command to ensure all previous work is queued
    bool workDone = false;
    WGPUQueueWorkDoneCallbackInfo callbackInfo = WGPU_QUEUE_WORK_DONE_CALLBACK_INFO_INIT;
    callbackInfo.mode = WGPUCallbackMode_WaitAnyOnly;
    callbackInfo.callback = [](WGPUQueueWorkDoneStatus status, WGPUStringView message, void* userdata1, void* userdata2) {
        bool* done = static_cast<bool*>(userdata1);
        if (status == WGPUQueueWorkDoneStatus_Success) {
            *done = true;
        }
    };
    callbackInfo.userdata1 = &workDone;

    WGPUFuture future = wgpuQueueOnSubmittedWorkDone(queuePtr->handle(), callbackInfo);

    // Properly wait for the queue work to complete
    if (queuePtr->getDevice() && queuePtr->getDevice()->getAdapter() && queuePtr->getDevice()->getAdapter()->getInstance()) {
        WGPUFutureWaitInfo waitInfo = WGPU_FUTURE_WAIT_INFO_INIT;
        waitInfo.future = future;
        wgpuInstanceWaitAny(queuePtr->getDevice()->getAdapter()->getInstance()->handle(), 1, &waitInfo, UINT64_MAX);
    }

    return workDone ? GFX_RESULT_SUCCESS : GFX_RESULT_ERROR_UNKNOWN;
}

// CommandEncoder functions
void webgpu_commandEncoderDestroy(GfxCommandEncoder commandEncoder)
{
    if (!commandEncoder) {
        return;
    }
    delete reinterpret_cast<gfx::webgpu::CommandEncoder*>(commandEncoder);
}

GfxResult webgpu_commandEncoderBeginRenderPass(GfxCommandEncoder commandEncoder,
    const GfxRenderPassDescriptor* descriptor,
    GfxRenderPassEncoder* outEncoder)
{
    if (!commandEncoder || !outEncoder || !descriptor) {
        return GFX_RESULT_ERROR_INVALID_PARAMETER;
    }

    auto* encoderPtr = reinterpret_cast<gfx::webgpu::CommandEncoder*>(commandEncoder);

    // Extract parameters from descriptor
    const GfxColorAttachment* colorAttachments = descriptor->colorAttachments;
    uint32_t colorAttachmentCount = descriptor->colorAttachmentCount;
    const GfxDepthStencilAttachment* depthStencilAttachment = descriptor->depthStencilAttachment;

    WGPURenderPassDescriptor wgpuDesc = WGPU_RENDER_PASS_DESCRIPTOR_INIT;

    std::vector<WGPURenderPassColorAttachment> wgpuColorAttachments;
    if (colorAttachmentCount > 0 && colorAttachments) {
        wgpuColorAttachments.reserve(colorAttachmentCount);

        for (uint32_t i = 0; i < colorAttachmentCount; ++i) {
            WGPURenderPassColorAttachment attachment = WGPU_RENDER_PASS_COLOR_ATTACHMENT_INIT;
            if (colorAttachments[i].view) {
                auto* viewPtr = reinterpret_cast<gfx::webgpu::TextureView*>(colorAttachments[i].view);
                attachment.view = viewPtr->handle();
                attachment.loadOp = WGPULoadOp_Clear;
                attachment.storeOp = WGPUStoreOp_Store;

                const GfxColor& color = colorAttachments[i].clearColor;
                attachment.clearValue = { color.r, color.g, color.b, color.a };
                }
            }
            wgpuColorAttachments.push_back(attachment);
        }

        wgpuDesc.colorAttachments = wgpuColorAttachments.data();
        wgpuDesc.colorAttachmentCount = colorAttachmentCount;
    }

    WGPURenderPassDepthStencilAttachment wgpuDepthStencil = WGPU_RENDER_PASS_DEPTH_STENCIL_ATTACHMENT_INIT;
    if (depthStencilAttachment) {
        auto* viewPtr = reinterpret_cast<gfx::webgpu::TextureView*>(depthStencilAttachment->view);
        wgpuDepthStencil.view = viewPtr->handle();
        wgpuDepthStencil.depthLoadOp = WGPULoadOp_Clear;
        wgpuDepthStencil.depthStoreOp = WGPUStoreOp_Store;
        wgpuDepthStencil.depthClearValue = depthStencilAttachment->depthClearValue;
        wgpuDepthStencil.stencilLoadOp = WGPULoadOp_Clear;
        wgpuDepthStencil.stencilStoreOp = WGPUStoreOp_Store;
        wgpuDepthStencil.stencilClearValue = depthStencilAttachment->stencilClearValue;

        wgpuDesc.depthStencilAttachment = &wgpuDepthStencil;
    }

    WGPURenderPassEncoder wgpuEncoder = wgpuCommandEncoderBeginRenderPass(encoderPtr->handle(), &wgpuDesc);
    if (!wgpuEncoder) {
        return GFX_RESULT_ERROR_UNKNOWN;
    }

    auto* renderPassEncoder = new gfx::webgpu::RenderPassEncoder(wgpuEncoder);
    *outEncoder = reinterpret_cast<GfxRenderPassEncoder>(renderPassEncoder);
    return GFX_RESULT_SUCCESS;
}

GfxResult webgpu_commandEncoderBeginComputePass(GfxCommandEncoder commandEncoder, const GfxComputePassDescriptor* descriptor,
    GfxComputePassEncoder* outEncoder)
{
    if (!commandEncoder || !descriptor || !outEncoder) {
        return GFX_RESULT_ERROR_INVALID_PARAMETER;
    }

    auto* encoderPtr = reinterpret_cast<gfx::webgpu::CommandEncoder*>(commandEncoder);

    WGPUComputePassDescriptor wgpuDesc = WGPU_COMPUTE_PASS_DESCRIPTOR_INIT;
    if (descriptor->label) {
        wgpuDesc.label = gfxStringView(descriptor->label);
    }

    WGPUComputePassEncoder wgpuEncoder = wgpuCommandEncoderBeginComputePass(encoderPtr->handle(), &wgpuDesc);
    if (!wgpuEncoder) {
        return GFX_RESULT_ERROR_UNKNOWN;
    }

    auto* computePassEncoder = new gfx::webgpu::ComputePassEncoder(wgpuEncoder);
    *outEncoder = reinterpret_cast<GfxComputePassEncoder>(computePassEncoder);
    return GFX_RESULT_SUCCESS;
}

void webgpu_commandEncoderCopyBufferToBuffer(GfxCommandEncoder commandEncoder,
    GfxBuffer source, uint64_t sourceOffset,
    GfxBuffer destination, uint64_t destinationOffset,
    uint64_t size)
{
    if (!commandEncoder || !source || !destination) {
        return;
    }

    auto* encoderPtr = reinterpret_cast<gfx::webgpu::CommandEncoder*>(commandEncoder);
    auto* srcPtr = reinterpret_cast<gfx::webgpu::Buffer*>(source);
    auto* dstPtr = reinterpret_cast<gfx::webgpu::Buffer*>(destination);

    wgpuCommandEncoderCopyBufferToBuffer(encoderPtr->handle(),
        srcPtr->handle(), sourceOffset,
        dstPtr->handle(), destinationOffset,
        size);
}

void webgpu_commandEncoderCopyBufferToTexture(GfxCommandEncoder commandEncoder,
    GfxBuffer source, uint64_t sourceOffset, uint32_t bytesPerRow,
    GfxTexture destination, const GfxOrigin3D* origin,
    const GfxExtent3D* extent, uint32_t mipLevel, GfxTextureLayout finalLayout)
{
    if (!commandEncoder || !source || !destination || !origin || !extent) {
        return;
    }

    auto* encoderPtr = reinterpret_cast<gfx::webgpu::CommandEncoder*>(commandEncoder);
    auto* srcPtr = reinterpret_cast<gfx::webgpu::Buffer*>(source);
    auto* dstPtr = reinterpret_cast<gfx::webgpu::Texture*>(destination);

    WGPUTexelCopyBufferInfo sourceInfo = WGPU_TEXEL_COPY_BUFFER_INFO_INIT;
    sourceInfo.buffer = srcPtr->handle();
    sourceInfo.layout.offset = sourceOffset;
    sourceInfo.layout.bytesPerRow = bytesPerRow;

    WGPUTexelCopyTextureInfo destInfo = WGPU_TEXEL_COPY_TEXTURE_INFO_INIT;
    destInfo.texture = dstPtr->handle();
    destInfo.mipLevel = mipLevel;
    destInfo.origin = { origin->x, origin->y, origin->z };

    WGPUExtent3D wgpuExtent = { extent->width, extent->height, extent->depth };

    wgpuCommandEncoderCopyBufferToTexture(encoderPtr->handle(), &sourceInfo, &destInfo, &wgpuExtent);

    (void)finalLayout; // WebGPU handles layout transitions automatically
}

void webgpu_commandEncoderCopyTextureToBuffer(GfxCommandEncoder commandEncoder,
    GfxTexture source, const GfxOrigin3D* origin, uint32_t mipLevel,
    GfxBuffer destination, uint64_t destinationOffset, uint32_t bytesPerRow,
    const GfxExtent3D* extent, GfxTextureLayout finalLayout)
{
    if (!commandEncoder || !source || !destination || !origin || !extent) {
        return;
    }

    auto* encoderPtr = reinterpret_cast<gfx::webgpu::CommandEncoder*>(commandEncoder);
    auto* srcPtr = reinterpret_cast<gfx::webgpu::Texture*>(source);
    auto* dstPtr = reinterpret_cast<gfx::webgpu::Buffer*>(destination);

    WGPUTexelCopyTextureInfo sourceInfo = WGPU_TEXEL_COPY_TEXTURE_INFO_INIT;
    sourceInfo.texture = srcPtr->handle();
    sourceInfo.mipLevel = mipLevel;
    sourceInfo.origin = { origin->x, origin->y, origin->z };

    WGPUTexelCopyBufferInfo destInfo = WGPU_TEXEL_COPY_BUFFER_INFO_INIT;
    destInfo.buffer = dstPtr->handle();
    destInfo.layout.offset = destinationOffset;
    destInfo.layout.bytesPerRow = bytesPerRow;

    WGPUExtent3D wgpuExtent = { extent->width, extent->height, extent->depth };

    wgpuCommandEncoderCopyTextureToBuffer(encoderPtr->handle(), &sourceInfo, &destInfo, &wgpuExtent);

    (void)finalLayout; // WebGPU handles layout transitions automatically
}

void webgpu_commandEncoderCopyTextureToTexture(GfxCommandEncoder commandEncoder,
    GfxTexture source, const GfxOrigin3D* sourceOrigin, uint32_t sourceMipLevel,
    GfxTexture destination, const GfxOrigin3D* destinationOrigin, uint32_t destinationMipLevel,
    const GfxExtent3D* extent, GfxTextureLayout srcFinalLayout, GfxTextureLayout dstFinalLayout)
{
    if (!commandEncoder || !source || !destination || !sourceOrigin || !destinationOrigin || !extent) {
        return;
    }

    auto* encoderPtr = reinterpret_cast<gfx::webgpu::CommandEncoder*>(commandEncoder);
    auto* srcPtr = reinterpret_cast<gfx::webgpu::Texture*>(source);
    auto* dstPtr = reinterpret_cast<gfx::webgpu::Texture*>(destination);

    // For 2D textures and arrays, extent->depth represents layer count
    // For 3D textures, it represents actual depth
    WGPUExtent3D srcSize = srcPtr->getSize();
    bool is3DTexture = (srcSize.depthOrArrayLayers > 1 && srcSize.height > 1);
    
    WGPUTexelCopyTextureInfo sourceInfo = WGPU_TEXEL_COPY_TEXTURE_INFO_INIT;
    sourceInfo.texture = srcPtr->handle();
    sourceInfo.mipLevel = sourceMipLevel;
    sourceInfo.origin = { sourceOrigin->x, sourceOrigin->y, is3DTexture ? sourceOrigin->z : 0 };

    WGPUTexelCopyTextureInfo destInfo = WGPU_TEXEL_COPY_TEXTURE_INFO_INIT;
    destInfo.texture = dstPtr->handle();
    destInfo.mipLevel = destinationMipLevel;
    destInfo.origin = { destinationOrigin->x, destinationOrigin->y, is3DTexture ? destinationOrigin->z : 0 };

    WGPUExtent3D wgpuExtent = { extent->width, extent->height, extent->depth };

    wgpuCommandEncoderCopyTextureToTexture(encoderPtr->handle(), &sourceInfo, &destInfo, &wgpuExtent);

    (void)srcFinalLayout; // WebGPU handles layout transitions automatically
    (void)dstFinalLayout;
}

void webgpu_commandEncoderPipelineBarrier(GfxCommandEncoder commandEncoder,
    const GfxMemoryBarrier* memoryBarriers, uint32_t memoryBarrierCount,
    const GfxBufferBarrier* bufferBarriers, uint32_t bufferBarrierCount,
    const GfxTextureBarrier* textureBarriers, uint32_t textureBarrierCount)
{
    // WebGPU handles synchronization and layout transitions automatically
    // This is a no-op for WebGPU backend
    (void)commandEncoder;
    (void)memoryBarriers;
    (void)memoryBarrierCount;
    (void)bufferBarriers;
    (void)bufferBarrierCount;
    (void)textureBarriers;
    (void)textureBarrierCount;
}

void webgpu_commandEncoderEnd(GfxCommandEncoder commandEncoder)
{
    // Handled in queueSubmit
}

// RenderPassEncoder functions
void webgpu_renderPassEncoderDestroy(GfxRenderPassEncoder renderPassEncoder)
{
    if (!renderPassEncoder) {
        return;
    }
    delete reinterpret_cast<gfx::webgpu::RenderPassEncoder*>(renderPassEncoder);
}

void webgpu_renderPassEncoderSetPipeline(GfxRenderPassEncoder renderPassEncoder, GfxRenderPipeline pipeline)
{
    if (!renderPassEncoder || !pipeline) {
        return;
    }

    auto* encoderPtr = reinterpret_cast<gfx::webgpu::RenderPassEncoder*>(renderPassEncoder);
    auto* pipelinePtr = reinterpret_cast<gfx::webgpu::RenderPipeline*>(pipeline);

    wgpuRenderPassEncoderSetPipeline(encoderPtr->handle(), pipelinePtr->handle());
}

void webgpu_renderPassEncoderSetBindGroup(GfxRenderPassEncoder renderPassEncoder, uint32_t index, GfxBindGroup bindGroup, const uint32_t* dynamicOffsets, uint32_t dynamicOffsetCount)
{
    if (!renderPassEncoder || !bindGroup) {
        return;
    }

    auto* encoderPtr = reinterpret_cast<gfx::webgpu::RenderPassEncoder*>(renderPassEncoder);
    auto* bindGroupPtr = reinterpret_cast<gfx::webgpu::BindGroup*>(bindGroup);

    wgpuRenderPassEncoderSetBindGroup(encoderPtr->handle(), index, bindGroupPtr->handle(), dynamicOffsetCount, dynamicOffsets);
}

void webgpu_renderPassEncoderSetVertexBuffer(GfxRenderPassEncoder renderPassEncoder, uint32_t slot,
    GfxBuffer buffer, uint64_t offset, uint64_t size)
{
    if (!renderPassEncoder || !buffer) {
        return;
    }

    auto* encoderPtr = reinterpret_cast<gfx::webgpu::RenderPassEncoder*>(renderPassEncoder);
    auto* bufferPtr = reinterpret_cast<gfx::webgpu::Buffer*>(buffer);

    wgpuRenderPassEncoderSetVertexBuffer(encoderPtr->handle(), slot, bufferPtr->handle(), offset, size);
}

void webgpu_renderPassEncoderSetIndexBuffer(GfxRenderPassEncoder renderPassEncoder,
    GfxBuffer buffer, GfxIndexFormat format,
    uint64_t offset, uint64_t size)
{
    if (!renderPassEncoder || !buffer) {
        return;
    }

    auto* encoderPtr = reinterpret_cast<gfx::webgpu::RenderPassEncoder*>(renderPassEncoder);
    auto* bufferPtr = reinterpret_cast<gfx::webgpu::Buffer*>(buffer);

    wgpuRenderPassEncoderSetIndexBuffer(encoderPtr->handle(), bufferPtr->handle(),
        gfxIndexFormatToWGPU(format), offset, size);
}

void webgpu_renderPassEncoderSetViewport(GfxRenderPassEncoder renderPassEncoder,
    const GfxViewport* viewport)
{
    if (!renderPassEncoder || !viewport) {
        return;
    }

    auto* encoderPtr = reinterpret_cast<gfx::webgpu::RenderPassEncoder*>(renderPassEncoder);
    wgpuRenderPassEncoderSetViewport(encoderPtr->handle(),
        viewport->x, viewport->y, viewport->width, viewport->height,
        viewport->minDepth, viewport->maxDepth);
}

void webgpu_renderPassEncoderSetScissorRect(GfxRenderPassEncoder renderPassEncoder,
    const GfxScissorRect* scissor)
{
    if (!renderPassEncoder || !scissor) {
        return;
    }

    auto* encoderPtr = reinterpret_cast<gfx::webgpu::RenderPassEncoder*>(renderPassEncoder);
    wgpuRenderPassEncoderSetScissorRect(encoderPtr->handle(),
        scissor->x, scissor->y, scissor->width, scissor->height);
}

void webgpu_renderPassEncoderDraw(GfxRenderPassEncoder renderPassEncoder,
    uint32_t vertexCount, uint32_t instanceCount,
    uint32_t firstVertex, uint32_t firstInstance)
{
    if (!renderPassEncoder) {
        return;
    }

    auto* encoderPtr = reinterpret_cast<gfx::webgpu::RenderPassEncoder*>(renderPassEncoder);
    wgpuRenderPassEncoderDraw(encoderPtr->handle(), vertexCount, instanceCount, firstVertex, firstInstance);
}

void webgpu_renderPassEncoderDrawIndexed(GfxRenderPassEncoder renderPassEncoder,
    uint32_t indexCount, uint32_t instanceCount,
    uint32_t firstIndex, int32_t baseVertex,
    uint32_t firstInstance)
{
    if (!renderPassEncoder) {
        return;
    }

    auto* encoderPtr = reinterpret_cast<gfx::webgpu::RenderPassEncoder*>(renderPassEncoder);
    wgpuRenderPassEncoderDrawIndexed(encoderPtr->handle(), indexCount, instanceCount,
        firstIndex, baseVertex, firstInstance);
}

void webgpu_renderPassEncoderEnd(GfxRenderPassEncoder renderPassEncoder)
{
    if (!renderPassEncoder) {
        return;
    }

    auto* encoderPtr = reinterpret_cast<gfx::webgpu::RenderPassEncoder*>(renderPassEncoder);
    wgpuRenderPassEncoderEnd(encoderPtr->handle());
}

// ComputePassEncoder functions
void webgpu_computePassEncoderDestroy(GfxComputePassEncoder computePassEncoder)
{
    if (!computePassEncoder) {
        return;
    }
    delete reinterpret_cast<gfx::webgpu::ComputePassEncoder*>(computePassEncoder);
}

void webgpu_computePassEncoderSetPipeline(GfxComputePassEncoder computePassEncoder, GfxComputePipeline pipeline)
{
    if (!computePassEncoder || !pipeline) {
        return;
    }

    auto* encoderPtr = reinterpret_cast<gfx::webgpu::ComputePassEncoder*>(computePassEncoder);
    auto* pipelinePtr = reinterpret_cast<gfx::webgpu::ComputePipeline*>(pipeline);

    wgpuComputePassEncoderSetPipeline(encoderPtr->handle(), pipelinePtr->handle());
}

void webgpu_computePassEncoderSetBindGroup(GfxComputePassEncoder computePassEncoder, uint32_t index, GfxBindGroup bindGroup, const uint32_t* dynamicOffsets, uint32_t dynamicOffsetCount)
{
    if (!computePassEncoder || !bindGroup) {
        return;
    }

    auto* encoderPtr = reinterpret_cast<gfx::webgpu::ComputePassEncoder*>(computePassEncoder);
    auto* bindGroupPtr = reinterpret_cast<gfx::webgpu::BindGroup*>(bindGroup);

    wgpuComputePassEncoderSetBindGroup(encoderPtr->handle(), index, bindGroupPtr->handle(), dynamicOffsetCount, dynamicOffsets);
}

void webgpu_computePassEncoderDispatchWorkgroups(GfxComputePassEncoder computePassEncoder,
    uint32_t workgroupCountX, uint32_t workgroupCountY,
    uint32_t workgroupCountZ)
{
    if (!computePassEncoder) {
        return;
    }

    auto* encoderPtr = reinterpret_cast<gfx::webgpu::ComputePassEncoder*>(computePassEncoder);
    wgpuComputePassEncoderDispatchWorkgroups(encoderPtr->handle(), workgroupCountX, workgroupCountY, workgroupCountZ);
}

void webgpu_computePassEncoderEnd(GfxComputePassEncoder computePassEncoder)
{
    if (!computePassEncoder) {
        return;
    }

    auto* encoderPtr = reinterpret_cast<gfx::webgpu::ComputePassEncoder*>(computePassEncoder);
    wgpuComputePassEncoderEnd(encoderPtr->handle());
}

// Fence functions (stubs for API compatibility)
void webgpu_fenceDestroy(GfxFence fence)
{
    if (!fence) {
        return;
    }
    delete reinterpret_cast<gfx::webgpu::Fence*>(fence);
}

GfxResult webgpu_fenceGetStatus(GfxFence fence, bool* isSignaled)
{
    if (!fence || !isSignaled) {
        return GFX_RESULT_ERROR_INVALID_PARAMETER;
    }

    auto* fencePtr = reinterpret_cast<gfx::webgpu::Fence*>(fence);
    *isSignaled = fencePtr->isSignaled();
    return GFX_RESULT_SUCCESS;
}

GfxResult webgpu_fenceWait(GfxFence fence, uint64_t timeoutNs)
{
    if (!fence) {
        return GFX_RESULT_ERROR_INVALID_PARAMETER;
    }

    auto* fencePtr = reinterpret_cast<gfx::webgpu::Fence*>(fence);
    return fencePtr->isSignaled() ? GFX_RESULT_SUCCESS : GFX_RESULT_TIMEOUT;
}

void webgpu_fenceReset(GfxFence fence)
{
    if (!fence) {
        return;
    }

    auto* fencePtr = reinterpret_cast<gfx::webgpu::Fence*>(fence);
    fencePtr->setSignaled(false);
}

// Semaphore functions (stubs for API compatibility)
void webgpu_semaphoreDestroy(GfxSemaphore semaphore)
{
    if (!semaphore) {
        return;
    }
    delete reinterpret_cast<gfx::webgpu::Semaphore*>(semaphore);
}

GfxSemaphoreType webgpu_semaphoreGetType(GfxSemaphore semaphore)
{
    if (!semaphore) {
        return GFX_SEMAPHORE_TYPE_BINARY;
    }
    return reinterpret_cast<gfx::webgpu::Semaphore*>(semaphore)->getType();
}

GfxResult webgpu_semaphoreSignal(GfxSemaphore semaphore, uint64_t value)
{
    if (!semaphore) {
        return GFX_RESULT_ERROR_INVALID_PARAMETER;
    }

    auto* semaphorePtr = reinterpret_cast<gfx::webgpu::Semaphore*>(semaphore);
    if (semaphorePtr->getType() == GFX_SEMAPHORE_TYPE_TIMELINE) {
        semaphorePtr->setValue(value);
    }
    return GFX_RESULT_SUCCESS;
}

GfxResult webgpu_semaphoreWait(GfxSemaphore semaphore, uint64_t value, uint64_t timeoutNs)
{
    if (!semaphore) {
        return GFX_RESULT_ERROR_INVALID_PARAMETER;
    }

    auto* semaphorePtr = reinterpret_cast<gfx::webgpu::Semaphore*>(semaphore);
    if (semaphorePtr->getType() == GFX_SEMAPHORE_TYPE_TIMELINE) {
        return (semaphorePtr->getValue() >= value) ? GFX_RESULT_SUCCESS : GFX_RESULT_TIMEOUT;
    }
    return GFX_RESULT_SUCCESS;
}

uint64_t webgpu_semaphoreGetValue(GfxSemaphore semaphore)
{
    if (!semaphore) {
        return 0;
    }
    return reinterpret_cast<gfx::webgpu::Semaphore*>(semaphore)->getValue();
}

namespace gfx::webgpu {

// ============================================================================
// Backend C++ Class Export
// ============================================================================

// Instance functions
GfxResult WebGPUBackend::createInstance(const GfxInstanceDescriptor* descriptor, GfxInstance* outInstance) const
{
    return webgpu_createInstance(descriptor, outInstance);
}
void WebGPUBackend::instanceDestroy(GfxInstance instance) const
{
    webgpu_instanceDestroy(instance);
}
void WebGPUBackend::instanceSetDebugCallback(GfxInstance instance, GfxDebugCallback callback, void* userData) const
{
    webgpu_instanceSetDebugCallback(instance, callback, userData);
}
GfxResult WebGPUBackend::instanceRequestAdapter(GfxInstance instance, const GfxAdapterDescriptor* descriptor, GfxAdapter* outAdapter) const
{
    return webgpu_instanceRequestAdapter(instance, descriptor, outAdapter);
}
uint32_t WebGPUBackend::instanceEnumerateAdapters(GfxInstance instance, GfxAdapter* adapters, uint32_t maxAdapters) const
{
    return webgpu_instanceEnumerateAdapters(instance, adapters, maxAdapters);
}

// Adapter functions
void WebGPUBackend::adapterDestroy(GfxAdapter adapter) const
{
    webgpu_adapterDestroy(adapter);
}
GfxResult WebGPUBackend::adapterCreateDevice(GfxAdapter adapter, const GfxDeviceDescriptor* descriptor, GfxDevice* outDevice) const
{
    return webgpu_adapterCreateDevice(adapter, descriptor, outDevice);
}
const char* WebGPUBackend::adapterGetName(GfxAdapter adapter) const
{
    return webgpu_adapterGetName(adapter);
}
GfxBackend WebGPUBackend::adapterGetBackend(GfxAdapter adapter) const
{
    return webgpu_adapterGetBackend(adapter);
}

// Device functions
void WebGPUBackend::deviceDestroy(GfxDevice device) const
{
    webgpu_deviceDestroy(device);
}
GfxQueue WebGPUBackend::deviceGetQueue(GfxDevice device) const
{
    return webgpu_deviceGetQueue(device);
}
GfxResult WebGPUBackend::deviceCreateSurface(GfxDevice device, const GfxSurfaceDescriptor* descriptor, GfxSurface* outSurface) const
{
    return webgpu_deviceCreateSurface(device, descriptor, outSurface);
}
GfxResult WebGPUBackend::deviceCreateSwapchain(GfxDevice device, GfxSurface surface, const GfxSwapchainDescriptor* descriptor, GfxSwapchain* outSwapchain) const
{
    return webgpu_deviceCreateSwapchain(device, surface, descriptor, outSwapchain);
}
GfxResult WebGPUBackend::deviceCreateBuffer(GfxDevice device, const GfxBufferDescriptor* descriptor, GfxBuffer* outBuffer) const
{
    return webgpu_deviceCreateBuffer(device, descriptor, outBuffer);
}
GfxResult WebGPUBackend::deviceCreateTexture(GfxDevice device, const GfxTextureDescriptor* descriptor, GfxTexture* outTexture) const
{
    return webgpu_deviceCreateTexture(device, descriptor, outTexture);
}
GfxResult WebGPUBackend::deviceCreateSampler(GfxDevice device, const GfxSamplerDescriptor* descriptor, GfxSampler* outSampler) const
{
    return webgpu_deviceCreateSampler(device, descriptor, outSampler);
}
GfxResult WebGPUBackend::deviceCreateShader(GfxDevice device, const GfxShaderDescriptor* descriptor, GfxShader* outShader) const
{
    return webgpu_deviceCreateShader(device, descriptor, outShader);
}
GfxResult WebGPUBackend::deviceCreateBindGroupLayout(GfxDevice device, const GfxBindGroupLayoutDescriptor* descriptor, GfxBindGroupLayout* outLayout) const
{
    return webgpu_deviceCreateBindGroupLayout(device, descriptor, outLayout);
}
GfxResult WebGPUBackend::deviceCreateBindGroup(GfxDevice device, const GfxBindGroupDescriptor* descriptor, GfxBindGroup* outBindGroup) const
{
    return webgpu_deviceCreateBindGroup(device, descriptor, outBindGroup);
}
GfxResult WebGPUBackend::deviceCreateRenderPipeline(GfxDevice device, const GfxRenderPipelineDescriptor* descriptor, GfxRenderPipeline* outPipeline) const
{
    return webgpu_deviceCreateRenderPipeline(device, descriptor, outPipeline);
}
GfxResult WebGPUBackend::deviceCreateComputePipeline(GfxDevice device, const GfxComputePipelineDescriptor* descriptor, GfxComputePipeline* outPipeline) const
{
    return webgpu_deviceCreateComputePipeline(device, descriptor, outPipeline);
}
GfxResult WebGPUBackend::deviceCreateCommandEncoder(GfxDevice device, const char* label, GfxCommandEncoder* outEncoder) const
{
    return webgpu_deviceCreateCommandEncoder(device, label, outEncoder);
}
GfxResult WebGPUBackend::deviceCreateFence(GfxDevice device, const GfxFenceDescriptor* descriptor, GfxFence* outFence) const
{
    return webgpu_deviceCreateFence(device, descriptor, outFence);
}
GfxResult WebGPUBackend::deviceCreateSemaphore(GfxDevice device, const GfxSemaphoreDescriptor* descriptor, GfxSemaphore* outSemaphore) const
{
    return webgpu_deviceCreateSemaphore(device, descriptor, outSemaphore);
}
void WebGPUBackend::deviceWaitIdle(GfxDevice device) const
{
    webgpu_deviceWaitIdle(device);
}
void WebGPUBackend::deviceGetLimits(GfxDevice device, GfxDeviceLimits* outLimits) const
{
    webgpu_deviceGetLimits(device, outLimits);
}

// Surface functions
void WebGPUBackend::surfaceDestroy(GfxSurface surface) const
{
    webgpu_surfaceDestroy(surface);
}
uint32_t WebGPUBackend::surfaceGetSupportedFormats(GfxSurface surface, GfxTextureFormat* formats, uint32_t maxFormats) const
{
    return webgpu_surfaceGetSupportedFormats(surface, formats, maxFormats);
}
uint32_t WebGPUBackend::surfaceGetSupportedPresentModes(GfxSurface surface, GfxPresentMode* presentModes, uint32_t maxModes) const
{
    return webgpu_surfaceGetSupportedPresentModes(surface, presentModes, maxModes);
}
GfxPlatformWindowHandle WebGPUBackend::surfaceGetPlatformHandle(GfxSurface surface) const
{
    return webgpu_surfaceGetPlatformHandle(surface);
}

// Swapchain functions
void WebGPUBackend::swapchainDestroy(GfxSwapchain swapchain) const
{
    webgpu_swapchainDestroy(swapchain);
}
uint32_t WebGPUBackend::swapchainGetWidth(GfxSwapchain swapchain) const
{
    return webgpu_swapchainGetWidth(swapchain);
}
uint32_t WebGPUBackend::swapchainGetHeight(GfxSwapchain swapchain) const
{
    return webgpu_swapchainGetHeight(swapchain);
}
GfxTextureFormat WebGPUBackend::swapchainGetFormat(GfxSwapchain swapchain) const
{
    return webgpu_swapchainGetFormat(swapchain);
}
uint32_t WebGPUBackend::swapchainGetBufferCount(GfxSwapchain swapchain) const
{
    return webgpu_swapchainGetBufferCount(swapchain);
}
GfxResult WebGPUBackend::swapchainAcquireNextImage(GfxSwapchain swapchain, uint64_t timeoutNs, GfxSemaphore imageAvailableSemaphore, GfxFence fence, uint32_t* outImageIndex) const
{
    return webgpu_swapchainAcquireNextImage(swapchain, timeoutNs, imageAvailableSemaphore, fence, outImageIndex);
}
GfxTextureView WebGPUBackend::swapchainGetImageView(GfxSwapchain swapchain, uint32_t imageIndex) const
{
    return webgpu_swapchainGetImageView(swapchain, imageIndex);
}
GfxTextureView WebGPUBackend::swapchainGetCurrentTextureView(GfxSwapchain swapchain) const
{
    return webgpu_swapchainGetCurrentTextureView(swapchain);
}
GfxResult WebGPUBackend::swapchainPresent(GfxSwapchain swapchain, const GfxPresentInfo* presentInfo) const
{
    return webgpu_swapchainPresent(swapchain, presentInfo);
}

// Buffer functions
void WebGPUBackend::bufferDestroy(GfxBuffer buffer) const
{
    webgpu_bufferDestroy(buffer);
}
uint64_t WebGPUBackend::bufferGetSize(GfxBuffer buffer) const
{
    return webgpu_bufferGetSize(buffer);
}
GfxBufferUsage WebGPUBackend::bufferGetUsage(GfxBuffer buffer) const
{
    return webgpu_bufferGetUsage(buffer);
}
GfxResult WebGPUBackend::bufferMap(GfxBuffer buffer, uint64_t offset, uint64_t size, void** outMappedPointer) const
{
    return webgpu_bufferMap(buffer, offset, size, outMappedPointer);
}
void WebGPUBackend::bufferUnmap(GfxBuffer buffer) const
{
    webgpu_bufferUnmap(buffer);
}

// Texture functions
void WebGPUBackend::textureDestroy(GfxTexture texture) const
{
    webgpu_textureDestroy(texture);
}
GfxExtent3D WebGPUBackend::textureGetSize(GfxTexture texture) const
{
    return webgpu_textureGetSize(texture);
}
GfxTextureFormat WebGPUBackend::textureGetFormat(GfxTexture texture) const
{
    return webgpu_textureGetFormat(texture);
}
uint32_t WebGPUBackend::textureGetMipLevelCount(GfxTexture texture) const
{
    return webgpu_textureGetMipLevelCount(texture);
}
GfxSampleCount WebGPUBackend::textureGetSampleCount(GfxTexture texture) const
{
    return webgpu_textureGetSampleCount(texture);
}
GfxTextureUsage WebGPUBackend::textureGetUsage(GfxTexture texture) const
{
    return webgpu_textureGetUsage(texture);
}
GfxTextureLayout WebGPUBackend::textureGetLayout(GfxTexture texture) const
{
    return webgpu_textureGetLayout(texture);
}
GfxResult WebGPUBackend::textureCreateView(GfxTexture texture, const GfxTextureViewDescriptor* descriptor, GfxTextureView* outView) const
{
    return webgpu_textureCreateView(texture, descriptor, outView);
}

// TextureView functions
void WebGPUBackend::textureViewDestroy(GfxTextureView textureView) const
{
    webgpu_textureViewDestroy(textureView);
}

// Sampler functions
void WebGPUBackend::samplerDestroy(GfxSampler sampler) const
{
    webgpu_samplerDestroy(sampler);
}

// Shader functions
void WebGPUBackend::shaderDestroy(GfxShader shader) const
{
    webgpu_shaderDestroy(shader);
}

// BindGroupLayout functions
void WebGPUBackend::bindGroupLayoutDestroy(GfxBindGroupLayout bindGroupLayout) const
{
    webgpu_bindGroupLayoutDestroy(bindGroupLayout);
}

// BindGroup functions
void WebGPUBackend::bindGroupDestroy(GfxBindGroup bindGroup) const
{
    webgpu_bindGroupDestroy(bindGroup);
}

// RenderPipeline functions
void WebGPUBackend::renderPipelineDestroy(GfxRenderPipeline renderPipeline) const
{
    webgpu_renderPipelineDestroy(renderPipeline);
}

// ComputePipeline functions
void WebGPUBackend::computePipelineDestroy(GfxComputePipeline computePipeline) const
{
    webgpu_computePipelineDestroy(computePipeline);
}

// Queue functions
GfxResult WebGPUBackend::queueSubmit(GfxQueue queue, const GfxSubmitInfo* submitInfo) const
{
    return webgpu_queueSubmit(queue, submitInfo);
}
void WebGPUBackend::queueWriteBuffer(GfxQueue queue, GfxBuffer buffer, uint64_t offset, const void* data, uint64_t size) const
{
    webgpu_queueWriteBuffer(queue, buffer, offset, data, size);
}
void WebGPUBackend::queueWriteTexture(GfxQueue queue, GfxTexture texture, const GfxOrigin3D* origin, uint32_t mipLevel,
    const void* data, uint64_t dataSize, uint32_t bytesPerRow, const GfxExtent3D* extent, GfxTextureLayout finalLayout) const
{
    webgpu_queueWriteTexture(queue, texture, origin, mipLevel, data, dataSize, bytesPerRow, extent, finalLayout);
}
GfxResult WebGPUBackend::queueWaitIdle(GfxQueue queue) const
{
    return webgpu_queueWaitIdle(queue);
}

// CommandEncoder functions
void WebGPUBackend::commandEncoderDestroy(GfxCommandEncoder commandEncoder) const
{
    webgpu_commandEncoderDestroy(commandEncoder);
}
GfxResult WebGPUBackend::commandEncoderBeginRenderPass(GfxCommandEncoder commandEncoder,
    const GfxRenderPassDescriptor* descriptor,
    GfxRenderPassEncoder* outRenderPass) const
{
    return webgpu_commandEncoderBeginRenderPass(commandEncoder, descriptor, outRenderPass);
}
GfxResult WebGPUBackend::commandEncoderBeginComputePass(GfxCommandEncoder commandEncoder, const GfxComputePassDescriptor* descriptor, GfxComputePassEncoder* outComputePass) const
{
    return webgpu_commandEncoderBeginComputePass(commandEncoder, descriptor, outComputePass);
}
void WebGPUBackend::commandEncoderCopyBufferToBuffer(GfxCommandEncoder commandEncoder, GfxBuffer source, uint64_t sourceOffset,
    GfxBuffer destination, uint64_t destinationOffset, uint64_t size) const
{
    webgpu_commandEncoderCopyBufferToBuffer(commandEncoder, source, sourceOffset, destination, destinationOffset, size);
}
void WebGPUBackend::commandEncoderCopyBufferToTexture(GfxCommandEncoder commandEncoder, GfxBuffer source, uint64_t sourceOffset, uint32_t bytesPerRow,
    GfxTexture destination, const GfxOrigin3D* origin, const GfxExtent3D* extent, uint32_t mipLevel, GfxTextureLayout finalLayout) const
{
    webgpu_commandEncoderCopyBufferToTexture(commandEncoder, source, sourceOffset, bytesPerRow, destination, origin, extent, mipLevel, finalLayout);
}
void WebGPUBackend::commandEncoderCopyTextureToBuffer(GfxCommandEncoder commandEncoder, GfxTexture source, const GfxOrigin3D* origin, uint32_t mipLevel,
    GfxBuffer destination, uint64_t destinationOffset, uint32_t bytesPerRow, const GfxExtent3D* extent, GfxTextureLayout finalLayout) const
{
    webgpu_commandEncoderCopyTextureToBuffer(commandEncoder, source, origin, mipLevel, destination, destinationOffset, bytesPerRow, extent, finalLayout);
}
void WebGPUBackend::commandEncoderCopyTextureToTexture(GfxCommandEncoder commandEncoder, GfxTexture source, const GfxOrigin3D* sourceOrigin, uint32_t sourceMipLevel,
    GfxTexture destination, const GfxOrigin3D* destinationOrigin, uint32_t destinationMipLevel, const GfxExtent3D* extent,
    GfxTextureLayout srcFinalLayout, GfxTextureLayout dstFinalLayout) const
{
    webgpu_commandEncoderCopyTextureToTexture(commandEncoder, source, sourceOrigin, sourceMipLevel, destination, destinationOrigin, destinationMipLevel, extent, srcFinalLayout, dstFinalLayout);
}
void WebGPUBackend::commandEncoderPipelineBarrier(GfxCommandEncoder commandEncoder,
    const GfxMemoryBarrier* memoryBarriers, uint32_t memoryBarrierCount,
    const GfxBufferBarrier* bufferBarriers, uint32_t bufferBarrierCount,
    const GfxTextureBarrier* textureBarriers, uint32_t textureBarrierCount) const
{
    webgpu_commandEncoderPipelineBarrier(commandEncoder, memoryBarriers, memoryBarrierCount, bufferBarriers, bufferBarrierCount, textureBarriers, textureBarrierCount);
}
void WebGPUBackend::commandEncoderEnd(GfxCommandEncoder commandEncoder) const
{
    webgpu_commandEncoderEnd(commandEncoder);
}
void WebGPUBackend::commandEncoderBegin(GfxCommandEncoder commandEncoder) const
{
    webgpu_commandEncoderBegin(commandEncoder);
}

// RenderPassEncoder functions
void WebGPUBackend::renderPassEncoderDestroy(GfxRenderPassEncoder renderPassEncoder) const
{
    webgpu_renderPassEncoderDestroy(renderPassEncoder);
}
void WebGPUBackend::renderPassEncoderSetPipeline(GfxRenderPassEncoder renderPassEncoder, GfxRenderPipeline pipeline) const
{
    webgpu_renderPassEncoderSetPipeline(renderPassEncoder, pipeline);
}
void WebGPUBackend::renderPassEncoderSetBindGroup(GfxRenderPassEncoder renderPassEncoder, uint32_t index, GfxBindGroup bindGroup, const uint32_t* dynamicOffsets, uint32_t dynamicOffsetCount) const
{
    webgpu_renderPassEncoderSetBindGroup(renderPassEncoder, index, bindGroup, dynamicOffsets, dynamicOffsetCount);
}
void WebGPUBackend::renderPassEncoderSetVertexBuffer(GfxRenderPassEncoder renderPassEncoder, uint32_t slot, GfxBuffer buffer, uint64_t offset, uint64_t size) const
{
    webgpu_renderPassEncoderSetVertexBuffer(renderPassEncoder, slot, buffer, offset, size);
}
void WebGPUBackend::renderPassEncoderSetIndexBuffer(GfxRenderPassEncoder renderPassEncoder, GfxBuffer buffer, GfxIndexFormat format, uint64_t offset, uint64_t size) const
{
    webgpu_renderPassEncoderSetIndexBuffer(renderPassEncoder, buffer, format, offset, size);
}
void WebGPUBackend::renderPassEncoderSetViewport(GfxRenderPassEncoder renderPassEncoder, const GfxViewport* viewport) const
{
    webgpu_renderPassEncoderSetViewport(renderPassEncoder, viewport);
}
void WebGPUBackend::renderPassEncoderSetScissorRect(GfxRenderPassEncoder renderPassEncoder, const GfxScissorRect* scissor) const
{
    webgpu_renderPassEncoderSetScissorRect(renderPassEncoder, scissor);
}
void WebGPUBackend::renderPassEncoderDraw(GfxRenderPassEncoder renderPassEncoder, uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance) const
{
    webgpu_renderPassEncoderDraw(renderPassEncoder, vertexCount, instanceCount, firstVertex, firstInstance);
}
void WebGPUBackend::renderPassEncoderDrawIndexed(GfxRenderPassEncoder renderPassEncoder, uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t baseVertex, uint32_t firstInstance) const
{
    webgpu_renderPassEncoderDrawIndexed(renderPassEncoder, indexCount, instanceCount, firstIndex, baseVertex, firstInstance);
}
void WebGPUBackend::renderPassEncoderEnd(GfxRenderPassEncoder renderPassEncoder) const
{
    webgpu_renderPassEncoderEnd(renderPassEncoder);
}

// ComputePassEncoder functions
void WebGPUBackend::computePassEncoderDestroy(GfxComputePassEncoder computePassEncoder) const
{
    webgpu_computePassEncoderDestroy(computePassEncoder);
}
void WebGPUBackend::computePassEncoderSetPipeline(GfxComputePassEncoder computePassEncoder, GfxComputePipeline pipeline) const
{
    webgpu_computePassEncoderSetPipeline(computePassEncoder, pipeline);
}
void WebGPUBackend::computePassEncoderSetBindGroup(GfxComputePassEncoder computePassEncoder, uint32_t index, GfxBindGroup bindGroup, const uint32_t* dynamicOffsets, uint32_t dynamicOffsetCount) const
{
    webgpu_computePassEncoderSetBindGroup(computePassEncoder, index, bindGroup, dynamicOffsets, dynamicOffsetCount);
}
void WebGPUBackend::computePassEncoderDispatchWorkgroups(GfxComputePassEncoder computePassEncoder, uint32_t workgroupCountX, uint32_t workgroupCountY, uint32_t workgroupCountZ) const
{
    webgpu_computePassEncoderDispatchWorkgroups(computePassEncoder, workgroupCountX, workgroupCountY, workgroupCountZ);
}
void WebGPUBackend::computePassEncoderEnd(GfxComputePassEncoder computePassEncoder) const
{
    webgpu_computePassEncoderEnd(computePassEncoder);
}

// Fence functions
void WebGPUBackend::fenceDestroy(GfxFence fence) const
{
    webgpu_fenceDestroy(fence);
}
GfxResult WebGPUBackend::fenceGetStatus(GfxFence fence) const
{
    return webgpu_fenceGetStatus(fence);
}
GfxResult WebGPUBackend::fenceWait(GfxFence fence, uint64_t timeoutNs) const
{
    return webgpu_fenceWait(fence, timeoutNs);
}
void WebGPUBackend::fenceReset(GfxFence fence) const
{
    webgpu_fenceReset(fence);
}

// Semaphore functions
void WebGPUBackend::semaphoreDestroy(GfxSemaphore semaphore) const
{
    webgpu_semaphoreDestroy(semaphore);
}
GfxSemaphoreType WebGPUBackend::semaphoreGetType(GfxSemaphore semaphore) const
{
    return webgpu_semaphoreGetType(semaphore);
}
GfxResult WebGPUBackend::semaphoreSignal(GfxSemaphore semaphore, uint64_t value) const
{
    return webgpu_semaphoreSignal(semaphore, value);
}
GfxResult WebGPUBackend::semaphoreWait(GfxSemaphore semaphore, uint64_t value, uint64_t timeoutNs) const
{
    return webgpu_semaphoreWait(semaphore, value, timeoutNs);
}
uint64_t WebGPUBackend::semaphoreGetValue(GfxSemaphore semaphore) const
{
    return webgpu_semaphoreGetValue(semaphore);
}

const IBackend* WebGPUBackend::create()
{
    static WebGPUBackend webgpuBackend;
    return &webgpuBackend;
}

} // namespace gfx::webgpu