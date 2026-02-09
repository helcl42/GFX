#include "Conversions.h"

#include "../core/command/CommandEncoder.h"
#include "../core/command/ComputePassEncoder.h"
#include "../core/command/RenderPassEncoder.h"
#include "../core/compute/ComputePipeline.h"
#include "../core/presentation/Surface.h"
#include "../core/presentation/Swapchain.h"
#include "../core/query/QuerySet.h"
#include "../core/render/Framebuffer.h"
#include "../core/render/RenderPass.h"
#include "../core/render/RenderPipeline.h"
#include "../core/resource/BindGroup.h"
#include "../core/resource/BindGroupLayout.h"
#include "../core/resource/Buffer.h"
#include "../core/resource/Sampler.h"
#include "../core/resource/Shader.h"
#include "../core/resource/Texture.h"
#include "../core/resource/TextureView.h"
#include "../core/sync/Fence.h"
#include "../core/sync/Semaphore.h"
#include "../core/system/Adapter.h"
#include "../core/system/Device.h"
#include "../core/system/Instance.h"
#include "../core/system/Queue.h"

#include <cstring>
#include <vector>

namespace gfx::backend::webgpu::converter {

// ============================================================================
// Extension Name Mapping
// ============================================================================

const char* instanceExtensionNameToGfx(const char* internalName)
{
    if (std::strcmp(internalName, core::extensions::SURFACE) == 0) {
        return GFX_INSTANCE_EXTENSION_SURFACE;
    }
    if (std::strcmp(internalName, core::extensions::DEBUG) == 0) {
        return GFX_INSTANCE_EXTENSION_DEBUG;
    }
    // Unknown extension - return as-is
    return internalName;
}

const char* deviceExtensionNameToGfx(const char* internalName)
{
    if (std::strcmp(internalName, core::extensions::SWAPCHAIN) == 0) {
        return GFX_DEVICE_EXTENSION_SWAPCHAIN;
    }
    if (std::strcmp(internalName, core::extensions::TIMELINE_SEMAPHORE) == 0) {
        return GFX_DEVICE_EXTENSION_TIMELINE_SEMAPHORE;
    }
    // Unknown extension - return as-is
    return internalName;
}

using namespace core;

// ============================================================================
// Device Limits Conversion
// ============================================================================

GfxDeviceLimits wgpuLimitsToGfxDeviceLimits(const WGPULimits& limits)
{
    GfxDeviceLimits gfxLimits{};
    gfxLimits.minUniformBufferOffsetAlignment = limits.minUniformBufferOffsetAlignment;
    gfxLimits.minStorageBufferOffsetAlignment = limits.minStorageBufferOffsetAlignment;
    gfxLimits.maxUniformBufferBindingSize = static_cast<uint32_t>(limits.maxUniformBufferBindingSize);
    gfxLimits.maxStorageBufferBindingSize = static_cast<uint32_t>(limits.maxStorageBufferBindingSize);
    gfxLimits.maxBufferSize = limits.maxBufferSize;
    gfxLimits.maxTextureDimension1D = limits.maxTextureDimension1D;
    gfxLimits.maxTextureDimension2D = limits.maxTextureDimension2D;
    gfxLimits.maxTextureDimension3D = limits.maxTextureDimension3D;
    gfxLimits.maxTextureArrayLayers = limits.maxTextureArrayLayers;
    return gfxLimits;
}

// ============================================================================
// Type Conversion Functions
// ============================================================================

core::SemaphoreType gfxSemaphoreTypeToWebGPUSemaphoreType(GfxSemaphoreType gfxType)
{
    switch (gfxType) {
    case GFX_SEMAPHORE_TYPE_BINARY:
        return core::SemaphoreType::Binary;
    case GFX_SEMAPHORE_TYPE_TIMELINE:
        return core::SemaphoreType::Timeline;
    default:
        return core::SemaphoreType::Binary;
    }
}

core::ShaderSourceType gfxShaderSourceTypeToWebGPUShaderSourceType(GfxShaderSourceType type)
{
    switch (type) {
    case GFX_SHADER_SOURCE_WGSL:
        return core::ShaderSourceType::WGSL;
    case GFX_SHADER_SOURCE_SPIRV:
        return core::ShaderSourceType::SPIRV;
    default:
        return core::ShaderSourceType::WGSL; // WebGPU defaults to WGSL
    }
}

WGPUQueryType gfxQueryTypeToWebGPUQueryType(GfxQueryType type)
{
    switch (type) {
    case GFX_QUERY_TYPE_OCCLUSION:
        return WGPUQueryType_Occlusion;
    case GFX_QUERY_TYPE_TIMESTAMP:
        return WGPUQueryType_Timestamp;
    default:
        return WGPUQueryType_Occlusion;
    }
}

// ============================================================================
// Adapter Type Conversion
// ============================================================================

GfxAdapterType wgpuAdapterTypeToGfxAdapterType(WGPUAdapterType adapterType)
{
    switch (adapterType) {
    case WGPUAdapterType_DiscreteGPU:
        return GFX_ADAPTER_TYPE_DISCRETE_GPU;
    case WGPUAdapterType_IntegratedGPU:
        return GFX_ADAPTER_TYPE_INTEGRATED_GPU;
    case WGPUAdapterType_CPU:
        return GFX_ADAPTER_TYPE_CPU;
    case WGPUAdapterType_Unknown:
    default:
        return GFX_ADAPTER_TYPE_UNKNOWN;
    }
}

// ============================================================================
// Adapter Info Conversion
// ============================================================================

GfxAdapterInfo wgpuAdapterToGfxAdapterInfo(const core::AdapterInfo& info)
{
    GfxAdapterInfo adapterInfo{};
    adapterInfo.name = info.name.c_str();
    adapterInfo.driverDescription = info.driverDescription.c_str();
    adapterInfo.vendorID = info.vendorID;
    adapterInfo.deviceID = info.deviceID;
    adapterInfo.backend = GFX_BACKEND_WEBGPU;
    adapterInfo.adapterType = wgpuAdapterTypeToGfxAdapterType(info.adapterType);
    return adapterInfo;
}

// ============================================================================
// Queue Family Conversion
// ============================================================================

GfxQueueFamilyProperties wgpuQueueFamilyPropertiesToGfx(const core::QueueFamilyProperties& props)
{
    // Build flags based on capabilities
    uint32_t flags = 0;
    if (props.supportsGraphics) {
        flags |= GFX_QUEUE_FLAG_GRAPHICS;
    }
    if (props.supportsCompute) {
        flags |= GFX_QUEUE_FLAG_COMPUTE;
    }
    if (props.supportsTransfer) {
        flags |= GFX_QUEUE_FLAG_TRANSFER;
    }

    GfxQueueFamilyProperties gfxProps = {};
    gfxProps.flags = static_cast<GfxQueueFlags>(flags);
    gfxProps.queueCount = props.queueCount;
    return gfxProps;
}

// ============================================================================
// CreateInfo Conversion Functions - GfxDescriptor to Internal CreateInfo
// ============================================================================

core::AdapterCreateInfo gfxDescriptorToWebGPUAdapterCreateInfo(const GfxAdapterDescriptor* descriptor)
{
    core::AdapterCreateInfo createInfo{};

    if (descriptor) {
        // Handle adapter index if specified
        if (descriptor->adapterIndex != UINT32_MAX) {
            createInfo.adapterIndex = descriptor->adapterIndex;
            createInfo.powerPreference = WGPUPowerPreference_Undefined;
            createInfo.forceFallbackAdapter = false;
        } else {
            // Fall back to preference-based selection
            createInfo.adapterIndex = UINT32_MAX;
            switch (descriptor->preference) {
            case GFX_ADAPTER_PREFERENCE_LOW_POWER:
                createInfo.powerPreference = WGPUPowerPreference_LowPower;
                createInfo.forceFallbackAdapter = false;
                break;
            case GFX_ADAPTER_PREFERENCE_HIGH_PERFORMANCE:
                createInfo.powerPreference = WGPUPowerPreference_HighPerformance;
                createInfo.forceFallbackAdapter = false;
                break;
            case GFX_ADAPTER_PREFERENCE_SOFTWARE:
                createInfo.powerPreference = WGPUPowerPreference_Undefined;
                createInfo.forceFallbackAdapter = true;
                break;
            default:
                createInfo.powerPreference = WGPUPowerPreference_Undefined;
                createInfo.forceFallbackAdapter = false;
                break;
            }
        }
    } else {
        createInfo.powerPreference = WGPUPowerPreference_Undefined;
        createInfo.forceFallbackAdapter = false;
    }

    return createInfo;
}

core::InstanceCreateInfo gfxDescriptorToWebGPUInstanceCreateInfo(const GfxInstanceDescriptor* descriptor)
{
    core::InstanceCreateInfo createInfo{};

    if (descriptor) {
        createInfo.applicationName = descriptor->applicationName ? descriptor->applicationName : "Gfx Application";
        createInfo.applicationVersion = descriptor->applicationVersion;

        // Convert enabled extensions from const char** to std::vector<std::string>
        if (descriptor->enabledExtensions && descriptor->enabledExtensionCount > 0) {
            createInfo.enabledExtensions.reserve(descriptor->enabledExtensionCount);
            for (uint32_t i = 0; i < descriptor->enabledExtensionCount; ++i) {
                createInfo.enabledExtensions.push_back(descriptor->enabledExtensions[i]);
            }
        }
    } else {
        createInfo.applicationName = "Gfx Application";
        createInfo.applicationVersion = 1;
    }

    return createInfo;
}

core::DeviceCreateInfo gfxDescriptorToWebGPUDeviceCreateInfo(const GfxDeviceDescriptor* descriptor)
{
    core::DeviceCreateInfo createInfo{};

    if (descriptor) {
        // Convert enabled extensions from const char** to std::vector<std::string>
        if (descriptor->enabledExtensions && descriptor->enabledExtensionCount > 0) {
            createInfo.enabledExtensions.reserve(descriptor->enabledExtensionCount);
            for (uint32_t i = 0; i < descriptor->enabledExtensionCount; ++i) {
                createInfo.enabledExtensions.push_back(descriptor->enabledExtensions[i]);
            }
        }
    }

    return createInfo;
}

core::BufferCreateInfo gfxDescriptorToWebGPUBufferCreateInfo(const GfxBufferDescriptor* descriptor)
{
    core::BufferCreateInfo createInfo{};
    createInfo.size = descriptor->size;
    createInfo.usage = gfxBufferUsageToWGPU(descriptor->usage);
    createInfo.memoryProperties = descriptor->memoryProperties;
    return createInfo;
}

core::BufferImportInfo gfxExternalDescriptorToWebGPUBufferImportInfo(const GfxBufferImportDescriptor* descriptor)
{
    core::BufferImportInfo importInfo{};
    importInfo.size = descriptor->size;
    importInfo.usage = gfxBufferUsageToWGPU(descriptor->usage);
    importInfo.memoryProperties = 0; // External buffers have unknown/unspecified memory properties
    return importInfo;
}

core::TextureCreateInfo gfxDescriptorToWebGPUTextureCreateInfo(const GfxTextureDescriptor* descriptor)
{
    core::TextureCreateInfo createInfo{};
    createInfo.format = gfxFormatToWGPUFormat(descriptor->format);
    createInfo.size.width = descriptor->size.width;
    createInfo.size.height = descriptor->size.height;
    // For 3D textures, use depth; for 1D/2D textures, use arrayLayerCount
    createInfo.size.depthOrArrayLayers = (descriptor->type == GFX_TEXTURE_TYPE_3D)
        ? descriptor->size.depth
        : (descriptor->arrayLayerCount > 0 ? descriptor->arrayLayerCount : 1);
    createInfo.usage = gfxTextureUsageToWGPU(descriptor->usage);
    createInfo.sampleCount = descriptor->sampleCount;
    createInfo.mipLevelCount = descriptor->mipLevelCount;
    createInfo.dimension = gfxTextureTypeToWGPUTextureDimension(descriptor->type);
    createInfo.arrayLayers = descriptor->arrayLayerCount > 0 ? descriptor->arrayLayerCount : 1;
    return createInfo;
}

core::TextureImportInfo gfxExternalDescriptorToWebGPUTextureImportInfo(const GfxTextureImportDescriptor* descriptor)
{
    core::TextureImportInfo importInfo{};
    importInfo.format = gfxFormatToWGPUFormat(descriptor->format);
    importInfo.size.width = descriptor->size.width;
    importInfo.size.height = descriptor->size.height;
    // For 3D textures, use depth; for 1D/2D textures, use arrayLayerCount
    importInfo.size.depthOrArrayLayers = (descriptor->type == GFX_TEXTURE_TYPE_3D)
        ? descriptor->size.depth
        : (descriptor->arrayLayerCount > 0 ? descriptor->arrayLayerCount : 1);
    importInfo.usage = gfxTextureUsageToWGPU(descriptor->usage);
    importInfo.sampleCount = descriptor->sampleCount;
    importInfo.mipLevelCount = descriptor->mipLevelCount;
    importInfo.dimension = gfxTextureTypeToWGPUTextureDimension(descriptor->type);
    importInfo.arrayLayers = descriptor->arrayLayerCount > 0 ? descriptor->arrayLayerCount : 1;
    return importInfo;
}

core::TextureViewCreateInfo gfxDescriptorToWebGPUTextureViewCreateInfo(const GfxTextureViewDescriptor* descriptor)
{
    core::TextureViewCreateInfo createInfo{};
    createInfo.viewDimension = gfxTextureViewTypeToWGPU(descriptor->viewType);
    createInfo.format = gfxFormatToWGPUFormat(descriptor->format);
    createInfo.baseMipLevel = descriptor->baseMipLevel;
    createInfo.mipLevelCount = descriptor->mipLevelCount;
    createInfo.baseArrayLayer = descriptor->baseArrayLayer;
    createInfo.arrayLayerCount = descriptor->arrayLayerCount;
    return createInfo;
}

core::ShaderSourceType gfxShaderSourceTypeToWebGPU(GfxShaderSourceType sourceType)
{
    switch (sourceType) {
    case GFX_SHADER_SOURCE_SPIRV:
        return core::ShaderSourceType::SPIRV;
    case GFX_SHADER_SOURCE_WGSL:
    default:
        return core::ShaderSourceType::WGSL;
    }
}

core::ShaderCreateInfo gfxDescriptorToWebGPUShaderCreateInfo(const GfxShaderDescriptor* descriptor)
{
    core::ShaderCreateInfo createInfo{};
    createInfo.sourceType = gfxShaderSourceTypeToWebGPU(descriptor->sourceType);
    createInfo.code = descriptor->code;
    createInfo.codeSize = descriptor->codeSize;
    createInfo.entryPoint = descriptor->entryPoint;
    return createInfo;
}

core::SamplerCreateInfo gfxDescriptorToWebGPUSamplerCreateInfo(const GfxSamplerDescriptor* descriptor)
{
    core::SamplerCreateInfo createInfo{};
    createInfo.addressModeU = gfxAddressModeToWGPU(descriptor->addressModeU);
    createInfo.addressModeV = gfxAddressModeToWGPU(descriptor->addressModeV);
    createInfo.addressModeW = gfxAddressModeToWGPU(descriptor->addressModeW);
    createInfo.magFilter = gfxFilterModeToWGPU(descriptor->magFilter);
    createInfo.minFilter = gfxFilterModeToWGPU(descriptor->minFilter);
    createInfo.mipmapFilter = gfxMipmapFilterModeToWGPU(descriptor->mipmapFilter);
    createInfo.lodMinClamp = descriptor->lodMinClamp;
    createInfo.lodMaxClamp = descriptor->lodMaxClamp;
    createInfo.maxAnisotropy = descriptor->maxAnisotropy;
    createInfo.compareFunction = gfxCompareFunctionToWGPU(descriptor->compare);
    return createInfo;
}

core::SemaphoreCreateInfo gfxDescriptorToWebGPUSemaphoreCreateInfo(const GfxSemaphoreDescriptor* descriptor)
{
    core::SemaphoreCreateInfo createInfo{};

    if (descriptor) {
        createInfo.type = gfxSemaphoreTypeToWebGPUSemaphoreType(descriptor->type);
        createInfo.initialValue = descriptor->initialValue;
    } else {
        createInfo.type = core::SemaphoreType::Binary;
        createInfo.initialValue = 0;
    }

    return createInfo;
}

core::FenceCreateInfo gfxDescriptorToWebGPUFenceCreateInfo(const GfxFenceDescriptor* descriptor)
{
    core::FenceCreateInfo createInfo{};

    if (descriptor) {
        createInfo.signaled = descriptor->signaled;
    } else {
        createInfo.signaled = false;
    }

    return createInfo;
}

core::PlatformWindowHandle gfxWindowHandleToWebGPUPlatformWindowHandle(const GfxPlatformWindowHandle& gfxHandle)
{
    core::PlatformWindowHandle handle{};

    switch (gfxHandle.windowingSystem) {
    case GFX_WINDOWING_SYSTEM_XCB:
        handle.platform = core::PlatformWindowHandle::Platform::Xcb;
        handle.handle.xcb.connection = gfxHandle.xcb.connection;
        handle.handle.xcb.window = gfxHandle.xcb.window;
        break;
    case GFX_WINDOWING_SYSTEM_XLIB:
        handle.platform = core::PlatformWindowHandle::Platform::Xlib;
        handle.handle.xlib.display = gfxHandle.xlib.display;
        handle.handle.xlib.window = gfxHandle.xlib.window;
        break;
    case GFX_WINDOWING_SYSTEM_WAYLAND:
        handle.platform = core::PlatformWindowHandle::Platform::Wayland;
        handle.handle.wayland.display = gfxHandle.wayland.display;
        handle.handle.wayland.surface = gfxHandle.wayland.surface;
        break;
    case GFX_WINDOWING_SYSTEM_WIN32:
        handle.platform = core::PlatformWindowHandle::Platform::Win32;
        handle.handle.win32.hinstance = gfxHandle.win32.hinstance;
        handle.handle.win32.hwnd = gfxHandle.win32.hwnd;
        break;
    case GFX_WINDOWING_SYSTEM_METAL:
        handle.platform = core::PlatformWindowHandle::Platform::Metal;
        handle.handle.metal.layer = gfxHandle.metal.layer;
        break;
    case GFX_WINDOWING_SYSTEM_EMSCRIPTEN:
        handle.platform = core::PlatformWindowHandle::Platform::Emscripten;
        handle.handle.emscripten.canvasSelector = gfxHandle.emscripten.canvasSelector;
        break;
    case GFX_WINDOWING_SYSTEM_ANDROID:
        handle.platform = core::PlatformWindowHandle::Platform::Android;
        handle.handle.android.window = gfxHandle.android.window;
        break;
    default:
        handle.platform = core::PlatformWindowHandle::Platform::Unknown;
        break;
    }

    return handle;
}

core::SurfaceCreateInfo gfxDescriptorToWebGPUSurfaceCreateInfo(const GfxSurfaceDescriptor* descriptor)
{
    core::SurfaceCreateInfo createInfo{};
    if (descriptor) {
        createInfo.windowHandle = gfxWindowHandleToWebGPUPlatformWindowHandle(descriptor->windowHandle);
    }
    return createInfo;
}

core::SwapchainCreateInfo gfxDescriptorToWebGPUSwapchainCreateInfo(const GfxSwapchainDescriptor* descriptor)
{
    core::SwapchainCreateInfo createInfo{};
    createInfo.width = descriptor->extent.width;
    createInfo.height = descriptor->extent.height;
    createInfo.format = gfxFormatToWGPUFormat(descriptor->format);
    createInfo.usage = gfxTextureUsageToWGPU(descriptor->usage);
    createInfo.presentMode = gfxPresentModeToWGPU(descriptor->presentMode);
    createInfo.imageCount = descriptor->imageCount;
    return createInfo;
}

core::BindGroupLayoutCreateInfo gfxDescriptorToWebGPUBindGroupLayoutCreateInfo(const GfxBindGroupLayoutDescriptor* descriptor)
{
    core::BindGroupLayoutCreateInfo createInfo{};

    for (uint32_t i = 0; i < descriptor->entryCount; ++i) {
        const auto& entry = descriptor->entries[i];

        core::BindGroupLayoutEntry layoutEntry{};
        layoutEntry.binding = entry.binding;

        // Convert visibility flags (bitwise)
        layoutEntry.visibility = WGPUShaderStage_None;
        if (entry.visibility & GFX_SHADER_STAGE_VERTEX) {
            layoutEntry.visibility |= WGPUShaderStage_Vertex;
        }
        if (entry.visibility & GFX_SHADER_STAGE_FRAGMENT) {
            layoutEntry.visibility |= WGPUShaderStage_Fragment;
        }
        if (entry.visibility & GFX_SHADER_STAGE_COMPUTE) {
            layoutEntry.visibility |= WGPUShaderStage_Compute;
        }

        // Initialize all to Undefined - only set the one we need
        layoutEntry.bufferType = WGPUBufferBindingType_Undefined;
        layoutEntry.bufferHasDynamicOffset = WGPU_FALSE;
        layoutEntry.bufferMinBindingSize = 0;

        layoutEntry.samplerType = WGPUSamplerBindingType_Undefined;

        layoutEntry.textureSampleType = WGPUTextureSampleType_Undefined;
        layoutEntry.textureViewDimension = WGPUTextureViewDimension_Undefined;
        layoutEntry.textureMultisampled = WGPU_FALSE;

        layoutEntry.storageTextureAccess = WGPUStorageTextureAccess_Undefined;
        layoutEntry.storageTextureFormat = WGPUTextureFormat_Undefined;
        layoutEntry.storageTextureViewDimension = WGPUTextureViewDimension_Undefined;

        // Convert GfxBindingType to WebGPU binding types
        switch (entry.type) {
        case GFX_BINDING_TYPE_BUFFER:
            layoutEntry.bufferType = WGPUBufferBindingType_Uniform;
            layoutEntry.bufferHasDynamicOffset = entry.buffer.hasDynamicOffset ? WGPU_TRUE : WGPU_FALSE;
            layoutEntry.bufferMinBindingSize = entry.buffer.minBindingSize;
            break;
        case GFX_BINDING_TYPE_SAMPLER:
            layoutEntry.samplerType = entry.sampler.comparison
                ? WGPUSamplerBindingType_Comparison
                : WGPUSamplerBindingType_Filtering;
            break;
        case GFX_BINDING_TYPE_TEXTURE:
            layoutEntry.textureSampleType = gfxTextureSampleTypeToWGPU(entry.texture.sampleType);
            layoutEntry.textureViewDimension = gfxTextureViewTypeToWGPU(entry.texture.viewDimension);
            layoutEntry.textureMultisampled = entry.texture.multisampled ? WGPU_TRUE : WGPU_FALSE;
            break;
        case GFX_BINDING_TYPE_STORAGE_TEXTURE:
            layoutEntry.storageTextureAccess = entry.storageTexture.writeOnly
                ? WGPUStorageTextureAccess_WriteOnly
                : WGPUStorageTextureAccess_ReadOnly;
            layoutEntry.storageTextureFormat = gfxFormatToWGPUFormat(entry.storageTexture.format);
            layoutEntry.storageTextureViewDimension = gfxTextureViewTypeToWGPU(entry.storageTexture.viewDimension);
            break;
        default:
            // Unknown type - leave as Undefined
            break;
        }

        createInfo.entries.push_back(layoutEntry);
    }

    return createInfo;
}

core::BindGroupCreateInfo gfxDescriptorToWebGPUBindGroupCreateInfo(const GfxBindGroupDescriptor* descriptor, WGPUBindGroupLayout layout)
{
    core::BindGroupCreateInfo createInfo{};
    createInfo.layout = layout;

    if (descriptor->entryCount > 0 && descriptor->entries) {
        createInfo.entries.reserve(descriptor->entryCount);

        for (uint32_t i = 0; i < descriptor->entryCount; ++i) {
            const auto& entry = descriptor->entries[i];
            core::BindGroupEntry bindEntry{};

            bindEntry.binding = entry.binding;

            switch (entry.type) {
            case GFX_BIND_GROUP_ENTRY_TYPE_BUFFER: {
                auto* buffer = toNative<core::Buffer>(entry.resource.buffer.buffer);
                bindEntry.buffer = buffer->handle();
                bindEntry.bufferOffset = entry.resource.buffer.offset;
                bindEntry.bufferSize = entry.resource.buffer.size;
                break;
            }
            case GFX_BIND_GROUP_ENTRY_TYPE_SAMPLER: {
                auto* sampler = toNative<core::Sampler>(entry.resource.sampler);
                bindEntry.sampler = sampler->handle();
                break;
            }
            case GFX_BIND_GROUP_ENTRY_TYPE_TEXTURE_VIEW: {
                auto* textureView = toNative<core::TextureView>(entry.resource.textureView);
                bindEntry.textureView = textureView->handle();
                break;
            }
            default:
                // Unknown type - skip
                break;
            }

            createInfo.entries.push_back(bindEntry);
        }
    }

    return createInfo;
}

core::RenderPipelineCreateInfo gfxDescriptorToWebGPURenderPipelineCreateInfo(const GfxRenderPipelineDescriptor* descriptor)
{
    core::RenderPipelineCreateInfo createInfo{};

    // Extract bind group layouts
    if (descriptor->bindGroupLayoutCount > 0 && descriptor->bindGroupLayouts) {
        createInfo.bindGroupLayouts.reserve(descriptor->bindGroupLayoutCount);
        for (uint32_t i = 0; i < descriptor->bindGroupLayoutCount; ++i) {
            auto* layout = toNative<BindGroupLayout>(descriptor->bindGroupLayouts[i]);
            createInfo.bindGroupLayouts.push_back(layout->handle());
        }
    }

    // Vertex state
    auto* vertexShader = toNative<Shader>(descriptor->vertex->module);
    createInfo.vertex.module = vertexShader->handle();
    createInfo.vertex.entryPoint = descriptor->vertex->entryPoint;

    // Convert vertex buffers
    if (descriptor->vertex->bufferCount > 0) {
        createInfo.vertex.buffers.reserve(descriptor->vertex->bufferCount);

        for (uint32_t i = 0; i < descriptor->vertex->bufferCount; ++i) {
            const auto& buffer = descriptor->vertex->buffers[i];
            VertexBufferLayout vbLayout{};
            vbLayout.arrayStride = buffer.arrayStride;
            vbLayout.stepMode = gfxVertexStepModeToWGPU(buffer.stepMode);

            // Convert attributes
            vbLayout.attributes.reserve(buffer.attributeCount);
            for (uint32_t j = 0; j < buffer.attributeCount; ++j) {
                const auto& attr = buffer.attributes[j];
                VertexAttribute vbAttr{};
                vbAttr.format = gfxFormatToWGPUVertexFormat(attr.format);
                vbAttr.offset = attr.offset;
                vbAttr.shaderLocation = attr.shaderLocation;
                vbLayout.attributes.push_back(vbAttr);
            }

            createInfo.vertex.buffers.push_back(std::move(vbLayout));
        }
    }

    // Fragment state (optional)
    if (descriptor->fragment) {
        FragmentState fragState{};
        auto* fragmentShader = toNative<Shader>(descriptor->fragment->module);
        fragState.module = fragmentShader->handle();
        fragState.entryPoint = descriptor->fragment->entryPoint;

        // RenderPass is mandatory - always extract formats from it
        auto* renderPass = toNative<RenderPass>(descriptor->renderPass);
        const auto& rpInfo = renderPass->getCreateInfo();

        // Use render pass formats, blend/writeMask from fragment descriptor if provided
        fragState.targets.reserve(rpInfo.colorAttachments.size());

        for (uint32_t i = 0; i < rpInfo.colorAttachments.size(); ++i) {
            ColorTargetState colorTarget{};
            colorTarget.format = rpInfo.colorAttachments[i].format;

            // Use writeMask and blend from fragment descriptor if available
            if (descriptor->fragment->targetCount > i) {
                const auto& target = descriptor->fragment->targets[i];
                colorTarget.writeMask = target.writeMask;

                if (target.blend) {
                    BlendState blend{};
                    blend.color.operation = gfxBlendOperationToWGPU(target.blend->color.operation);
                    blend.color.srcFactor = gfxBlendFactorToWGPU(target.blend->color.srcFactor);
                    blend.color.dstFactor = gfxBlendFactorToWGPU(target.blend->color.dstFactor);
                    blend.alpha.operation = gfxBlendOperationToWGPU(target.blend->alpha.operation);
                    blend.alpha.srcFactor = gfxBlendFactorToWGPU(target.blend->alpha.srcFactor);
                    blend.alpha.dstFactor = gfxBlendFactorToWGPU(target.blend->alpha.dstFactor);
                    colorTarget.blend = blend;
                }
            } else {
                // Default write mask if not specified
                colorTarget.writeMask = GFX_COLOR_WRITE_MASK_ALL;
            }

            fragState.targets.push_back(std::move(colorTarget));
        }

        createInfo.fragment = std::move(fragState);
    }

    // Primitive state
    createInfo.primitive.topology = gfxPrimitiveTopologyToWGPU(descriptor->primitive->topology);
    createInfo.primitive.frontFace = gfxFrontFaceToWGPU(descriptor->primitive->frontFace);
    createInfo.primitive.cullMode = gfxCullModeToWGPU(descriptor->primitive->cullMode);
    createInfo.primitive.stripIndexFormat = gfxIndexFormatToWGPU(descriptor->primitive->stripIndexFormat);

    // Depth/stencil state (optional)
    if (descriptor->depthStencil) {
        DepthStencilState dsState{};
        dsState.format = gfxFormatToWGPUFormat(descriptor->depthStencil->format);
        dsState.depthWriteEnabled = descriptor->depthStencil->depthWriteEnabled;
        dsState.depthCompare = gfxCompareFunctionToWGPU(descriptor->depthStencil->depthCompare);

        // Stencil state
        dsState.stencilFront.compare = gfxCompareFunctionToWGPU(descriptor->depthStencil->stencilFront.compare);
        dsState.stencilFront.failOp = gfxStencilOperationToWGPU(descriptor->depthStencil->stencilFront.failOp);
        dsState.stencilFront.depthFailOp = gfxStencilOperationToWGPU(descriptor->depthStencil->stencilFront.depthFailOp);
        dsState.stencilFront.passOp = gfxStencilOperationToWGPU(descriptor->depthStencil->stencilFront.passOp);

        dsState.stencilBack.compare = gfxCompareFunctionToWGPU(descriptor->depthStencil->stencilBack.compare);
        dsState.stencilBack.failOp = gfxStencilOperationToWGPU(descriptor->depthStencil->stencilBack.failOp);
        dsState.stencilBack.depthFailOp = gfxStencilOperationToWGPU(descriptor->depthStencil->stencilBack.depthFailOp);
        dsState.stencilBack.passOp = gfxStencilOperationToWGPU(descriptor->depthStencil->stencilBack.passOp);

        dsState.stencilReadMask = descriptor->depthStencil->stencilReadMask;
        dsState.stencilWriteMask = descriptor->depthStencil->stencilWriteMask;
        dsState.depthBias = descriptor->depthStencil->depthBias;
        dsState.depthBiasSlopeScale = descriptor->depthStencil->depthBiasSlopeScale;
        dsState.depthBiasClamp = descriptor->depthStencil->depthBiasClamp;

        createInfo.depthStencil = std::move(dsState);
    }

    // Multisample state
    createInfo.sampleCount = descriptor->sampleCount;

    return createInfo;
}

core::ComputePipelineCreateInfo gfxDescriptorToWebGPUComputePipelineCreateInfo(const GfxComputePipelineDescriptor* descriptor)
{
    core::ComputePipelineCreateInfo createInfo{};

    // Extract bind group layouts
    if (descriptor->bindGroupLayoutCount > 0 && descriptor->bindGroupLayouts) {
        createInfo.bindGroupLayouts.reserve(descriptor->bindGroupLayoutCount);
        for (uint32_t i = 0; i < descriptor->bindGroupLayoutCount; ++i) {
            auto* layout = toNative<BindGroupLayout>(descriptor->bindGroupLayouts[i]);
            createInfo.bindGroupLayouts.push_back(layout->handle());
        }
    }

    // Extract shader module
    auto* shader = toNative<Shader>(descriptor->compute);
    createInfo.module = shader->handle();
    createInfo.entryPoint = descriptor->entryPoint;

    return createInfo;
}

core::CommandEncoderCreateInfo gfxDescriptorToWebGPUCommandEncoderCreateInfo(const GfxCommandEncoderDescriptor* descriptor)
{
    core::CommandEncoderCreateInfo createInfo;
    createInfo.label = descriptor->label;
    return createInfo;
}

core::SubmitInfo gfxDescriptorToWebGPUSubmitInfo(const GfxSubmitDescriptor* descriptor)
{
    core::SubmitInfo submitInfo{};
    // Note: Array pointer conversions use reinterpret_cast as toNative<> is for individual objects
    submitInfo.commandEncoders = reinterpret_cast<CommandEncoder**>(descriptor->commandEncoders);
    submitInfo.commandEncoderCount = descriptor->commandEncoderCount;
    submitInfo.signalFence = toNative<Fence>(descriptor->signalFence);
    submitInfo.waitSemaphores = reinterpret_cast<Semaphore**>(descriptor->waitSemaphores);
    submitInfo.waitValues = descriptor->waitValues;
    submitInfo.waitSemaphoreCount = descriptor->waitSemaphoreCount;
    submitInfo.signalSemaphores = reinterpret_cast<Semaphore**>(descriptor->signalSemaphores);
    submitInfo.signalValues = descriptor->signalValues;
    submitInfo.signalSemaphoreCount = descriptor->signalSemaphoreCount;
    return submitInfo;
}

core::QuerySetCreateInfo gfxDescriptorToWebGPUQuerySetCreateInfo(const GfxQuerySetDescriptor* descriptor)
{
    core::QuerySetCreateInfo createInfo{};

    if (descriptor) {
        createInfo.label = descriptor->label;
        createInfo.type = gfxQueryTypeToWebGPUQueryType(descriptor->type);
        createInfo.count = descriptor->count;
    } else {
        createInfo.type = WGPUQueryType_Occlusion;
        createInfo.count = 1;
    }

    return createInfo;
}

// ============================================================================
// Reverse Conversions - Internal to Gfx API types
// ============================================================================

GfxBufferUsageFlags webgpuBufferUsageToGfxBufferUsage(WGPUBufferUsage usage)
{
    uint32_t gfxUsage = GFX_BUFFER_USAGE_NONE;
    if (usage & WGPUBufferUsage_MapRead) {
        gfxUsage |= GFX_BUFFER_USAGE_MAP_READ;
    }
    if (usage & WGPUBufferUsage_MapWrite) {
        gfxUsage |= GFX_BUFFER_USAGE_MAP_WRITE;
    }
    if (usage & WGPUBufferUsage_CopySrc) {
        gfxUsage |= GFX_BUFFER_USAGE_COPY_SRC;
    }
    if (usage & WGPUBufferUsage_CopyDst) {
        gfxUsage |= GFX_BUFFER_USAGE_COPY_DST;
    }
    if (usage & WGPUBufferUsage_Index) {
        gfxUsage |= GFX_BUFFER_USAGE_INDEX;
    }
    if (usage & WGPUBufferUsage_Vertex) {
        gfxUsage |= GFX_BUFFER_USAGE_VERTEX;
    }
    if (usage & WGPUBufferUsage_Uniform) {
        gfxUsage |= GFX_BUFFER_USAGE_UNIFORM;
    }
    if (usage & WGPUBufferUsage_Storage) {
        gfxUsage |= GFX_BUFFER_USAGE_STORAGE;
    }
    if (usage & WGPUBufferUsage_Indirect) {
        gfxUsage |= GFX_BUFFER_USAGE_INDIRECT;
    }
    return gfxUsage;
}

GfxSemaphoreType webgpuSemaphoreTypeToGfxSemaphoreType(core::SemaphoreType type)
{
    switch (type) {
    case core::SemaphoreType::Binary:
        return GFX_SEMAPHORE_TYPE_BINARY;
    case core::SemaphoreType::Timeline:
        return GFX_SEMAPHORE_TYPE_TIMELINE;
    default:
        return GFX_SEMAPHORE_TYPE_BINARY;
    }
}

GfxTextureInfo wgpuTextureInfoToGfxTextureInfo(const core::TextureInfo& info)
{
    GfxTextureInfo gfxInfo{};
    gfxInfo.type = wgpuTextureDimensionToGfxTextureType(info.dimension);
    gfxInfo.size = wgpuExtent3DToGfxExtent3D(info.size);
    gfxInfo.arrayLayerCount = info.arrayLayers;
    gfxInfo.mipLevelCount = info.mipLevels;
    gfxInfo.sampleCount = wgpuSampleCountToGfxSampleCount(info.sampleCount);
    gfxInfo.format = wgpuFormatToGfxFormat(info.format);
    gfxInfo.usage = wgpuTextureUsageToGfxTextureUsage(info.usage);
    return gfxInfo;
}

GfxSurfaceInfo wgpuSurfaceInfoToGfxSurfaceInfo(const core::SurfaceInfo& surfaceInfo)
{
    GfxSurfaceInfo info{};
    info.minImageCount = surfaceInfo.minImageCount;
    info.maxImageCount = surfaceInfo.maxImageCount;
    info.minExtent = { surfaceInfo.minWidth, surfaceInfo.minHeight };
    info.maxExtent = { surfaceInfo.maxWidth, surfaceInfo.maxHeight };
    return info;
}

GfxSwapchainInfo wgpuSwapchainInfoToGfxSwapchainInfo(const core::SwapchainInfo& info)
{
    GfxSwapchainInfo gfxInfo{};
    gfxInfo.extent = { info.width, info.height };
    gfxInfo.format = wgpuFormatToGfxFormat(info.format);
    gfxInfo.imageCount = info.imageCount;
    gfxInfo.presentMode = wgpuPresentModeToGfxPresentMode(info.presentMode);
    return gfxInfo;
}

GfxBufferInfo wgpuBufferToGfxBufferInfo(const core::BufferInfo& info)
{
    GfxBufferInfo gfxInfo{};
    gfxInfo.size = info.size;
    gfxInfo.usage = webgpuBufferUsageToGfxBufferUsage(info.usage);
    gfxInfo.memoryProperties = info.memoryProperties;
    return gfxInfo;
}

// ============================================================================
// Conversion Function Implementations
// ============================================================================

WGPUStringView gfxStringView(const char* str)
{
    if (!str) {
        return WGPUStringView{ nullptr, WGPU_STRLEN };
    }
    return WGPUStringView{ str, WGPU_STRLEN };
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
    case GFX_TEXTURE_FORMAT_STENCIL8:
        return WGPUTextureFormat_Stencil8;
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
    case WGPUTextureFormat_Stencil8:
        return GFX_TEXTURE_FORMAT_STENCIL8;
    case WGPUTextureFormat_Depth24PlusStencil8:
        return GFX_TEXTURE_FORMAT_DEPTH24_PLUS_STENCIL8;
    case WGPUTextureFormat_Depth32FloatStencil8:
        return GFX_TEXTURE_FORMAT_DEPTH32_FLOAT_STENCIL8;
    default:
        return GFX_TEXTURE_FORMAT_UNDEFINED;
    }
}

GfxPresentMode wgpuPresentModeToGfxPresentMode(WGPUPresentMode mode)
{
    switch (mode) {
    case WGPUPresentMode_Immediate:
        return GFX_PRESENT_MODE_IMMEDIATE;
    case WGPUPresentMode_Mailbox:
        return GFX_PRESENT_MODE_MAILBOX;
    case WGPUPresentMode_Fifo:
        return GFX_PRESENT_MODE_FIFO;
    case WGPUPresentMode_FifoRelaxed:
        return GFX_PRESENT_MODE_FIFO_RELAXED;
    default:
        return GFX_PRESENT_MODE_FIFO;
    }
}

GfxSampleCount wgpuSampleCountToGfxSampleCount(uint32_t sampleCount)
{
    switch (sampleCount) {
    case 1:
        return GFX_SAMPLE_COUNT_1;
    case 2:
        return GFX_SAMPLE_COUNT_2;
    case 4:
        return GFX_SAMPLE_COUNT_4;
    case 8:
        return GFX_SAMPLE_COUNT_8;
    case 16:
        return GFX_SAMPLE_COUNT_16;
    case 32:
        return GFX_SAMPLE_COUNT_32;
    case 64:
        return GFX_SAMPLE_COUNT_64;
    default:
        return GFX_SAMPLE_COUNT_1;
    }
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

bool formatHasStencil(GfxTextureFormat format)
{
    switch (format) {
    case GFX_TEXTURE_FORMAT_STENCIL8:
    case GFX_TEXTURE_FORMAT_DEPTH24_PLUS_STENCIL8:
    case GFX_TEXTURE_FORMAT_DEPTH32_FLOAT_STENCIL8:
        return true;
    default:
        return false;
    }
}

WGPULoadOp gfxLoadOpToWGPULoadOp(GfxLoadOp loadOp)
{
    switch (loadOp) {
    case GFX_LOAD_OP_LOAD:
        return WGPULoadOp_Load;
    case GFX_LOAD_OP_CLEAR:
        return WGPULoadOp_Clear;
    case GFX_LOAD_OP_DONT_CARE:
        return WGPULoadOp_Undefined;
    default:
        return WGPULoadOp_Undefined;
    }
}

WGPUStoreOp gfxStoreOpToWGPUStoreOp(GfxStoreOp storeOp)
{
    switch (storeOp) {
    case GFX_STORE_OP_STORE:
        return WGPUStoreOp_Store;
    case GFX_STORE_OP_DONT_CARE:
        return WGPUStoreOp_Discard;
    default:
        return WGPUStoreOp_Undefined;
    }
}

WGPUBufferUsage gfxBufferUsageToWGPU(GfxBufferUsageFlags usage)
{
    WGPUBufferUsage wgpuUsage = WGPUBufferUsage_None;
    if (usage & GFX_BUFFER_USAGE_MAP_READ) {
        wgpuUsage |= WGPUBufferUsage_MapRead;
    }
    if (usage & GFX_BUFFER_USAGE_MAP_WRITE) {
        wgpuUsage |= WGPUBufferUsage_MapWrite;
    }
    if (usage & GFX_BUFFER_USAGE_COPY_SRC) {
        wgpuUsage |= WGPUBufferUsage_CopySrc;
    }
    if (usage & GFX_BUFFER_USAGE_COPY_DST) {
        wgpuUsage |= WGPUBufferUsage_CopyDst;
    }
    if (usage & GFX_BUFFER_USAGE_INDEX) {
        wgpuUsage |= WGPUBufferUsage_Index;
    }
    if (usage & GFX_BUFFER_USAGE_VERTEX) {
        wgpuUsage |= WGPUBufferUsage_Vertex;
    }
    if (usage & GFX_BUFFER_USAGE_UNIFORM) {
        wgpuUsage |= WGPUBufferUsage_Uniform;
    }
    if (usage & GFX_BUFFER_USAGE_STORAGE) {
        wgpuUsage |= WGPUBufferUsage_Storage;
    }
    if (usage & GFX_BUFFER_USAGE_INDIRECT) {
        wgpuUsage |= WGPUBufferUsage_Indirect;
    }
    return wgpuUsage;
}

WGPUTextureUsage gfxTextureUsageToWGPU(GfxTextureUsageFlags usage)
{
    WGPUTextureUsage wgpuUsage = WGPUTextureUsage_None;
    if (usage & GFX_TEXTURE_USAGE_COPY_SRC) {
        wgpuUsage |= WGPUTextureUsage_CopySrc;
    }
    if (usage & GFX_TEXTURE_USAGE_COPY_DST) {
        wgpuUsage |= WGPUTextureUsage_CopyDst;
    }
    if (usage & GFX_TEXTURE_USAGE_TEXTURE_BINDING) {
        wgpuUsage |= WGPUTextureUsage_TextureBinding;
    }
    if (usage & GFX_TEXTURE_USAGE_STORAGE_BINDING) {
        wgpuUsage |= WGPUTextureUsage_StorageBinding;
    }
    if (usage & GFX_TEXTURE_USAGE_RENDER_ATTACHMENT) {
        wgpuUsage |= WGPUTextureUsage_RenderAttachment;
    }
    return wgpuUsage;
}

GfxTextureUsageFlags wgpuTextureUsageToGfxTextureUsage(WGPUTextureUsage usage)
{
    uint32_t gfxUsage = 0;
    if (usage & WGPUTextureUsage_CopySrc) {
        gfxUsage |= GFX_TEXTURE_USAGE_COPY_SRC;
    }
    if (usage & WGPUTextureUsage_CopyDst) {
        gfxUsage |= GFX_TEXTURE_USAGE_COPY_DST;
    }
    if (usage & WGPUTextureUsage_TextureBinding) {
        gfxUsage |= GFX_TEXTURE_USAGE_TEXTURE_BINDING;
    }
    if (usage & WGPUTextureUsage_StorageBinding) {
        gfxUsage |= GFX_TEXTURE_USAGE_STORAGE_BINDING;
    }
    if (usage & WGPUTextureUsage_RenderAttachment) {
        gfxUsage |= GFX_TEXTURE_USAGE_RENDER_ATTACHMENT;
    }
    return gfxUsage;
}

WGPUAddressMode gfxAddressModeToWGPU(GfxAddressMode mode)
{
    switch (mode) {
    case GFX_ADDRESS_MODE_REPEAT:
        return WGPUAddressMode_Repeat;
    case GFX_ADDRESS_MODE_MIRROR_REPEAT:
        return WGPUAddressMode_MirrorRepeat;
    case GFX_ADDRESS_MODE_CLAMP_TO_EDGE:
        return WGPUAddressMode_ClampToEdge;
    default:
        return WGPUAddressMode_Undefined;
    }
}

WGPUFilterMode gfxFilterModeToWGPU(GfxFilterMode mode)
{
    return (mode == GFX_FILTER_MODE_LINEAR) ? WGPUFilterMode_Linear : WGPUFilterMode_Nearest;
}

WGPUMipmapFilterMode gfxMipmapFilterModeToWGPU(GfxFilterMode mode)
{
    return (mode == GFX_FILTER_MODE_LINEAR) ? WGPUMipmapFilterMode_Linear : WGPUMipmapFilterMode_Nearest;
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
        return WGPUPrimitiveTopology_Undefined;
    }
}

WGPUFrontFace gfxFrontFaceToWGPU(GfxFrontFace frontFace)
{
    return (frontFace == GFX_FRONT_FACE_COUNTER_CLOCKWISE) ? WGPUFrontFace_CCW : WGPUFrontFace_CW;
}

WGPUCullMode gfxCullModeToWGPU(GfxCullMode cullMode)
{
    switch (cullMode) {
    case GFX_CULL_MODE_NONE:
        return WGPUCullMode_None;
    case GFX_CULL_MODE_FRONT:
        return WGPUCullMode_Front;
    case GFX_CULL_MODE_BACK:
        return WGPUCullMode_Back;
    default:
        return WGPUCullMode_Undefined;
    }
}

WGPUIndexFormat gfxIndexFormatToWGPU(GfxIndexFormat format)
{
    switch (format) {
    case GFX_INDEX_FORMAT_UINT16:
        return WGPUIndexFormat_Uint16;
    case GFX_INDEX_FORMAT_UINT32:
        return WGPUIndexFormat_Uint32;
    case GFX_INDEX_FORMAT_UNDEFINED:
    default:
        return WGPUIndexFormat_Undefined;
    }
}

WGPUVertexStepMode gfxVertexStepModeToWGPU(GfxVertexStepMode mode)
{
    switch (mode) {
    case GFX_VERTEX_STEP_MODE_INSTANCE:
        return WGPUVertexStepMode_Instance;
    case GFX_VERTEX_STEP_MODE_VERTEX:
    default:
        return WGPUVertexStepMode_Vertex;
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
        return WGPUBlendOperation_Undefined;
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
        return WGPUCompareFunction_Undefined;
    }
}

WGPUStencilOperation gfxStencilOperationToWGPU(GfxStencilOperation op)
{
    switch (op) {
    case GFX_STENCIL_OPERATION_KEEP:
        return WGPUStencilOperation_Keep;
    case GFX_STENCIL_OPERATION_ZERO:
        return WGPUStencilOperation_Zero;
    case GFX_STENCIL_OPERATION_REPLACE:
        return WGPUStencilOperation_Replace;
    case GFX_STENCIL_OPERATION_INVERT:
        return WGPUStencilOperation_Invert;
    case GFX_STENCIL_OPERATION_INCREMENT_CLAMP:
        return WGPUStencilOperation_IncrementClamp;
    case GFX_STENCIL_OPERATION_DECREMENT_CLAMP:
        return WGPUStencilOperation_DecrementClamp;
    case GFX_STENCIL_OPERATION_INCREMENT_WRAP:
        return WGPUStencilOperation_IncrementWrap;
    case GFX_STENCIL_OPERATION_DECREMENT_WRAP:
        return WGPUStencilOperation_DecrementWrap;
    default:
        return WGPUStencilOperation_Undefined;
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
        return WGPUTextureSampleType_Undefined;
    }
}

WGPUVertexFormat gfxFormatToWGPUVertexFormat(GfxTextureFormat format)
{
    switch (format) {
    case GFX_TEXTURE_FORMAT_R32_FLOAT:
        return WGPUVertexFormat_Float32;
    case GFX_TEXTURE_FORMAT_R32G32_FLOAT:
        return WGPUVertexFormat_Float32x2;
    case GFX_TEXTURE_FORMAT_R32G32B32_FLOAT:
        return WGPUVertexFormat_Float32x3;
    case GFX_TEXTURE_FORMAT_R32G32B32A32_FLOAT:
        return WGPUVertexFormat_Float32x4;
    case GFX_TEXTURE_FORMAT_R16G16_FLOAT:
        return WGPUVertexFormat_Float16x2;
    case GFX_TEXTURE_FORMAT_R16G16B16A16_FLOAT:
        return WGPUVertexFormat_Float16x4;
    case GFX_TEXTURE_FORMAT_R8G8B8A8_UNORM:
        return WGPUVertexFormat_Unorm8x4;
    case GFX_TEXTURE_FORMAT_R8G8B8A8_UNORM_SRGB:
        return WGPUVertexFormat_Unorm8x4;
    default:
        return static_cast<WGPUVertexFormat>(0);
    }
}

WGPUTextureDimension gfxTextureTypeToWGPUTextureDimension(GfxTextureType type)
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

GfxTextureType wgpuTextureDimensionToGfxTextureType(WGPUTextureDimension dimension)
{
    switch (dimension) {
    case WGPUTextureDimension_1D:
        return GFX_TEXTURE_TYPE_1D;
    case WGPUTextureDimension_2D:
        return GFX_TEXTURE_TYPE_2D; // Note: Can't distinguish CUBE from this alone
    case WGPUTextureDimension_3D:
        return GFX_TEXTURE_TYPE_3D;
    default:
        return GFX_TEXTURE_TYPE_2D;
    }
}

WGPUTextureViewDimension gfxTextureViewTypeToWGPU(GfxTextureViewType type)
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
        return WGPUTextureViewDimension_Undefined;
    }
}

WGPUOrigin3D gfxOrigin3DToWGPUOrigin3D(const GfxOrigin3D* origin)
{
    if (!origin) {
        return { 0, 0, 0 };
    }
    return { static_cast<uint32_t>(origin->x), static_cast<uint32_t>(origin->y), static_cast<uint32_t>(origin->z) };
}

WGPUExtent3D gfxExtent3DToWGPUExtent3D(const GfxExtent3D* extent)
{
    if (!extent) {
        return { 0, 0, 0 };
    }
    return { extent->width, extent->height, extent->depth };
}

GfxExtent3D wgpuExtent3DToGfxExtent3D(const WGPUExtent3D& extent)
{
    return { extent.width, extent.height, extent.depthOrArrayLayers };
}

core::RenderPassCreateInfo gfxRenderPassDescriptorToRenderPassCreateInfo(const GfxRenderPassDescriptor* descriptor)
{
    core::RenderPassCreateInfo createInfo{};

    // For WebGPU, RenderPass stores ops info (views come from framebuffer at begin time)
    // Convert color attachment ops
    for (uint32_t i = 0; i < descriptor->colorAttachmentCount; ++i) {
        const GfxRenderPassColorAttachment& colorAtt = descriptor->colorAttachments[i];
        const GfxRenderPassColorAttachmentTarget& target = colorAtt.target;

        RenderPassColorAttachment attachment{};
        attachment.format = gfxFormatToWGPUFormat(target.format);
        attachment.loadOp = gfxLoadOpToWGPULoadOp(target.ops.loadOp);
        attachment.storeOp = gfxStoreOpToWGPUStoreOp(target.ops.storeOp);

        createInfo.colorAttachments.push_back(attachment);
    }

    // Convert depth/stencil attachment ops if present
    if (descriptor->depthStencilAttachment) {
        const GfxRenderPassDepthStencilAttachment& depthAtt = *descriptor->depthStencilAttachment;
        const GfxRenderPassDepthStencilAttachmentTarget& target = depthAtt.target;

        RenderPassDepthStencilAttachment depthStencilAttachment{};
        depthStencilAttachment.format = gfxFormatToWGPUFormat(target.format);
        depthStencilAttachment.depthLoadOp = gfxLoadOpToWGPULoadOp(target.depthOps.loadOp);
        depthStencilAttachment.depthStoreOp = gfxStoreOpToWGPUStoreOp(target.depthOps.storeOp);
        depthStencilAttachment.stencilLoadOp = gfxLoadOpToWGPULoadOp(target.stencilOps.loadOp);
        depthStencilAttachment.stencilStoreOp = gfxStoreOpToWGPUStoreOp(target.stencilOps.storeOp);

        createInfo.depthStencilAttachment = depthStencilAttachment;
    }

    return createInfo;
}

core::FramebufferCreateInfo gfxFramebufferDescriptorToFramebufferCreateInfo(const GfxFramebufferDescriptor* descriptor)
{
    core::FramebufferCreateInfo createInfo{};

    // Convert color attachment views and resolve targets - store pointers
    for (uint32_t i = 0; i < descriptor->colorAttachmentCount; ++i) {
        const GfxFramebufferAttachment& colorAtt = descriptor->colorAttachments[i];

        auto* view = toNative<TextureView>(colorAtt.view);
        createInfo.colorAttachmentViews.push_back(view); // Store pointer

        // Add resolve target if provided
        if (colorAtt.resolveTarget) {
            auto* resolveView = toNative<TextureView>(colorAtt.resolveTarget);
            createInfo.colorResolveTargetViews.push_back(resolveView); // Store pointer
        } else {
            createInfo.colorResolveTargetViews.push_back(nullptr);
        }
    }

    // Convert depth/stencil attachment view if present
    if (descriptor->depthStencilAttachment.view) {
        auto* view = toNative<TextureView>(descriptor->depthStencilAttachment.view);
        createInfo.depthStencilAttachmentView = view; // Store pointer

        // Convert depth/stencil resolve target if present
        if (descriptor->depthStencilAttachment.resolveTarget) {
            auto* resolveView = toNative<TextureView>(descriptor->depthStencilAttachment.resolveTarget);
            createInfo.depthStencilResolveTargetView = resolveView; // Store pointer
        }
    }

    createInfo.width = descriptor->extent.width;
    createInfo.height = descriptor->extent.height;

    return createInfo;
}

core::RenderPassEncoderBeginInfo gfxRenderPassBeginDescriptorToBeginInfo(const GfxRenderPassBeginDescriptor* descriptor)
{
    core::RenderPassEncoderBeginInfo beginInfo{};

    // Convert color clear values
    for (uint32_t i = 0; i < descriptor->colorClearValueCount; ++i) {
        const GfxColor& color = descriptor->colorClearValues[i];
        beginInfo.colorClearValues.push_back({ color.r, color.g, color.b, color.a });
    }

    beginInfo.depthClearValue = descriptor->depthClearValue;
    beginInfo.stencilClearValue = descriptor->stencilClearValue;

    return beginInfo;
}

core::ComputePassEncoderCreateInfo gfxComputePassBeginDescriptorToCreateInfo(const GfxComputePassBeginDescriptor* descriptor)
{
    core::ComputePassEncoderCreateInfo createInfo{};
    createInfo.label = descriptor->label;
    return createInfo;
}

} // namespace gfx::backend::webgpu::converter
