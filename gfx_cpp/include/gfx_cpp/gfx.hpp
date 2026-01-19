#ifndef GFX_CPP_GFX_HPP
#define GFX_CPP_GFX_HPP

#include <cstdint>
#include <cstring>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace gfx {

// ============================================================================
// Core Enumerations
// ============================================================================

enum class Backend {
    Vulkan,
    WebGPU,
    Auto
};

enum class AdapterType {
    DiscreteGPU,
    IntegratedGPU,
    CPU,
    Unknown
};

enum class AdapterPreference {
    Undefined,
    LowPower,
    HighPerformance,
    Software
};

enum class PresentMode {
    Immediate, // No vsync, immediate presentation
    Fifo, // Vsync, first-in-first-out queue
    FifoRelaxed, // Vsync with relaxed timing
    Mailbox // Triple buffering
};

enum class PrimitiveTopology {
    PointList,
    LineList,
    LineStrip,
    TriangleList,
    TriangleStrip
};

enum class FrontFace {
    CounterClockwise,
    Clockwise
};

enum class CullMode {
    None,
    Front,
    Back,
    FrontAndBack
};

enum class PolygonMode {
    Fill,
    Line,
    Point
};

enum class IndexFormat {
    Uint16,
    Uint32
};

enum class TextureFormat {
    Undefined,
    R8Unorm,
    R8G8Unorm,
    R8G8B8A8Unorm,
    R8G8B8A8UnormSrgb,
    B8G8R8A8Unorm,
    B8G8R8A8UnormSrgb,
    R16Float,
    R16G16Float,
    R16G16B16A16Float,
    R32Float,
    R32G32Float,
    R32G32B32Float,
    R32G32B32A32Float,
    Depth16Unorm,
    Depth24Plus,
    Depth32Float,
    Depth24PlusStencil8,
    Depth32FloatStencil8
};

enum class TextureType {
    Texture1D,
    Texture2D,
    Texture3D,
    TextureCube
};

enum class TextureViewType {
    View1D,
    View2D,
    View3D,
    ViewCube,
    View1DArray,
    View2DArray,
    ViewCubeArray
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

enum class ShaderStage : uint32_t {
    None = 0,
    Vertex = 1 << 0,
    Fragment = 1 << 1,
    Compute = 1 << 2
};

enum class FilterMode {
    Nearest,
    Linear
};

enum class AddressMode {
    Repeat,
    MirrorRepeat,
    ClampToEdge
};

enum class CompareFunction {
    Undefined,
    Never,
    Less,
    Equal,
    LessEqual,
    Greater,
    NotEqual,
    GreaterEqual,
    Always
};

enum class BlendOperation {
    Add,
    Subtract,
    ReverseSubtract,
    Min,
    Max
};

enum class BlendFactor {
    Zero,
    One,
    Src,
    OneMinusSrc,
    SrcAlpha,
    OneMinusSrcAlpha,
    Dst,
    OneMinusDst,
    DstAlpha,
    OneMinusDstAlpha,
    SrcAlphaSaturated,
    Constant,
    OneMinusConstant
};

enum class StencilOperation {
    Keep,
    Zero,
    Replace,
    IncrementClamp,
    DecrementClamp,
    Invert,
    IncrementWrap,
    DecrementWrap
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

enum class ShaderSourceType {
    WGSL, // WGSL text source (for WebGPU)
    SPIRV // SPIR-V binary (for Vulkan)
};

// Synchronization enums
enum class FenceStatus {
    Unsignaled,
    Signaled,
    Error
};

enum class SemaphoreType {
    Binary,
    Timeline
};

enum class InstanceFeatureType {
    Surface
};

enum class DeviceFeatureType {
    Swapchain
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

enum class LoadOp {
    Load, // Load existing contents
    Clear, // Clear to specified clear value
    DontCare // Don't care about initial contents (better performance on tiled GPUs)
};

enum class StoreOp {
    Store, // Store contents after render pass
    DontCare // Don't care about contents after render pass (better performance for transient attachments)
};

enum class TextureLayout {
    Undefined,
    General,
    ColorAttachment,
    DepthStencilAttachment,
    DepthStencilReadOnly,
    ShaderReadOnly,
    TransferSrc,
    TransferDst,
    PresentSrc
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
    Android = 6,
    Unknown = -1
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

// ============================================================================
// Logging
// ============================================================================

enum class LogLevel {
    Error,
    Warning,
    Info,
    Debug
};

using LogCallback = std::function<void(LogLevel level, const std::string& message)>;

// ============================================================================
// Descriptor Structures
// ============================================================================

struct InstanceDescriptor {
    Backend backend = Backend::Auto;
    bool enableValidation = false;
    std::string applicationName = "GfxWrapper Application";
    uint32_t applicationVersion = 1;
    std::vector<InstanceFeatureType> enabledFeatures;
};

struct AdapterDescriptor {
    uint32_t adapterIndex = UINT32_MAX; // Adapter index from enumeration (use UINT32_MAX to ignore)
    AdapterPreference preference = AdapterPreference::Undefined; // Used only when adapterIndex is UINT32_MAX
};

struct DeviceDescriptor {
    std::string label;
    float queuePriority = 1.0f; // Queue priority (0.0 to 1.0, higher = more priority)
    std::vector<DeviceFeatureType> enabledFeatures;
};

struct BufferDescriptor {
    std::string label;
    uint64_t size = 0;
    BufferUsage usage = BufferUsage::None;
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
    std::string code;
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
    All = 0xF // R | G | B | A
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
    std::optional<IndexFormat> stripIndexFormat;
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
    uint32_t width = 0;
    uint32_t height = 0;
};

struct SwapchainDescriptor {
    std::string label;
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

struct ColorAttachmentOps {
    LoadOp loadOp = LoadOp::Clear;
    StoreOp storeOp = StoreOp::Store;
    Color clearColor = { 0.0f, 0.0f, 0.0f, 0.0f };
};

struct ColorAttachmentTarget {
    std::shared_ptr<TextureView> view;
    ColorAttachmentOps ops;
    TextureLayout finalLayout = TextureLayout::Undefined;
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
    RenderPassColorAttachmentTarget* resolveTarget = nullptr;
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
    RenderPassDepthStencilAttachmentTarget* resolveTarget = nullptr;
};

struct RenderPassCreateDescriptor {
    std::string label;
    std::vector<RenderPassColorAttachment> colorAttachments;
    RenderPassDepthStencilAttachment* depthStencilAttachment = nullptr;
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
    FramebufferDepthStencilAttachment* depthStencilAttachment = nullptr;
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

// Legacy inline render pass structures (for backwards compatibility)
struct ColorAttachment {
    ColorAttachmentTarget target;
    ColorAttachmentTarget* resolveTarget = nullptr;
};

struct DepthAttachmentOps {
    LoadOp loadOp = LoadOp::Clear;
    StoreOp storeOp = StoreOp::Store;
    float clearValue = 1.0f;
};

struct StencilAttachmentOps {
    LoadOp loadOp = LoadOp::Clear;
    StoreOp storeOp = StoreOp::Store;
    uint32_t clearValue = 0;
};

struct DepthStencilAttachmentTarget {
    std::shared_ptr<TextureView> view;
    DepthAttachmentOps* depthOps = nullptr; // Optional: set to nullptr if not used
    StencilAttachmentOps* stencilOps = nullptr; // Optional: set to nullptr if not used
    TextureLayout finalLayout = TextureLayout::Undefined;
};

struct DepthStencilAttachment {
    DepthStencilAttachmentTarget target;
    DepthStencilAttachmentTarget* resolveTarget = nullptr;
};

struct RenderPassDescriptor {
    std::string label;
    std::vector<ColorAttachment> colorAttachments;
    DepthStencilAttachment* depthStencilAttachment = nullptr; // nullptr if not used
};

struct ComputePassBeginDescriptor {
    std::string label;
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
    virtual Result acquireNextImage(uint64_t timeout,
        std::shared_ptr<Semaphore> signalSemaphore,
        std::shared_ptr<Fence> signalFence,
        uint32_t* imageIndex)
        = 0;

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

    // Mapping functions
    virtual void* map(uint64_t offset = 0, uint64_t size = 0) = 0;
    virtual void unmap() = 0;

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

    virtual void copyBufferToBuffer(
        std::shared_ptr<Buffer> source, uint64_t sourceOffset,
        std::shared_ptr<Buffer> destination, uint64_t destinationOffset,
        uint64_t size)
        = 0;

    virtual void copyBufferToTexture(
        std::shared_ptr<Buffer> source, uint64_t sourceOffset, uint32_t bytesPerRow,
        std::shared_ptr<Texture> destination, const Origin3D& origin,
        const Extent3D& extent, uint32_t mipLevel, TextureLayout finalLayout)
        = 0;

    virtual void copyTextureToBuffer(
        std::shared_ptr<Texture> source, const Origin3D& origin, uint32_t mipLevel,
        std::shared_ptr<Buffer> destination, uint64_t destinationOffset, uint32_t bytesPerRow,
        const Extent3D& extent, TextureLayout finalLayout)
        = 0;

    virtual void copyTextureToTexture(
        std::shared_ptr<Texture> source, const Origin3D& sourceOrigin, uint32_t sourceMipLevel, TextureLayout sourceFinalLayout,
        std::shared_ptr<Texture> destination, const Origin3D& destinationOrigin, uint32_t destinationMipLevel, TextureLayout destinationFinalLayout,
        const Extent3D& extent)
        = 0;

    virtual void blitTextureToTexture(
        std::shared_ptr<Texture> source, const Origin3D& sourceOrigin, const Extent3D& sourceExtent, uint32_t sourceMipLevel, TextureLayout sourceFinalLayout,
        std::shared_ptr<Texture> destination, const Origin3D& destinationOrigin, const Extent3D& destinationExtent, uint32_t destinationMipLevel, TextureLayout destinationFinalLayout,
        FilterMode filter)
        = 0;

    virtual void pipelineBarrier(
        const std::vector<MemoryBarrier>& memoryBarriers = {},
        const std::vector<BufferBarrier>& bufferBarriers = {},
        const std::vector<TextureBarrier>& textureBarriers = {})
        = 0;

    virtual void generateMipmaps(std::shared_ptr<Texture> texture) = 0;
    virtual void generateMipmapsRange(std::shared_ptr<Texture> texture, uint32_t baseMipLevel, uint32_t levelCount) = 0;

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

class Queue {
public:
    virtual ~Queue() = default;

    virtual void submit(const SubmitDescriptor& submitInfo) = 0;
    virtual void writeBuffer(std::shared_ptr<Buffer> buffer, uint64_t offset, const void* data, uint64_t size) = 0;
    virtual void writeTexture(
        std::shared_ptr<Texture> texture, const Origin3D& origin, uint32_t mipLevel,
        const void* data, uint64_t dataSize, uint32_t bytesPerRow,
        const Extent3D& extent, TextureLayout finalLayout)
        = 0;
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

    // Generic surface creation - works with any windowing system
    virtual std::shared_ptr<Surface> createSurface(const SurfaceDescriptor& descriptor) = 0;
    virtual std::shared_ptr<Swapchain> createSwapchain(
        std::shared_ptr<Surface> surface,
        const SwapchainDescriptor& descriptor)
        = 0;

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

    virtual void waitIdle() = 0;
    virtual DeviceLimits getLimits() const = 0;
};

class Adapter {
public:
    virtual ~Adapter() = default;

    virtual std::shared_ptr<Device> createDevice(const DeviceDescriptor& descriptor = {}) = 0;
    virtual AdapterInfo getInfo() const = 0;
    virtual DeviceLimits getLimits() const = 0;
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

// Set global log callback for all logging output
void setLogCallback(LogCallback callback);

namespace utils {
    // Alignment helpers - align buffer offsets/sizes to device requirements
    uint64_t alignUp(uint64_t value, uint64_t alignment);
    uint64_t alignDown(uint64_t value, uint64_t alignment);

    // Helper to deduce access flags from texture layout
    AccessFlags getAccessFlagsForLayout(TextureLayout layout);
} // namespace utils

} // namespace gfx

#endif // GFX_CPP_GFX_HPP