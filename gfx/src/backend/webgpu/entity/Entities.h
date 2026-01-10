#pragma once

#include "CreateInfo.h"

#include "../common/WebGPUCommon.h"

#if defined(__EMSCRIPTEN__)
#include <emscripten.h>
// Platform-specific includes for surface creation
#elif defined(_WIN32)
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

#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_map>

// ============================================================================
// Internal C++ Classes with RAII
// ============================================================================

namespace gfx::webgpu {

namespace {
    WGPUStringView toStringView(const char* str)
    {
        if (!str) {
            return WGPUStringView{ nullptr, WGPU_STRLEN };
        }
        return WGPUStringView{ str, WGPU_STRLEN };
    }
} // namespace

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
class RenderPass;
class Framebuffer;
class RenderPassEncoder;
class ComputePassEncoder;
class BindGroupLayout;
class BindGroup;
class Fence;
class Semaphore;
class Blit;

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

    // Constructor 1: Request adapter based on preferences
    Adapter(Instance* instance, const AdapterCreateInfo& createInfo)
        : m_adapter(nullptr)
        , m_instance(instance)
    {
        if (!instance) {
            throw std::runtime_error("Invalid instance for adapter creation");
        }

        WGPURequestAdapterOptions options = WGPU_REQUEST_ADAPTER_OPTIONS_INIT;
        options.powerPreference = createInfo.powerPreference;
        options.forceFallbackAdapter = createInfo.forceFallbackAdapter ? WGPU_TRUE : WGPU_FALSE;

        // Use a struct to track callback completion
        struct AdapterRequestContext {
            WGPUAdapter* outAdapter;
            bool completed;
            WGPURequestAdapterStatus status;
        } context = { &m_adapter, false, WGPURequestAdapterStatus_Error };

        WGPURequestAdapterCallbackInfo callbackInfo = WGPU_REQUEST_ADAPTER_CALLBACK_INFO_INIT;
        callbackInfo.mode = WGPUCallbackMode_WaitAnyOnly;
        callbackInfo.callback = [](WGPURequestAdapterStatus status, WGPUAdapter adapter, WGPUStringView message, void* userdata1, void* userdata2) {
            auto* ctx = static_cast<AdapterRequestContext*>(userdata1);
            ctx->status = status;
            ctx->completed = true;

            if (status == WGPURequestAdapterStatus_Success && adapter) {
                *ctx->outAdapter = adapter;
            } else if (message.data) {
                fprintf(stderr, "Error: Failed to request adapter: %.*s\n",
                    (int)message.length, message.data);
            }
            (void)userdata2; // Unused
        };
        callbackInfo.userdata1 = &context;

        WGPUFuture future = wgpuInstanceRequestAdapter(instance->handle(), &options, callbackInfo);

        // Use WaitAny to properly wait for the callback
        WGPUFutureWaitInfo waitInfo = WGPU_FUTURE_WAIT_INFO_INIT;
        waitInfo.future = future;
        wgpuInstanceWaitAny(instance->handle(), 1, &waitInfo, UINT64_MAX);

        if (!context.completed) {
            throw std::runtime_error("Adapter request timed out");
        }

        if (!m_adapter) {
            throw std::runtime_error("Failed to request adapter");
        }

        m_info = createAdapterInfo();
    }

    // Constructor 2: Wrap existing WGPUAdapter (used by enumerate)
    Adapter(WGPUAdapter adapter, Instance* instance)
        : m_adapter(adapter)
        , m_instance(instance)
        , m_info(createAdapterInfo())
    {
    }

    // Static method to enumerate all available adapters
    // NOTE: WebGPU doesn't have a native enumerate API - returns the default adapter if available
    // Each adapter returned must be freed by the caller using the backend's adapterDestroy method
    // (e.g., gfxAdapterDestroy() in the public API)
    static uint32_t enumerate(Instance* instance, Adapter** outAdapters, uint32_t maxAdapters)
    {
        if (!instance) {
            return 0;
        }

        // Try to request the default adapter
        try {
            AdapterCreateInfo createInfo{};
            createInfo.powerPreference = WGPUPowerPreference_Undefined;
            createInfo.forceFallbackAdapter = false;

            auto* adapter = new Adapter(instance, createInfo);

            // If just querying the count (no output array), clean up and return count
            if (!outAdapters || maxAdapters == 0) {
                delete adapter;
                return 1;
            }

            // Return the adapter
            outAdapters[0] = adapter;
            return 1;
        } catch (...) {
            return 0;
        }
    }

    ~Adapter()
    {
        if (m_adapter) {
            wgpuAdapterRelease(m_adapter);
        }
    }

    WGPUAdapter handle() const { return m_adapter; }
    Instance* getInstance() const { return m_instance; }

    const gfx::webgpu::AdapterInfo& getInfo() const { return m_info; }

    WGPULimits getLimits() const
    {
        WGPULimits limits = WGPU_LIMITS_INIT;
        WGPUStatus status = wgpuAdapterGetLimits(m_adapter, &limits);
        if (status != WGPUStatus_Success) {
            throw std::runtime_error("Failed to get adapter limits");
        }
        return limits;
    }

private:
    AdapterInfo createAdapterInfo() const
    {
        gfx::webgpu::AdapterInfo adapterInfo{};

#ifdef __EMSCRIPTEN__
        // Emscripten's wgpuAdapterGetInfo has issues - provide reasonable defaults
        adapterInfo.name = "WebGPU Adapter";
        adapterInfo.driverDescription = "WebGPU";
        adapterInfo.vendorID = 0;
        adapterInfo.deviceID = 0;
        adapterInfo.adapterType = WGPUAdapterType_Unknown;
#else
        WGPUAdapterInfo info = WGPU_ADAPTER_INFO_INIT;
        wgpuAdapterGetInfo(m_adapter, &info);

        adapterInfo.name = info.device.data ? std::string(info.device.data, info.device.length) : "Unknown";
        adapterInfo.driverDescription = info.description.data ? std::string(info.description.data, info.description.length) : "";
        adapterInfo.vendorID = info.vendorID;
        adapterInfo.deviceID = info.deviceID;
        adapterInfo.adapterType = info.adapterType;

        wgpuAdapterInfoFreeMembers(info);
#endif
        return adapterInfo;
    }

private:
    WGPUAdapter m_adapter = nullptr;
    Instance* m_instance = nullptr; // Non-owning
    gfx::webgpu::AdapterInfo m_info;
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
        if (m_queue) {
            wgpuQueueRelease(m_queue);
        }
    }

    WGPUQueue handle() const { return m_queue; }
    Device* getDevice() const { return m_device; }

    // Submit command encoders with optional fence signaling
    bool submit(const SubmitInfo& submitInfo);

    // Write data directly to a buffer
    void writeBuffer(Buffer* buffer, uint64_t offset, const void* data, uint64_t size);

    // Write data directly to a texture
    void writeTexture(Texture* texture, uint32_t mipLevel,
        const WGPUOrigin3D& origin, const void* data, uint64_t dataSize,
        uint32_t bytesPerRow, const WGPUExtent3D& extent);

    // Wait for all submitted work to complete
    bool waitIdle();

private:
    WGPUQueue m_queue = nullptr;
    Device* m_device = nullptr; // Non-owning pointer to parent device
};

class Device {
public:
    // Prevent copying
    Device(const Device&) = delete;
    Device& operator=(const Device&) = delete;

    // Constructor 1: Request device from adapter with createInfo
    Device(Adapter* adapter, const DeviceCreateInfo& createInfo)
        : m_device(nullptr)
        , m_adapter(adapter)
    {
        if (!adapter) {
            throw std::runtime_error("Invalid adapter for device creation");
        }

        WGPUUncapturedErrorCallbackInfo errorCallbackInfo = WGPU_UNCAPTURED_ERROR_CALLBACK_INFO_INIT;
        errorCallbackInfo.callback = [](WGPUDevice const*, WGPUErrorType type, WGPUStringView message, void*, void*) {
            fprintf(stderr, "[WebGPU Uncaptured Error] Type: %d, Message: %.*s\n",
                static_cast<int>(type), static_cast<int>(message.length), message.data);
        };

        WGPUDeviceLostCallbackInfo deviceLostCallbackInfo = WGPU_DEVICE_LOST_CALLBACK_INFO_INIT;
        deviceLostCallbackInfo.mode = WGPUCallbackMode_AllowSpontaneous;
        deviceLostCallbackInfo.callback = [](WGPUDevice const*, WGPUDeviceLostReason reason, WGPUStringView message, void*, void*) {
            fprintf(stderr, "[WebGPU Device Lost] Reason: %d, Message: %.*s\n",
                static_cast<int>(reason), static_cast<int>(message.length), message.data);
        };

        WGPUDeviceDescriptor wgpuDesc = WGPU_DEVICE_DESCRIPTOR_INIT;
        wgpuDesc.uncapturedErrorCallbackInfo = errorCallbackInfo;
        wgpuDesc.deviceLostCallbackInfo = deviceLostCallbackInfo;
        // DeviceCreateInfo is currently empty, but we keep it for future extensibility
        (void)createInfo;

        struct DeviceRequestContext {
            WGPUDevice* outDevice;
            bool completed;
            WGPURequestDeviceStatus status;
        } context = { &m_device, false, WGPURequestDeviceStatus_Error };

        WGPURequestDeviceCallbackInfo callbackInfo = WGPU_REQUEST_DEVICE_CALLBACK_INFO_INIT;
        callbackInfo.mode = WGPUCallbackMode_WaitAnyOnly;
        callbackInfo.callback = [](WGPURequestDeviceStatus status, WGPUDevice device, WGPUStringView message, void* userdata1, void* userdata2) {
            auto* ctx = static_cast<DeviceRequestContext*>(userdata1);
            ctx->status = status;
            ctx->completed = true;

            if (status == WGPURequestDeviceStatus_Success && device) {
                *ctx->outDevice = device;
            } else if (message.data) {
                fprintf(stderr, "Error: Failed to request device: %.*s\n",
                    (int)message.length, message.data);
            }
            (void)userdata2; // Unused
        };
        callbackInfo.userdata1 = &context;

        WGPUFuture future = wgpuAdapterRequestDevice(adapter->handle(), &wgpuDesc, callbackInfo);

        // Use WaitAny to properly wait for the callback
        if (adapter->getInstance()) {
            WGPUFutureWaitInfo waitInfo = WGPU_FUTURE_WAIT_INFO_INIT;
            waitInfo.future = future;
            wgpuInstanceWaitAny(adapter->getInstance()->handle(), 1, &waitInfo, UINT64_MAX);
        }

        if (!context.completed) {
            throw std::runtime_error("Device request timed out");
        }

        if (!m_device) {
            throw std::runtime_error("Failed to request device");
        }

        // Create queue
        WGPUQueue wgpuQueue = wgpuDeviceGetQueue(m_device);
        if (!wgpuQueue) {
            throw std::runtime_error("Failed to get default queue from WGPUDevice");
        }
        m_queue = std::make_unique<Queue>(wgpuQueue, this);

        // Create blit helper
        m_blit = std::make_unique<Blit>(m_device);
    }

    ~Device()
    {
        if (m_device) {
            // Release queue first
            m_queue.reset();
            // Destroy device to ensure proper cleanup of internal resources
            wgpuDeviceDestroy(m_device);
            wgpuDeviceRelease(m_device);
        }
    }

    WGPUDevice handle() const { return m_device; }
    Queue* getQueue() { return m_queue.get(); }
    Adapter* getAdapter() { return m_adapter; }

    WGPULimits getLimits() const
    {
        WGPULimits limits = WGPU_LIMITS_INIT;
        WGPUStatus status = wgpuDeviceGetLimits(m_device, &limits);
        if (status != WGPUStatus_Success) {
            throw std::runtime_error("Failed to get device limits");
        }
        return limits;
    }

    void waitIdle() const
    {
        WGPUQueueWorkDoneCallbackInfo callbackInfo = WGPU_QUEUE_WORK_DONE_CALLBACK_INFO_INIT;
        callbackInfo.mode = WGPUCallbackMode_WaitAnyOnly;
        callbackInfo.callback = [](WGPUQueueWorkDoneStatus status, WGPUStringView message, void* userdata1, void* userdata2) {
            (void)status;
            (void)message;
            (void)userdata1;
            (void)userdata2;
        };
        WGPUFuture future = wgpuQueueOnSubmittedWorkDone(m_queue->handle(), callbackInfo);

        // Wait for the work to complete
        WGPUInstance instance = m_adapter->getInstance()->handle();
        WGPUFutureWaitInfo waitInfo = WGPU_FUTURE_WAIT_INFO_INIT;
        waitInfo.future = future;
        wgpuInstanceWaitAny(instance, 1, &waitInfo, UINT64_MAX);
    }

    Blit* getBlit()
    {
        return m_blit.get();
    }

private:
    WGPUDevice m_device = nullptr;
    Adapter* m_adapter = nullptr; // Non-owning pointer
    std::unique_ptr<Queue> m_queue;
    std::unique_ptr<Blit> m_blit;
};

class Buffer {
public:
    // Prevent copying
    Buffer(const Buffer&) = delete;
    Buffer& operator=(const Buffer&) = delete;

    // Owning constructor - creates and manages WGPUBuffer
    Buffer(Device* device, const BufferCreateInfo& createInfo)
        : m_device(device)
        , m_ownsResources(true)
        , m_size(createInfo.size)
        , m_usage(createInfo.usage)
    {
        WGPUBufferDescriptor desc = WGPU_BUFFER_DESCRIPTOR_INIT;
        desc.size = m_size;
        desc.usage = m_usage;
        desc.mappedAtCreation = false;

        m_buffer = wgpuDeviceCreateBuffer(m_device->handle(), &desc);
        if (!m_buffer) {
            throw std::runtime_error("Failed to create WebGPU buffer");
        }
    }

    // Non-owning constructor for imported buffers
    Buffer(Device* device, WGPUBuffer buffer, const BufferImportInfo& importInfo)
        : m_device(device)
        , m_ownsResources(false)
        , m_buffer(buffer)
        , m_size(importInfo.size)
        , m_usage(importInfo.usage)
    {
    }

    ~Buffer()
    {
        if (m_ownsResources && m_buffer) {
            wgpuBufferRelease(m_buffer);
        }
    }

    WGPUBuffer handle() const { return m_buffer; }
    uint64_t getSize() const { return m_size; }
    BufferUsage getUsage() const { return m_usage; }
    Device* getDevice() const { return m_device; }

    // Map buffer for CPU access
    // Returns mapped pointer on success, nullptr on failure
    void* map(uint64_t offset, uint64_t size)
    {
        // If size is 0, map the entire buffer from offset
        uint64_t mapSize = size;
        if (mapSize == 0) {
            mapSize = m_size - offset;
        }

        // Determine map mode based on buffer usage
        WGPUMapMode mapMode = WGPUMapMode_None;
        if (m_usage & WGPUBufferUsage_MapRead) {
            mapMode |= WGPUMapMode_Read;
        }
        if (m_usage & WGPUBufferUsage_MapWrite) {
            mapMode |= WGPUMapMode_Write;
        }

        if (mapMode == WGPUMapMode_None) {
            return nullptr;
        }

        // Set up async mapping with synchronous wait
        struct MapCallbackData {
            WGPUMapAsyncStatus status = WGPUMapAsyncStatus_Error;
            bool completed = false;
        };
        MapCallbackData callbackData;

        WGPUBufferMapCallbackInfo callbackInfo = WGPU_BUFFER_MAP_CALLBACK_INFO_INIT;
        callbackInfo.mode = WGPUCallbackMode_WaitAnyOnly;
        callbackInfo.callback = [](WGPUMapAsyncStatus status, WGPUStringView, void* userdata1, void*) {
            auto* data = static_cast<MapCallbackData*>(userdata1);
            data->status = status;
            data->completed = true;
        };
        callbackInfo.userdata1 = &callbackData;

        WGPUFuture future = wgpuBufferMapAsync(m_buffer, mapMode, offset, mapSize, callbackInfo);

        // Properly wait for the mapping to complete
        if (m_device && m_device->getAdapter() && m_device->getAdapter()->getInstance()) {
            WGPUFutureWaitInfo waitInfo = WGPU_FUTURE_WAIT_INFO_INIT;
            waitInfo.future = future;
            wgpuInstanceWaitAny(m_device->getAdapter()->getInstance()->handle(), 1, &waitInfo, UINT64_MAX);
        }

        if (!callbackData.completed || callbackData.status != WGPUMapAsyncStatus_Success) {
            return nullptr;
        }

        // Get the mapped range
        void* mappedData = wgpuBufferGetMappedRange(m_buffer, offset, mapSize);
        if (!mappedData) {
            wgpuBufferUnmap(m_buffer);
            return nullptr;
        }

        return mappedData;
    }

    void unmap()
    {
        wgpuBufferUnmap(m_buffer);
    }

private:
    Device* m_device = nullptr; // Non-owning pointer for device operations
    bool m_ownsResources = true;
    WGPUBuffer m_buffer = nullptr;
    uint64_t m_size = 0;
    BufferUsage m_usage = WGPUBufferUsage_None;
};

class Texture {
public:
    // Prevent copying
    Texture(const Texture&) = delete;
    Texture& operator=(const Texture&) = delete;

    // Owning constructor - creates and manages WGPUTexture
    Texture(Device* device, const TextureCreateInfo& createInfo)
        : m_device(device)
        , m_ownsResources(true)
        , m_info(createTextureInfo(createInfo))
    {
        WGPUTextureDescriptor desc = WGPU_TEXTURE_DESCRIPTOR_INIT;
        desc.dimension = createInfo.dimension;
        desc.size = createInfo.size; // Already has correct depthOrArrayLayers
        desc.format = createInfo.format;
        desc.mipLevelCount = createInfo.mipLevelCount;
        desc.sampleCount = createInfo.sampleCount;
        desc.usage = createInfo.usage;
        desc.viewFormatCount = 0;
        desc.viewFormats = nullptr;

        m_texture = wgpuDeviceCreateTexture(m_device->handle(), &desc);
        if (!m_texture) {
            throw std::runtime_error("Failed to create WebGPU texture");
        }
    }

    // Non-owning constructor - wraps an existing WGPUTexture
    Texture(Device* device, WGPUTexture texture, const TextureCreateInfo& createInfo)
        : m_device(device)
        , m_ownsResources(false)
        , m_texture(texture)
        , m_info(createTextureInfo(createInfo))
    {
    }

    // Non-owning constructor for imported textures
    Texture(Device* device, WGPUTexture texture, const TextureImportInfo& importInfo)
        : m_device(device)
        , m_ownsResources(false)
        , m_texture(texture)
        , m_info(createTextureInfo(importInfo))
    {
    }

    ~Texture()
    {
        if (m_ownsResources && m_texture) {
            wgpuTextureRelease(m_texture);
        }
    }

    WGPUTexture handle() const { return m_texture; }
    WGPUTextureDimension getDimension() const { return m_info.dimension; }
    WGPUExtent3D getSize() const { return m_info.size; }
    uint32_t getArrayLayers() const { return m_info.arrayLayers; }
    WGPUTextureFormat getFormat() const { return m_info.format; }
    uint32_t getMipLevels() const { return m_info.mipLevels; }
    uint32_t getSampleCount() const { return m_info.sampleCount; }
    WGPUTextureUsage getUsage() const { return m_info.usage; }
    const TextureInfo& getInfo() const { return m_info; }

    void generateMipmaps(CommandEncoder* encoder);
    void generateMipmapsRange(CommandEncoder* encoder, uint32_t baseMipLevel, uint32_t levelCount);

private:
    static TextureInfo createTextureInfo(const TextureCreateInfo& createInfo)
    {
        TextureInfo info{};
        info.dimension = createInfo.dimension;
        info.size = createInfo.size;
        info.arrayLayers = createInfo.size.depthOrArrayLayers;
        info.format = createInfo.format;
        info.mipLevels = createInfo.mipLevelCount;
        info.sampleCount = createInfo.sampleCount;
        info.usage = createInfo.usage;
        return info;
    }

    static TextureInfo createTextureInfo(const TextureImportInfo& importInfo)
    {
        TextureInfo info{};
        info.dimension = importInfo.dimension;
        info.size = importInfo.size;
        info.arrayLayers = importInfo.size.depthOrArrayLayers;
        info.format = importInfo.format;
        info.mipLevels = importInfo.mipLevelCount;
        info.sampleCount = importInfo.sampleCount;
        info.usage = importInfo.usage;
        return info;
    }

private:
    Device* m_device = nullptr; // Non-owning pointer for device operations
    bool m_ownsResources = true;
    WGPUTexture m_texture = nullptr;
    TextureInfo m_info = {};
};

class TextureView {
public:
    // Prevent copying
    TextureView(const TextureView&) = delete;
    TextureView& operator=(const TextureView&) = delete;

    // Constructor 1: Wrap existing WGPUTextureView (legacy, used by swapchain)
    TextureView(WGPUTextureView view, Texture* texture = nullptr)
        : m_view(view)
        , m_texture(texture)
    {
    }

    // Constructor 2: Create view from Texture with TextureViewCreateInfo
    TextureView(Texture* texture, const TextureViewCreateInfo& createInfo)
        : m_texture(texture)
    {
        WGPUTextureViewDescriptor desc = WGPU_TEXTURE_VIEW_DESCRIPTOR_INIT;
        desc.dimension = createInfo.viewDimension;
        desc.format = createInfo.format;
        desc.baseMipLevel = createInfo.baseMipLevel;
        desc.mipLevelCount = createInfo.mipLevelCount;
        desc.baseArrayLayer = createInfo.baseArrayLayer;
        desc.arrayLayerCount = createInfo.arrayLayerCount;

        m_view = wgpuTextureCreateView(texture->handle(), &desc);
        if (!m_view) {
            throw std::runtime_error("Failed to create WebGPU texture view");
        }
    }

    TextureView(Swapchain* swapchain)
        : m_view(nullptr)
        , m_texture(nullptr)
        , m_swapchain(swapchain)
    {
    }

    ~TextureView()
    {
        if (m_view) {
            wgpuTextureViewRelease(m_view);
        }
    }

    WGPUTextureView handle() const;
    Texture* getTexture() { return m_texture; }

private:
    WGPUTextureView m_view = nullptr;
    Texture* m_texture = nullptr; // Non-owning
    Swapchain* m_swapchain = nullptr; // Non-owning
};

class Sampler {
public:
    // Prevent copying
    Sampler(const Sampler&) = delete;
    Sampler& operator=(const Sampler&) = delete;

    Sampler(Device* device, const SamplerCreateInfo& createInfo)
        : m_device(device)
    {
        WGPUSamplerDescriptor desc = WGPU_SAMPLER_DESCRIPTOR_INIT;
        desc.addressModeU = createInfo.addressModeU;
        desc.addressModeV = createInfo.addressModeV;
        desc.addressModeW = createInfo.addressModeW;
        desc.magFilter = createInfo.magFilter;
        desc.minFilter = createInfo.minFilter;
        desc.mipmapFilter = createInfo.mipmapFilter;
        desc.lodMinClamp = createInfo.lodMinClamp;
        desc.lodMaxClamp = createInfo.lodMaxClamp;
        desc.maxAnisotropy = createInfo.maxAnisotropy;
        desc.compare = createInfo.compareFunction;

        m_sampler = wgpuDeviceCreateSampler(m_device->handle(), &desc);
        if (!m_sampler) {
            throw std::runtime_error("Failed to create WebGPU sampler");
        }
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
    Device* m_device = nullptr; // Non-owning pointer for device operations
};

class Shader {
public:
    // Prevent copying
    Shader(const Shader&) = delete;
    Shader& operator=(const Shader&) = delete;

    Shader(Device* device, const ShaderCreateInfo& createInfo)
        : m_device(device)
    {
        WGPUShaderSourceWGSL wgslDesc = WGPU_SHADER_SOURCE_WGSL_INIT;
        // codeSize may include null terminator, but WGPUStringView expects length without it
        size_t codeLength = createInfo.codeSize;
        const char* codeStr = static_cast<const char*>(createInfo.code);
        // If last char is null terminator, exclude it from length
        if (codeLength > 0 && codeStr[codeLength - 1] == '\0') {
            codeLength--;
        }
        wgslDesc.code = { codeStr, codeLength };

        WGPUShaderModuleDescriptor desc = WGPU_SHADER_MODULE_DESCRIPTOR_INIT;
        desc.nextInChain = &wgslDesc.chain;

        m_module = wgpuDeviceCreateShaderModule(m_device->handle(), &desc);
        if (!m_module) {
            throw std::runtime_error("Failed to create WebGPU shader module");
        }
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
    Device* m_device = nullptr; // Non-owning pointer for device operations
};

class BindGroupLayout {
public:
    // Prevent copying
    BindGroupLayout(const BindGroupLayout&) = delete;
    BindGroupLayout& operator=(const BindGroupLayout&) = delete;

    BindGroupLayout(Device* device, const BindGroupLayoutCreateInfo& createInfo)
    {
        WGPUBindGroupLayoutDescriptor desc = WGPU_BIND_GROUP_LAYOUT_DESCRIPTOR_INIT;

        std::vector<WGPUBindGroupLayoutEntry> wgpuEntries;
        wgpuEntries.reserve(createInfo.entries.size());

        for (const auto& entry : createInfo.entries) {
            WGPUBindGroupLayoutEntry wgpuEntry = WGPU_BIND_GROUP_LAYOUT_ENTRY_INIT;
            wgpuEntry.binding = entry.binding;
            wgpuEntry.visibility = entry.visibility;

            // Set binding type based on which field is used
            if (entry.bufferType != WGPUBufferBindingType_Undefined) {
                wgpuEntry.buffer.type = entry.bufferType;
                wgpuEntry.buffer.hasDynamicOffset = entry.bufferHasDynamicOffset;
                wgpuEntry.buffer.minBindingSize = entry.bufferMinBindingSize;
            }
            if (entry.samplerType != WGPUSamplerBindingType_Undefined) {
                wgpuEntry.sampler.type = entry.samplerType;
            }
            if (entry.textureSampleType != WGPUTextureSampleType_Undefined) {
                wgpuEntry.texture.sampleType = entry.textureSampleType;
                wgpuEntry.texture.viewDimension = entry.textureViewDimension;
                wgpuEntry.texture.multisampled = entry.textureMultisampled;
            }
            if (entry.storageTextureAccess != WGPUStorageTextureAccess_Undefined) {
                wgpuEntry.storageTexture.access = entry.storageTextureAccess;
                wgpuEntry.storageTexture.format = entry.storageTextureFormat;
                wgpuEntry.storageTexture.viewDimension = entry.storageTextureViewDimension;
            }

            wgpuEntries.push_back(wgpuEntry);
        }

        desc.entryCount = static_cast<uint32_t>(wgpuEntries.size());
        desc.entries = wgpuEntries.data();

        m_layout = wgpuDeviceCreateBindGroupLayout(device->handle(), &desc);
        if (!m_layout) {
            throw std::runtime_error("Failed to create WebGPU BindGroupLayout");
        }
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

    BindGroup(Device* device, const BindGroupCreateInfo& createInfo)
        : m_device(device)
    {
        WGPUBindGroupDescriptor desc = WGPU_BIND_GROUP_DESCRIPTOR_INIT;
        desc.layout = createInfo.layout;

        std::vector<WGPUBindGroupEntry> wgpuEntries;
        wgpuEntries.reserve(createInfo.entries.size());

        for (const auto& entry : createInfo.entries) {
            WGPUBindGroupEntry wgpuEntry = WGPU_BIND_GROUP_ENTRY_INIT;
            wgpuEntry.binding = entry.binding;
            wgpuEntry.buffer = entry.buffer;
            wgpuEntry.offset = entry.bufferOffset;
            wgpuEntry.size = entry.bufferSize;
            wgpuEntry.sampler = entry.sampler;
            wgpuEntry.textureView = entry.textureView;
            wgpuEntries.push_back(wgpuEntry);
        }

        desc.entries = wgpuEntries.data();
        desc.entryCount = static_cast<uint32_t>(wgpuEntries.size());

        m_bindGroup = wgpuDeviceCreateBindGroup(m_device->handle(), &desc);
        if (!m_bindGroup) {
            throw std::runtime_error("Failed to create WebGPU BindGroup");
        }
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
    Device* m_device = nullptr; // Non-owning pointer for device operations
};

class RenderPipeline {
public:
    // Prevent copying
    RenderPipeline(const RenderPipeline&) = delete;
    RenderPipeline& operator=(const RenderPipeline&) = delete;

    RenderPipeline(Device* device, const RenderPipelineCreateInfo& createInfo)
    {
        WGPURenderPipelineDescriptor desc = WGPU_RENDER_PIPELINE_DESCRIPTOR_INIT;

        // Create pipeline layout if bind group layouts are provided
        WGPUPipelineLayout pipelineLayout = nullptr;
        if (!createInfo.bindGroupLayouts.empty()) {
            WGPUPipelineLayoutDescriptor layoutDesc = WGPU_PIPELINE_LAYOUT_DESCRIPTOR_INIT;
            layoutDesc.bindGroupLayouts = createInfo.bindGroupLayouts.data();
            layoutDesc.bindGroupLayoutCount = static_cast<uint32_t>(createInfo.bindGroupLayouts.size());
            pipelineLayout = wgpuDeviceCreatePipelineLayout(device->handle(), &layoutDesc);
            desc.layout = pipelineLayout;
        }

        // Vertex state
        WGPUVertexState vertexState = WGPU_VERTEX_STATE_INIT;
        vertexState.module = createInfo.vertex.module;
        vertexState.entryPoint = { createInfo.vertex.entryPoint, WGPU_STRLEN };

        // Convert vertex buffers
        std::vector<WGPUVertexBufferLayout> vertexBuffers;
        std::vector<std::vector<WGPUVertexAttribute>> allAttributes;

        if (!createInfo.vertex.buffers.empty()) {
            vertexBuffers.reserve(createInfo.vertex.buffers.size());
            allAttributes.reserve(createInfo.vertex.buffers.size());

            for (const auto& buffer : createInfo.vertex.buffers) {
                std::vector<WGPUVertexAttribute> attributes;
                attributes.reserve(buffer.attributes.size());

                for (const auto& attr : buffer.attributes) {
                    WGPUVertexAttribute wgpuAttr = WGPU_VERTEX_ATTRIBUTE_INIT;
                    wgpuAttr.format = attr.format;
                    wgpuAttr.offset = attr.offset;
                    wgpuAttr.shaderLocation = attr.shaderLocation;
                    attributes.push_back(wgpuAttr);
                }

                allAttributes.push_back(std::move(attributes));

                WGPUVertexBufferLayout wgpuBuffer = WGPU_VERTEX_BUFFER_LAYOUT_INIT;
                wgpuBuffer.arrayStride = buffer.arrayStride;
                wgpuBuffer.stepMode = buffer.stepMode;
                wgpuBuffer.attributes = allAttributes.back().data();
                wgpuBuffer.attributeCount = static_cast<uint32_t>(allAttributes.back().size());
                vertexBuffers.push_back(wgpuBuffer);
            }

            vertexState.buffers = vertexBuffers.data();
            vertexState.bufferCount = static_cast<uint32_t>(vertexBuffers.size());
        }

        desc.vertex = vertexState;

        // Fragment state (optional)
        WGPUFragmentState fragmentState = WGPU_FRAGMENT_STATE_INIT;
        std::vector<WGPUColorTargetState> colorTargets;
        std::vector<WGPUBlendState> blendStates;

        if (createInfo.fragment.has_value()) {
            fragmentState.module = createInfo.fragment->module;
            fragmentState.entryPoint = { createInfo.fragment->entryPoint, WGPU_STRLEN };

            if (!createInfo.fragment->targets.empty()) {
                colorTargets.reserve(createInfo.fragment->targets.size());

                for (const auto& target : createInfo.fragment->targets) {
                    WGPUColorTargetState wgpuTarget = WGPU_COLOR_TARGET_STATE_INIT;
                    wgpuTarget.format = target.format;
                    wgpuTarget.writeMask = target.writeMask;

                    if (target.blend.has_value()) {
                        WGPUBlendState blend = WGPU_BLEND_STATE_INIT;
                        blend.color.operation = target.blend->color.operation;
                        blend.color.srcFactor = target.blend->color.srcFactor;
                        blend.color.dstFactor = target.blend->color.dstFactor;
                        blend.alpha.operation = target.blend->alpha.operation;
                        blend.alpha.srcFactor = target.blend->alpha.srcFactor;
                        blend.alpha.dstFactor = target.blend->alpha.dstFactor;
                        blendStates.push_back(blend);
                        wgpuTarget.blend = &blendStates.back();
                    }

                    colorTargets.push_back(wgpuTarget);
                }

                fragmentState.targets = colorTargets.data();
                fragmentState.targetCount = static_cast<uint32_t>(colorTargets.size());
            }

            desc.fragment = &fragmentState;
        }

        // Primitive state
        WGPUPrimitiveState primitiveState = WGPU_PRIMITIVE_STATE_INIT;
        primitiveState.topology = createInfo.primitive.topology;
        primitiveState.frontFace = createInfo.primitive.frontFace;
        primitiveState.cullMode = createInfo.primitive.cullMode;
        primitiveState.stripIndexFormat = createInfo.primitive.stripIndexFormat;
        desc.primitive = primitiveState;

        // Depth/stencil state (optional)
        WGPUDepthStencilState depthStencilState = WGPU_DEPTH_STENCIL_STATE_INIT;
        if (createInfo.depthStencil.has_value()) {
            depthStencilState.format = createInfo.depthStencil->format;
            depthStencilState.depthWriteEnabled = createInfo.depthStencil->depthWriteEnabled ? WGPUOptionalBool_True : WGPUOptionalBool_False;
            depthStencilState.depthCompare = createInfo.depthStencil->depthCompare;

            depthStencilState.stencilFront.compare = createInfo.depthStencil->stencilFront.compare;
            depthStencilState.stencilFront.failOp = createInfo.depthStencil->stencilFront.failOp;
            depthStencilState.stencilFront.depthFailOp = createInfo.depthStencil->stencilFront.depthFailOp;
            depthStencilState.stencilFront.passOp = createInfo.depthStencil->stencilFront.passOp;

            depthStencilState.stencilBack.compare = createInfo.depthStencil->stencilBack.compare;
            depthStencilState.stencilBack.failOp = createInfo.depthStencil->stencilBack.failOp;
            depthStencilState.stencilBack.depthFailOp = createInfo.depthStencil->stencilBack.depthFailOp;
            depthStencilState.stencilBack.passOp = createInfo.depthStencil->stencilBack.passOp;

            depthStencilState.stencilReadMask = createInfo.depthStencil->stencilReadMask;
            depthStencilState.stencilWriteMask = createInfo.depthStencil->stencilWriteMask;
            depthStencilState.depthBias = createInfo.depthStencil->depthBias;
            depthStencilState.depthBiasSlopeScale = createInfo.depthStencil->depthBiasSlopeScale;
            depthStencilState.depthBiasClamp = createInfo.depthStencil->depthBiasClamp;

            desc.depthStencil = &depthStencilState;
        }

        // Multisample state
        WGPUMultisampleState multisampleState = WGPU_MULTISAMPLE_STATE_INIT;
        multisampleState.count = createInfo.sampleCount;
        desc.multisample = multisampleState;

        m_pipeline = wgpuDeviceCreateRenderPipeline(device->handle(), &desc);

        // Release the pipeline layout if we created one (pipeline holds its own reference)
        if (pipelineLayout) {
            wgpuPipelineLayoutRelease(pipelineLayout);
        }

        if (!m_pipeline) {
            throw std::runtime_error("Failed to create WebGPU RenderPipeline");
        }
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

    ComputePipeline(Device* device, const ComputePipelineCreateInfo& createInfo)
    {
        WGPUComputePipelineDescriptor desc = WGPU_COMPUTE_PIPELINE_DESCRIPTOR_INIT;

        // Create pipeline layout if bind group layouts are provided
        WGPUPipelineLayout pipelineLayout = nullptr;
        if (!createInfo.bindGroupLayouts.empty()) {
            WGPUPipelineLayoutDescriptor layoutDesc = WGPU_PIPELINE_LAYOUT_DESCRIPTOR_INIT;
            layoutDesc.bindGroupLayouts = createInfo.bindGroupLayouts.data();
            layoutDesc.bindGroupLayoutCount = static_cast<uint32_t>(createInfo.bindGroupLayouts.size());
            pipelineLayout = wgpuDeviceCreatePipelineLayout(device->handle(), &layoutDesc);
            desc.layout = pipelineLayout;
        }

        desc.compute.module = createInfo.module;
        desc.compute.entryPoint = { createInfo.entryPoint, WGPU_STRLEN };

        m_pipeline = wgpuDeviceCreateComputePipeline(device->handle(), &desc);

        // Release the pipeline layout if we created one (pipeline holds its own reference)
        if (pipelineLayout) {
            wgpuPipelineLayoutRelease(pipelineLayout);
        }

        if (!m_pipeline) {
            throw std::runtime_error("Failed to create WebGPU ComputePipeline");
        }
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

    CommandEncoder(Device* device, const CommandEncoderCreateInfo& createInfo)
        : m_device(device)
        , m_finished(false)
    {
        WGPUCommandEncoderDescriptor desc = WGPU_COMMAND_ENCODER_DESCRIPTOR_INIT;
        if (createInfo.label) {
            desc.label = toStringView(createInfo.label);
        }

        m_encoder = wgpuDeviceCreateCommandEncoder(m_device->handle(), &desc);
        if (!m_encoder) {
            throw std::runtime_error("Failed to create WebGPU CommandEncoder");
        }
    }

    ~CommandEncoder()
    {
        if (m_encoder) {
            wgpuCommandEncoderRelease(m_encoder);
        }
    }

    WGPUCommandEncoder handle() const { return m_encoder; }
    Device* getDevice() const { return m_device; }

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
        m_encoder = wgpuDeviceCreateCommandEncoder(m_device->handle(), &desc);
        if (!m_encoder) {
            return false;
        }

        m_finished = false;
        return true;
    }

    // Copy operations
    void copyBufferToBuffer(Buffer* source, uint64_t sourceOffset,
        Buffer* destination, uint64_t destinationOffset, uint64_t size);

    void copyBufferToTexture(Buffer* source, uint64_t sourceOffset, uint32_t bytesPerRow,
        Texture* destination, const WGPUOrigin3D& origin,
        const WGPUExtent3D& extent, uint32_t mipLevel);

    void copyTextureToBuffer(Texture* source, const WGPUOrigin3D& origin,
        uint32_t mipLevel, Buffer* destination, uint64_t destinationOffset, uint32_t bytesPerRow,
        const WGPUExtent3D& extent);

    void copyTextureToTexture(Texture* source, const WGPUOrigin3D& sourceOrigin,
        uint32_t sourceMipLevel, Texture* destination, const WGPUOrigin3D& destinationOrigin,
        uint32_t destinationMipLevel, const WGPUExtent3D& extent);

    void blitTextureToTexture(Texture* source, const WGPUOrigin3D& sourceOrigin, const WGPUExtent3D& sourceExtent, uint32_t sourceMipLevel,
        Texture* destination, const WGPUOrigin3D& destinationOrigin, const WGPUExtent3D& destinationExtent, uint32_t destinationMipLevel,
        WGPUFilterMode filter);

private:
    Device* m_device = nullptr; // Non-owning pointer
    WGPUCommandEncoder m_encoder = nullptr;
    bool m_finished = false;
};

class RenderPass {
public:
    // Prevent copying
    RenderPass(const RenderPass&) = delete;
    RenderPass& operator=(const RenderPass&) = delete;

    RenderPass(Device* device, const RenderPassCreateInfo& createInfo)
        : m_device(device)
        , m_createInfo(createInfo)
    {
        // WebGPU doesn't create persistent render pass objects
        // Just store the configuration for later use
    }

    ~RenderPass() = default;

    const RenderPassCreateInfo& getCreateInfo() const { return m_createInfo; }
    Device* getDevice() const { return m_device; }

private:
    Device* m_device = nullptr; // Non-owning pointer
    RenderPassCreateInfo m_createInfo;
};

class Framebuffer {
public:
    // Prevent copying
    Framebuffer(const Framebuffer&) = delete;
    Framebuffer& operator=(const Framebuffer&) = delete;

    Framebuffer(Device* device, const FramebufferCreateInfo& createInfo)
        : m_device(device)
        , m_createInfo(createInfo)
    {
        // WebGPU doesn't have framebuffer objects
        // Just store the views for use in beginRenderPass
    }

    ~Framebuffer() = default;

    const FramebufferCreateInfo& getCreateInfo() const { return m_createInfo; }
    Device* getDevice() const { return m_device; }

private:
    Device* m_device = nullptr; // Non-owning pointer
    FramebufferCreateInfo m_createInfo;
};

class RenderPassEncoder {
public:
    // Prevent copying
    RenderPassEncoder(const RenderPassEncoder&) = delete;
    RenderPassEncoder& operator=(const RenderPassEncoder&) = delete;

    RenderPassEncoder(CommandEncoder* commandEncoder, RenderPass* renderPass, Framebuffer* framebuffer, const RenderPassEncoderBeginInfo& beginInfo);

    ~RenderPassEncoder()
    {
        if (m_encoder) {
            if (!m_ended) {
                wgpuRenderPassEncoderEnd(m_encoder);
            }
            wgpuRenderPassEncoderRelease(m_encoder);
        }
    }

    void setPipeline(WGPURenderPipeline pipeline)
    {
        wgpuRenderPassEncoderSetPipeline(m_encoder, pipeline);
    }

    void setBindGroup(uint32_t index, WGPUBindGroup bindGroup, const uint32_t* dynamicOffsets, uint32_t dynamicOffsetCount)
    {
        wgpuRenderPassEncoderSetBindGroup(m_encoder, index, bindGroup, dynamicOffsetCount, dynamicOffsets);
    }

    void setVertexBuffer(uint32_t slot, WGPUBuffer buffer, uint64_t offset, uint64_t size)
    {
        wgpuRenderPassEncoderSetVertexBuffer(m_encoder, slot, buffer, offset, size);
    }

    void setIndexBuffer(WGPUBuffer buffer, WGPUIndexFormat format, uint64_t offset, uint64_t size)
    {
        wgpuRenderPassEncoderSetIndexBuffer(m_encoder, buffer, format, offset, size);
    }

    void setViewport(float x, float y, float width, float height, float minDepth, float maxDepth)
    {
        wgpuRenderPassEncoderSetViewport(m_encoder, x, y, width, height, minDepth, maxDepth);
    }

    void setScissorRect(uint32_t x, uint32_t y, uint32_t width, uint32_t height)
    {
        wgpuRenderPassEncoderSetScissorRect(m_encoder, x, y, width, height);
    }

    void draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance)
    {
        wgpuRenderPassEncoderDraw(m_encoder, vertexCount, instanceCount, firstVertex, firstInstance);
    }

    void drawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t baseVertex, uint32_t firstInstance)
    {
        wgpuRenderPassEncoderDrawIndexed(m_encoder, indexCount, instanceCount, firstIndex, baseVertex, firstInstance);
    }

    WGPURenderPassEncoder handle() const { return m_encoder; }

private:
    WGPURenderPassEncoder m_encoder = nullptr;
    bool m_ended = false;
};

class ComputePassEncoder {
public:
    // Prevent copying
    ComputePassEncoder(const ComputePassEncoder&) = delete;
    ComputePassEncoder& operator=(const ComputePassEncoder&) = delete;

    ComputePassEncoder(CommandEncoder* commandEncoder, const ComputePassEncoderCreateInfo& createInfo);

    ~ComputePassEncoder()
    {
        if (m_encoder) {
            if (!m_ended) {
                wgpuComputePassEncoderEnd(m_encoder);
            }
            wgpuComputePassEncoderRelease(m_encoder);
        }
    }

    void setPipeline(WGPUComputePipeline pipeline)
    {
        wgpuComputePassEncoderSetPipeline(m_encoder, pipeline);
    }

    void setBindGroup(uint32_t index, WGPUBindGroup bindGroup, const uint32_t* dynamicOffsets, uint32_t dynamicOffsetCount)
    {
        wgpuComputePassEncoderSetBindGroup(m_encoder, index, bindGroup, dynamicOffsetCount, dynamicOffsets);
    }

    void dispatchWorkgroups(uint32_t workgroupCountX, uint32_t workgroupCountY, uint32_t workgroupCountZ)
    {
        wgpuComputePassEncoderDispatchWorkgroups(m_encoder, workgroupCountX, workgroupCountY, workgroupCountZ);
    }

    WGPUComputePassEncoder handle() const { return m_encoder; }

private:
    WGPUComputePassEncoder m_encoder = nullptr;
    bool m_ended = false;
};

class Surface {
public:
    // Prevent copying
    Surface(const Surface&) = delete;
    Surface& operator=(const Surface&) = delete;

    Surface(WGPUInstance instance, WGPUAdapter adapter, const SurfaceCreateInfo& createInfo)
        : m_adapter(adapter)
    {
        switch (createInfo.windowHandle.platform) {
#if defined(_WIN32)
        case gfx::webgpu::PlatformWindowHandle::Platform::Win32:
            m_surface = createSurfaceWin32(instance, createInfo.windowHandle);
            break;
#elif defined(__ANDROID__)
        case gfx::webgpu::PlatformWindowHandle::Platform::Android:
            m_surface = createSurfaceAndroid(instance, createInfo.windowHandle);
            break;
#elif defined(__linux__)
        case gfx::webgpu::PlatformWindowHandle::Platform::Xlib:
            m_surface = createSurfaceXlib(instance, createInfo.windowHandle);
            break;
        case gfx::webgpu::PlatformWindowHandle::Platform::Xcb:
            m_surface = createSurfaceXCB(instance, createInfo.windowHandle);
            break;
        case gfx::webgpu::PlatformWindowHandle::Platform::Wayland:
            m_surface = createSurfaceWayland(instance, createInfo.windowHandle);
            break;
#elif defined(__APPLE__)
        case gfx::webgpu::PlatformWindowHandle::Platform::Cocoa:
            m_surface = createSurfaceMetal(instance, createInfo.windowHandle);
            break;
#elif defined(__EMSCRIPTEN__)
        case gfx::webgpu::PlatformWindowHandle::Platform::Emscripten:
            m_surface = createSurfaceEmscripten(instance, createInfo.windowHandle);
            break;
#endif
        default:
            throw std::runtime_error("Unsupported windowing system for WebGPU surface creation");
        }
    }

    ~Surface()
    {
        if (m_surface) {
            wgpuSurfaceRelease(m_surface);
        }
    }

    WGPUAdapter adapter() const { return m_adapter; }
    WGPUSurface handle() const { return m_surface; }

    // Query surface capabilities and return them
    // Caller is responsible for calling wgpuSurfaceCapabilitiesFreeMembers
    WGPUSurfaceCapabilities getCapabilities() const
    {
        WGPUSurfaceCapabilities capabilities = WGPU_SURFACE_CAPABILITIES_INIT;
        wgpuSurfaceGetCapabilities(m_surface, m_adapter, &capabilities);
        return capabilities;
    }

private:
#if defined(_WIN32)
    static WGPUSurface createSurfaceWin32(WGPUInstance instance, const gfx::webgpu::PlatformWindowHandle& windowHandle)
    {
        if (!windowHandle.handle.win32.hwnd || !windowHandle.handle.win32.hinstance) {
            throw std::runtime_error("Invalid Win32 window or instance handle");
        }

        WGPUSurfaceSourceWindowsHWND source = WGPU_SURFACE_SOURCE_WINDOWS_HWND_INIT;
        source.hwnd = windowHandle.handle.win32.hwnd;
        source.hinstance = windowHandle.handle.win32.hinstance;

        WGPUSurfaceDescriptor surfaceDesc = WGPU_SURFACE_DESCRIPTOR_INIT;
        surfaceDesc.label = toStringView("Win32 Surface");
        surfaceDesc.nextInChain = (WGPUChainedStruct*)&source;

        return wgpuInstanceCreateSurface(instance, &surfaceDesc);
    }
#elif defined(__ANDROID__)
    static WGPUSurface createSurfaceAndroid(WGPUInstance instance, const gfx::webgpu::PlatformWindowHandle& windowHandle)
    {
        if (!windowHandle.handle.android.window) {
            throw std::runtime_error("Invalid Android window handle");
        }

        WGPUSurfaceSourceAndroidNativeWindow source = WGPU_SURFACE_SOURCE_ANDROID_NATIVE_WINDOW_INIT;
        source.window = windowHandle.android.window;

        WGPUSurfaceDescriptor surfaceDesc = WGPU_SURFACE_DESCRIPTOR_INIT;
        surfaceDesc.label = toStringView("Android Surface");
        surfaceDesc.nextInChain = (WGPUChainedStruct*)&source;

        return wgpuInstanceCreateSurface(instance, &surfaceDesc);
    }
#elif defined(__linux__)
    static WGPUSurface createSurfaceXlib(WGPUInstance instance, const gfx::webgpu::PlatformWindowHandle& windowHandle)
    {
        if (!windowHandle.handle.xlib.window || !windowHandle.handle.xlib.display) {
            throw std::runtime_error("Invalid Xlib window or display handle");
        }

        WGPUSurfaceSourceXlibWindow source = WGPU_SURFACE_SOURCE_XLIB_WINDOW_INIT;
        source.display = windowHandle.handle.xlib.display;
        source.window = windowHandle.handle.xlib.window;

        WGPUSurfaceDescriptor surfaceDesc = WGPU_SURFACE_DESCRIPTOR_INIT;
        surfaceDesc.label = toStringView("X11 Surface");
        surfaceDesc.nextInChain = (WGPUChainedStruct*)&source;

        return wgpuInstanceCreateSurface(instance, &surfaceDesc);
    }

    static WGPUSurface createSurfaceXCB(WGPUInstance instance, const gfx::webgpu::PlatformWindowHandle& windowHandle)
    {
        if (!windowHandle.handle.xcb.window || !windowHandle.handle.xcb.connection) {
            throw std::runtime_error("Invalid XCB window or connection handle");
        }

        WGPUSurfaceSourceXCBWindow source = WGPU_SURFACE_SOURCE_XCB_WINDOW_INIT;
        source.connection = windowHandle.handle.xcb.connection;
        source.window = windowHandle.handle.xcb.window;

        WGPUSurfaceDescriptor surfaceDesc = WGPU_SURFACE_DESCRIPTOR_INIT;
        surfaceDesc.label = toStringView("XCB Surface");
        surfaceDesc.nextInChain = (WGPUChainedStruct*)&source;

        return wgpuInstanceCreateSurface(instance, &surfaceDesc);
    }

    static WGPUSurface createSurfaceWayland(WGPUInstance instance, const gfx::webgpu::PlatformWindowHandle& windowHandle)
    {
        if (!windowHandle.handle.wayland.surface || !windowHandle.handle.wayland.display) {
            throw std::runtime_error("Invalid Wayland surface or display handle");
        }

        WGPUSurfaceSourceWaylandSurface source = WGPU_SURFACE_SOURCE_WAYLAND_SURFACE_INIT;
        source.display = windowHandle.handle.wayland.display;
        source.surface = windowHandle.handle.wayland.surface;

        WGPUSurfaceDescriptor surfaceDesc = WGPU_SURFACE_DESCRIPTOR_INIT;
        surfaceDesc.label = toStringView("Wayland Surface");
        surfaceDesc.nextInChain = (WGPUChainedStruct*)&source;

        return wgpuInstanceCreateSurface(instance, &surfaceDesc);
    }
#elif defined(__APPLE__)
    static WGPUSurface createSurfaceMetal(WGPUInstance instance, const gfx::webgpu::PlatformWindowHandle& windowHandle)
    {
        if (!windowHandle.handle.metalLayer) {
            throw std::runtime_error("Invalid Metal layer handle");
        }

        WGPUSurfaceSourceMetalLayer source = WGPU_SURFACE_SOURCE_METAL_LAYER_INIT;
        source.layer = windowHandle.handle.metalLayer;

        WGPUSurfaceDescriptor surfaceDesc = WGPU_SURFACE_DESCRIPTOR_INIT;
        surfaceDesc.label = toStringView("Metal Surface");
        surfaceDesc.nextInChain = (WGPUChainedStruct*)&source;

        return wgpuInstanceCreateSurface(instance, &surfaceDesc);
    }
#elif defined(__EMSCRIPTEN__)
    static WGPUSurface createSurfaceEmscripten(WGPUInstance instance, const gfx::webgpu::PlatformWindowHandle& windowHandle)
    {
        if (!windowHandle.handle.emscripten.canvasSelector) {
            throw std::runtime_error("Invalid Emscripten canvas selector");
        }

        WGPUEmscriptenSurfaceSourceCanvasHTMLSelector canvasDesc = WGPU_EMSCRIPTEN_SURFACE_SOURCE_CANVAS_HTML_SELECTOR_INIT;
        canvasDesc.selector = toStringView(windowHandle.handle.emscripten.canvasSelector);

        WGPUSurfaceDescriptor surfaceDesc = WGPU_SURFACE_DESCRIPTOR_INIT;
        surfaceDesc.nextInChain = (WGPUChainedStruct*)&canvasDesc;

        return wgpuInstanceCreateSurface(instance, &surfaceDesc);
    }
#endif

private:
    WGPUAdapter m_adapter = nullptr;
    WGPUSurface m_surface = nullptr;
};

class Swapchain {
public:
    // Prevent copying
    Swapchain(const Swapchain&) = delete;
    Swapchain& operator=(const Swapchain&) = delete;

    Swapchain(gfx::webgpu::Device* device, gfx::webgpu::Surface* surface, const SwapchainCreateInfo& createInfo)
        : m_device(device)
        , m_surface(surface->handle())
        , m_width(createInfo.width)
        , m_height(createInfo.height)
        , m_format(createInfo.format)
        , m_presentMode(createInfo.presentMode)
        , m_imageCount(createInfo.imageCount)
    {

        // Get surface capabilities
        WGPUSurfaceCapabilities capabilities = surface->getCapabilities();

        // Choose format
        WGPUTextureFormat selectedFormat = WGPUTextureFormat_Undefined;
        for (size_t i = 0; i < capabilities.formatCount; ++i) {
            if (capabilities.formats[i] == m_format) {
                selectedFormat = capabilities.formats[i];
                break;
            }
        }

        // If requested format not found, fall back to first available format
        if (selectedFormat == WGPUTextureFormat_Undefined && capabilities.formatCount > 0) {
            selectedFormat = capabilities.formats[0];
            fprintf(stderr, "[WebGPU Swapchain] Requested format %d not supported, using format %d\n", m_format, selectedFormat);
        }

        if (selectedFormat == WGPUTextureFormat_Undefined) {
            throw std::runtime_error("No supported surface formats available for swapchain");
        }

        m_format = selectedFormat;

        // Choose present mode
        WGPUPresentMode selectedPresentMode = WGPUPresentMode_Undefined;
        for (size_t i = 0; i < capabilities.presentModeCount; ++i) {
            if (capabilities.presentModes[i] == createInfo.presentMode) {
                selectedPresentMode = capabilities.presentModes[i];
                break;
            }
        }

        // If requested present mode not found, fall back to first available mode
        if (selectedPresentMode == WGPUPresentMode_Undefined && capabilities.presentModeCount > 0) {
            selectedPresentMode = capabilities.presentModes[0];
            fprintf(stderr, "[WebGPU Swapchain] Requested present mode %d not supported, using mode %d\n", createInfo.presentMode, selectedPresentMode);
        }

        if (selectedPresentMode == WGPUPresentMode_Undefined) {
            throw std::runtime_error("No supported present modes available for swapchain");
        }

        m_presentMode = selectedPresentMode;

        // Create stable TextureView wrapper ONCE (never deleted until swapchain destroyed)
        // This allows multiple framebuffers to reference the same TextureView*
        m_currentView = new gfx::webgpu::TextureView(this);

        // Configure surface for direct rendering
        WGPUSurfaceConfiguration config = WGPU_SURFACE_CONFIGURATION_INIT;
        config.device = m_device->handle();
        config.format = m_format;
        config.usage = createInfo.usage;
        config.width = createInfo.width;
        config.height = createInfo.height;
        config.presentMode = m_presentMode;
        config.alphaMode = WGPUCompositeAlphaMode_Auto;

        wgpuSurfaceConfigure(m_surface, &config);

        // Free capabilities using the proper WebGPU function
        wgpuSurfaceCapabilitiesFreeMembers(capabilities);
    }

    ~Swapchain()
    {
        if (m_currentView) {
            delete m_currentView;
        }
        if (m_currentRawView) {
            wgpuTextureViewRelease(m_currentRawView);
        }
        if (m_currentTexture) {
            wgpuTextureRelease(m_currentTexture);
        }
    }

    // Accessors
    WGPUSurface handle() const { return m_surface; }
    WGPUDevice device() const { return m_device->handle(); }
    uint32_t getWidth() const { return m_width; }
    uint32_t getHeight() const { return m_height; }
    WGPUTextureFormat getFormat() const { return m_format; }
    WGPUPresentMode getPresentMode() const { return m_presentMode; }
    uint32_t getImageCount() const { return m_imageCount; }

    // Swapchain operations (call in order: acquireNextImage -> getCurrentTextureView -> present)
    WGPUSurfaceGetCurrentTextureStatus acquireNextImage()
    {
        // Clean up previous frame's raw view handle only
        // Keep m_currentView wrapper stable for framebuffer references
        if (m_currentRawView) {
            wgpuTextureViewRelease(m_currentRawView);
            m_currentRawView = nullptr;
        }

        WGPUSurfaceTexture surfaceTexture = WGPU_SURFACE_TEXTURE_INIT;
        wgpuSurfaceGetCurrentTexture(m_surface, &surfaceTexture);

        if (surfaceTexture.status == WGPUSurfaceGetCurrentTextureStatus_SuccessOptimal || surfaceTexture.status == WGPUSurfaceGetCurrentTextureStatus_SuccessSuboptimal) {
            if (m_currentTexture) {
                wgpuTextureRelease(m_currentTexture);
            }
            m_currentTexture = surfaceTexture.texture;

            // Create the view from the new texture
            m_currentRawView = wgpuTextureCreateView(m_currentTexture, nullptr);
            if (!m_currentRawView) {
                fprintf(stderr, "[WebGPU] Failed to create texture view\n");
            }
        } else if (surfaceTexture.texture) {
            wgpuTextureRelease(surfaceTexture.texture);
        }

        return surfaceTexture.status;
    }

    gfx::webgpu::TextureView* getCurrentTextureView()
    {
        // Just return the stable wrapper (created in constructor)
        // It will dynamically resolve to the current texture when handle() is called
        // No need to check m_currentTexture here - that's checked in handle()
        return m_currentView;
    }

    WGPUTextureView getRawCurrentTextureView() const
    {
        return m_currentRawView;
    }

    void present()
    {
#ifndef __EMSCRIPTEN__
        wgpuSurfacePresent(m_surface);
#endif
        if (m_currentTexture) {
            wgpuTextureRelease(m_currentTexture);
            m_currentTexture = nullptr;
        }
    }

private:
    Device* m_device = nullptr; // Non-owning
    WGPUSurface m_surface = nullptr; // Non-owning
    uint32_t m_width = 0;
    uint32_t m_height = 0;
    WGPUTextureFormat m_format = WGPUTextureFormat_Undefined;
    WGPUPresentMode m_presentMode = WGPUPresentMode_Fifo;
    uint32_t m_imageCount = 0;
    WGPUTexture m_currentTexture = nullptr; // Current frame texture from surface
    WGPUTextureView m_currentRawView = nullptr; // Current frame raw view handle
    gfx::webgpu::TextureView* m_currentView = nullptr; // Current frame view wrapper (owned)
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

class Blit {
public:
    // Prevent copying
    Blit(const Blit&) = delete;
    Blit& operator=(const Blit&) = delete;

    Blit(WGPUDevice device)
        : m_device(device)
    {
        // Create shader module for 2D textures with source region support
        const char* shader2DCode = R"(
            struct SourceRegion {
                uvMin: vec2f,
                uvMax: vec2f,
            }
            
            struct VertexOutput {
                @builtin(position) position: vec4f,
                @location(0) texCoord: vec2f,
            }
            
            @group(0) @binding(0) var srcTexture: texture_2d<f32>;
            @group(0) @binding(1) var srcSampler: sampler;
            @group(0) @binding(2) var<uniform> sourceRegion: SourceRegion;
            
            @vertex
            fn vs_main(@builtin(vertex_index) vertexIndex: u32) -> VertexOutput {
                var output: VertexOutput;
                let x = f32((vertexIndex & 1u) << 1u) - 1.0;
                let y = 1.0 - f32((vertexIndex & 2u));
                output.position = vec4f(x, y, 0.0, 1.0);
                // Map vertex coordinates [0,1] to source region
                let uv = vec2f((x + 1.0) * 0.5, (1.0 - y) * 0.5);
                output.texCoord = mix(sourceRegion.uvMin, sourceRegion.uvMax, uv);
                return output;
            }
            
            @fragment
            fn fs_main(input: VertexOutput) -> @location(0) vec4f {
                return textureSample(srcTexture, srcSampler, input.texCoord);
            }
        )";

        WGPUShaderSourceWGSL wgslSource = {};
        wgslSource.chain.sType = WGPUSType_ShaderSourceWGSL;
        wgslSource.code = toStringView(shader2DCode);

        WGPUShaderModuleDescriptor shaderDesc = {};
        shaderDesc.nextInChain = &wgslSource.chain;
        m_shaderModule = wgpuDeviceCreateShaderModule(m_device, &shaderDesc);

        // Create bind group layout with uniform buffer for source region
        WGPUBindGroupLayoutEntry bgLayoutEntries[3] = {};
        bgLayoutEntries[0].binding = 0;
        bgLayoutEntries[0].visibility = WGPUShaderStage_Fragment;
        bgLayoutEntries[0].texture.sampleType = WGPUTextureSampleType_Float;
        bgLayoutEntries[0].texture.viewDimension = WGPUTextureViewDimension_2D;

        bgLayoutEntries[1].binding = 1;
        bgLayoutEntries[1].visibility = WGPUShaderStage_Fragment;
        bgLayoutEntries[1].sampler.type = WGPUSamplerBindingType_Filtering;

        bgLayoutEntries[2].binding = 2;
        bgLayoutEntries[2].visibility = WGPUShaderStage_Vertex;
        bgLayoutEntries[2].buffer.type = WGPUBufferBindingType_Uniform;
        bgLayoutEntries[2].buffer.minBindingSize = 16; // vec2f + vec2f = 16 bytes

        WGPUBindGroupLayoutDescriptor bgLayoutDesc = {};
        bgLayoutDesc.entryCount = 3;
        bgLayoutDesc.entries = bgLayoutEntries;
        m_bindGroupLayout = wgpuDeviceCreateBindGroupLayout(m_device, &bgLayoutDesc);

        // Create pipeline layout
        WGPUPipelineLayoutDescriptor pipelineLayoutDesc = {};
        pipelineLayoutDesc.bindGroupLayoutCount = 1;
        pipelineLayoutDesc.bindGroupLayouts = &m_bindGroupLayout;
        m_pipelineLayout = wgpuDeviceCreatePipelineLayout(m_device, &pipelineLayoutDesc);
    }

    ~Blit()
    {
        if (m_pipelineLayout) {
            wgpuPipelineLayoutRelease(m_pipelineLayout);
        }
        if (m_bindGroupLayout) {
            wgpuBindGroupLayoutRelease(m_bindGroupLayout);
        }
        if (m_shaderModule) {
            wgpuShaderModuleRelease(m_shaderModule);
        }
    }

    void execute(WGPUCommandEncoder commandEncoder,
        WGPUTexture srcTexture, const WGPUOrigin3D& srcOrigin, const WGPUExtent3D& srcExtent, uint32_t srcMipLevel,
        WGPUTexture dstTexture, const WGPUOrigin3D& dstOrigin, const WGPUExtent3D& dstExtent, uint32_t dstMipLevel,
        WGPUFilterMode filterMode)
    {
        // Create sampler with requested filter mode
        WGPUSamplerDescriptor samplerDesc = {};
        samplerDesc.addressModeU = WGPUAddressMode_ClampToEdge;
        samplerDesc.addressModeV = WGPUAddressMode_ClampToEdge;
        samplerDesc.addressModeW = WGPUAddressMode_ClampToEdge;
        samplerDesc.magFilter = filterMode;
        samplerDesc.minFilter = filterMode;
        samplerDesc.mipmapFilter = WGPUMipmapFilterMode_Nearest;
        WGPUSampler sampler = wgpuDeviceCreateSampler(m_device, &samplerDesc);

        // Query texture dimension - use 2D for now, can be extended for arrays/3D later
        WGPUTextureDimension srcDimension = wgpuTextureGetDimension(srcTexture);
        WGPUTextureViewDimension viewDimension = WGPUTextureViewDimension_2D;

        // Map texture dimension to view dimension
        // For now we only support 2D, but this can be extended
        if (srcDimension == WGPUTextureDimension_2D) {
            viewDimension = WGPUTextureViewDimension_2D;
        } else {
            // 1D and 3D textures not yet supported by the shader
            fprintf(stderr, "[WebGPU Blit] Only 2D textures are currently supported\n");
        }

        // Create texture view for source
        WGPUTextureViewDescriptor srcViewDesc = {};
        srcViewDesc.format = wgpuTextureGetFormat(srcTexture);
        srcViewDesc.dimension = viewDimension;
        srcViewDesc.baseMipLevel = srcMipLevel;
        srcViewDesc.mipLevelCount = 1;
        srcViewDesc.baseArrayLayer = 0;
        srcViewDesc.arrayLayerCount = 1;
        WGPUTextureView srcView = wgpuTextureCreateView(srcTexture, &srcViewDesc);

        // Calculate source texture size at mip level
        uint32_t srcTexWidth = std::max(1u, wgpuTextureGetWidth(srcTexture) >> srcMipLevel);
        uint32_t srcTexHeight = std::max(1u, wgpuTextureGetHeight(srcTexture) >> srcMipLevel);

        // Calculate UV coordinates for source region
        struct SourceRegionData {
            float uvMinX, uvMinY;
            float uvMaxX, uvMaxY;
        } regionData;
        regionData.uvMinX = static_cast<float>(srcOrigin.x) / static_cast<float>(srcTexWidth);
        regionData.uvMinY = static_cast<float>(srcOrigin.y) / static_cast<float>(srcTexHeight);
        regionData.uvMaxX = static_cast<float>(srcOrigin.x + srcExtent.width) / static_cast<float>(srcTexWidth);
        regionData.uvMaxY = static_cast<float>(srcOrigin.y + srcExtent.height) / static_cast<float>(srcTexHeight);

        // Create uniform buffer for source region
        WGPUBufferDescriptor uniformBufferDesc = {};
        uniformBufferDesc.size = sizeof(SourceRegionData);
        uniformBufferDesc.usage = WGPUBufferUsage_Uniform | WGPUBufferUsage_CopyDst;
        WGPUBuffer uniformBuffer = wgpuDeviceCreateBuffer(m_device, &uniformBufferDesc);
        wgpuQueueWriteBuffer(wgpuDeviceGetQueue(m_device), uniformBuffer, 0, &regionData, sizeof(SourceRegionData));

        // Create bind group
        WGPUBindGroupEntry bgEntries[3] = {};
        bgEntries[0].binding = 0;
        bgEntries[0].textureView = srcView;

        bgEntries[1].binding = 1;
        bgEntries[1].sampler = sampler;

        bgEntries[2].binding = 2;
        bgEntries[2].buffer = uniformBuffer;
        bgEntries[2].size = sizeof(SourceRegionData);

        WGPUBindGroupDescriptor bgDesc = {};
        bgDesc.layout = m_bindGroupLayout;
        bgDesc.entryCount = 3;
        bgDesc.entries = bgEntries;
        WGPUBindGroup bindGroup = wgpuDeviceCreateBindGroup(m_device, &bgDesc);

        // Get or create render pipeline for this format
        WGPUTextureFormat dstFormat = wgpuTextureGetFormat(dstTexture);
        WGPURenderPipeline pipeline = getOrCreatePipeline(dstFormat);

        // Create texture view for destination
        WGPUTextureViewDescriptor dstViewDesc = {};
        dstViewDesc.format = dstFormat;
        dstViewDesc.dimension = WGPUTextureViewDimension_2D;
        dstViewDesc.baseMipLevel = dstMipLevel;
        dstViewDesc.mipLevelCount = 1;
        dstViewDesc.baseArrayLayer = 0;
        dstViewDesc.arrayLayerCount = 1;
        WGPUTextureView dstView = wgpuTextureCreateView(dstTexture, &dstViewDesc);

        // Create render pass
        WGPURenderPassColorAttachment colorAttachment = {};
        colorAttachment.view = dstView;
        colorAttachment.loadOp = WGPULoadOp_Load;
        colorAttachment.storeOp = WGPUStoreOp_Store;

        WGPURenderPassDescriptor renderPassDesc = {};
        renderPassDesc.colorAttachmentCount = 1;
        renderPassDesc.colorAttachments = &colorAttachment;

        WGPURenderPassEncoder renderPass = wgpuCommandEncoderBeginRenderPass(commandEncoder, &renderPassDesc);
        wgpuRenderPassEncoderSetPipeline(renderPass, pipeline);
        wgpuRenderPassEncoderSetBindGroup(renderPass, 0, bindGroup, 0, nullptr);

        // Set viewport to destination region for proper scaling
        wgpuRenderPassEncoderSetViewport(renderPass,
            static_cast<float>(dstOrigin.x), static_cast<float>(dstOrigin.y),
            static_cast<float>(dstExtent.width), static_cast<float>(dstExtent.height),
            0.0f, 1.0f);

        // Set scissor to destination region
        wgpuRenderPassEncoderSetScissorRect(renderPass,
            dstOrigin.x, dstOrigin.y,
            dstExtent.width, dstExtent.height);

        wgpuRenderPassEncoderDraw(renderPass, 4, 1, 0, 0);
        wgpuRenderPassEncoderEnd(renderPass);

        // Cleanup temporary resources
        wgpuRenderPassEncoderRelease(renderPass);
        wgpuTextureViewRelease(dstView);
        wgpuBindGroupRelease(bindGroup);
        wgpuBufferRelease(uniformBuffer);
        wgpuTextureViewRelease(srcView);
        wgpuSamplerRelease(sampler);
    }

private:
    WGPURenderPipeline getOrCreatePipeline(WGPUTextureFormat format)
    {
        // Check cache
        auto it = m_pipelines.find(format);
        if (it != m_pipelines.end()) {
            return it->second;
        }

        // Create new pipeline for this format
        WGPUColorTargetState colorTarget = {};
        colorTarget.format = format;
        colorTarget.writeMask = WGPUColorWriteMask_All;

        WGPUFragmentState fragmentState = {};
        fragmentState.module = m_shaderModule;
        fragmentState.entryPoint = toStringView("fs_main");
        fragmentState.targetCount = 1;
        fragmentState.targets = &colorTarget;

        WGPURenderPipelineDescriptor pipelineDesc = {};
        pipelineDesc.layout = m_pipelineLayout;
        pipelineDesc.vertex.module = m_shaderModule;
        pipelineDesc.vertex.entryPoint = toStringView("vs_main");
        pipelineDesc.primitive.topology = WGPUPrimitiveTopology_TriangleStrip;
        pipelineDesc.fragment = &fragmentState;

        WGPURenderPipeline pipeline = wgpuDeviceCreateRenderPipeline(m_device, &pipelineDesc);
        m_pipelines[format] = pipeline;
        return pipeline;
    }

    WGPUDevice m_device = nullptr;
    WGPUShaderModule m_shaderModule = nullptr;
    WGPUBindGroupLayout m_bindGroupLayout = nullptr;
    WGPUPipelineLayout m_pipelineLayout = nullptr;
    std::unordered_map<WGPUTextureFormat, WGPURenderPipeline> m_pipelines;
};

} // namespace gfx::webgpu
