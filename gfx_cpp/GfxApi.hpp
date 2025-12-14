#pragma once

#include <cstdint>
#include <cstring>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <variant>
#include <vector>

// Forward declarations to avoid including heavy headers
struct WGPUInstanceImpl;
struct VkInstance_T;
using WGPUInstance = WGPUInstanceImpl*;
using VkInstance = VkInstance_T*;

namespace gfx {

// ============================================================================
// Core Enumerations
// ============================================================================

enum class Backend {
    Vulkan,
    WebGPU,
    Auto
};

enum class PowerPreference {
    Undefined,
    LowPower,
    HighPerformance
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
    uint32_t x = 0;
    uint32_t y = 0;
    uint32_t z = 0;

    Origin3D() = default;
    Origin3D(uint32_t px, uint32_t py = 0, uint32_t pz = 0)
        : x(px)
        , y(py)
        , z(pz)
    {
    }
};

// ============================================================================
// Platform Abstraction
// ============================================================================

// Platform-specific window handles with proper typing
#ifdef _WIN32
struct PlatformWindowHandle {
    void* hwnd = nullptr; // HWND - Window handle
    void* hinstance = nullptr; // HINSTANCE - Application instance

    PlatformWindowHandle() = default;
    PlatformWindowHandle(void* window, void* instance = nullptr)
        : hwnd(window)
        , hinstance(instance)
    {
    }
};

#elif defined(__linux__)
struct PlatformWindowHandle {
    void* window = nullptr; // Window (X11) or wl_surface* (Wayland)
    void* display = nullptr; // Display* (X11) or wl_display* (Wayland)
    bool isWayland = false; // true for Wayland, false for X11

    PlatformWindowHandle() = default;
    PlatformWindowHandle(void* win, void* disp, bool wayland = false)
        : window(win)
        , display(disp)
        , isWayland(wayland)
    {
    }
};

#elif defined(__APPLE__)
struct PlatformWindowHandle {
    void* nsWindow = nullptr; // NSWindow*
    void* metalLayer = nullptr; // CAMetalLayer* (optional)

    PlatformWindowHandle() = default;
    explicit PlatformWindowHandle(void* window, void* layer = nullptr)
        : nsWindow(window)
        , metalLayer(layer)
    {
    }
};

#else
// Generic fallback for other platforms
struct PlatformWindowHandle {
    void* handle = nullptr;
    void* display = nullptr;
    void* extra = nullptr;

    PlatformWindowHandle() = default;
    PlatformWindowHandle(void* h, void* d = nullptr, void* e = nullptr)
        : handle(h)
        , display(d)
        , extra(e)
    {
    }
};
#endif

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
class Fence;
class Semaphore;

// ============================================================================
// Descriptor Structures
// ============================================================================

struct InstanceDescriptor {
    Backend backend = Backend::Auto;
    bool enableValidation = false;
    std::string applicationName = "GfxWrapper Application";
    uint32_t applicationVersion = 1;

    // Optional: Required extensions (backend-specific)
    std::vector<std::string> requiredExtensions;
};

struct AdapterDescriptor {
    PowerPreference powerPreference = PowerPreference::Undefined;
    bool forceFallbackAdapter = false;
};

struct DeviceDescriptor {
    std::string label;
    std::vector<std::string> requiredFeatures;
};

struct BufferDescriptor {
    std::string label;
    uint64_t size = 0;
    BufferUsage usage = BufferUsage::None;
    bool mappedAtCreation = false;
};

struct TextureDescriptor {
    std::string label;
    Extent3D size;
    uint32_t mipLevelCount = 1;
    uint32_t sampleCount = 1;
    TextureFormat format = TextureFormat::Undefined;
    TextureUsage usage = TextureUsage::None;
};

struct TextureViewDescriptor {
    std::string label;
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
    std::optional<CompareFunction> compare;
    uint16_t maxAnisotropy = 1;
};

struct ShaderDescriptor {
    std::string label;
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

struct ColorTargetState {
    TextureFormat format = TextureFormat::Undefined;
    std::optional<BlendState> blend;
    uint32_t writeMask = 0xF; // All channels
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
    bool frontFaceCounterClockwise = true;
    bool cullBackFace = false;
    bool unclippedDepth = false;
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
    VertexState vertex;
    std::optional<FragmentState> fragment;
    PrimitiveState primitive;
    std::optional<DepthStencilState> depthStencil;
    uint32_t sampleCount = 1;
    std::vector<std::shared_ptr<BindGroupLayout>> bindGroupLayouts; // Bind group layouts used by the pipeline
};

struct ComputePipelineDescriptor {
    std::string label;
    std::shared_ptr<Shader> compute;
    std::string entryPoint = "main";
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
    };

    struct StorageTextureBinding {
        TextureFormat format = TextureFormat::Undefined;
        bool writeOnly = true;
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
    uint32_t bufferCount = 2; // Double buffering by default
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

struct SubmitInfo {
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

// ============================================================================
// Surface and Swapchain Classes
// ============================================================================

class Surface {
public:
    virtual ~Surface() = default;

    virtual uint32_t getWidth() const = 0;
    virtual uint32_t getHeight() const = 0;
    virtual void resize(uint32_t width, uint32_t height) = 0;

    // Get supported formats and present modes for this surface
    virtual std::vector<TextureFormat> getSupportedFormats() const = 0;
    virtual std::vector<PresentMode> getSupportedPresentModes() const = 0;

    // Get the underlying platform handle
    virtual PlatformWindowHandle getPlatformHandle() const = 0;
};

class Swapchain {
public:
    virtual ~Swapchain() = default;

    virtual uint32_t getWidth() const = 0;
    virtual uint32_t getHeight() const = 0;
    virtual TextureFormat getFormat() const = 0;
    virtual uint32_t getBufferCount() const = 0;

    // Get the current frame's texture view for rendering
    virtual std::shared_ptr<TextureView> getCurrentTextureView() = 0;

    // Present the current frame
    virtual void present() = 0;

    // Resize the swapchain (recreates internal resources)
    virtual void resize(uint32_t width, uint32_t height) = 0;

    // Check if swapchain needs to be recreated (e.g., window resized, lost device)
    virtual bool needsRecreation() const = 0;
};

// ============================================================================
// Resource Classes
// ============================================================================

class Buffer {
public:
    virtual ~Buffer() = default;

    virtual uint64_t getSize() const = 0;
    virtual BufferUsage getUsage() const = 0;

    // Mapping functions
    virtual void* mapAsync(uint64_t offset = 0, uint64_t size = 0) = 0;
    virtual void unmap() = 0;

    // Convenience functions
    template <typename T>
    T* map(uint64_t offset = 0)
    {
        return static_cast<T*>(mapAsync(offset, sizeof(T)));
    }

    template <typename T>
    void write(const std::vector<T>& data, uint64_t offset = 0)
    {
        void* ptr = mapAsync(offset, data.size() * sizeof(T));
        if (ptr) {
            std::memcpy(ptr, data.data(), data.size() * sizeof(T));
            unmap();
        }
    }
};

class Texture {
public:
    virtual ~Texture() = default;

    virtual Extent3D getSize() const = 0;
    virtual TextureFormat getFormat() const = 0;
    virtual uint32_t getMipLevelCount() const = 0;
    virtual uint32_t getSampleCount() const = 0;
    virtual TextureUsage getUsage() const = 0;

    virtual std::shared_ptr<TextureView> createView(const TextureViewDescriptor& descriptor = {}) = 0;
};

class TextureView {
public:
    virtual ~TextureView() = default;
    virtual std::shared_ptr<Texture> getTexture() = 0;
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

class RenderPassEncoder {
public:
    virtual ~RenderPassEncoder() = default;

    virtual void setPipeline(std::shared_ptr<RenderPipeline> pipeline) = 0;
    virtual void setBindGroup(uint32_t index, std::shared_ptr<BindGroup> bindGroup) = 0;
    virtual void setVertexBuffer(uint32_t slot, std::shared_ptr<Buffer> buffer, uint64_t offset = 0, uint64_t size = 0) = 0;
    virtual void setIndexBuffer(std::shared_ptr<Buffer> buffer, IndexFormat format, uint64_t offset = 0, uint64_t size = 0) = 0;

    virtual void draw(uint32_t vertexCount, uint32_t instanceCount = 1, uint32_t firstVertex = 0, uint32_t firstInstance = 0) = 0;
    virtual void drawIndexed(uint32_t indexCount, uint32_t instanceCount = 1, uint32_t firstIndex = 0, int32_t baseVertex = 0, uint32_t firstInstance = 0) = 0;

    virtual void end() = 0;
};

class ComputePassEncoder {
public:
    virtual ~ComputePassEncoder() = default;

    virtual void setPipeline(std::shared_ptr<ComputePipeline> pipeline) = 0;
    virtual void setBindGroup(uint32_t index, std::shared_ptr<BindGroup> bindGroup) = 0;
    virtual void dispatchWorkgroups(uint32_t workgroupCountX, uint32_t workgroupCountY = 1, uint32_t workgroupCountZ = 1) = 0;

    virtual void end() = 0;
};

class CommandEncoder {
public:
    virtual ~CommandEncoder() = default;

    virtual std::shared_ptr<RenderPassEncoder> beginRenderPass(
        const std::vector<std::shared_ptr<TextureView>>& colorAttachments,
        const std::vector<Color>& clearColors = {},
        std::shared_ptr<TextureView> depthStencilAttachment = nullptr,
        float depthClearValue = 1.0f,
        uint32_t stencilClearValue = 0)
        = 0;

    virtual std::shared_ptr<ComputePassEncoder> beginComputePass(const std::string& label = "") = 0;

    virtual void copyBufferToBuffer(
        std::shared_ptr<Buffer> source, uint64_t sourceOffset,
        std::shared_ptr<Buffer> destination, uint64_t destinationOffset,
        uint64_t size)
        = 0;

    virtual void copyBufferToTexture(
        std::shared_ptr<Buffer> source, uint64_t sourceOffset, uint32_t bytesPerRow,
        std::shared_ptr<Texture> destination, const Origin3D& origin,
        const Extent3D& extent, uint32_t mipLevel = 0)
        = 0;

    virtual void copyTextureToBuffer(
        std::shared_ptr<Texture> source, const Origin3D& origin, uint32_t mipLevel,
        std::shared_ptr<Buffer> destination, uint64_t destinationOffset, uint32_t bytesPerRow,
        const Extent3D& extent)
        = 0;

    virtual void finish() = 0;
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

    virtual void submit(std::shared_ptr<CommandEncoder> commandEncoder) = 0;
    virtual void submitWithSync(const SubmitInfo& submitInfo) = 0;
    virtual void writeBuffer(std::shared_ptr<Buffer> buffer, uint64_t offset, const void* data, uint64_t size) = 0;
    virtual void writeTexture(
        std::shared_ptr<Texture> texture, const Origin3D& origin, uint32_t mipLevel,
        const void* data, uint64_t dataSize, uint32_t bytesPerRow,
        const Extent3D& extent)
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
    virtual std::shared_ptr<Texture> createTexture(const TextureDescriptor& descriptor) = 0;
    virtual std::shared_ptr<Sampler> createSampler(const SamplerDescriptor& descriptor = {}) = 0;
    virtual std::shared_ptr<Shader> createShader(const ShaderDescriptor& descriptor) = 0;

    virtual std::shared_ptr<BindGroupLayout> createBindGroupLayout(const BindGroupLayoutDescriptor& descriptor) = 0;
    virtual std::shared_ptr<BindGroup> createBindGroup(const BindGroupDescriptor& descriptor) = 0;

    virtual std::shared_ptr<RenderPipeline> createRenderPipeline(const RenderPipelineDescriptor& descriptor) = 0;
    virtual std::shared_ptr<ComputePipeline> createComputePipeline(const ComputePipelineDescriptor& descriptor) = 0;

    virtual std::shared_ptr<CommandEncoder> createCommandEncoder(const std::string& label = "") = 0;

    // Synchronization objects
    virtual std::shared_ptr<Fence> createFence(const FenceDescriptor& descriptor = {}) = 0;
    virtual std::shared_ptr<Semaphore> createSemaphore(const SemaphoreDescriptor& descriptor = {}) = 0;

    virtual void waitIdle() = 0;
};

class Adapter {
public:
    virtual ~Adapter() = default;

    virtual std::shared_ptr<Device> createDevice(const DeviceDescriptor& descriptor = {}) = 0;
    virtual std::string getName() const = 0;
    virtual Backend getBackend() const = 0;
};

class Instance {
public:
    virtual ~Instance() = default;

    virtual std::shared_ptr<Adapter> requestAdapter(const AdapterDescriptor& descriptor = {}) = 0;
    virtual std::vector<std::shared_ptr<Adapter>> enumerateAdapters() = 0;
};

// ============================================================================
// Utility Functions (Optional Helper)
// ============================================================================

namespace utils {
    // Helper to get required extensions for different windowing systems
    // These are optional utilities - not part of the core API

#ifdef GLFW_VERSION
    // If GLFW is available, provide a helper (optional)
    std::vector<std::string> getRequiredExtensions(void* glfwWindow, Backend backend);
    PlatformWindowHandle createPlatformHandle(void* glfwWindow);
#endif

#ifdef _WIN32
    // Win32 helpers (optional)
    PlatformWindowHandle createWin32Handle(void* hwnd, void* hinstance = nullptr);
#endif

#ifdef __linux__
    // X11 helpers (optional)
    PlatformWindowHandle createX11Handle(void* window, void* display);
#endif

    // Generic helper for raw platform handles
    PlatformWindowHandle createGenericHandle(void* handle);
} // namespace utils

// ============================================================================
// Factory Functions
// ============================================================================

std::shared_ptr<Instance> createInstance(const InstanceDescriptor& descriptor = {});

} // namespace gfx