#pragma once

#include "../common/WebGPUCommon.h"
#include "CreateInfo.h"

#include <memory>
#include <stdexcept>
#include <string>

// ============================================================================
// Internal C++ Classes with RAII
// ============================================================================

namespace gfx::webgpu {

// Forward declarations
class Instance;
class Adapter;
class Device;
class Queue;
class Surface;
class Swapchain;
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
class BindGroupLayout;
class BindGroup;
class Fence;
class Semaphore;

class Instance {
public:
    // Prevent copying
    Instance(const Instance&) = delete;
    Instance& operator=(const Instance&) = delete;

    Instance(const InstanceCreateInfo& createInfo)
    {
        (void)createInfo; // Descriptor not used in current implementation
        // Request TimedWaitAny feature for proper async callback handling
        static const WGPUInstanceFeatureName requiredFeatures[] = {
            WGPUInstanceFeatureName_TimedWaitAny
        };

        WGPUInstanceDescriptor wgpu_desc = WGPU_INSTANCE_DESCRIPTOR_INIT;
        wgpu_desc.requiredFeatureCount = 1;
        wgpu_desc.requiredFeatures = requiredFeatures;
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
        // Don't add ref - emdawnwebgpu doesn't provide wgpuQueueAddRef
        // The queue is owned by the device and automatically destroyed with it
    }

    ~Queue()
    {
        // Don't release - emdawnwebgpu doesn't provide wgpuQueueRelease
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

    Device(WGPUDevice device, Adapter* adapter)
        : m_device(device)
        , m_adapter(adapter)
    {
        if (!m_device) {
            throw std::runtime_error("Invalid WGPUDevice provided to Device constructor");
        }

        WGPUQueue wgpuQueue = wgpuDeviceGetQueue(m_device);
        if (!wgpuQueue) {
            throw std::runtime_error("Failed to get default queue from WGPUDevice");
        }
        m_queue = std::make_unique<Queue>(wgpuQueue, this);
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

    Buffer(WGPUBuffer buffer, uint64_t size, BufferUsage usage, Device* device)
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
    BufferUsage getUsage() const { return m_usage; }
    Device* getDevice() const { return m_device; }

private:
    WGPUBuffer m_buffer = nullptr;
    uint64_t m_size = 0;
    BufferUsage m_usage = WGPUBufferUsage_None;
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

    CommandEncoder(WGPUDevice device, WGPUCommandEncoder encoder)
        : m_device(device)
        , m_encoder(encoder)
        , m_finished(false)
    {
    }

    ~CommandEncoder()
    {
        if (m_encoder) {
            wgpuCommandEncoderRelease(m_encoder);
        }
    }

    WGPUCommandEncoder handle() const { return m_encoder; }

    void markFinished() { m_finished = true; }
    bool isFinished() const { return m_finished; }

    // Recreate the encoder if it has been finished
    bool recreateIfNeeded()
    {
        if (!m_finished) {
            return true; // Already valid
        }

        // Release old encoder
        if (m_encoder) {
            wgpuCommandEncoderRelease(m_encoder);
            m_encoder = nullptr;
        }

        // Create new encoder
        WGPUCommandEncoderDescriptor desc = WGPU_COMMAND_ENCODER_DESCRIPTOR_INIT;
        m_encoder = wgpuDeviceCreateCommandEncoder(m_device, &desc);
        if (!m_encoder) {
            return false;
        }

        m_finished = false;
        return true;
    }

private:
    WGPUDevice m_device = nullptr;
    WGPUCommandEncoder m_encoder = nullptr;
    bool m_finished = false;
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

    Surface(WGPUSurface surface, WGPUAdapter adapter)
        : m_surface(surface)
        , m_adapter(adapter)
    {
    }

    ~Surface()
    {
        if (m_surface) {
            wgpuSurfaceRelease(m_surface);
        }
    }

    WGPUSurface handle() const { return m_surface; }
    WGPUAdapter adapter() const { return m_adapter; }

private:
    WGPUSurface m_surface = nullptr;
    WGPUAdapter m_adapter = nullptr;
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
        // Release cached view if any
        if (m_currentView) {
            delete m_currentView;
            m_currentView = nullptr;
        }
        // Release cached texture if any
        if (m_currentTexture) {
            wgpuTextureRelease(m_currentTexture);
            m_currentTexture = nullptr;
        }
        // Don't release surface or device - they're not owned
    }

    WGPUSurface surface() const { return m_surface; }
    WGPUDevice device() const { return m_device; }
    uint32_t getWidth() const { return m_width; }
    uint32_t getHeight() const { return m_height; }
    WGPUTextureFormat getFormat() const { return m_format; }
    WGPUPresentMode getPresentMode() const { return m_presentMode; }
    uint32_t getBufferCount() const { return m_bufferCount; }

    void setSize(uint32_t width, uint32_t height)
    {
        m_width = width;
        m_height = height;
    }

    // Cache the current texture so it persists across view creation and present
    void setCurrentTexture(WGPUTexture texture)
    {
        // Release old texture if any
        if (m_currentTexture) {
            wgpuTextureRelease(m_currentTexture);
        }
        m_currentTexture = texture;

        // Also clear the cached view when texture changes
        if (m_currentView) {
            delete m_currentView;
            m_currentView = nullptr;
        }
    }
    WGPUTexture getCurrentTexture() const { return m_currentTexture; }

    // Cache texture view so it can be reused and cleaned up automatically
    void setCurrentView(gfx::webgpu::TextureView* view)
    {
        // Release old view if any
        if (m_currentView) {
            delete m_currentView;
        }
        m_currentView = view;
    }
    gfx::webgpu::TextureView* getCurrentView() const { return m_currentView; }

private:
    WGPUSurface m_surface = nullptr; // Non-owning
    WGPUDevice m_device = nullptr; // Non-owning
    uint32_t m_width = 0;
    uint32_t m_height = 0;
    WGPUTextureFormat m_format = WGPUTextureFormat_Undefined;
    WGPUPresentMode m_presentMode = WGPUPresentMode_Fifo;
    uint32_t m_bufferCount = 0;
    WGPUTexture m_currentTexture = nullptr; // Cache current texture between acquire and present
    gfx::webgpu::TextureView* m_currentView = nullptr; // Cache current texture view
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

    Semaphore(SemaphoreType type, uint64_t value)
        : m_type(type)
        , m_value(value)
    {
    }

    ~Semaphore() = default;

    SemaphoreType getType() const { return m_type; }
    uint64_t getValue() const { return m_value; }
    void setValue(uint64_t value) { m_value = value; }

private:
    SemaphoreType m_type = SemaphoreType::Binary;
    uint64_t m_value = 0;
};

} // namespace gfx::webgpu
