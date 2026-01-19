#include "Device.h"

#include "Queue.h"

#include "../command/CommandEncoder.h"
#include "../compute/ComputePipeline.h"
#include "../presentation/Surface.h"
#include "../presentation/Swapchain.h"
#include "../render/Framebuffer.h"
#include "../render/RenderPass.h"
#include "../render/RenderPipeline.h"
#include "../resource/BindGroup.h"
#include "../resource/BindGroupLayout.h"
#include "../resource/Buffer.h"
#include "../resource/Sampler.h"
#include "../resource/Shader.h"
#include "../resource/Texture.h"
#include "../resource/TextureView.h"
#include "../sync/Fence.h"
#include "../sync/Semaphore.h"

#include "../../converter/Conversions.h"

#include <stdexcept>

namespace gfx {

DeviceImpl::DeviceImpl(GfxDevice h)
    : m_handle(h)
{
    GfxQueue queueHandle = nullptr;
    if (gfxDeviceGetQueue(m_handle, &queueHandle) == GFX_RESULT_SUCCESS && queueHandle) {
        m_queue = std::make_shared<QueueImpl>(queueHandle);
    }
}

DeviceImpl::~DeviceImpl()
{
    if (m_handle) {
        gfxDeviceWaitIdle(m_handle);
        gfxDeviceDestroy(m_handle);
    }
}

std::shared_ptr<Queue> DeviceImpl::getQueue()
{
    return m_queue;
}

std::shared_ptr<Surface> DeviceImpl::createSurface(const SurfaceDescriptor& descriptor)
{
    GfxSurfaceDescriptor cDesc = {};
    cDesc.label = descriptor.label.c_str();
    cDesc.windowHandle = cppHandleToCHandle(descriptor.windowHandle);

    GfxSurface surface = nullptr;
    GfxResult result = gfxDeviceCreateSurface(m_handle, &cDesc, &surface);
    if (result != GFX_RESULT_SUCCESS || !surface) {
        throw std::runtime_error("Failed to create surface");
    }
    return std::make_shared<SurfaceImpl>(surface);
}

std::shared_ptr<Swapchain> DeviceImpl::createSwapchain(std::shared_ptr<Surface> surface, const SwapchainDescriptor& descriptor)
{
    auto surfaceImpl = std::dynamic_pointer_cast<SurfaceImpl>(surface);
    if (!surfaceImpl) {
        throw std::runtime_error("Invalid surface type");
    }

    GfxSwapchainDescriptor cDesc = {};
    cDesc.label = descriptor.label.c_str();
    cDesc.width = descriptor.width;
    cDesc.height = descriptor.height;
    cDesc.format = cppFormatToCFormat(descriptor.format);
    cDesc.usage = cppTextureUsageToCUsage(descriptor.usage);
    cDesc.presentMode = cppPresentModeToCPresentMode(descriptor.presentMode);
    cDesc.imageCount = descriptor.imageCount;

    GfxSwapchain swapchain = nullptr;
    GfxResult result = gfxDeviceCreateSwapchain(m_handle, surfaceImpl->getHandle(), &cDesc, &swapchain);
    if (result != GFX_RESULT_SUCCESS || !swapchain) {
        throw std::runtime_error("Failed to create swapchain");
    }
    return std::make_shared<SwapchainImpl>(swapchain);
}

std::shared_ptr<Buffer> DeviceImpl::createBuffer(const BufferDescriptor& descriptor)
{
    GfxBufferDescriptor cDesc = {};
    cDesc.label = descriptor.label.c_str();
    cDesc.size = descriptor.size;
    cDesc.usage = cppBufferUsageToCUsage(descriptor.usage);

    GfxBuffer buffer = nullptr;
    GfxResult result = gfxDeviceCreateBuffer(m_handle, &cDesc, &buffer);
    if (result != GFX_RESULT_SUCCESS || !buffer) {
        throw std::runtime_error("Failed to create buffer");
    }
    return std::make_shared<BufferImpl>(buffer);
}

std::shared_ptr<Buffer> DeviceImpl::importBuffer(const BufferImportDescriptor& descriptor)
{
    GfxBufferImportDescriptor cDesc = {};
    cDesc.label = descriptor.label.c_str();
    cDesc.nativeHandle = descriptor.nativeHandle;
    cDesc.size = descriptor.size;
    cDesc.usage = cppBufferUsageToCUsage(descriptor.usage);

    GfxBuffer buffer = nullptr;
    GfxResult result = gfxDeviceImportBuffer(m_handle, &cDesc, &buffer);
    if (result != GFX_RESULT_SUCCESS || !buffer) {
        throw std::runtime_error("Failed to import buffer");
    }
    return std::make_shared<BufferImpl>(buffer);
}

std::shared_ptr<Texture> DeviceImpl::createTexture(const TextureDescriptor& descriptor)
{
    GfxTextureDescriptor cDesc = {};
    cDesc.label = descriptor.label.c_str();
    cDesc.type = cppTextureTypeToCType(descriptor.type);
    cDesc.size = { descriptor.size.width, descriptor.size.height, descriptor.size.depth };
    cDesc.arrayLayerCount = descriptor.arrayLayerCount;
    cDesc.mipLevelCount = descriptor.mipLevelCount;
    cDesc.sampleCount = cppSampleCountToCCount(descriptor.sampleCount);
    cDesc.format = cppFormatToCFormat(descriptor.format);
    cDesc.usage = cppTextureUsageToCUsage(descriptor.usage);

    GfxTexture texture = nullptr;
    GfxResult result = gfxDeviceCreateTexture(m_handle, &cDesc, &texture);
    if (result != GFX_RESULT_SUCCESS || !texture) {
        throw std::runtime_error("Failed to create texture");
    }
    return std::make_shared<TextureImpl>(texture);
}

std::shared_ptr<Texture> DeviceImpl::importTexture(const TextureImportDescriptor& descriptor)
{
    GfxTextureImportDescriptor cDesc = {};
    cDesc.label = descriptor.label.c_str();
    cDesc.nativeHandle = descriptor.nativeHandle;
    cDesc.type = cppTextureTypeToCType(descriptor.type);
    cDesc.size = { descriptor.size.width, descriptor.size.height, descriptor.size.depth };
    cDesc.arrayLayerCount = descriptor.arrayLayerCount;
    cDesc.mipLevelCount = descriptor.mipLevelCount;
    cDesc.sampleCount = cppSampleCountToCCount(descriptor.sampleCount);
    cDesc.format = cppFormatToCFormat(descriptor.format);
    cDesc.usage = cppTextureUsageToCUsage(descriptor.usage);
    cDesc.currentLayout = cppLayoutToCLayout(descriptor.currentLayout);

    GfxTexture texture = nullptr;
    GfxResult result = gfxDeviceImportTexture(m_handle, &cDesc, &texture);
    if (result != GFX_RESULT_SUCCESS || !texture) {
        throw std::runtime_error("Failed to import texture");
    }
    return std::make_shared<TextureImpl>(texture);
}

std::shared_ptr<Sampler> DeviceImpl::createSampler(const SamplerDescriptor& descriptor)
{
    GfxSamplerDescriptor cDesc = {};
    cDesc.label = descriptor.label.c_str();
    cDesc.addressModeU = cppAddressModeToCAddressMode(descriptor.addressModeU);
    cDesc.addressModeV = cppAddressModeToCAddressMode(descriptor.addressModeV);
    cDesc.addressModeW = cppAddressModeToCAddressMode(descriptor.addressModeW);
    cDesc.magFilter = cppFilterModeToCFilterMode(descriptor.magFilter);
    cDesc.minFilter = cppFilterModeToCFilterMode(descriptor.minFilter);
    cDesc.mipmapFilter = cppFilterModeToCFilterMode(descriptor.mipmapFilter);
    cDesc.lodMinClamp = descriptor.lodMinClamp;
    cDesc.lodMaxClamp = descriptor.lodMaxClamp;
    cDesc.maxAnisotropy = descriptor.maxAnisotropy;
    cDesc.compare = cppCompareFunctionToCCompareFunction(descriptor.compare);

    GfxSampler sampler = nullptr;
    GfxResult result = gfxDeviceCreateSampler(m_handle, &cDesc, &sampler);
    if (result != GFX_RESULT_SUCCESS || !sampler) {
        throw std::runtime_error("Failed to create sampler");
    }
    return std::make_shared<SamplerImpl>(sampler);
}

std::shared_ptr<Shader> DeviceImpl::createShader(const ShaderDescriptor& descriptor)
{
    GfxShaderDescriptor cDesc = {};
    cDesc.label = descriptor.label.c_str();
    cDesc.sourceType = cppShaderSourceTypeToCShaderSourceType(descriptor.sourceType);
    cDesc.code = descriptor.code.c_str();
    cDesc.codeSize = descriptor.code.size(); // Set the actual binary size
    cDesc.entryPoint = descriptor.entryPoint.c_str();

    GfxShader shader = nullptr;
    GfxResult result = gfxDeviceCreateShader(m_handle, &cDesc, &shader);
    if (result != GFX_RESULT_SUCCESS || !shader) {
        throw std::runtime_error("Failed to create shader");
    }
    return std::make_shared<ShaderImpl>(shader);
}

std::shared_ptr<BindGroupLayout> DeviceImpl::createBindGroupLayout(const BindGroupLayoutDescriptor& descriptor)
{
    // Convert entries properly
    std::vector<GfxBindGroupLayoutEntry> cEntries(descriptor.entries.size());
    for (size_t i = 0; i < descriptor.entries.size(); ++i) {
        const auto& entry = descriptor.entries[i];
        cEntries[i].binding = entry.binding;
        cEntries[i].visibility = cppShaderStageToCShaderStage(entry.visibility);

        // Determine binding type based on variant index
        if (entry.resource.index() == 0) {
            // Buffer binding
            cEntries[i].type = GFX_BINDING_TYPE_BUFFER;
            const auto& buffer = std::get<BindGroupLayoutEntry::BufferBinding>(entry.resource);
            cEntries[i].buffer.hasDynamicOffset = buffer.hasDynamicOffset;
            cEntries[i].buffer.minBindingSize = buffer.minBindingSize;
        } else if (entry.resource.index() == 1) {
            // Sampler binding
            cEntries[i].type = GFX_BINDING_TYPE_SAMPLER;
            const auto& sampler = std::get<BindGroupLayoutEntry::SamplerBinding>(entry.resource);
            cEntries[i].sampler.comparison = sampler.comparison;
        } else if (entry.resource.index() == 2) {
            // Texture binding
            cEntries[i].type = GFX_BINDING_TYPE_TEXTURE;
            const auto& texture = std::get<BindGroupLayoutEntry::TextureBinding>(entry.resource);
            cEntries[i].texture.multisampled = texture.multisampled;
            cEntries[i].texture.viewDimension = cppTextureViewTypeToCType(texture.viewDimension);
        } else if (entry.resource.index() == 3) {
            // Storage texture binding
            cEntries[i].type = GFX_BINDING_TYPE_STORAGE_TEXTURE;
            const auto& storageTexture = std::get<BindGroupLayoutEntry::StorageTextureBinding>(entry.resource);
            cEntries[i].storageTexture.format = cppFormatToCFormat(storageTexture.format);
            cEntries[i].storageTexture.writeOnly = storageTexture.writeOnly;
            cEntries[i].storageTexture.viewDimension = cppTextureViewTypeToCType(storageTexture.viewDimension);
        }
    }

    GfxBindGroupLayoutDescriptor cDesc = {};
    cDesc.label = descriptor.label.c_str();
    cDesc.entries = cEntries.data();
    cDesc.entryCount = static_cast<uint32_t>(cEntries.size());

    GfxBindGroupLayout layout = nullptr;
    GfxResult result = gfxDeviceCreateBindGroupLayout(m_handle, &cDesc, &layout);
    if (result != GFX_RESULT_SUCCESS || !layout) {
        throw std::runtime_error("Failed to create bind group layout");
    }
    return std::make_shared<BindGroupLayoutImpl>(layout);
}

std::shared_ptr<BindGroup> DeviceImpl::createBindGroup(const BindGroupDescriptor& descriptor)
{
    auto layoutImpl = std::dynamic_pointer_cast<BindGroupLayoutImpl>(descriptor.layout);
    if (!layoutImpl) {
        throw std::runtime_error("Invalid bind group layout type");
    }

    // Convert entries properly
    std::vector<GfxBindGroupEntry> cEntries(descriptor.entries.size());
    for (size_t i = 0; i < descriptor.entries.size(); ++i) {
        const auto& entry = descriptor.entries[i];
        cEntries[i].binding = entry.binding;

        // Determine type based on variant index
        if (entry.resource.index() == 0) {
            // Buffer resource
            cEntries[i].type = GFX_BIND_GROUP_ENTRY_TYPE_BUFFER;
            auto buffer = std::get<std::shared_ptr<Buffer>>(entry.resource);
            auto bufferImpl = std::dynamic_pointer_cast<BufferImpl>(buffer);
            if (bufferImpl) {
                cEntries[i].resource.buffer.buffer = bufferImpl->getHandle();
                cEntries[i].resource.buffer.offset = entry.offset;
                cEntries[i].resource.buffer.size = entry.size;
            }
        } else if (entry.resource.index() == 1) {
            // Sampler resource
            cEntries[i].type = GFX_BIND_GROUP_ENTRY_TYPE_SAMPLER;
            auto sampler = std::get<std::shared_ptr<Sampler>>(entry.resource);
            auto samplerImpl = std::dynamic_pointer_cast<SamplerImpl>(sampler);
            if (samplerImpl) {
                cEntries[i].resource.sampler = samplerImpl->getHandle();
            }
        } else if (entry.resource.index() == 2) {
            // TextureView resource
            cEntries[i].type = GFX_BIND_GROUP_ENTRY_TYPE_TEXTURE_VIEW;
            auto textureView = std::get<std::shared_ptr<TextureView>>(entry.resource);
            auto textureViewImpl = std::dynamic_pointer_cast<TextureViewImpl>(textureView);
            if (textureViewImpl) {
                cEntries[i].resource.textureView = textureViewImpl->getHandle();
            }
        }
    }

    GfxBindGroupDescriptor cDesc = {};
    cDesc.label = descriptor.label.c_str();
    cDesc.layout = layoutImpl->getHandle();
    cDesc.entries = cEntries.data();
    cDesc.entryCount = static_cast<uint32_t>(cEntries.size());

    GfxBindGroup bindGroup = nullptr;
    GfxResult result = gfxDeviceCreateBindGroup(m_handle, &cDesc, &bindGroup);
    if (result != GFX_RESULT_SUCCESS || !bindGroup) {
        throw std::runtime_error("Failed to create bind group");
    }
    return std::make_shared<BindGroupImpl>(bindGroup);
}

std::shared_ptr<RenderPipeline> DeviceImpl::createRenderPipeline(const RenderPipelineDescriptor& descriptor)
{
    // Extract shader handles
    auto vertexShaderImpl = std::dynamic_pointer_cast<ShaderImpl>(descriptor.vertex.module);
    if (!vertexShaderImpl) {
        throw std::runtime_error("Invalid vertex shader type");
    }

    // Convert vertex attributes
    std::vector<std::vector<GfxVertexAttribute>> cAttributesPerBuffer;
    std::vector<GfxVertexBufferLayout> cVertexBuffers;

    for (const auto& buffer : descriptor.vertex.buffers) {
        std::vector<GfxVertexAttribute> cAttributes;
        for (const auto& attr : buffer.attributes) {
            GfxVertexAttribute cAttr = {};
            cAttr.format = cppFormatToCFormat(attr.format);
            cAttr.offset = attr.offset;
            cAttr.shaderLocation = attr.shaderLocation;
            cAttributes.push_back(cAttr);
        }
        cAttributesPerBuffer.push_back(std::move(cAttributes));

        GfxVertexBufferLayout cBuffer = {};
        cBuffer.arrayStride = buffer.arrayStride;
        cBuffer.attributes = cAttributesPerBuffer.back().data();
        cBuffer.attributeCount = static_cast<uint32_t>(cAttributesPerBuffer.back().size());
        cBuffer.stepModeInstance = buffer.stepModeInstance;
        cVertexBuffers.push_back(cBuffer);
    }

    // Vertex state
    GfxVertexState cVertexState = {};
    cVertexState.module = vertexShaderImpl->getHandle();
    cVertexState.entryPoint = descriptor.vertex.entryPoint.c_str();
    cVertexState.buffers = cVertexBuffers.empty() ? nullptr : cVertexBuffers.data();
    cVertexState.bufferCount = static_cast<uint32_t>(cVertexBuffers.size());

    // Fragment state (optional)
    GfxFragmentState cFragmentState = {};
    std::vector<GfxColorTargetState> cColorTargets;
    std::vector<GfxBlendState> cBlendStates;
    GfxFragmentState* pFragmentState = nullptr;

    if (descriptor.fragment.has_value()) {
        const auto& fragment = *descriptor.fragment;
        auto fragmentShaderImpl = std::dynamic_pointer_cast<ShaderImpl>(fragment.module);
        if (!fragmentShaderImpl) {
            throw std::runtime_error("Invalid fragment shader type");
        }

        // Convert color targets
        for (const auto& target : fragment.targets) {
            GfxColorTargetState cTarget = {};
            cTarget.format = cppFormatToCFormat(target.format);
            cTarget.writeMask = target.writeMask;

            // Convert blend state if present
            if (target.blend.has_value()) {
                GfxBlendState cBlend = {};
                cBlend.color.operation = cppBlendOperationToCBlendOperation(target.blend->color.operation);
                cBlend.color.srcFactor = cppBlendFactorToCBlendFactor(target.blend->color.srcFactor);
                cBlend.color.dstFactor = cppBlendFactorToCBlendFactor(target.blend->color.dstFactor);
                cBlend.alpha.operation = cppBlendOperationToCBlendOperation(target.blend->alpha.operation);
                cBlend.alpha.srcFactor = cppBlendFactorToCBlendFactor(target.blend->alpha.srcFactor);
                cBlend.alpha.dstFactor = cppBlendFactorToCBlendFactor(target.blend->alpha.dstFactor);
                cBlendStates.push_back(cBlend);
                cTarget.blend = &cBlendStates.back();
            } else {
                cTarget.blend = nullptr;
            }

            cColorTargets.push_back(cTarget);
        }

        cFragmentState.module = fragmentShaderImpl->getHandle();
        cFragmentState.entryPoint = fragment.entryPoint.c_str();
        cFragmentState.targets = cColorTargets.data();
        cFragmentState.targetCount = static_cast<uint32_t>(cColorTargets.size());
        pFragmentState = &cFragmentState;
    }

    // Primitive state
    GfxPrimitiveState cPrimitiveState = {};
    cPrimitiveState.topology = cppPrimitiveTopologyToCPrimitiveTopology(descriptor.primitive.topology);
    cPrimitiveState.frontFace = cppFrontFaceToCFrontFace(descriptor.primitive.frontFace);
    cPrimitiveState.cullMode = cppCullModeToCCullMode(descriptor.primitive.cullMode);
    cPrimitiveState.polygonMode = cppPolygonModeToCPolygonMode(descriptor.primitive.polygonMode);

    GfxIndexFormat cStripIndexFormat;
    if (descriptor.primitive.stripIndexFormat.has_value()) {
        cStripIndexFormat = (*descriptor.primitive.stripIndexFormat == IndexFormat::Uint16)
            ? GFX_INDEX_FORMAT_UINT16
            : GFX_INDEX_FORMAT_UINT32;
        cPrimitiveState.stripIndexFormat = &cStripIndexFormat;
    } else {
        cPrimitiveState.stripIndexFormat = nullptr;
    }

    // Depth/stencil state (optional)
    GfxDepthStencilState cDepthStencilState = {};
    GfxDepthStencilState* pDepthStencilState = nullptr;

    if (descriptor.depthStencil.has_value()) {
        const auto& depthStencil = *descriptor.depthStencil;
        cDepthStencilState.format = cppFormatToCFormat(depthStencil.format);
        cDepthStencilState.depthWriteEnabled = depthStencil.depthWriteEnabled;
        cDepthStencilState.depthCompare = cppCompareFunctionToCCompareFunction(depthStencil.depthCompare);

        cDepthStencilState.stencilFront.compare = cppCompareFunctionToCCompareFunction(depthStencil.stencilFront.compare);
        cDepthStencilState.stencilFront.failOp = cppStencilOperationToCStencilOperation(depthStencil.stencilFront.failOp);
        cDepthStencilState.stencilFront.depthFailOp = cppStencilOperationToCStencilOperation(depthStencil.stencilFront.depthFailOp);
        cDepthStencilState.stencilFront.passOp = cppStencilOperationToCStencilOperation(depthStencil.stencilFront.passOp);

        cDepthStencilState.stencilBack.compare = cppCompareFunctionToCCompareFunction(depthStencil.stencilBack.compare);
        cDepthStencilState.stencilBack.failOp = cppStencilOperationToCStencilOperation(depthStencil.stencilBack.failOp);
        cDepthStencilState.stencilBack.depthFailOp = cppStencilOperationToCStencilOperation(depthStencil.stencilBack.depthFailOp);
        cDepthStencilState.stencilBack.passOp = cppStencilOperationToCStencilOperation(depthStencil.stencilBack.passOp);

        cDepthStencilState.stencilReadMask = depthStencil.stencilReadMask;
        cDepthStencilState.stencilWriteMask = depthStencil.stencilWriteMask;
        cDepthStencilState.depthBias = depthStencil.depthBias;
        cDepthStencilState.depthBiasSlopeScale = depthStencil.depthBiasSlopeScale;
        cDepthStencilState.depthBiasClamp = depthStencil.depthBiasClamp;

        pDepthStencilState = &cDepthStencilState;
    }

    // Create pipeline descriptor
    GfxRenderPipelineDescriptor cDesc = {};
    cDesc.label = descriptor.label.c_str();

    // Extract render pass handle
    auto renderPassImpl = std::dynamic_pointer_cast<RenderPassImpl>(descriptor.renderPass);
    if (!renderPassImpl) {
        throw std::runtime_error("Invalid render pass type");
    }
    cDesc.renderPass = renderPassImpl->getHandle();

    cDesc.vertex = &cVertexState;
    cDesc.fragment = pFragmentState;
    cDesc.primitive = &cPrimitiveState;
    cDesc.depthStencil = pDepthStencilState;
    cDesc.sampleCount = cppSampleCountToCCount(descriptor.sampleCount);

    // Convert bind group layouts
    std::vector<GfxBindGroupLayout> cBindGroupLayouts;
    for (const auto& layout : descriptor.bindGroupLayouts) {
        auto layoutImpl = std::dynamic_pointer_cast<BindGroupLayoutImpl>(layout);
        if (layoutImpl) {
            cBindGroupLayouts.push_back(layoutImpl->getHandle());
        }
    }
    cDesc.bindGroupLayouts = cBindGroupLayouts.empty() ? nullptr : cBindGroupLayouts.data();
    cDesc.bindGroupLayoutCount = static_cast<uint32_t>(cBindGroupLayouts.size());

    GfxRenderPipeline pipeline = nullptr;
    GfxResult result = gfxDeviceCreateRenderPipeline(m_handle, &cDesc, &pipeline);
    if (result != GFX_RESULT_SUCCESS || !pipeline) {
        throw std::runtime_error("Failed to create render pipeline");
    }
    return std::make_shared<RenderPipelineImpl>(pipeline);
}

std::shared_ptr<ComputePipeline> DeviceImpl::createComputePipeline(const ComputePipelineDescriptor& descriptor)
{
    auto shaderImpl = std::dynamic_pointer_cast<ShaderImpl>(descriptor.compute);
    if (!shaderImpl) {
        throw std::runtime_error("Invalid shader type");
    }

    // Convert bind group layouts to C handles
    std::vector<GfxBindGroupLayout> bindGroupLayoutHandles;
    for (const auto& layout : descriptor.bindGroupLayouts) {
        auto layoutImpl = std::dynamic_pointer_cast<BindGroupLayoutImpl>(layout);
        if (layoutImpl) {
            bindGroupLayoutHandles.push_back(layoutImpl->getHandle());
        }
    }

    GfxComputePipelineDescriptor cDesc = {};
    cDesc.label = descriptor.label.c_str();
    cDesc.compute = shaderImpl->getHandle();
    cDesc.entryPoint = descriptor.entryPoint.c_str();
    cDesc.bindGroupLayouts = bindGroupLayoutHandles.empty() ? nullptr : bindGroupLayoutHandles.data();
    cDesc.bindGroupLayoutCount = static_cast<uint32_t>(bindGroupLayoutHandles.size());

    GfxComputePipeline pipeline = nullptr;
    GfxResult result = gfxDeviceCreateComputePipeline(m_handle, &cDesc, &pipeline);
    if (result != GFX_RESULT_SUCCESS || !pipeline) {
        throw std::runtime_error("Failed to create compute pipeline");
    }
    return std::make_shared<ComputePipelineImpl>(pipeline);
}

std::shared_ptr<RenderPass> DeviceImpl::createRenderPass(const RenderPassCreateDescriptor& descriptor)
{
    // Convert color attachments
    std::vector<GfxRenderPassColorAttachment> cColorAttachments;
    std::vector<GfxRenderPassColorAttachmentTarget> cColorTargets;
    std::vector<GfxRenderPassColorAttachmentTarget> cColorResolveTargets;

    for (const auto& attachment : descriptor.colorAttachments) {
        GfxRenderPassColorAttachment cAttachment = {};

        GfxRenderPassColorAttachmentTarget cTarget = {};
        cTarget.format = cppFormatToCFormat(attachment.target.format);
        cTarget.sampleCount = cppSampleCountToCCount(attachment.target.sampleCount);
        cTarget.ops.loadOp = cppLoadOpToCLoadOp(attachment.target.loadOp);
        cTarget.ops.storeOp = cppStoreOpToCStoreOp(attachment.target.storeOp);
        cTarget.finalLayout = cppLayoutToCLayout(attachment.target.finalLayout);
        cColorTargets.push_back(cTarget);
        cAttachment.target = cColorTargets.back();

        if (attachment.resolveTarget) {
            GfxRenderPassColorAttachmentTarget cResolveTarget = {};
            cResolveTarget.format = cppFormatToCFormat(attachment.resolveTarget->format);
            cResolveTarget.sampleCount = cppSampleCountToCCount(attachment.resolveTarget->sampleCount);
            cResolveTarget.ops.loadOp = cppLoadOpToCLoadOp(attachment.resolveTarget->loadOp);
            cResolveTarget.ops.storeOp = cppStoreOpToCStoreOp(attachment.resolveTarget->storeOp);
            cResolveTarget.finalLayout = cppLayoutToCLayout(attachment.resolveTarget->finalLayout);
            cColorResolveTargets.push_back(cResolveTarget);
            cAttachment.resolveTarget = &cColorResolveTargets.back();
        } else {
            cAttachment.resolveTarget = nullptr;
        }

        cColorAttachments.push_back(cAttachment);
    }

    // Convert depth/stencil attachment if present
    GfxRenderPassDepthStencilAttachment cDepthStencilAttachment = {};
    GfxRenderPassDepthStencilAttachmentTarget cDepthTarget = {};
    GfxRenderPassDepthStencilAttachmentTarget cDepthResolveTarget = {};
    const GfxRenderPassDepthStencilAttachment* cDepthStencilPtr = nullptr;

    if (descriptor.depthStencilAttachment) {
        cDepthTarget.format = cppFormatToCFormat(descriptor.depthStencilAttachment->target.format);
        cDepthTarget.sampleCount = cppSampleCountToCCount(descriptor.depthStencilAttachment->target.sampleCount);
        cDepthTarget.depthOps.loadOp = cppLoadOpToCLoadOp(descriptor.depthStencilAttachment->target.depthLoadOp);
        cDepthTarget.depthOps.storeOp = cppStoreOpToCStoreOp(descriptor.depthStencilAttachment->target.depthStoreOp);
        cDepthTarget.stencilOps.loadOp = cppLoadOpToCLoadOp(descriptor.depthStencilAttachment->target.stencilLoadOp);
        cDepthTarget.stencilOps.storeOp = cppStoreOpToCStoreOp(descriptor.depthStencilAttachment->target.stencilStoreOp);
        cDepthTarget.finalLayout = cppLayoutToCLayout(descriptor.depthStencilAttachment->target.finalLayout);
        cDepthStencilAttachment.target = cDepthTarget;

        if (descriptor.depthStencilAttachment->resolveTarget) {
            cDepthResolveTarget.format = cppFormatToCFormat(descriptor.depthStencilAttachment->resolveTarget->format);
            cDepthResolveTarget.sampleCount = cppSampleCountToCCount(descriptor.depthStencilAttachment->resolveTarget->sampleCount);
            cDepthResolveTarget.depthOps.loadOp = cppLoadOpToCLoadOp(descriptor.depthStencilAttachment->resolveTarget->depthLoadOp);
            cDepthResolveTarget.depthOps.storeOp = cppStoreOpToCStoreOp(descriptor.depthStencilAttachment->resolveTarget->depthStoreOp);
            cDepthResolveTarget.stencilOps.loadOp = cppLoadOpToCLoadOp(descriptor.depthStencilAttachment->resolveTarget->stencilLoadOp);
            cDepthResolveTarget.stencilOps.storeOp = cppStoreOpToCStoreOp(descriptor.depthStencilAttachment->resolveTarget->stencilStoreOp);
            cDepthResolveTarget.finalLayout = cppLayoutToCLayout(descriptor.depthStencilAttachment->resolveTarget->finalLayout);
            cDepthStencilAttachment.resolveTarget = &cDepthResolveTarget;
        } else {
            cDepthStencilAttachment.resolveTarget = nullptr;
        }

        cDepthStencilPtr = &cDepthStencilAttachment;
    }

    GfxRenderPassDescriptor cDesc = {};
    cDesc.label = descriptor.label.c_str();
    cDesc.colorAttachments = cColorAttachments.empty() ? nullptr : cColorAttachments.data();
    cDesc.colorAttachmentCount = static_cast<uint32_t>(cColorAttachments.size());
    cDesc.depthStencilAttachment = cDepthStencilPtr;

    GfxRenderPass renderPass = nullptr;
    GfxResult result = gfxDeviceCreateRenderPass(m_handle, &cDesc, &renderPass);
    if (result != GFX_RESULT_SUCCESS || !renderPass) {
        throw std::runtime_error("Failed to create render pass");
    }
    return std::make_shared<RenderPassImpl>(renderPass);
}

std::shared_ptr<Framebuffer> DeviceImpl::createFramebuffer(const FramebufferDescriptor& descriptor)
{
    auto renderPassImpl = std::dynamic_pointer_cast<RenderPassImpl>(descriptor.renderPass);
    if (!renderPassImpl) {
        throw std::runtime_error("Invalid render pass type");
    }

    // Convert color attachments
    std::vector<GfxFramebufferAttachment> cColorAttachments;
    for (const auto& attachment : descriptor.colorAttachments) {
        GfxFramebufferAttachment cAttachment = {};

        auto viewImpl = std::dynamic_pointer_cast<TextureViewImpl>(attachment.view);
        if (!viewImpl) {
            throw std::runtime_error("Invalid texture view type");
        }
        cAttachment.view = viewImpl->getHandle();

        if (attachment.resolveTarget) {
            auto resolveImpl = std::dynamic_pointer_cast<TextureViewImpl>(attachment.resolveTarget);
            if (!resolveImpl) {
                throw std::runtime_error("Invalid resolve target texture view type");
            }
            cAttachment.resolveTarget = resolveImpl->getHandle();
        } else {
            cAttachment.resolveTarget = nullptr;
        }

        cColorAttachments.push_back(cAttachment);
    }

    // Convert depth/stencil attachment if present
    GfxFramebufferAttachment cDepthStencilAttachment = { nullptr, nullptr };

    if (descriptor.depthStencilAttachment) {
        auto viewImpl = std::dynamic_pointer_cast<TextureViewImpl>(descriptor.depthStencilAttachment->view);
        if (!viewImpl) {
            throw std::runtime_error("Invalid depth/stencil texture view type");
        }
        cDepthStencilAttachment.view = viewImpl->getHandle();

        if (descriptor.depthStencilAttachment->resolveTarget) {
            auto resolveImpl = std::dynamic_pointer_cast<TextureViewImpl>(descriptor.depthStencilAttachment->resolveTarget);
            if (!resolveImpl) {
                throw std::runtime_error("Invalid depth/stencil resolve target texture view type");
            }
            cDepthStencilAttachment.resolveTarget = resolveImpl->getHandle();
        }
    }

    GfxFramebufferDescriptor cDesc = {};
    cDesc.label = descriptor.label.c_str();
    cDesc.renderPass = renderPassImpl->getHandle();
    cDesc.colorAttachments = cColorAttachments.empty() ? nullptr : cColorAttachments.data();
    cDesc.colorAttachmentCount = static_cast<uint32_t>(cColorAttachments.size());
    cDesc.depthStencilAttachment = cDepthStencilAttachment;
    cDesc.width = descriptor.width;
    cDesc.height = descriptor.height;

    GfxFramebuffer framebuffer = nullptr;
    GfxResult result = gfxDeviceCreateFramebuffer(m_handle, &cDesc, &framebuffer);
    if (result != GFX_RESULT_SUCCESS || !framebuffer) {
        throw std::runtime_error("Failed to create framebuffer");
    }
    return std::make_shared<FramebufferImpl>(framebuffer, renderPassImpl->getHandle());
}

std::shared_ptr<CommandEncoder> DeviceImpl::createCommandEncoder(const CommandEncoderDescriptor& descriptor)
{
    GfxCommandEncoderDescriptor cDesc = {};
    cDesc.label = descriptor.label.c_str();

    GfxCommandEncoder encoder = nullptr;
    GfxResult result = gfxDeviceCreateCommandEncoder(m_handle, &cDesc, &encoder);
    if (result != GFX_RESULT_SUCCESS || !encoder) {
        throw std::runtime_error("Failed to create command encoder");
    }
    return std::make_shared<CommandEncoderImpl>(encoder);
}

std::shared_ptr<Fence> DeviceImpl::createFence(const FenceDescriptor& descriptor)
{
    GfxFenceDescriptor cDesc = {};
    cDesc.label = descriptor.label.c_str();
    cDesc.signaled = descriptor.signaled;

    GfxFence fence = nullptr;
    GfxResult result = gfxDeviceCreateFence(m_handle, &cDesc, &fence);
    if (result != GFX_RESULT_SUCCESS || !fence) {
        throw std::runtime_error("Failed to create fence");
    }
    return std::make_shared<FenceImpl>(fence);
}

std::shared_ptr<Semaphore> DeviceImpl::createSemaphore(const SemaphoreDescriptor& descriptor)
{
    GfxSemaphoreDescriptor cDesc = {};
    cDesc.label = descriptor.label.c_str();
    cDesc.type = cppSemaphoreTypeToCSemaphoreType(descriptor.type);
    cDesc.initialValue = descriptor.initialValue;

    GfxSemaphore semaphore = nullptr;
    GfxResult result = gfxDeviceCreateSemaphore(m_handle, &cDesc, &semaphore);
    if (result != GFX_RESULT_SUCCESS || !semaphore) {
        throw std::runtime_error("Failed to create semaphore");
    }
    return std::make_shared<SemaphoreImpl>(semaphore);
}

void DeviceImpl::waitIdle()
{
    gfxDeviceWaitIdle(m_handle);
}

DeviceLimits DeviceImpl::getLimits() const
{
    GfxDeviceLimits cLimits;
    gfxDeviceGetLimits(m_handle, &cLimits);

    DeviceLimits limits;
    limits.minUniformBufferOffsetAlignment = cLimits.minUniformBufferOffsetAlignment;
    limits.minStorageBufferOffsetAlignment = cLimits.minStorageBufferOffsetAlignment;
    limits.maxUniformBufferBindingSize = cLimits.maxUniformBufferBindingSize;
    limits.maxStorageBufferBindingSize = cLimits.maxStorageBufferBindingSize;
    limits.maxBufferSize = cLimits.maxBufferSize;
    limits.maxTextureDimension1D = cLimits.maxTextureDimension1D;
    limits.maxTextureDimension2D = cLimits.maxTextureDimension2D;
    limits.maxTextureDimension3D = cLimits.maxTextureDimension3D;
    limits.maxTextureArrayLayers = cLimits.maxTextureArrayLayers;
    return limits;
}

} // namespace gfx
