#ifndef GFX_CPP_GFX_HPP
#define GFX_CPP_GFX_HPP

#include <cstdint>
#include <cstring>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <tuple>
#include <variant>
#include <vector>

namespace gfx {

// ============================================================================
// Core Enumerations
// ============================================================================

enum class Backend : int32_t {
    Vulkan = 0,
    WebGPU = 1,
    Auto = 2
};

enum class AdapterType : int32_t {
    DiscreteGPU = 0,
    IntegratedGPU = 1,
    CPU = 2,
    Unknown = 3
};

enum class AdapterPreference : int32_t {
    Undefined = 0,
    LowPower = 1,
    HighPerformance = 2,
    Software = 3
};

enum class PresentMode : int32_t {
    Immediate = 0, // No vsync, immediate presentation
    Fifo = 1, // Vsync, first-in-first-out queue
    FifoRelaxed = 2, // Vsync with relaxed timing
    Mailbox = 3 // Triple buffering
};

enum class PrimitiveTopology : int32_t {
    PointList = 0,
    LineList = 1,
    LineStrip = 2,
    TriangleList = 3,
    TriangleStrip = 4
};

enum class FrontFace : int32_t {
    CounterClockwise = 0,
    Clockwise = 1
};

enum class CullMode : int32_t {
    None = 0,
    Front = 1,
    Back = 2,
    FrontAndBack = 3
};

enum class PolygonMode : int32_t {
    Fill = 0,
    Line = 1,
    Point = 2
};

enum class IndexFormat : int32_t {
    Undefined = 0,
    Uint16 = 1,
    Uint32 = 2
};

enum class TextureFormat : int32_t {
    Undefined = 0,
    R8Unorm = 1,
    R8G8Unorm = 2,
    R8G8B8A8Unorm = 3,
    R8G8B8A8UnormSrgb = 4,
    B8G8R8A8Unorm = 5,
    B8G8R8A8UnormSrgb = 6,
    R16Float = 7,
    R16G16Float = 8,
    R16G16B16A16Float = 9,
    R32Float = 10,
    R32G32Float = 11,
    R32G32B32Float = 12,
    R32G32B32A32Float = 13,
    Depth16Unorm = 14,
    Depth24Plus = 15,
    Depth32Float = 16,
    Stencil8 = 17,
    Depth24PlusStencil8 = 18,
    Depth32FloatStencil8 = 19
};

enum class TextureType : int32_t {
    Texture1D = 0,
    Texture2D = 1,
    Texture3D = 2,
    TextureCube = 3
};

enum class TextureViewType : int32_t {
    View1D = 0,
    View2D = 1,
    View3D = 2,
    ViewCube = 3,
    View1DArray = 4,
    View2DArray = 5,
    ViewCubeArray = 6
};

enum class TextureUsage : uint32_t {
    None = 0,
    CopySrc = 1 << 0,
    CopyDst = 1 << 1,
    TextureBinding = 1 << 2,
    StorageBinding = 1 << 3,
    RenderAttachment = 1 << 4
};

enum class BufferUsage : uint32_t {
    None = 0,
    MapRead = 1 << 0,
    MapWrite = 1 << 1,
    CopySrc = 1 << 2,
    CopyDst = 1 << 3,
    Index = 1 << 4,
    Vertex = 1 << 5,
    Uniform = 1 << 6,
    Storage = 1 << 7,
    Indirect = 1 << 8
};

enum class MemoryProperty : uint32_t {
    DeviceLocal = 1 << 0,
    HostVisible = 1 << 1,
    HostCoherent = 1 << 2,
    HostCached = 1 << 3
};

enum class ShaderStage : uint32_t {
    None = 0,
    Vertex = 1 << 0,
    Fragment = 1 << 1,
    Compute = 1 << 2
};

enum class FilterMode : int32_t {
    Nearest = 0,
    Linear = 1
};

enum class AddressMode : int32_t {
    Repeat = 0,
    MirrorRepeat = 1,
    ClampToEdge = 2
};

enum class CompareFunction : int32_t {
    Undefined = 0,
    Never = 1,
    Less = 2,
    Equal = 3,
    LessEqual = 4,
    Greater = 5,
    NotEqual = 6,
    GreaterEqual = 7,
    Always = 8
};

enum class BlendOperation : int32_t {
    Add = 0,
    Subtract = 1,
    ReverseSubtract = 2,
    Min = 3,
    Max = 4
};

enum class BlendFactor : int32_t {
    Zero = 0,
    One = 1,
    Src = 2,
    OneMinusSrc = 3,
    SrcAlpha = 4,
    OneMinusSrcAlpha = 5,
    Dst = 6,
    OneMinusDst = 7,
    DstAlpha = 8,
    OneMinusDstAlpha = 9,
    SrcAlphaSaturated = 10,
    Constant = 11,
    OneMinusConstant = 12
};

enum class StencilOperation : int32_t {
    Keep = 0,
    Zero = 1,
    Replace = 2,
    IncrementClamp = 3,
    DecrementClamp = 4,
    Invert = 5,
    IncrementWrap = 6,
    DecrementWrap = 7
};

enum class SampleCount {
    Count1 = 1,
    Count2 = 2,
    Count4 = 4,
    Count8 = 8,
    Count16 = 16,
    Count32 = 32,
    Count64 = 64
};

enum class ShaderSourceType : int32_t {
    WGSL = 0, // WGSL text source (for WebGPU)
    SPIRV = 1 // SPIR-V binary (for Vulkan)
};

// Synchronization enums
enum class FenceStatus : int32_t {
    Unsignaled = 0,
    Signaled = 1,
    Error = 2
};

enum class SemaphoreType : int32_t {
    Binary = 0,
    Timeline = 1
};

enum class QueryType : int32_t {
    Occlusion = 0,
    Timestamp = 1
};

// Extension name constants (matching C API)
constexpr const char* INSTANCE_EXTENSION_SURFACE = "gfx_surface";
constexpr const char* INSTANCE_EXTENSION_DEBUG = "gfx_debug";
constexpr const char* DEVICE_EXTENSION_SWAPCHAIN = "gfx_swapchain";
constexpr const char* DEVICE_EXTENSION_TIMELINE_SEMAPHORE = "gfx_timeline_semaphore";
constexpr const char* DEVICE_EXTENSION_MULTIVIEW = "gfx_multiview";
constexpr const char* DEVICE_EXTENSION_ANISOTROPIC_FILTERING = "gfx_anisotropic_filtering";

enum class QueueFlags : uint32_t {
    None = 0,
    Graphics = 0x00000001,
    Compute = 0x00000002,
    Transfer = 0x00000004,
    SparseBinding = 0x00000008
};

enum class Result {
    Success = 0,
    Timeout = 1,
    NotReady = 2,
    // Error codes (negative values)
    ErrorInvalidArgument = -1,
    ErrorNotFound = -2,
    ErrorOutOfMemory = -3,
    ErrorDeviceLost = -4,
    ErrorSurfaceLost = -5,
    ErrorOutOfDate = -6,
    ErrorBackendNotLoaded = -7,
    ErrorFeatureNotSupported = -8,
    ErrorUnknown = -9
};

enum class LoadOp : int32_t {
    Load = 0, // Load existing contents
    Clear = 1, // Clear to specified clear value
    DontCare = 2 // Don't care about initial contents (better performance on tiled GPUs)
};

enum class StoreOp : int32_t {
    Store = 0, // Store contents after render pass
    DontCare = 1 // Don't care about contents after render pass (better performance for transient attachments)
};

enum class TextureLayout : int32_t {
    Undefined = 0,
    General = 1,
    ColorAttachment = 2,
    DepthStencilAttachment = 3,
    DepthStencilReadOnly = 4,
    ShaderReadOnly = 5,
    TransferSrc = 6,
    TransferDst = 7,
    PresentSrc = 8
};

enum class PipelineStage : uint32_t {
    None = 0,
    TopOfPipe = 1 << 0, // 0x00000001
    DrawIndirect = 1 << 1, // 0x00000002
    VertexInput = 1 << 2, // 0x00000004
    VertexShader = 1 << 3, // 0x00000008
    TessellationControlShader = 1 << 4, // 0x00000010
    TessellationEvaluationShader = 1 << 5, // 0x00000020
    GeometryShader = 1 << 6, // 0x00000040
    FragmentShader = 1 << 7, // 0x00000080
    EarlyFragmentTests = 1 << 8, // 0x00000100
    LateFragmentTests = 1 << 9, // 0x00000200
    ColorAttachmentOutput = 1 << 10, // 0x00000400
    ComputeShader = 1 << 11, // 0x00000800
    Transfer = 1 << 12, // 0x00001000
    BottomOfPipe = 1 << 13, // 0x00002000
    AllGraphics = 0x0000FFFF,
    AllCommands = 1 << 16 // 0x00010000
};

enum class AccessFlags : uint32_t {
    None = 0,
    IndirectCommandRead = 1 << 0,
    IndexRead = 1 << 1,
    VertexAttributeRead = 1 << 2,
    UniformRead = 1 << 3,
    InputAttachmentRead = 1 << 4,
    ShaderRead = 1 << 5,
    ShaderWrite = 1 << 6,
    ColorAttachmentRead = 1 << 7,
    ColorAttachmentWrite = 1 << 8,
    DepthStencilAttachmentRead = 1 << 9,
    DepthStencilAttachmentWrite = 1 << 10,
    TransferRead = 1 << 11,
    TransferWrite = 1 << 12,
    MemoryRead = 1 << 14,
    MemoryWrite = 1 << 15
};

// ============================================================================
// Utility Classes
// ============================================================================

template <typename T>
inline T operator|(T a, T b)
{
    using U = std::underlying_type_t<T>;
    return static_cast<T>(static_cast<U>(a) | static_cast<U>(b));
}

template <typename T>
inline T operator&(T a, T b)
{
    using U = std::underlying_type_t<T>;
    return static_cast<T>(static_cast<U>(a) & static_cast<U>(b));
}

struct Color {
    float r = 0.0f;
    float g = 0.0f;
    float b = 0.0f;
    float a = 1.0f;

    Color() = default;
    Color(float red, float green, float blue, float alpha = 1.0f)
        : r(red)
        , g(green)
        , b(blue)
        , a(alpha)
    {
    }
};

struct Extent3D {
    uint32_t width = 0;
    uint32_t height = 1;
    uint32_t depth = 1;

    Extent3D() = default;
    Extent3D(uint32_t w, uint32_t h = 1, uint32_t d = 1)
        : width(w)
        , height(h)
        , depth(d)
    {
    }
};

struct Origin3D {
    int32_t x = 0;
    int32_t y = 0;
    int32_t z = 0;

    Origin3D() = default;
    Origin3D(int32_t px, int32_t py = 0, int32_t pz = 0)
        : x(px)
        , y(py)
        , z(pz)
    {
    }
};

// ============================================================================
// Platform Abstraction
// ============================================================================

// Common windowing system enum for all platforms
enum class WindowingSystem {
    Win32 = 0,
    Xlib = 1,
    Wayland = 2,
    XCB = 3,
    Metal = 4,
    Emscripten = 5,
    Android = 6
};

// Common platform window handle struct with union for all windowing systems
struct PlatformWindowHandle {
    struct Win32Handle {
        void* hwnd = nullptr; // HWND - Window handle
        void* hinstance; // HINSTANCE - Application instance
    };
    struct XlibHandle {
        void* display; // Display*
        unsigned long window; // Window
    };
    struct WaylandHandle {
        void* surface; // wl_surface*
        void* display; // wl_display*
    };
    struct XcbHandle {
        void* connection; // xcb_connection_t*
        uint32_t window; // xcb_window_t
    };
    struct MetalHandle {
        void* layer; // CAMetalLayer* (optional)
    };
    struct EmscriptenHandle {
        const char* canvasSelector = nullptr; // CSS selector for canvas element (e.g., "#canvas")
    };
    struct AndroidHandle {
        void* window;
    };

    union WindowHandleUnion {
        Win32Handle win32;
        XlibHandle xlib;
        WaylandHandle wayland;
        XcbHandle xcb;
        MetalHandle metal;
        EmscriptenHandle emscripten;
        AndroidHandle android;

        // Pick a default active member:
        constexpr WindowHandleUnion()
            : win32{}
        {
        }
    };

    WindowingSystem windowingSystem{};
    WindowHandleUnion handle{};

    // Factory methods for each windowing system
    static PlatformWindowHandle fromWin32(void* hwnd, void* hinstance = nullptr)
    {
        PlatformWindowHandle h{};
        h.windowingSystem = WindowingSystem::Win32;
        h.handle.win32.hwnd = hwnd;
        h.handle.win32.hinstance = hinstance;
        return h;
    }

    static PlatformWindowHandle fromXlib(void* display, unsigned long window)
    {
        PlatformWindowHandle h{};
        h.windowingSystem = WindowingSystem::Xlib;
        h.handle.xlib.display = display;
        h.handle.xlib.window = window;
        return h;
    }

    static PlatformWindowHandle fromWayland(void* surface, void* display)
    {
        PlatformWindowHandle h{};
        h.windowingSystem = WindowingSystem::Wayland;
        h.handle.wayland.surface = surface;
        h.handle.wayland.display = display;
        return h;
    }

    static PlatformWindowHandle fromXCB(void* connection, uint32_t window)
    {
        PlatformWindowHandle h{};
        h.windowingSystem = WindowingSystem::XCB;
        h.handle.xcb.connection = connection;
        h.handle.xcb.window = window;
        return h;
    }

    static PlatformWindowHandle fromMetal(void* layer)
    {
        PlatformWindowHandle h{};
        h.windowingSystem = WindowingSystem::Metal;
        h.handle.metal.layer = layer;
        return h;
    }

    static PlatformWindowHandle fromEmscripten(const char* canvasSelector)
    {
        PlatformWindowHandle h{};
        h.windowingSystem = WindowingSystem::Emscripten;
        h.handle.emscripten.canvasSelector = canvasSelector;
        return h;
    }

    static PlatformWindowHandle fromAndroid(void* window)
    {
        PlatformWindowHandle h{};
        h.windowingSystem = WindowingSystem::Android;
        h.handle.android.window = window;
        return h;
    }
};

// ============================================================================
// Forward Declarations
// ============================================================================

class Instance;
class Adapter;
class Device;
class Queue;
class Buffer;
class Texture;
class TextureView;
class Sampler;
class Shader;
class RenderPipeline;
class ComputePipeline;
class CommandEncoder;
class RenderPassEncoder;
class ComputePassEncoder;
class BindGroup;
class BindGroupLayout;
class Surface;
class Swapchain;
class RenderPass;
class Framebuffer;
class Fence;
class Semaphore;
class QuerySet;

// ============================================================================
// Logging
// ============================================================================

enum class LogLevel : int32_t {
    Error = 0,
    Warning = 1,
    Info = 2,
    Debug = 3
};

using LogCallback = std::function<void(LogLevel level, const std::string& message)>;

// ============================================================================
// Descriptor Structures
// ============================================================================

struct InstanceDescriptor {
    Backend backend = Backend::Auto;
    std::string applicationName = "GfxCpp Application";
    uint32_t applicationVersion = 1;
    std::vector<std::string> enabledExtensions;
};

struct AdapterDescriptor {
    uint32_t adapterIndex = UINT32_MAX; // Adapter index from enumeration (use UINT32_MAX to ignore)
    AdapterPreference preference = AdapterPreference::Undefined; // Used only when adapterIndex is UINT32_MAX
};

struct QueueFamilyProperties {
    QueueFlags flags = QueueFlags::None;
    uint32_t queueCount = 0;
};

struct QueueRequest {
    uint32_t queueFamilyIndex = 0;
    uint32_t queueIndex = 0;
    float priority = 1.0f;
};

struct DeviceDescriptor {
    std::string label;
    std::vector<std::string> enabledExtensions;
    std::vector<QueueRequest> queueRequests; // Optional: specify which queues to create
};

struct BufferDescriptor {
    std::string label;
    uint64_t size = 0;
    BufferUsage usage = BufferUsage::None;
    MemoryProperty memoryProperties = MemoryProperty::DeviceLocal;
};

struct BufferImportDescriptor {
    std::string label;
    void* nativeHandle = nullptr; // VkBuffer or WGPUBuffer (cast to void*)
    uint64_t size = 0;
    BufferUsage usage = BufferUsage::None;
};

struct BufferInfo {
    uint64_t size = 0;
    BufferUsage usage = BufferUsage::None;
};

struct TextureInfo {
    TextureType type = TextureType::Texture2D;
    Extent3D size;
    uint32_t arrayLayerCount = 1;
    uint32_t mipLevelCount = 1;
    SampleCount sampleCount = SampleCount::Count1;
    TextureFormat format = TextureFormat::Undefined;
    TextureUsage usage = TextureUsage::None;
};

struct TextureDescriptor {
    std::string label;
    TextureType type = TextureType::Texture2D;
    Extent3D size;
    uint32_t arrayLayerCount = 1;
    uint32_t mipLevelCount = 1;
    SampleCount sampleCount = SampleCount::Count1;
    TextureFormat format = TextureFormat::Undefined;
    TextureUsage usage = TextureUsage::None;
};

struct TextureImportDescriptor {
    std::string label;
    void* nativeHandle = nullptr; // VkImage or WGPUTexture (cast to void*)
    TextureType type = TextureType::Texture2D;
    Extent3D size;
    uint32_t arrayLayerCount = 1;
    uint32_t mipLevelCount = 1;
    SampleCount sampleCount = SampleCount::Count1;
    TextureFormat format = TextureFormat::Undefined;
    TextureUsage usage = TextureUsage::None;
    TextureLayout currentLayout = TextureLayout::Undefined; // Current layout of the imported texture
};

struct TextureViewDescriptor {
    std::string label;
    TextureViewType viewType = TextureViewType::View2D;
    TextureFormat format = TextureFormat::Undefined;
    uint32_t baseMipLevel = 0;
    uint32_t mipLevelCount = 1;
    uint32_t baseArrayLayer = 0;
    uint32_t arrayLayerCount = 1;
};

struct SamplerDescriptor {
    std::string label;
    AddressMode addressModeU = AddressMode::ClampToEdge;
    AddressMode addressModeV = AddressMode::ClampToEdge;
    AddressMode addressModeW = AddressMode::ClampToEdge;
    FilterMode magFilter = FilterMode::Nearest;
    FilterMode minFilter = FilterMode::Nearest;
    FilterMode mipmapFilter = FilterMode::Nearest;
    float lodMinClamp = 0.0f;
    float lodMaxClamp = 32.0f;
    CompareFunction compare = CompareFunction::Undefined;
    uint16_t maxAnisotropy = 1;
};

struct ShaderDescriptor {
    std::string label;
    ShaderSourceType sourceType = ShaderSourceType::SPIRV; // Default to SPIR-V for compatibility
    std::vector<uint8_t> code;
    std::string entryPoint = "main";
};

struct BlendComponent {
    BlendOperation operation = BlendOperation::Add;
    BlendFactor srcFactor = BlendFactor::One;
    BlendFactor dstFactor = BlendFactor::Zero;
};

struct BlendState {
    BlendComponent color;
    BlendComponent alpha;
};

// Color write mask flags (can be combined with bitwise OR)
enum ColorWriteMask : uint32_t {
    None = 0x0,
    Red = 0x1,
    Green = 0x2,
    Blue = 0x4,
    Alpha = 0x8,
    All = Red | Green | Blue | Alpha // R | G | B | A
};

struct ColorTargetState {
    TextureFormat format = TextureFormat::Undefined;
    std::optional<BlendState> blend;
    uint32_t writeMask = ColorWriteMask::All;
};

struct VertexAttribute {
    TextureFormat format = TextureFormat::Undefined;
    uint64_t offset = 0;
    uint32_t shaderLocation = 0;
};

struct VertexBufferLayout {
    uint64_t arrayStride = 0;
    std::vector<VertexAttribute> attributes;
    bool stepModeInstance = false; // false = vertex, true = instance
};

struct VertexState {
    std::shared_ptr<Shader> module;
    std::string entryPoint = "main";
    std::vector<VertexBufferLayout> buffers;
};

struct FragmentState {
    std::shared_ptr<Shader> module;
    std::string entryPoint = "main";
    std::vector<ColorTargetState> targets;
};

struct PrimitiveState {
    PrimitiveTopology topology = PrimitiveTopology::TriangleList;
    IndexFormat stripIndexFormat = IndexFormat::Undefined;
    FrontFace frontFace = FrontFace::CounterClockwise;
    CullMode cullMode = CullMode::None;
    PolygonMode polygonMode = PolygonMode::Fill;
};

struct StencilFaceState {
    CompareFunction compare = CompareFunction::Always;
    StencilOperation failOp = StencilOperation::Keep;
    StencilOperation depthFailOp = StencilOperation::Keep;
    StencilOperation passOp = StencilOperation::Keep;
};

struct DepthStencilState {
    TextureFormat format = TextureFormat::Depth32Float;
    bool depthWriteEnabled = true;
    CompareFunction depthCompare = CompareFunction::Less;
    StencilFaceState stencilFront;
    StencilFaceState stencilBack;
    uint32_t stencilReadMask = 0xFF;
    uint32_t stencilWriteMask = 0xFF;
    int32_t depthBias = 0;
    float depthBiasSlopeScale = 0.0f;
    float depthBiasClamp = 0.0f;
};

struct RenderPipelineDescriptor {
    std::string label;
    std::shared_ptr<RenderPass> renderPass; // Render pass this pipeline will be used with
    VertexState vertex;
    std::optional<FragmentState> fragment;
    PrimitiveState primitive;
    std::optional<DepthStencilState> depthStencil;
    SampleCount sampleCount = SampleCount::Count1;
    std::vector<std::shared_ptr<BindGroupLayout>> bindGroupLayouts; // Bind group layouts used by the pipeline
};

struct ComputePipelineDescriptor {
    std::string label;
    std::shared_ptr<Shader> compute;
    std::string entryPoint = "main";
    std::vector<std::shared_ptr<BindGroupLayout>> bindGroupLayouts; // Bind group layouts used by the pipeline
};

struct BindGroupLayoutEntry {
    uint32_t binding = 0;
    ShaderStage visibility = ShaderStage::None;

    // Resource type (exactly one should be set)
    struct BufferBinding {
        bool hasDynamicOffset = false;
        uint64_t minBindingSize = 0;
    };

    struct SamplerBinding {
        bool comparison = false;
    };

    struct TextureBinding {
        bool multisampled = false;
        TextureViewType viewDimension = TextureViewType::View2D;
    };

    struct StorageTextureBinding {
        TextureFormat format = TextureFormat::Undefined;
        bool writeOnly = true;
        TextureViewType viewDimension = TextureViewType::View2D;
    };

    std::variant<BufferBinding, SamplerBinding, TextureBinding, StorageTextureBinding> resource;
};

struct BindGroupLayoutDescriptor {
    std::string label;
    std::vector<BindGroupLayoutEntry> entries;
};

struct BindGroupEntry {
    uint32_t binding = 0;

    // Resource (exactly one should be set)
    std::variant<
        std::shared_ptr<Buffer>,
        std::shared_ptr<Sampler>,
        std::shared_ptr<TextureView>>
        resource;

    // For buffer bindings
    uint64_t offset = 0;
    uint64_t size = 0; // 0 means whole buffer
};

struct BindGroupDescriptor {
    std::string label;
    std::shared_ptr<BindGroupLayout> layout;
    std::vector<BindGroupEntry> entries;
};

// Generic surface descriptor - completely windowing-system agnostic
struct SurfaceDescriptor {
    std::string label;
    PlatformWindowHandle windowHandle; // Generic platform handle
};

struct SwapchainDescriptor {
    std::string label;
    std::shared_ptr<Surface> surface;
    uint32_t width = 0;
    uint32_t height = 0;
    TextureFormat format = TextureFormat::B8G8R8A8Unorm;
    TextureUsage usage = TextureUsage::RenderAttachment;
    PresentMode presentMode = PresentMode::Fifo;
    uint32_t imageCount = 2; // Double buffering by default
};

struct SwapchainInfo {
    uint32_t width = 0;
    uint32_t height = 0;
    TextureFormat format = TextureFormat::Undefined;
    PresentMode presentMode = PresentMode::Fifo;
    uint32_t imageCount = 0;
};

struct FenceDescriptor {
    std::string label;
    bool signaled = false; // Initial state - true for signaled, false for unsignaled
};

struct SemaphoreDescriptor {
    std::string label;
    SemaphoreType type = SemaphoreType::Binary;
    uint64_t initialValue = 0; // For timeline semaphores, ignored for binary
};

struct QuerySetDescriptor {
    std::string label;
    QueryType type = QueryType::Occlusion;
    uint32_t count = 1; // Number of queries in the set
};

struct CommandEncoderDescriptor {
    std::string label;
};

struct DeviceLimits {
    uint64_t minUniformBufferOffsetAlignment = 0;
    uint64_t minStorageBufferOffsetAlignment = 0;
    uint32_t maxUniformBufferBindingSize = 0;
    uint32_t maxStorageBufferBindingSize = 0;
    uint64_t maxBufferSize = 0;
    uint32_t maxTextureDimension1D = 0;
    uint32_t maxTextureDimension2D = 0;
    uint32_t maxTextureDimension3D = 0;
    uint32_t maxTextureArrayLayers = 0;
};

struct AdapterInfo {
    std::string name; // Device name (e.g., "NVIDIA GeForce RTX 4090")
    std::string driverDescription; // Driver description (may be empty for WebGPU)
    uint32_t vendorID = 0; // PCI vendor ID (0x1002=AMD, 0x10DE=NVIDIA, 0x8086=Intel, 0=Unknown)
    uint32_t deviceID = 0; // PCI device ID (0=Unknown)
    AdapterType adapterType = AdapterType::Unknown; // Discrete, Integrated, CPU, or Unknown
    Backend backend = Backend::Auto; // Vulkan or WebGPU
};

struct SubmitDescriptor {
    std::vector<std::shared_ptr<CommandEncoder>> commandEncoders;

    // Wait semaphores (must be signaled before execution)
    std::vector<std::shared_ptr<Semaphore>> waitSemaphores;
    std::vector<uint64_t> waitValues; // For timeline semaphores, empty for binary

    // Signal semaphores (will be signaled after execution)
    std::vector<std::shared_ptr<Semaphore>> signalSemaphores;
    std::vector<uint64_t> signalValues; // For timeline semaphores, empty for binary

    // Optional fence to signal when all commands complete
    std::shared_ptr<Fence> signalFence;
};

struct PresentInfo {
    // Wait semaphores (must be signaled before presentation)
    std::vector<std::shared_ptr<Semaphore>> waitSemaphores;
    std::vector<uint64_t> waitValues; // For timeline semaphores, empty for binary
};

struct MemoryBarrier {
    PipelineStage srcStageMask = PipelineStage::None;
    PipelineStage dstStageMask = PipelineStage::None;
    AccessFlags srcAccessMask = AccessFlags::None;
    AccessFlags dstAccessMask = AccessFlags::None;
};

struct BufferBarrier {
    std::shared_ptr<Buffer> buffer;
    PipelineStage srcStageMask = PipelineStage::None;
    PipelineStage dstStageMask = PipelineStage::None;
    AccessFlags srcAccessMask = AccessFlags::None;
    AccessFlags dstAccessMask = AccessFlags::None;
    uint64_t offset = 0;
    uint64_t size = 0; // 0 means whole buffer
};

struct TextureBarrier {
    std::shared_ptr<Texture> texture;
    TextureLayout oldLayout = TextureLayout::Undefined;
    TextureLayout newLayout = TextureLayout::Undefined;
    PipelineStage srcStageMask = PipelineStage::None;
    PipelineStage dstStageMask = PipelineStage::None;
    AccessFlags srcAccessMask = AccessFlags::None;
    AccessFlags dstAccessMask = AccessFlags::None;
    uint32_t baseMipLevel = 0;
    uint32_t mipLevelCount = 1;
    uint32_t baseArrayLayer = 0;
    uint32_t arrayLayerCount = 1;
};

// Render Pass API structures (cached, reusable render pass objects)
struct RenderPassColorAttachmentTarget {
    TextureFormat format = TextureFormat::Undefined;
    SampleCount sampleCount = SampleCount::Count1;
    LoadOp loadOp = LoadOp::Clear;
    StoreOp storeOp = StoreOp::Store;
    TextureLayout finalLayout = TextureLayout::Undefined;
};

struct RenderPassColorAttachment {
    RenderPassColorAttachmentTarget target;
    std::optional<RenderPassColorAttachmentTarget> resolveTarget;
};

struct RenderPassDepthStencilAttachmentTarget {
    TextureFormat format = TextureFormat::Undefined;
    SampleCount sampleCount = SampleCount::Count1;
    LoadOp depthLoadOp = LoadOp::Clear;
    StoreOp depthStoreOp = StoreOp::Store;
    LoadOp stencilLoadOp = LoadOp::Clear;
    StoreOp stencilStoreOp = StoreOp::Store;
    TextureLayout finalLayout = TextureLayout::Undefined;
};

struct RenderPassDepthStencilAttachment {
    RenderPassDepthStencilAttachmentTarget target;
    std::optional<RenderPassDepthStencilAttachmentTarget> resolveTarget;
};

struct RenderPassCreateDescriptor {
    std::string label;
    std::vector<RenderPassColorAttachment> colorAttachments;
    std::optional<RenderPassDepthStencilAttachment> depthStencilAttachment;
};

// Framebuffer structures
struct FramebufferColorAttachment {
    std::shared_ptr<TextureView> view;
    std::shared_ptr<TextureView> resolveTarget; // nullptr if not used
};

struct FramebufferDepthStencilAttachment {
    std::shared_ptr<TextureView> view;
    std::shared_ptr<TextureView> resolveTarget; // nullptr if not used
};

struct FramebufferDescriptor {
    std::string label;
    std::shared_ptr<RenderPass> renderPass;
    std::vector<FramebufferColorAttachment> colorAttachments;
    std::optional<FramebufferDepthStencilAttachment> depthStencilAttachment;
    uint32_t width;
    uint32_t height;
};

// Render pass begin descriptor (runtime values)
struct RenderPassBeginDescriptor {
    std::shared_ptr<Framebuffer> framebuffer;
    std::vector<Color> colorClearValues;
    float depthClearValue = 1.0f;
    uint32_t stencilClearValue = 0;
};

struct ComputePassBeginDescriptor {
    std::string label;
};

// Copy/Blit descriptors
struct CopyBufferToBufferDescriptor {
    std::shared_ptr<Buffer> source;
    uint64_t sourceOffset = 0;
    std::shared_ptr<Buffer> destination;
    uint64_t destinationOffset = 0;
    uint64_t size = 0;
};

struct CopyBufferToTextureDescriptor {
    std::shared_ptr<Buffer> source;
    uint64_t sourceOffset = 0;
    std::shared_ptr<Texture> destination;
    Origin3D origin = {};
    Extent3D extent = {};
    uint32_t mipLevel = 0;
    TextureLayout finalLayout = TextureLayout::Undefined;
};

struct CopyTextureToBufferDescriptor {
    std::shared_ptr<Texture> source;
    Origin3D origin = {};
    uint32_t mipLevel = 0;
    std::shared_ptr<Buffer> destination;
    uint64_t destinationOffset = 0;
    Extent3D extent = {};
    TextureLayout finalLayout = TextureLayout::Undefined;
};

struct CopyTextureToTextureDescriptor {
    std::shared_ptr<Texture> source;
    Origin3D sourceOrigin = {};
    uint32_t sourceMipLevel = 0;
    TextureLayout sourceFinalLayout = TextureLayout::Undefined;
    std::shared_ptr<Texture> destination;
    Origin3D destinationOrigin = {};
    uint32_t destinationMipLevel = 0;
    TextureLayout destinationFinalLayout = TextureLayout::Undefined;
    Extent3D extent = {};
};

struct BlitTextureToTextureDescriptor {
    std::shared_ptr<Texture> source;
    Origin3D sourceOrigin = {};
    Extent3D sourceExtent = {};
    uint32_t sourceMipLevel = 0;
    TextureLayout sourceFinalLayout = TextureLayout::Undefined;
    std::shared_ptr<Texture> destination;
    Origin3D destinationOrigin = {};
    Extent3D destinationExtent = {};
    uint32_t destinationMipLevel = 0;
    TextureLayout destinationFinalLayout = TextureLayout::Undefined;
    FilterMode filter = FilterMode::Nearest;
};

struct PipelineBarrierDescriptor {
    std::vector<MemoryBarrier> memoryBarriers = {};
    std::vector<BufferBarrier> bufferBarriers = {};
    std::vector<TextureBarrier> textureBarriers = {};
};

// ============================================================================
// Surface and Swapchain Classes
// ============================================================================

class Surface {
public:
    virtual ~Surface() = default;

    // Get supported formats and present modes for this surface
    virtual std::vector<TextureFormat> getSupportedFormats() const = 0;
    virtual std::vector<PresentMode> getSupportedPresentModes() const = 0;
};

class Swapchain {
public:
    virtual ~Swapchain() = default;

    virtual SwapchainInfo getInfo() const = 0;

    // Get the current frame's texture view for rendering
    virtual std::shared_ptr<TextureView> getCurrentTextureView() = 0;

    // Present the current frame
    virtual Result acquireNextImage(uint64_t timeout, std::shared_ptr<Semaphore> signalSemaphore, std::shared_ptr<Fence> signalFence, uint32_t* imageIndex) = 0;

    // Get texture view for a specific swapchain image index
    virtual std::shared_ptr<TextureView> getTextureView(uint32_t index) = 0;

    // Present with explicit synchronization
    virtual Result present(const PresentInfo& info) = 0;
};

// ============================================================================
// Resource Classes
// ============================================================================

class Buffer {
public:
    virtual ~Buffer() = default;

    virtual BufferInfo getInfo() const = 0;
    virtual void* getNativeHandle() const = 0;

    // Mapping functions
    virtual void* map(uint64_t offset = 0, uint64_t size = 0) = 0;
    virtual void unmap() = 0;
    virtual void flushMappedRange(uint64_t offset, uint64_t size) = 0;
    virtual void invalidateMappedRange(uint64_t offset, uint64_t size) = 0;

    // Convenience functions
    template <typename T>
    T* map(uint64_t offset = 0)
    {
        return static_cast<T*>(map(offset, sizeof(T)));
    }

    template <typename T>
    void write(const std::vector<T>& data, uint64_t offset = 0)
    {
        void* ptr = map(offset, data.size() * sizeof(T));
        if (ptr) {
            std::memcpy(ptr, data.data(), data.size() * sizeof(T));
            unmap();
        }
    }
};

class Texture {
public:
    virtual ~Texture() = default;

    virtual TextureInfo getInfo() = 0;
    virtual void* getNativeHandle() const = 0;
    virtual TextureLayout getLayout() const = 0;

    virtual std::shared_ptr<TextureView> createView(const TextureViewDescriptor& descriptor = {}) = 0;
};

class TextureView {
public:
    virtual ~TextureView() = default;
};

class Sampler {
public:
    virtual ~Sampler() = default;
};

class Shader {
public:
    virtual ~Shader() = default;
};

class BindGroupLayout {
public:
    virtual ~BindGroupLayout() = default;
};

class BindGroup {
public:
    virtual ~BindGroup() = default;
};

class RenderPipeline {
public:
    virtual ~RenderPipeline() = default;
};

class ComputePipeline {
public:
    virtual ~ComputePipeline() = default;
};

class RenderPass {
public:
    virtual ~RenderPass() = default;
};

class Framebuffer {
public:
    virtual ~Framebuffer() = default;
};

class RenderPassEncoder {
public:
    virtual ~RenderPassEncoder() = default;

    virtual void setPipeline(std::shared_ptr<RenderPipeline> pipeline) = 0;
    virtual void setBindGroup(uint32_t index, std::shared_ptr<BindGroup> bindGroup, const uint32_t* dynamicOffsets = nullptr, uint32_t dynamicOffsetCount = 0) = 0;
    virtual void setVertexBuffer(uint32_t slot, std::shared_ptr<Buffer> buffer, uint64_t offset = 0, uint64_t size = 0) = 0;
    virtual void setIndexBuffer(std::shared_ptr<Buffer> buffer, IndexFormat format, uint64_t offset = 0, uint64_t size = UINT64_MAX) = 0;
    virtual void setViewport(float x, float y, float width, float height, float minDepth = 0.0f, float maxDepth = 1.0f) = 0;
    virtual void setScissorRect(int32_t x, int32_t y, uint32_t width, uint32_t height) = 0;

    virtual void draw(uint32_t vertexCount, uint32_t instanceCount = 1, uint32_t firstVertex = 0, uint32_t firstInstance = 0) = 0;
    virtual void drawIndexed(uint32_t indexCount, uint32_t instanceCount = 1, uint32_t firstIndex = 0, int32_t baseVertex = 0, uint32_t firstInstance = 0) = 0;
    virtual void drawIndirect(std::shared_ptr<Buffer> indirectBuffer, uint64_t indirectOffset) = 0;
    virtual void drawIndexedIndirect(std::shared_ptr<Buffer> indirectBuffer, uint64_t indirectOffset) = 0;

    virtual void beginOcclusionQuery(std::shared_ptr<QuerySet> querySet, uint32_t queryIndex) = 0;
    virtual void endOcclusionQuery() = 0;
};

class ComputePassEncoder {
public:
    virtual ~ComputePassEncoder() = default;

    virtual void setPipeline(std::shared_ptr<ComputePipeline> pipeline) = 0;
    virtual void setBindGroup(uint32_t index, std::shared_ptr<BindGroup> bindGroup, const uint32_t* dynamicOffsets = nullptr, uint32_t dynamicOffsetCount = 0) = 0;
    virtual void dispatch(uint32_t workgroupCountX, uint32_t workgroupCountY = 1, uint32_t workgroupCountZ = 1) = 0;
    virtual void dispatchIndirect(std::shared_ptr<Buffer> indirectBuffer, uint64_t indirectOffset) = 0;
};

class CommandEncoder {
public:
    virtual ~CommandEncoder() = default;

    virtual std::shared_ptr<RenderPassEncoder> beginRenderPass(const RenderPassBeginDescriptor& descriptor) = 0;

    virtual std::shared_ptr<ComputePassEncoder> beginComputePass(const ComputePassBeginDescriptor& descriptor) = 0;

    virtual void copyBufferToBuffer(const CopyBufferToBufferDescriptor& descriptor) = 0;
    virtual void copyBufferToTexture(const CopyBufferToTextureDescriptor& descriptor) = 0;
    virtual void copyTextureToBuffer(const CopyTextureToBufferDescriptor& descriptor) = 0;
    virtual void copyTextureToTexture(const CopyTextureToTextureDescriptor& descriptor) = 0;
    virtual void blitTextureToTexture(const BlitTextureToTextureDescriptor& descriptor) = 0;

    virtual void pipelineBarrier(const PipelineBarrierDescriptor& descriptor) = 0;

    virtual void generateMipmaps(std::shared_ptr<Texture> texture) = 0;
    virtual void generateMipmapsRange(std::shared_ptr<Texture> texture, uint32_t baseMipLevel, uint32_t levelCount) = 0;

    virtual void writeTimestamp(std::shared_ptr<QuerySet> querySet, uint32_t queryIndex) = 0;
    virtual void resolveQuerySet(std::shared_ptr<QuerySet> querySet, uint32_t firstQuery, uint32_t queryCount, std::shared_ptr<Buffer> destinationBuffer, uint64_t destinationOffset) = 0;

    virtual void end() = 0;
    virtual void begin() = 0;
};

// ============================================================================
// Synchronization Classes
// ============================================================================

class Fence {
public:
    virtual ~Fence() = default;

    virtual FenceStatus getStatus() const = 0;
    virtual bool wait(uint64_t timeoutNanoseconds = UINT64_MAX) = 0; // Returns true if signaled, false if timeout
    virtual void reset() = 0;

    // Static utility for waiting on multiple fences
    static bool waitMultiple(const std::vector<std::shared_ptr<Fence>>& fences, bool waitAll, uint64_t timeoutNanoseconds = UINT64_MAX);
};

class Semaphore {
public:
    virtual ~Semaphore() = default;

    virtual SemaphoreType getType() const = 0;

    // For timeline semaphores
    virtual uint64_t getValue() const = 0;
    virtual void signal(uint64_t value) = 0;
    virtual bool wait(uint64_t value, uint64_t timeoutNanoseconds = UINT64_MAX) = 0;
};

class QuerySet {
public:
    virtual ~QuerySet() = default;

    virtual QueryType getType() const = 0;
    virtual uint32_t getCount() const = 0;
};

class Queue {
public:
    virtual ~Queue() = default;

    virtual void submit(const SubmitDescriptor& submitInfo) = 0;
    virtual void writeBuffer(std::shared_ptr<Buffer> buffer, uint64_t offset, const void* data, uint64_t size) = 0;
    virtual void writeTexture(std::shared_ptr<Texture> texture, const Origin3D& origin, uint32_t mipLevel, const void* data, uint64_t dataSize, const Extent3D& extent, TextureLayout finalLayout) = 0;
    virtual void waitIdle() = 0;

    template <typename T>
    void writeBuffer(std::shared_ptr<Buffer> buffer, uint64_t offset, const std::vector<T>& data)
    {
        writeBuffer(buffer, offset, data.data(), data.size() * sizeof(T));
    }
};

class Device {
public:
    virtual ~Device() = default;

    virtual std::shared_ptr<Queue> getQueue() = 0;
    virtual std::shared_ptr<Queue> getQueueByIndex(uint32_t queueFamilyIndex, uint32_t queueIndex) = 0;

    // Generic surface creation - works with any windowing system
    virtual std::shared_ptr<Surface> createSurface(const SurfaceDescriptor& descriptor) = 0;
    virtual std::shared_ptr<Swapchain> createSwapchain(const SwapchainDescriptor& descriptor) = 0;

    virtual std::shared_ptr<Buffer> createBuffer(const BufferDescriptor& descriptor) = 0;
    virtual std::shared_ptr<Buffer> importBuffer(const BufferImportDescriptor& descriptor) = 0;
    virtual std::shared_ptr<Texture> createTexture(const TextureDescriptor& descriptor) = 0;
    virtual std::shared_ptr<Texture> importTexture(const TextureImportDescriptor& descriptor) = 0;
    virtual std::shared_ptr<Sampler> createSampler(const SamplerDescriptor& descriptor = {}) = 0;
    virtual std::shared_ptr<Shader> createShader(const ShaderDescriptor& descriptor) = 0;

    virtual std::shared_ptr<BindGroupLayout> createBindGroupLayout(const BindGroupLayoutDescriptor& descriptor) = 0;
    virtual std::shared_ptr<BindGroup> createBindGroup(const BindGroupDescriptor& descriptor) = 0;

    virtual std::shared_ptr<RenderPipeline> createRenderPipeline(const RenderPipelineDescriptor& descriptor) = 0;
    virtual std::shared_ptr<ComputePipeline> createComputePipeline(const ComputePipelineDescriptor& descriptor) = 0;

    virtual std::shared_ptr<RenderPass> createRenderPass(const RenderPassCreateDescriptor& descriptor) = 0;
    virtual std::shared_ptr<Framebuffer> createFramebuffer(const FramebufferDescriptor& descriptor) = 0;

    virtual std::shared_ptr<CommandEncoder> createCommandEncoder(const CommandEncoderDescriptor& descriptor = {}) = 0;

    // Synchronization objects
    virtual std::shared_ptr<Fence> createFence(const FenceDescriptor& descriptor = {}) = 0;
    virtual std::shared_ptr<Semaphore> createSemaphore(const SemaphoreDescriptor& descriptor = {}) = 0;
    virtual std::shared_ptr<QuerySet> createQuerySet(const QuerySetDescriptor& descriptor) = 0;

    virtual void waitIdle() = 0;
    virtual DeviceLimits getLimits() const = 0;
    virtual bool supportsShaderFormat(ShaderSourceType format) const = 0;
};

class Adapter {
public:
    virtual ~Adapter() = default;

    virtual std::shared_ptr<Device> createDevice(const DeviceDescriptor& descriptor = {}) = 0;
    virtual AdapterInfo getInfo() const = 0;
    virtual DeviceLimits getLimits() const = 0;

    // Queue family enumeration
    virtual std::vector<QueueFamilyProperties> enumerateQueueFamilies() const = 0;
    virtual bool getQueueFamilySurfaceSupport(uint32_t queueFamilyIndex, Surface* surface) const = 0;

    // Device extension enumeration
    virtual std::vector<std::string> enumerateExtensions() const = 0;
};

class Instance {
public:
    virtual ~Instance() = default;

    virtual std::shared_ptr<Adapter> requestAdapter(const AdapterDescriptor& descriptor = {}) = 0;
    virtual std::vector<std::shared_ptr<Adapter>> enumerateAdapters() = 0;
};

// ============================================================================
// Factory Functions
// ============================================================================

std::shared_ptr<Instance> createInstance(const InstanceDescriptor& descriptor = {});

// ============================================================================
// Utility Functions
// ============================================================================

// Enumerate available instance extensions for a backend
std::vector<std::string> enumerateInstanceExtensions(Backend backend);

// Set global log callback for all logging output
void setLogCallback(LogCallback callback);

// Get runtime library version - returns major, minor, patch
std::tuple<uint32_t, uint32_t, uint32_t> getVersion();

namespace utils {
    // Alignment helpers - align buffer offsets/sizes to device requirements
    uint64_t alignUp(uint64_t value, uint64_t alignment);
    uint64_t alignDown(uint64_t value, uint64_t alignment);

    // Helper to deduce access flags from texture layout
    AccessFlags getAccessFlagsForLayout(TextureLayout layout);

    // Get bytes per pixel for a texture format
    uint32_t getFormatBytesPerPixel(TextureFormat format);
} // namespace utils

} // namespace gfx

#endif // GFX_CPP_GFX_HPP