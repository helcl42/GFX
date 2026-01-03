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

    // Constructor 1: Request adapter based on preferences
    Adapter(Instance* instance, const AdapterCreateInfo& createInfo)
        : m_adapter(nullptr)
        , m_instance(instance)
        , m_name("WebGPU Adapter")
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
        callbackInfo.callback = [](WGPURequestAdapterStatus status, WGPUAdapter adapter,
            WGPUStringView message, void* userdata1, void* userdata2) {
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
    }

    // Constructor 2: Wrap existing WGPUAdapter (used by enumerate)
    Adapter(WGPUAdapter adapter, Instance* instance)
        : m_adapter(adapter)
        , m_instance(instance)
        , m_name("WebGPU Adapter")
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
        callbackInfo.callback = [](WGPURequestDeviceStatus status, WGPUDevice device,
            WGPUStringView message, void* userdata1, void* userdata2) {
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

    Buffer(WGPUDevice device, const BufferCreateInfo& createInfo, Device* deviceObj = nullptr)
        : m_device(device)
        , m_deviceObj(deviceObj)
        , m_size(createInfo.size)
        , m_usage(createInfo.usage)
    {
        WGPUBufferDescriptor desc = WGPU_BUFFER_DESCRIPTOR_INIT;
        desc.size = m_size;
        desc.usage = m_usage;
        desc.mappedAtCreation = false;

        m_buffer = wgpuDeviceCreateBuffer(m_device, &desc);
        if (!m_buffer) {
            throw std::runtime_error("Failed to create WebGPU buffer");
        }
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
    Device* getDevice() const { return m_deviceObj; }

private:
    WGPUBuffer m_buffer = nullptr;
    WGPUDevice m_device = nullptr;
    Device* m_deviceObj = nullptr; // Non-owning pointer for operations that need it
    uint64_t m_size = 0;
    BufferUsage m_usage = WGPUBufferUsage_None;
};

class Texture {
public:
    // Prevent copying
    Texture(const Texture&) = delete;
    Texture& operator=(const Texture&) = delete;

    Texture(WGPUDevice device, const TextureCreateInfo& createInfo)
        : m_device(device)
        , m_size(createInfo.size)
        , m_format(createInfo.format)
        , m_mipLevels(createInfo.mipLevelCount)
        , m_sampleCount(createInfo.sampleCount)
        , m_usage(createInfo.usage)
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

        m_texture = wgpuDeviceCreateTexture(m_device, &desc);
        if (!m_texture) {
            throw std::runtime_error("Failed to create WebGPU texture");
        }
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
    WGPUDevice m_device = nullptr;
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

    Sampler(WGPUDevice device, const SamplerCreateInfo& createInfo)
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

        m_sampler = wgpuDeviceCreateSampler(m_device, &desc);
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
    WGPUDevice m_device = nullptr;
};

class Shader {
public:
    // Prevent copying
    Shader(const Shader&) = delete;
    Shader& operator=(const Shader&) = delete;

    Shader(WGPUDevice device, const ShaderCreateInfo& createInfo)
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

        m_module = wgpuDeviceCreateShaderModule(m_device, &desc);
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
    WGPUDevice m_device = nullptr;
};

class BindGroupLayout {
public:
    // Prevent copying
    BindGroupLayout(const BindGroupLayout&) = delete;
    BindGroupLayout& operator=(const BindGroupLayout&) = delete;

    BindGroupLayout(WGPUDevice device, const BindGroupLayoutCreateInfo& createInfo)
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

        m_layout = wgpuDeviceCreateBindGroupLayout(device, &desc);
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

    BindGroup(WGPUDevice device, const BindGroupCreateInfo& createInfo)
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

        m_bindGroup = wgpuDeviceCreateBindGroup(m_device, &desc);
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
    WGPUDevice m_device = nullptr;
};

class RenderPipeline {
public:
    // Prevent copying
    RenderPipeline(const RenderPipeline&) = delete;
    RenderPipeline& operator=(const RenderPipeline&) = delete;

    RenderPipeline(WGPUDevice device, const RenderPipelineCreateInfo& createInfo)
        : m_device(device)
    {
        WGPURenderPipelineDescriptor desc = WGPU_RENDER_PIPELINE_DESCRIPTOR_INIT;

        // Create pipeline layout if bind group layouts are provided
        WGPUPipelineLayout pipelineLayout = nullptr;
        if (!createInfo.bindGroupLayouts.empty()) {
            WGPUPipelineLayoutDescriptor layoutDesc = WGPU_PIPELINE_LAYOUT_DESCRIPTOR_INIT;
            layoutDesc.bindGroupLayouts = createInfo.bindGroupLayouts.data();
            layoutDesc.bindGroupLayoutCount = static_cast<uint32_t>(createInfo.bindGroupLayouts.size());
            pipelineLayout = wgpuDeviceCreatePipelineLayout(device, &layoutDesc);
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

        m_pipeline = wgpuDeviceCreateRenderPipeline(device, &desc);

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
    WGPUDevice m_device = nullptr;
};

class ComputePipeline {
public:
    // Prevent copying
    ComputePipeline(const ComputePipeline&) = delete;
    ComputePipeline& operator=(const ComputePipeline&) = delete;

    ComputePipeline(WGPUDevice device, const ComputePipelineCreateInfo& createInfo)
        : m_device(device)
    {
        WGPUComputePipelineDescriptor desc = WGPU_COMPUTE_PIPELINE_DESCRIPTOR_INIT;

        // Create pipeline layout if bind group layouts are provided
        WGPUPipelineLayout pipelineLayout = nullptr;
        if (!createInfo.bindGroupLayouts.empty()) {
            WGPUPipelineLayoutDescriptor layoutDesc = WGPU_PIPELINE_LAYOUT_DESCRIPTOR_INIT;
            layoutDesc.bindGroupLayouts = createInfo.bindGroupLayouts.data();
            layoutDesc.bindGroupLayoutCount = static_cast<uint32_t>(createInfo.bindGroupLayouts.size());
            pipelineLayout = wgpuDeviceCreatePipelineLayout(device, &layoutDesc);
            desc.layout = pipelineLayout;
        }

        desc.compute.module = createInfo.module;
        desc.compute.entryPoint = { createInfo.entryPoint, WGPU_STRLEN };

        m_pipeline = wgpuDeviceCreateComputePipeline(device, &desc);

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
    WGPUDevice m_device = nullptr;
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
            if (!m_ended) {
                wgpuRenderPassEncoderEnd(m_encoder);
            }
            wgpuRenderPassEncoderRelease(m_encoder);
        }
    }

    void end()
    {
        if (m_encoder && !m_ended) {
            wgpuRenderPassEncoderEnd(m_encoder);
            m_ended = true;
        }
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

    ComputePassEncoder(WGPUComputePassEncoder encoder)
        : m_encoder(encoder)
    {
    }

    ~ComputePassEncoder()
    {
        if (m_encoder) {
            if (!m_ended) {
                wgpuComputePassEncoderEnd(m_encoder);
            }
            wgpuComputePassEncoderRelease(m_encoder);
        }
    }

    void end()
    {
        if (m_encoder && !m_ended) {
            wgpuComputePassEncoderEnd(m_encoder);
            m_ended = true;
        }
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
