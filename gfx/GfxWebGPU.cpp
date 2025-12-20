#include "../dependencies/include/webgpu/webgpu.h"
#include "GfxApi.h"
#include "GfxBackend.h"

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

// ============================================================================
// Internal C++ Classes with RAII
// ============================================================================

namespace gfx::webgpu {

class Instance {
    WGPUInstance instance_ = nullptr;

public:
    // Prevent copying
    Instance(const Instance&) = delete;
    Instance& operator=(const Instance&) = delete;

    Instance(const GfxInstanceDescriptor* descriptor)
    {
        WGPUInstanceDescriptor wgpu_desc = WGPU_INSTANCE_DESCRIPTOR_INIT;
        instance_ = wgpuCreateInstance(&wgpu_desc);
    }

    ~Instance()
    {
        if (instance_) {
            wgpuInstanceRelease(instance_);
        }
    }

    WGPUInstance handle() const { return instance_; }
};

class Adapter {
    WGPUAdapter adapter_ = nullptr;
    std::string name_;

public:
    // Prevent copying
    Adapter(const Adapter&) = delete;
    Adapter& operator=(const Adapter&) = delete;

    Adapter(WGPUAdapter adapter)
        : adapter_(adapter)
        , name_("WebGPU Adapter")
    {
    }

    ~Adapter()
    {
        if (adapter_) {
            wgpuAdapterRelease(adapter_);
        }
    }

    WGPUAdapter handle() const { return adapter_; }
    const char* getName() const { return name_.c_str(); }
};

class Queue {
    WGPUQueue queue_ = nullptr;

public:
    // Prevent copying
    Queue(const Queue&) = delete;
    Queue& operator=(const Queue&) = delete;

    Queue(WGPUQueue queue)
        : queue_(queue)
    {
        if (queue_) {
            wgpuQueueAddRef(queue_);
        }
    }

    ~Queue()
    {
        if (queue_) {
            wgpuQueueRelease(queue_);
        }
    }

    WGPUQueue handle() const { return queue_; }
};

class Device {
    WGPUDevice device_ = nullptr;
    Adapter* adapter_ = nullptr; // Non-owning pointer
    std::unique_ptr<Queue> queue_;

public:
    // Prevent copying
    Device(const Device&) = delete;
    Device& operator=(const Device&) = delete;

    Device(Adapter* adapter, WGPUDevice device)
        : adapter_(adapter)
        , device_(device)
    {
        if (device_) {
            WGPUQueue wgpu_queue = wgpuDeviceGetQueue(device_);
            queue_ = std::make_unique<Queue>(wgpu_queue);
        }
    }

    ~Device()
    {
        queue_.reset();
        if (device_) {
            wgpuDeviceRelease(device_);
        }
    }

    WGPUDevice handle() const { return device_; }
    Queue* getQueue() { return queue_.get(); }
    Adapter* getAdapter() { return adapter_; }
};

class Buffer {
    WGPUBuffer buffer_ = nullptr;
    uint64_t size_ = 0;
    GfxBufferUsage usage_ = GFX_BUFFER_USAGE_NONE;

public:
    // Prevent copying
    Buffer(const Buffer&) = delete;
    Buffer& operator=(const Buffer&) = delete;

    Buffer(WGPUBuffer buffer, uint64_t size, GfxBufferUsage usage)
        : buffer_(buffer)
        , size_(size)
        , usage_(usage)
    {
    }

    ~Buffer()
    {
        if (buffer_) {
            wgpuBufferRelease(buffer_);
        }
    }

    WGPUBuffer handle() const { return buffer_; }
    uint64_t getSize() const { return size_; }
    GfxBufferUsage getUsage() const { return usage_; }
};

class Texture {
    WGPUTexture texture_ = nullptr;
    WGPUExtent3D size_ = {};
    WGPUTextureFormat format_ = WGPUTextureFormat_Undefined;
    uint32_t mipLevels_ = 0;
    uint32_t sampleCount_ = 0;
    WGPUTextureUsage usage_ = WGPUTextureUsage_None;

public:
    // Prevent copying
    Texture(const Texture&) = delete;
    Texture& operator=(const Texture&) = delete;

    Texture(WGPUTexture texture, WGPUExtent3D size, WGPUTextureFormat format,
        uint32_t mipLevels, uint32_t sampleCount, WGPUTextureUsage usage)
        : texture_(texture)
        , size_(size)
        , format_(format)
        , mipLevels_(mipLevels)
        , sampleCount_(sampleCount)
        , usage_(usage)
    {
    }

    ~Texture()
    {
        if (texture_) {
            wgpuTextureRelease(texture_);
        }
    }

    WGPUTexture handle() const { return texture_; }
    WGPUExtent3D getSize() const { return size_; }
    WGPUTextureFormat getFormat() const { return format_; }
    uint32_t getMipLevels() const { return mipLevels_; }
    uint32_t getSampleCount() const { return sampleCount_; }
    WGPUTextureUsage getUsage() const { return usage_; }
};

class TextureView {
    WGPUTextureView view_ = nullptr;
    Texture* texture_ = nullptr; // Non-owning

public:
    // Prevent copying
    TextureView(const TextureView&) = delete;
    TextureView& operator=(const TextureView&) = delete;

    TextureView(WGPUTextureView view, Texture* texture = nullptr)
        : view_(view)
        , texture_(texture)
    {
    }

    ~TextureView()
    {
        if (view_) {
            wgpuTextureViewRelease(view_);
        }
    }

    WGPUTextureView handle() const { return view_; }
    Texture* getTexture() { return texture_; }
};

class Sampler {
    WGPUSampler sampler_ = nullptr;

public:
    // Prevent copying
    Sampler(const Sampler&) = delete;
    Sampler& operator=(const Sampler&) = delete;

    Sampler(WGPUSampler sampler)
        : sampler_(sampler)
    {
    }

    ~Sampler()
    {
        if (sampler_) {
            wgpuSamplerRelease(sampler_);
        }
    }

    WGPUSampler handle() const { return sampler_; }
};

class Shader {
    WGPUShaderModule module_ = nullptr;

public:
    // Prevent copying
    Shader(const Shader&) = delete;
    Shader& operator=(const Shader&) = delete;

    Shader(WGPUShaderModule module)
        : module_(module)
    {
    }

    ~Shader()
    {
        if (module_) {
            wgpuShaderModuleRelease(module_);
        }
    }

    WGPUShaderModule handle() const { return module_; }
};

class BindGroupLayout {
    WGPUBindGroupLayout layout_ = nullptr;

public:
    // Prevent copying
    BindGroupLayout(const BindGroupLayout&) = delete;
    BindGroupLayout& operator=(const BindGroupLayout&) = delete;

    BindGroupLayout(WGPUBindGroupLayout layout)
        : layout_(layout)
    {
    }

    ~BindGroupLayout()
    {
        if (layout_) {
            wgpuBindGroupLayoutRelease(layout_);
        }
    }

    WGPUBindGroupLayout handle() const { return layout_; }
};

class BindGroup {
    WGPUBindGroup bindGroup_ = nullptr;

public:
    // Prevent copying
    BindGroup(const BindGroup&) = delete;
    BindGroup& operator=(const BindGroup&) = delete;

    BindGroup(WGPUBindGroup bindGroup)
        : bindGroup_(bindGroup)
    {
    }

    ~BindGroup()
    {
        if (bindGroup_) {
            wgpuBindGroupRelease(bindGroup_);
        }
    }

    WGPUBindGroup handle() const { return bindGroup_; }
};

class RenderPipeline {
    WGPURenderPipeline pipeline_ = nullptr;

public:
    // Prevent copying
    RenderPipeline(const RenderPipeline&) = delete;
    RenderPipeline& operator=(const RenderPipeline&) = delete;

    RenderPipeline(WGPURenderPipeline pipeline)
        : pipeline_(pipeline)
    {
    }

    ~RenderPipeline()
    {
        if (pipeline_) {
            wgpuRenderPipelineRelease(pipeline_);
        }
    }

    WGPURenderPipeline handle() const { return pipeline_; }
};

class ComputePipeline {
    WGPUComputePipeline pipeline_ = nullptr;

public:
    // Prevent copying
    ComputePipeline(const ComputePipeline&) = delete;
    ComputePipeline& operator=(const ComputePipeline&) = delete;

    ComputePipeline(WGPUComputePipeline pipeline)
        : pipeline_(pipeline)
    {
    }

    ~ComputePipeline()
    {
        if (pipeline_) {
            wgpuComputePipelineRelease(pipeline_);
        }
    }

    WGPUComputePipeline handle() const { return pipeline_; }
};

class CommandEncoder {
    WGPUCommandEncoder encoder_ = nullptr;

public:
    // Prevent copying
    CommandEncoder(const CommandEncoder&) = delete;
    CommandEncoder& operator=(const CommandEncoder&) = delete;

    CommandEncoder(WGPUCommandEncoder encoder)
        : encoder_(encoder)
    {
    }

    ~CommandEncoder()
    {
        if (encoder_) {
            wgpuCommandEncoderRelease(encoder_);
        }
    }

    WGPUCommandEncoder handle() const { return encoder_; }
};

class RenderPassEncoder {
    WGPURenderPassEncoder encoder_ = nullptr;

public:
    // Prevent copying
    RenderPassEncoder(const RenderPassEncoder&) = delete;
    RenderPassEncoder& operator=(const RenderPassEncoder&) = delete;

    RenderPassEncoder(WGPURenderPassEncoder encoder)
        : encoder_(encoder)
    {
    }

    ~RenderPassEncoder()
    {
        if (encoder_) {
            wgpuRenderPassEncoderRelease(encoder_);
        }
    }

    WGPURenderPassEncoder handle() const { return encoder_; }
};

class ComputePassEncoder {
    WGPUComputePassEncoder encoder_ = nullptr;

public:
    // Prevent copying
    ComputePassEncoder(const ComputePassEncoder&) = delete;
    ComputePassEncoder& operator=(const ComputePassEncoder&) = delete;

    ComputePassEncoder(WGPUComputePassEncoder encoder)
        : encoder_(encoder)
    {
    }

    ~ComputePassEncoder()
    {
        if (encoder_) {
            wgpuComputePassEncoderRelease(encoder_);
        }
    }

    WGPUComputePassEncoder handle() const { return encoder_; }
};

class Surface {
    WGPUSurface surface_ = nullptr;
    uint32_t width_ = 0;
    uint32_t height_ = 0;
    GfxPlatformWindowHandle windowHandle_ = {};

public:
    // Prevent copying
    Surface(const Surface&) = delete;
    Surface& operator=(const Surface&) = delete;

    Surface(WGPUSurface surface, uint32_t width, uint32_t height, const GfxPlatformWindowHandle& windowHandle)
        : surface_(surface)
        , width_(width)
        , height_(height)
        , windowHandle_(windowHandle)
    {
    }

    ~Surface()
    {
        if (surface_) {
            wgpuSurfaceRelease(surface_);
        }
    }

    WGPUSurface handle() const { return surface_; }
    uint32_t getWidth() const { return width_; }
    uint32_t getHeight() const { return height_; }
    void setSize(uint32_t width, uint32_t height)
    {
        width_ = width;
        height_ = height;
    }
    const GfxPlatformWindowHandle& getWindowHandle() const { return windowHandle_; }
};

class Swapchain {
    WGPUSurface surface_ = nullptr; // Non-owning
    WGPUDevice device_ = nullptr; // Non-owning
    uint32_t width_ = 0;
    uint32_t height_ = 0;
    WGPUTextureFormat format_ = WGPUTextureFormat_Undefined;
    WGPUPresentMode presentMode_ = WGPUPresentMode_Fifo;
    uint32_t bufferCount_ = 0;

public:
    // Prevent copying
    Swapchain(const Swapchain&) = delete;
    Swapchain& operator=(const Swapchain&) = delete;

    Swapchain(WGPUSurface surface, WGPUDevice device, uint32_t width, uint32_t height,
        WGPUTextureFormat format, WGPUPresentMode presentMode, uint32_t bufferCount)
        : surface_(surface)
        , device_(device)
        , width_(width)
        , height_(height)
        , format_(format)
        , presentMode_(presentMode)
        , bufferCount_(bufferCount)
    {
    }

    ~Swapchain()
    {
        // Don't release surface or device - they're not owned
    }

    WGPUSurface surface() const { return surface_; }
    WGPUDevice device() const { return device_; }
    uint32_t getWidth() const { return width_; }
    uint32_t getHeight() const { return height_; }
    WGPUTextureFormat getFormat() const { return format_; }
    uint32_t getBufferCount() const { return bufferCount_; }

    void setSize(uint32_t width, uint32_t height)
    {
        width_ = width;
        height_ = height;
    }
};

class Fence {
    bool signaled_ = false;

public:
    // Prevent copying
    Fence(const Fence&) = delete;
    Fence& operator=(const Fence&) = delete;

    Fence(bool signaled)
        : signaled_(signaled)
    {
    }

    ~Fence() = default;

    bool isSignaled() const { return signaled_; }
    void setSignaled(bool signaled) { signaled_ = signaled; }
};

class Semaphore {
    GfxSemaphoreType type_ = GFX_SEMAPHORE_TYPE_BINARY;
    uint64_t value_ = 0;

public:
    // Prevent copying
    Semaphore(const Semaphore&) = delete;
    Semaphore& operator=(const Semaphore&) = delete;

    Semaphore(GfxSemaphoreType type, uint64_t value)
        : type_(type)
        , value_(value)
    {
    }

    ~Semaphore() = default;

    GfxSemaphoreType getType() const { return type_; }
    uint64_t getValue() const { return value_; }
    void setValue(uint64_t value) { value_ = value; }
};

} // namespace gfx::webgpu

// ============================================================================
// C API Functions
// ============================================================================

// Callback helpers for async operations
struct AdapterRequestData {
    GfxAdapter* outAdapter = nullptr;
    gfx::webgpu::Instance* instance = nullptr;
};

static void adapterRequestCallback(WGPURequestAdapterStatus status, WGPUAdapter adapter,
    WGPUStringView message, void* userdata1, void* userdata2)
{
    auto* data = static_cast<AdapterRequestData*>(userdata1);
    if (status == WGPURequestAdapterStatus_Success && adapter && data) {
        auto* adapterObj = new gfx::webgpu::Adapter(adapter);
        *data->outAdapter = reinterpret_cast<GfxAdapter>(adapterObj);
    }
}

struct DeviceRequestData {
    GfxDevice* outDevice = nullptr;
    gfx::webgpu::Adapter* adapter = nullptr;
};

static void deviceRequestCallback(WGPURequestDeviceStatus status, WGPUDevice device,
    WGPUStringView message, void* userdata1, void* userdata2)
{
    auto* data = static_cast<DeviceRequestData*>(userdata1);
    if (status == WGPURequestDeviceStatus_Success && device && data) {
        auto* deviceObj = new gfx::webgpu::Device(data->adapter, device);
        *data->outDevice = reinterpret_cast<GfxDevice>(deviceObj);
    }
}

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

    AdapterRequestData data;
    data.outAdapter = outAdapter;
    data.instance = inst;

    WGPURequestAdapterCallbackInfo callbackInfo = WGPU_REQUEST_ADAPTER_CALLBACK_INFO_INIT;
    callbackInfo.mode = WGPUCallbackMode_WaitAnyOnly;
    callbackInfo.callback = adapterRequestCallback;
    callbackInfo.userdata1 = &data;

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

    DeviceRequestData data;
    data.outDevice = outDevice;
    data.adapter = adapterPtr;

    WGPURequestDeviceCallbackInfo callbackInfo = WGPU_REQUEST_DEVICE_CALLBACK_INFO_INIT;
    callbackInfo.mode = WGPUCallbackMode_WaitAnyOnly;
    callbackInfo.callback = deviceRequestCallback;
    callbackInfo.userdata1 = &data;

    WGPUFuture future = wgpuAdapterRequestDevice(adapterPtr->handle(), &wgpuDesc, callbackInfo);

    // Note: Would need instance handle for proper waiting - simplified here
    // wgpuInstanceWaitAny(instance, 1, &waitInfo, UINT64_MAX);

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

    auto* surface = new gfx::webgpu::Surface(wgpuSurface, descriptor->width,
        descriptor->height, descriptor->windowHandle);
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
    for (size_t i = 0; i < capabilities.formatCount; i++) {
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
    for (size_t i = 0; i < capabilities.presentModeCount; i++) {
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

    auto* buffer = new gfx::webgpu::Buffer(wgpuBuffer, descriptor->size, descriptor->usage);
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
    wgpuDesc.size = { descriptor->size.width, descriptor->size.height, descriptor->size.depth };
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
        switch (*descriptor->compare) {
        case GFX_COMPARE_FUNCTION_NEVER:
            wgpuDesc.compare = WGPUCompareFunction_Never;
            break;
        case GFX_COMPARE_FUNCTION_LESS:
            wgpuDesc.compare = WGPUCompareFunction_Less;
            break;
        case GFX_COMPARE_FUNCTION_EQUAL:
            wgpuDesc.compare = WGPUCompareFunction_Equal;
            break;
        case GFX_COMPARE_FUNCTION_LESS_EQUAL:
            wgpuDesc.compare = WGPUCompareFunction_LessEqual;
            break;
        case GFX_COMPARE_FUNCTION_GREATER:
            wgpuDesc.compare = WGPUCompareFunction_Greater;
            break;
        case GFX_COMPARE_FUNCTION_NOT_EQUAL:
            wgpuDesc.compare = WGPUCompareFunction_NotEqual;
            break;
        case GFX_COMPARE_FUNCTION_GREATER_EQUAL:
            wgpuDesc.compare = WGPUCompareFunction_GreaterEqual;
            break;
        case GFX_COMPARE_FUNCTION_ALWAYS:
            wgpuDesc.compare = WGPUCompareFunction_Always;
            break;
        default:
            wgpuDesc.compare = WGPUCompareFunction_Undefined;
            break;
        }
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

        for (uint32_t i = 0; i < descriptor->entryCount; i++) {
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
                wgpuEntry.texture.sampleType = WGPUTextureSampleType_Float;
                wgpuEntry.texture.viewDimension = WGPUTextureViewDimension_2D;
                wgpuEntry.texture.multisampled = entry.texture.multisampled ? WGPU_TRUE : WGPU_FALSE;
                break;
            case GFX_BINDING_TYPE_STORAGE_TEXTURE:
                wgpuEntry.storageTexture.access = entry.storageTexture.writeOnly ? WGPUStorageTextureAccess_WriteOnly : WGPUStorageTextureAccess_ReadOnly;
                wgpuEntry.storageTexture.format = gfxFormatToWGPUFormat(entry.storageTexture.format);
                wgpuEntry.storageTexture.viewDimension = WGPUTextureViewDimension_2D;
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

        for (uint32_t i = 0; i < descriptor->entryCount; i++) {
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

        for (uint32_t i = 0; i < descriptor->vertex.bufferCount; i++) {
            const auto& buffer = descriptor->vertex.buffers[i];

            std::vector<WGPUVertexAttribute> attributes;
            attributes.reserve(buffer.attributeCount);

            for (uint32_t j = 0; j < buffer.attributeCount; j++) {
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

            for (uint32_t i = 0; i < descriptor->fragment->targetCount; i++) {
                const auto& target = descriptor->fragment->targets[i];
                WGPUColorTargetState wgpuTarget = WGPU_COLOR_TARGET_STATE_INIT;
                wgpuTarget.format = gfxFormatToWGPUFormat(target.format);
                wgpuTarget.writeMask = target.writeMask;

                if (target.blend) {
                    WGPUBlendState blend = WGPU_BLEND_STATE_INIT;

                    // Color blend
                    switch (target.blend->color.operation) {
                    case GFX_BLEND_OPERATION_ADD:
                        blend.color.operation = WGPUBlendOperation_Add;
                        break;
                    case GFX_BLEND_OPERATION_SUBTRACT:
                        blend.color.operation = WGPUBlendOperation_Subtract;
                        break;
                    case GFX_BLEND_OPERATION_REVERSE_SUBTRACT:
                        blend.color.operation = WGPUBlendOperation_ReverseSubtract;
                        break;
                    case GFX_BLEND_OPERATION_MIN:
                        blend.color.operation = WGPUBlendOperation_Min;
                        break;
                    case GFX_BLEND_OPERATION_MAX:
                        blend.color.operation = WGPUBlendOperation_Max;
                        break;
                    }

                    // Simplified blend factor mapping
                    blend.color.srcFactor = WGPUBlendFactor_One;
                    blend.color.dstFactor = WGPUBlendFactor_Zero;
                    blend.alpha = blend.color;

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
    primitiveState.unclippedDepth = descriptor->primitive.unclippedDepth ? WGPU_TRUE : WGPU_FALSE;

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

// Surface functions
void webgpu_surfaceDestroy(GfxSurface surface)
{
    if (!surface) {
        return;
    }
    delete reinterpret_cast<gfx::webgpu::Surface*>(surface);
}

uint32_t webgpu_surfaceGetWidth(GfxSurface surface)
{
    if (!surface) {
        return 0;
    }
    return reinterpret_cast<gfx::webgpu::Surface*>(surface)->getWidth();
}

uint32_t webgpu_surfaceGetHeight(GfxSurface surface)
{
    if (!surface) {
        return 0;
    }
    return reinterpret_cast<gfx::webgpu::Surface*>(surface)->getHeight();
}

void webgpu_surfaceResize(GfxSurface surface, uint32_t width, uint32_t height)
{
    if (!surface) {
        return;
    }
    reinterpret_cast<gfx::webgpu::Surface*>(surface)->setSize(width, height);
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

GfxResult webgpu_swapchainPresentWithSync(GfxSwapchain swapchain, const GfxPresentInfo* presentInfo)
{
    if (!swapchain) {
        return GFX_RESULT_ERROR_INVALID_PARAMETER;
    }
    
    // WebGPU doesn't support explicit wait semaphores for present
    // The queue submission already ensures ordering, so we just present
    (void)presentInfo; // Wait semaphores are noted but not used in WebGPU
    
    return webgpu_swapchainPresent(swapchain);
}

GfxResult webgpu_swapchainPresent(GfxSwapchain swapchain)
{
    if (!swapchain) {
        return GFX_RESULT_ERROR_INVALID_PARAMETER;
    }

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

void webgpu_swapchainResize(GfxSwapchain swapchain, uint32_t width, uint32_t height)
{
    if (!swapchain) {
        return;
    }

    auto* swapchainPtr = reinterpret_cast<gfx::webgpu::Swapchain*>(swapchain);
    swapchainPtr->setSize(width, height);

    // Reconfigure surface
    WGPUSurfaceConfiguration config = WGPU_SURFACE_CONFIGURATION_INIT;
    config.device = swapchainPtr->device();
    config.format = swapchainPtr->getFormat();
    config.usage = WGPUTextureUsage_RenderAttachment;
    config.width = width;
    config.height = height;
    config.presentMode = WGPUPresentMode_Fifo;
    config.alphaMode = WGPUCompositeAlphaMode_Auto;

    wgpuSurfaceConfigure(swapchainPtr->surface(), &config);
}

bool webgpu_swapchainNeedsRecreation(GfxSwapchain swapchain)
{
    if (!swapchain) {
        return false;
    }

    auto* swapchainPtr = reinterpret_cast<gfx::webgpu::Swapchain*>(swapchain);

    WGPUSurfaceTexture surfaceTexture = WGPU_SURFACE_TEXTURE_INIT;
    wgpuSurfaceGetCurrentTexture(swapchainPtr->surface(), &surfaceTexture);

    bool needsRecreation = false;
    switch (surfaceTexture.status) {
    case WGPUSurfaceGetCurrentTextureStatus_SuccessOptimal:
    case WGPUSurfaceGetCurrentTextureStatus_SuccessSuboptimal:
        needsRecreation = false;
        break;
    case WGPUSurfaceGetCurrentTextureStatus_Timeout:
    case WGPUSurfaceGetCurrentTextureStatus_Outdated:
    case WGPUSurfaceGetCurrentTextureStatus_Lost:
    case WGPUSurfaceGetCurrentTextureStatus_Error:
        needsRecreation = true;
        break;
    }

    if (surfaceTexture.texture) {
        wgpuTextureRelease(surfaceTexture.texture);
    }

    return needsRecreation;
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

GfxResult webgpu_bufferMapAsync(GfxBuffer buffer, uint64_t offset, uint64_t size, void** mappedPointer)
{
    // WebGPU buffer mapping is async - simplified stub
    if (!buffer || !mappedPointer) {
        return GFX_RESULT_ERROR_INVALID_PARAMETER;
    }
    *mappedPointer = nullptr;
    return GFX_RESULT_ERROR_UNSUPPORTED;
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

GfxTexture webgpu_textureViewGetTexture(GfxTextureView textureView)
{
    if (!textureView) {
        return nullptr;
    }
    auto* viewPtr = reinterpret_cast<gfx::webgpu::TextureView*>(textureView);
    return reinterpret_cast<GfxTexture>(viewPtr->getTexture());
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
GfxResult webgpu_queueSubmit(GfxQueue queue, GfxCommandEncoder commandEncoder)
{
    if (!queue || !commandEncoder) {
        return GFX_RESULT_ERROR_INVALID_PARAMETER;
    }

    auto* queuePtr = reinterpret_cast<gfx::webgpu::Queue*>(queue);
    auto* encoderPtr = reinterpret_cast<gfx::webgpu::CommandEncoder*>(commandEncoder);

    WGPUCommandBufferDescriptor cmdDesc = WGPU_COMMAND_BUFFER_DESCRIPTOR_INIT;
    WGPUCommandBuffer cmdBuffer = wgpuCommandEncoderFinish(encoderPtr->handle(), &cmdDesc);

    if (cmdBuffer) {
        wgpuQueueSubmit(queuePtr->handle(), 1, &cmdBuffer);
        wgpuCommandBufferRelease(cmdBuffer);
        return GFX_RESULT_SUCCESS;
    }

    return GFX_RESULT_ERROR_UNKNOWN;
}

GfxResult webgpu_queueSubmitWithSync(GfxQueue queue, const GfxSubmitInfo* submitInfo)
{
    if (!queue || !submitInfo) {
        return GFX_RESULT_ERROR_INVALID_PARAMETER;
    }

    auto* queuePtr = reinterpret_cast<gfx::webgpu::Queue*>(queue);

    // WebGPU doesn't support semaphore-based sync - just submit command buffers
    for (uint32_t i = 0; i < submitInfo->commandEncoderCount; i++) {
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

    // Note: To properly wait, we would need the WGPUInstance handle
    // Since we don't have it here, this is a best-effort synchronization
    // In a complete implementation, the Queue should store a reference to Instance
    // For now, the callback with WaitAnyOnly mode will block until work completes

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
    const GfxTextureView* colorAttachments,
    uint32_t colorAttachmentCount,
    const GfxColor* clearColors,
    GfxTextureView depthStencilAttachment,
    float depthClearValue,
    uint32_t stencilClearValue,
    GfxRenderPassEncoder* outEncoder)
{
    if (!commandEncoder || !outEncoder) {
        return GFX_RESULT_ERROR_INVALID_PARAMETER;
    }

    auto* encoderPtr = reinterpret_cast<gfx::webgpu::CommandEncoder*>(commandEncoder);

    WGPURenderPassDescriptor wgpuDesc = WGPU_RENDER_PASS_DESCRIPTOR_INIT;

    std::vector<WGPURenderPassColorAttachment> wgpuColorAttachments;
    if (colorAttachmentCount > 0 && colorAttachments) {
        wgpuColorAttachments.reserve(colorAttachmentCount);

        for (uint32_t i = 0; i < colorAttachmentCount; i++) {
            WGPURenderPassColorAttachment attachment = WGPU_RENDER_PASS_COLOR_ATTACHMENT_INIT;
            if (colorAttachments[i]) {
                auto* viewPtr = reinterpret_cast<gfx::webgpu::TextureView*>(colorAttachments[i]);
                attachment.view = viewPtr->handle();
                attachment.loadOp = WGPULoadOp_Clear;
                attachment.storeOp = WGPUStoreOp_Store;

                if (clearColors) {
                    attachment.clearValue = { clearColors[i].r, clearColors[i].g,
                        clearColors[i].b, clearColors[i].a };
                }
            }
            wgpuColorAttachments.push_back(attachment);
        }

        wgpuDesc.colorAttachments = wgpuColorAttachments.data();
        wgpuDesc.colorAttachmentCount = colorAttachmentCount;
    }

    WGPURenderPassDepthStencilAttachment wgpuDepthStencil = WGPU_RENDER_PASS_DEPTH_STENCIL_ATTACHMENT_INIT;
    if (depthStencilAttachment) {
        auto* viewPtr = reinterpret_cast<gfx::webgpu::TextureView*>(depthStencilAttachment);
        wgpuDepthStencil.view = viewPtr->handle();
        wgpuDepthStencil.depthLoadOp = WGPULoadOp_Clear;
        wgpuDepthStencil.depthStoreOp = WGPUStoreOp_Store;
        wgpuDepthStencil.depthClearValue = depthClearValue;
        wgpuDepthStencil.stencilLoadOp = WGPULoadOp_Clear;
        wgpuDepthStencil.stencilStoreOp = WGPUStoreOp_Store;
        wgpuDepthStencil.stencilClearValue = stencilClearValue;

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

GfxResult webgpu_commandEncoderBeginComputePass(GfxCommandEncoder commandEncoder, const char* label,
    GfxComputePassEncoder* outEncoder)
{
    if (!commandEncoder || !outEncoder) {
        return GFX_RESULT_ERROR_INVALID_PARAMETER;
    }

    auto* encoderPtr = reinterpret_cast<gfx::webgpu::CommandEncoder*>(commandEncoder);

    WGPUComputePassDescriptor wgpuDesc = WGPU_COMPUTE_PASS_DESCRIPTOR_INIT;
    if (label) {
        wgpuDesc.label = gfxStringView(label);
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

    WGPUTexelCopyTextureInfo sourceInfo = WGPU_TEXEL_COPY_TEXTURE_INFO_INIT;
    sourceInfo.texture = srcPtr->handle();
    sourceInfo.mipLevel = sourceMipLevel;
    sourceInfo.origin = { sourceOrigin->x, sourceOrigin->y, sourceOrigin->z };

    WGPUTexelCopyTextureInfo destInfo = WGPU_TEXEL_COPY_TEXTURE_INFO_INIT;
    destInfo.texture = dstPtr->handle();
    destInfo.mipLevel = destinationMipLevel;
    destInfo.origin = { destinationOrigin->x, destinationOrigin->y, destinationOrigin->z };

    WGPUExtent3D wgpuExtent = { extent->width, extent->height, extent->depth };

    wgpuCommandEncoderCopyTextureToTexture(encoderPtr->handle(), &sourceInfo, &destInfo, &wgpuExtent);
    
    (void)srcFinalLayout; // WebGPU handles layout transitions automatically
    (void)dstFinalLayout;
}

void webgpu_commandEncoderPipelineBarrier(GfxCommandEncoder commandEncoder,
    const GfxTextureBarrier* textureBarriers, uint32_t textureBarrierCount)
{
    // WebGPU handles synchronization and layout transitions automatically
    // This is a no-op for WebGPU backend
    (void)commandEncoder;
    (void)textureBarriers;
    (void)textureBarrierCount;
}

void webgpu_commandEncoderFinish(GfxCommandEncoder commandEncoder)
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

void webgpu_renderPassEncoderSetBindGroup(GfxRenderPassEncoder renderPassEncoder, uint32_t index, GfxBindGroup bindGroup)
{
    if (!renderPassEncoder || !bindGroup) {
        return;
    }

    auto* encoderPtr = reinterpret_cast<gfx::webgpu::RenderPassEncoder*>(renderPassEncoder);
    auto* bindGroupPtr = reinterpret_cast<gfx::webgpu::BindGroup*>(bindGroup);

    wgpuRenderPassEncoderSetBindGroup(encoderPtr->handle(), index, bindGroupPtr->handle(), 0, nullptr);
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

void webgpu_computePassEncoderSetBindGroup(GfxComputePassEncoder computePassEncoder, uint32_t index, GfxBindGroup bindGroup)
{
    if (!computePassEncoder || !bindGroup) {
        return;
    }

    auto* encoderPtr = reinterpret_cast<gfx::webgpu::ComputePassEncoder*>(computePassEncoder);
    auto* bindGroupPtr = reinterpret_cast<gfx::webgpu::BindGroup*>(bindGroup);

    wgpuComputePassEncoderSetBindGroup(encoderPtr->handle(), index, bindGroupPtr->handle(), 0, nullptr);
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

// ============================================================================
// Backend Function Table Export
// ============================================================================

static const GfxBackendAPI webGpuBackendApi = {
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
    .swapchainAcquireNextImage = webgpu_swapchainAcquireNextImage,
    .swapchainGetImageView = webgpu_swapchainGetImageView,
    .swapchainGetCurrentTextureView = webgpu_swapchainGetCurrentTextureView,
    .swapchainPresentWithSync = webgpu_swapchainPresentWithSync,
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
    .textureGetLayout = webgpu_textureGetLayout,
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
    .commandEncoderCopyTextureToTexture = webgpu_commandEncoderCopyTextureToTexture,
    .commandEncoderPipelineBarrier = webgpu_commandEncoderPipelineBarrier,
    .commandEncoderFinish = webgpu_commandEncoderFinish,
    .renderPassEncoderDestroy = webgpu_renderPassEncoderDestroy,
    .renderPassEncoderSetPipeline = webgpu_renderPassEncoderSetPipeline,
    .renderPassEncoderSetBindGroup = webgpu_renderPassEncoderSetBindGroup,
    .renderPassEncoderSetVertexBuffer = webgpu_renderPassEncoderSetVertexBuffer,
    .renderPassEncoderSetIndexBuffer = webgpu_renderPassEncoderSetIndexBuffer,
    .renderPassEncoderSetViewport = webgpu_renderPassEncoderSetViewport,
    .renderPassEncoderSetScissorRect = webgpu_renderPassEncoderSetScissorRect,
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

extern "C" {

const GfxBackendAPI* gfxGetWebgpuBackend(void)
{
    return &webGpuBackendApi;
}

} // extern "C"
