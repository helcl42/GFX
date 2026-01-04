#include "GfxWebGPUConverter.h"

#include "../entity/CreateInfo.h"
#include "../entity/Entities.h"

namespace gfx::webgpu::converter {

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

gfx::webgpu::SemaphoreType gfxSemaphoreTypeToWebGPUSemaphoreType(GfxSemaphoreType gfxType)
{
    switch (gfxType) {
    case GFX_SEMAPHORE_TYPE_BINARY:
        return gfx::webgpu::SemaphoreType::Binary;
    case GFX_SEMAPHORE_TYPE_TIMELINE:
        return gfx::webgpu::SemaphoreType::Timeline;
    default:
        return gfx::webgpu::SemaphoreType::Binary;
    }
}

// ============================================================================
// CreateInfo Conversion Functions - GfxDescriptor to Internal CreateInfo
// ============================================================================

gfx::webgpu::AdapterCreateInfo gfxDescriptorToWebGPUAdapterCreateInfo(const GfxAdapterDescriptor* descriptor)
{
    gfx::webgpu::AdapterCreateInfo createInfo{};

    if (descriptor) {
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
    } else {
        createInfo.powerPreference = WGPUPowerPreference_Undefined;
        createInfo.forceFallbackAdapter = false;
    }

    return createInfo;
}

gfx::webgpu::InstanceCreateInfo gfxDescriptorToWebGPUInstanceCreateInfo(const GfxInstanceDescriptor* descriptor)
{
    gfx::webgpu::InstanceCreateInfo createInfo{};
    createInfo.enableValidation = descriptor ? descriptor->enableValidation : false;
    return createInfo;
}

gfx::webgpu::DeviceCreateInfo gfxDescriptorToWebGPUDeviceCreateInfo(const GfxDeviceDescriptor* descriptor)
{
    (void)descriptor;
    gfx::webgpu::DeviceCreateInfo createInfo{};
    return createInfo;
}

gfx::webgpu::BufferCreateInfo gfxDescriptorToWebGPUBufferCreateInfo(const GfxBufferDescriptor* descriptor)
{
    gfx::webgpu::BufferCreateInfo createInfo{};
    createInfo.size = descriptor->size;
    createInfo.usage = gfxBufferUsageToWGPU(descriptor->usage);
    return createInfo;
}

gfx::webgpu::TextureCreateInfo gfxDescriptorToWebGPUTextureCreateInfo(const GfxTextureDescriptor* descriptor)
{
    gfx::webgpu::TextureCreateInfo createInfo{};
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
    createInfo.dimension = gfxTextureTypeToWGPU(descriptor->type);
    createInfo.arrayLayers = descriptor->arrayLayerCount > 0 ? descriptor->arrayLayerCount : 1;
    return createInfo;
}

gfx::webgpu::TextureViewCreateInfo gfxDescriptorToWebGPUTextureViewCreateInfo(const GfxTextureViewDescriptor* descriptor)
{
    gfx::webgpu::TextureViewCreateInfo createInfo{};
    createInfo.viewDimension = gfxTextureViewTypeToWGPU(descriptor->viewType);
    createInfo.format = gfxFormatToWGPUFormat(descriptor->format);
    createInfo.baseMipLevel = descriptor ? descriptor->baseMipLevel : 0;
    createInfo.mipLevelCount = descriptor ? descriptor->mipLevelCount : 1;
    createInfo.baseArrayLayer = descriptor ? descriptor->baseArrayLayer : 0;
    createInfo.arrayLayerCount = descriptor ? descriptor->arrayLayerCount : 1;
    return createInfo;
}

gfx::webgpu::ShaderCreateInfo gfxDescriptorToWebGPUShaderCreateInfo(const GfxShaderDescriptor* descriptor)
{
    gfx::webgpu::ShaderCreateInfo createInfo{};
    createInfo.code = descriptor->code;
    createInfo.codeSize = descriptor->codeSize;
    createInfo.entryPoint = descriptor->entryPoint;
    return createInfo;
}

gfx::webgpu::SamplerCreateInfo gfxDescriptorToWebGPUSamplerCreateInfo(const GfxSamplerDescriptor* descriptor)
{
    gfx::webgpu::SamplerCreateInfo createInfo{};
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

gfx::webgpu::SemaphoreCreateInfo gfxDescriptorToWebGPUSemaphoreCreateInfo(const GfxSemaphoreDescriptor* descriptor)
{
    gfx::webgpu::SemaphoreCreateInfo createInfo{};
    createInfo.type = descriptor ? gfxSemaphoreTypeToWebGPUSemaphoreType(descriptor->type)
                                 : gfx::webgpu::SemaphoreType::Binary;
    createInfo.initialValue = descriptor ? descriptor->initialValue : 0;
    return createInfo;
}

gfx::webgpu::FenceCreateInfo gfxDescriptorToWebGPUFenceCreateInfo(const GfxFenceDescriptor* descriptor)
{
    gfx::webgpu::FenceCreateInfo createInfo{};
    createInfo.signaled = descriptor && descriptor->signaled;
    return createInfo;
}

gfx::webgpu::PlatformWindowHandle gfxWindowHandleToWebGPUPlatformWindowHandle(const GfxPlatformWindowHandle& gfxHandle)
{
    gfx::webgpu::PlatformWindowHandle handle{};

    switch (gfxHandle.windowingSystem) {
    case GFX_WINDOWING_SYSTEM_XCB:
        handle.platform = gfx::webgpu::PlatformWindowHandle::Platform::Xcb;
        handle.handle.xcb.connection = gfxHandle.xcb.connection;
        handle.handle.xcb.window = gfxHandle.xcb.window;
        break;
    case GFX_WINDOWING_SYSTEM_XLIB:
        handle.platform = gfx::webgpu::PlatformWindowHandle::Platform::Xlib;
        handle.handle.xlib.display = gfxHandle.xlib.display;
        handle.handle.xlib.window = gfxHandle.xlib.window;
        break;
    case GFX_WINDOWING_SYSTEM_WAYLAND:
        handle.platform = gfx::webgpu::PlatformWindowHandle::Platform::Wayland;
        handle.handle.wayland.display = gfxHandle.wayland.display;
        handle.handle.wayland.surface = gfxHandle.wayland.surface;
        break;
    case GFX_WINDOWING_SYSTEM_WIN32:
        handle.platform = gfx::webgpu::PlatformWindowHandle::Platform::Win32;
        handle.handle.win32.hinstance = gfxHandle.win32.hinstance;
        handle.handle.win32.hwnd = gfxHandle.win32.hwnd;
        break;
    case GFX_WINDOWING_SYSTEM_METAL:
        handle.platform = gfx::webgpu::PlatformWindowHandle::Platform::Metal;
        handle.handle.metal.layer = gfxHandle.metal.layer;
        break;
    case GFX_WINDOWING_SYSTEM_EMSCRIPTEN:
        handle.platform = gfx::webgpu::PlatformWindowHandle::Platform::Emscripten;
        handle.handle.emscripten.canvasSelector = gfxHandle.emscripten.canvasSelector;
        break;
    case GFX_WINDOWING_SYSTEM_ANDROID:
        handle.platform = gfx::webgpu::PlatformWindowHandle::Platform::Android;
        handle.handle.android.window = gfxHandle.android.window;
        break;
    default:
        handle.platform = gfx::webgpu::PlatformWindowHandle::Platform::Unknown;
        break;
    }

    return handle;
}

gfx::webgpu::SurfaceCreateInfo gfxDescriptorToWebGPUSurfaceCreateInfo(const GfxSurfaceDescriptor* descriptor)
{
    gfx::webgpu::SurfaceCreateInfo createInfo{};
    if (descriptor) {
        createInfo.windowHandle = gfxWindowHandleToWebGPUPlatformWindowHandle(descriptor->windowHandle);
    }
    return createInfo;
}

gfx::webgpu::SwapchainCreateInfo gfxDescriptorToWebGPUSwapchainCreateInfo(const GfxSwapchainDescriptor* descriptor)
{
    gfx::webgpu::SwapchainCreateInfo createInfo{};
    createInfo.width = descriptor->width;
    createInfo.height = descriptor->height;
    createInfo.format = gfxFormatToWGPUFormat(descriptor->format);
    createInfo.usage = gfxTextureUsageToWGPU(descriptor->usage);
    createInfo.presentMode = gfxPresentModeToWGPU(descriptor->presentMode);
    createInfo.bufferCount = descriptor->bufferCount;
    return createInfo;
}

gfx::webgpu::BindGroupLayoutCreateInfo gfxDescriptorToWebGPUBindGroupLayoutCreateInfo(const GfxBindGroupLayoutDescriptor* descriptor)
{
    gfx::webgpu::BindGroupLayoutCreateInfo createInfo{};

    for (uint32_t i = 0; i < descriptor->entryCount; ++i) {
        const auto& entry = descriptor->entries[i];

        gfx::webgpu::BindGroupLayoutEntry layoutEntry{};
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
        }

        createInfo.entries.push_back(layoutEntry);
    }

    return createInfo;
}

gfx::webgpu::BindGroupCreateInfo gfxDescriptorToWebGPUBindGroupCreateInfo(const GfxBindGroupDescriptor* descriptor, WGPUBindGroupLayout layout)
{
    gfx::webgpu::BindGroupCreateInfo createInfo{};
    createInfo.layout = layout;

    if (descriptor->entryCount > 0 && descriptor->entries) {
        createInfo.entries.reserve(descriptor->entryCount);

        for (uint32_t i = 0; i < descriptor->entryCount; ++i) {
            const auto& entry = descriptor->entries[i];
            gfx::webgpu::BindGroupEntry bindEntry{};

            bindEntry.binding = entry.binding;

            switch (entry.type) {
            case GFX_BIND_GROUP_ENTRY_TYPE_BUFFER: {
                auto* buffer = reinterpret_cast<gfx::webgpu::Buffer*>(entry.resource.buffer.buffer);
                bindEntry.buffer = buffer->handle();
                bindEntry.bufferOffset = entry.resource.buffer.offset;
                bindEntry.bufferSize = entry.resource.buffer.size;
                break;
            }
            case GFX_BIND_GROUP_ENTRY_TYPE_SAMPLER: {
                auto* sampler = reinterpret_cast<gfx::webgpu::Sampler*>(entry.resource.sampler);
                bindEntry.sampler = sampler->handle();
                break;
            }
            case GFX_BIND_GROUP_ENTRY_TYPE_TEXTURE_VIEW: {
                auto* textureView = reinterpret_cast<gfx::webgpu::TextureView*>(entry.resource.textureView);
                bindEntry.textureView = textureView->handle();
                break;
            }
            }

            createInfo.entries.push_back(bindEntry);
        }
    }

    return createInfo;
}

gfx::webgpu::RenderPipelineCreateInfo gfxDescriptorToWebGPURenderPipelineCreateInfo(const GfxRenderPipelineDescriptor* descriptor)
{
    gfx::webgpu::RenderPipelineCreateInfo createInfo{};

    // Extract bind group layouts
    if (descriptor->bindGroupLayoutCount > 0 && descriptor->bindGroupLayouts) {
        createInfo.bindGroupLayouts.reserve(descriptor->bindGroupLayoutCount);
        for (uint32_t i = 0; i < descriptor->bindGroupLayoutCount; ++i) {
            auto* layout = reinterpret_cast<gfx::webgpu::BindGroupLayout*>(descriptor->bindGroupLayouts[i]);
            createInfo.bindGroupLayouts.push_back(layout->handle());
        }
    }

    // Vertex state
    auto* vertexShader = reinterpret_cast<gfx::webgpu::Shader*>(descriptor->vertex->module);
    createInfo.vertex.module = vertexShader->handle();
    createInfo.vertex.entryPoint = descriptor->vertex->entryPoint;

    // Convert vertex buffers
    if (descriptor->vertex->bufferCount > 0) {
        createInfo.vertex.buffers.reserve(descriptor->vertex->bufferCount);

        for (uint32_t i = 0; i < descriptor->vertex->bufferCount; ++i) {
            const auto& buffer = descriptor->vertex->buffers[i];
            gfx::webgpu::VertexBufferLayout vbLayout{};
            vbLayout.arrayStride = buffer.arrayStride;
            vbLayout.stepMode = buffer.stepModeInstance ? WGPUVertexStepMode_Instance : WGPUVertexStepMode_Vertex;

            // Convert attributes
            vbLayout.attributes.reserve(buffer.attributeCount);
            for (uint32_t j = 0; j < buffer.attributeCount; ++j) {
                const auto& attr = buffer.attributes[j];
                gfx::webgpu::VertexAttribute vbAttr{};
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
        gfx::webgpu::FragmentState fragState{};
        auto* fragmentShader = reinterpret_cast<gfx::webgpu::Shader*>(descriptor->fragment->module);
        fragState.module = fragmentShader->handle();
        fragState.entryPoint = descriptor->fragment->entryPoint;

        // Convert color targets
        if (descriptor->fragment->targetCount > 0) {
            fragState.targets.reserve(descriptor->fragment->targetCount);

            for (uint32_t i = 0; i < descriptor->fragment->targetCount; ++i) {
                const auto& target = descriptor->fragment->targets[i];
                gfx::webgpu::ColorTargetState colorTarget{};
                colorTarget.format = gfxFormatToWGPUFormat(target.format);
                colorTarget.writeMask = target.writeMask;

                if (target.blend) {
                    gfx::webgpu::BlendState blend{};
                    blend.color.operation = gfxBlendOperationToWGPU(target.blend->color.operation);
                    blend.color.srcFactor = gfxBlendFactorToWGPU(target.blend->color.srcFactor);
                    blend.color.dstFactor = gfxBlendFactorToWGPU(target.blend->color.dstFactor);
                    blend.alpha.operation = gfxBlendOperationToWGPU(target.blend->alpha.operation);
                    blend.alpha.srcFactor = gfxBlendFactorToWGPU(target.blend->alpha.srcFactor);
                    blend.alpha.dstFactor = gfxBlendFactorToWGPU(target.blend->alpha.dstFactor);
                    colorTarget.blend = blend;
                }

                fragState.targets.push_back(std::move(colorTarget));
            }
        }

        createInfo.fragment = std::move(fragState);
    }

    // Primitive state
    createInfo.primitive.topology = gfxPrimitiveTopologyToWGPU(descriptor->primitive->topology);
    createInfo.primitive.frontFace = gfxFrontFaceToWGPU(descriptor->primitive->frontFace);
    createInfo.primitive.cullMode = gfxCullModeToWGPU(descriptor->primitive->cullMode);
    createInfo.primitive.stripIndexFormat = descriptor->primitive->stripIndexFormat
        ? gfxIndexFormatToWGPU(*descriptor->primitive->stripIndexFormat)
        : WGPUIndexFormat_Undefined;

    // Depth/stencil state (optional)
    if (descriptor->depthStencil) {
        gfx::webgpu::DepthStencilState dsState{};
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

gfx::webgpu::ComputePipelineCreateInfo gfxDescriptorToWebGPUComputePipelineCreateInfo(const GfxComputePipelineDescriptor* descriptor)
{
    gfx::webgpu::ComputePipelineCreateInfo createInfo{};

    // Extract bind group layouts
    if (descriptor->bindGroupLayoutCount > 0 && descriptor->bindGroupLayouts) {
        createInfo.bindGroupLayouts.reserve(descriptor->bindGroupLayoutCount);
        for (uint32_t i = 0; i < descriptor->bindGroupLayoutCount; ++i) {
            auto* layout = reinterpret_cast<gfx::webgpu::BindGroupLayout*>(descriptor->bindGroupLayouts[i]);
            createInfo.bindGroupLayouts.push_back(layout->handle());
        }
    }

    // Extract shader module
    auto* shader = reinterpret_cast<gfx::webgpu::Shader*>(descriptor->compute);
    createInfo.module = shader->handle();
    createInfo.entryPoint = descriptor->entryPoint;

    return createInfo;
}

gfx::webgpu::CommandEncoderCreateInfo gfxDescriptorToWebGPUCommandEncoderCreateInfo(const GfxCommandEncoderDescriptor* descriptor)
{
    gfx::webgpu::CommandEncoderCreateInfo createInfo;
    createInfo.label = descriptor->label;
    return createInfo;
}

gfx::webgpu::SubmitInfo gfxDescriptorToWebGPUSubmitInfo(const GfxSubmitInfo* descriptor)
{
    gfx::webgpu::SubmitInfo submitInfo{};
    submitInfo.commandEncoders = reinterpret_cast<gfx::webgpu::CommandEncoder**>(descriptor->commandEncoders);
    submitInfo.commandEncoderCount = descriptor->commandEncoderCount;
    submitInfo.signalFence = reinterpret_cast<gfx::webgpu::Fence*>(descriptor->signalFence);
    // Note: WebGPU doesn't support semaphores, so wait/signal semaphores are ignored
    return submitInfo;
}

// ============================================================================
// Reverse Conversions - Internal to Gfx API types
// ============================================================================

GfxBufferUsage webgpuBufferUsageToGfxBufferUsage(WGPUBufferUsage usage)
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
    return static_cast<GfxBufferUsage>(gfxUsage);
}

GfxSemaphoreType webgpuSemaphoreTypeToGfxSemaphoreType(gfx::webgpu::SemaphoreType type)
{
    switch (type) {
    case gfx::webgpu::SemaphoreType::Binary:
        return GFX_SEMAPHORE_TYPE_BINARY;
    case gfx::webgpu::SemaphoreType::Timeline:
        return GFX_SEMAPHORE_TYPE_TIMELINE;
    default:
        return GFX_SEMAPHORE_TYPE_BINARY;
    }
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

GfxTextureUsage wgpuTextureUsageToGfxTextureUsage(WGPUTextureUsage usage)
{
    uint32_t gfx_usage = GFX_TEXTURE_USAGE_NONE;
    if (usage & WGPUTextureUsage_CopySrc) {
        gfx_usage |= GFX_TEXTURE_USAGE_COPY_SRC;
    }
    if (usage & WGPUTextureUsage_CopyDst) {
        gfx_usage |= GFX_TEXTURE_USAGE_COPY_DST;
    }
    if (usage & WGPUTextureUsage_TextureBinding) {
        gfx_usage |= GFX_TEXTURE_USAGE_TEXTURE_BINDING;
    }
    if (usage & WGPUTextureUsage_StorageBinding) {
        gfx_usage |= GFX_TEXTURE_USAGE_STORAGE_BINDING;
    }
    if (usage & WGPUTextureUsage_RenderAttachment) {
        gfx_usage |= GFX_TEXTURE_USAGE_RENDER_ATTACHMENT;
    }
    return static_cast<GfxTextureUsage>(gfx_usage);
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

WGPUTextureDimension gfxTextureTypeToWGPU(GfxTextureType type)
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

} // namespace gfx::webgpu::converter
