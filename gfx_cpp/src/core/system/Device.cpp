#include "Device.h"

#include "Queue.h"

#include "../command/CommandEncoder.h"
#include "../compute/ComputePipeline.h"
#include "../presentation/Surface.h"
#include "../presentation/Swapchain.h"
#include "../query/QuerySet.h"
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
    GfxResult result = gfxDeviceGetQueue(m_handle, &queueHandle);
    if (result != GFX_RESULT_SUCCESS || !queueHandle) {
        throw std::runtime_error("Failed to get device queue");
    }
    m_queue = std::make_shared<QueueImpl>(queueHandle);
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

std::shared_ptr<Queue> DeviceImpl::getQueueByIndex(uint32_t queueFamilyIndex, uint32_t queueIndex)
{
    GfxQueue queueHandle = nullptr;
    GfxResult result = gfxDeviceGetQueueByIndex(m_handle, queueFamilyIndex, queueIndex, &queueHandle);
    if (result != GFX_RESULT_SUCCESS || !queueHandle) {
        throw std::runtime_error("Failed to get queue by index");
    }
    return std::make_shared<QueueImpl>(queueHandle);
}

std::shared_ptr<Surface> DeviceImpl::createSurface(const SurfaceDescriptor& descriptor)
{
    GfxSurfaceDescriptor cDesc;
    convertSurfaceDescriptor(descriptor, cDesc);

    GfxSurface surface = nullptr;
    GfxResult result = gfxDeviceCreateSurface(m_handle, &cDesc, &surface);
    if (result != GFX_RESULT_SUCCESS || !surface) {
        throw std::runtime_error("Failed to create surface");
    }
    return std::make_shared<SurfaceImpl>(surface);
}

std::shared_ptr<Swapchain> DeviceImpl::createSwapchain(const SwapchainDescriptor& descriptor)
{
    if (!descriptor.surface) {
        throw std::runtime_error("Surface is required in SwapchainDescriptor");
    }

    auto surfaceImpl = std::dynamic_pointer_cast<SurfaceImpl>(descriptor.surface);
    if (!surfaceImpl) {
        throw std::runtime_error("Invalid surface type");
    }

    GfxSwapchainDescriptor cDesc;
    convertSwapchainDescriptor(descriptor, cDesc, surfaceImpl->getHandle());

    GfxSwapchain swapchain = nullptr;
    GfxResult result = gfxDeviceCreateSwapchain(m_handle, &cDesc, &swapchain);
    if (result != GFX_RESULT_SUCCESS || !swapchain) {
        throw std::runtime_error("Failed to create swapchain");
    }
    return std::make_shared<SwapchainImpl>(swapchain);
}

std::shared_ptr<Buffer> DeviceImpl::createBuffer(const BufferDescriptor& descriptor)
{
    GfxBufferDescriptor cDesc;
    convertBufferDescriptor(descriptor, cDesc);

    GfxBuffer buffer = nullptr;
    GfxResult result = gfxDeviceCreateBuffer(m_handle, &cDesc, &buffer);
    if (result != GFX_RESULT_SUCCESS || !buffer) {
        throw std::runtime_error("Failed to create buffer");
    }
    return std::make_shared<BufferImpl>(buffer);
}

std::shared_ptr<Buffer> DeviceImpl::importBuffer(const BufferImportDescriptor& descriptor)
{
    GfxBufferImportDescriptor cDesc;
    convertBufferImportDescriptor(descriptor, cDesc);

    GfxBuffer buffer = nullptr;
    GfxResult result = gfxDeviceImportBuffer(m_handle, &cDesc, &buffer);
    if (result != GFX_RESULT_SUCCESS || !buffer) {
        throw std::runtime_error("Failed to import buffer");
    }
    return std::make_shared<BufferImpl>(buffer);
}

std::shared_ptr<Texture> DeviceImpl::createTexture(const TextureDescriptor& descriptor)
{
    GfxTextureDescriptor cDesc;
    convertTextureDescriptor(descriptor, cDesc);

    GfxTexture texture = nullptr;
    GfxResult result = gfxDeviceCreateTexture(m_handle, &cDesc, &texture);
    if (result != GFX_RESULT_SUCCESS || !texture) {
        throw std::runtime_error("Failed to create texture");
    }
    return std::make_shared<TextureImpl>(texture);
}

std::shared_ptr<Texture> DeviceImpl::importTexture(const TextureImportDescriptor& descriptor)
{
    GfxTextureImportDescriptor cDesc;
    convertTextureImportDescriptor(descriptor, cDesc);

    GfxTexture texture = nullptr;
    GfxResult result = gfxDeviceImportTexture(m_handle, &cDesc, &texture);
    if (result != GFX_RESULT_SUCCESS || !texture) {
        throw std::runtime_error("Failed to import texture");
    }
    return std::make_shared<TextureImpl>(texture);
}

std::shared_ptr<Sampler> DeviceImpl::createSampler(const SamplerDescriptor& descriptor)
{
    GfxSamplerDescriptor cDesc;
    convertSamplerDescriptor(descriptor, cDesc);

    GfxSampler sampler = nullptr;
    GfxResult result = gfxDeviceCreateSampler(m_handle, &cDesc, &sampler);
    if (result != GFX_RESULT_SUCCESS || !sampler) {
        throw std::runtime_error("Failed to create sampler");
    }
    return std::make_shared<SamplerImpl>(sampler);
}

std::shared_ptr<Shader> DeviceImpl::createShader(const ShaderDescriptor& descriptor)
{
    GfxShaderDescriptor cDesc;
    convertShaderDescriptor(descriptor, cDesc);

    GfxShader shader = nullptr;
    GfxResult result = gfxDeviceCreateShader(m_handle, &cDesc, &shader);
    if (result != GFX_RESULT_SUCCESS || !shader) {
        throw std::runtime_error("Failed to create shader");
    }
    return std::make_shared<ShaderImpl>(shader);
}

std::shared_ptr<BindGroupLayout> DeviceImpl::createBindGroupLayout(const BindGroupLayoutDescriptor& descriptor)
{
    std::vector<GfxBindGroupLayoutEntry> cEntries;
    GfxBindGroupLayoutDescriptor cDesc;
    convertBindGroupLayoutDescriptor(descriptor, cEntries, cDesc);

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

    std::vector<GfxBindGroupEntry> cEntries;
    GfxBindGroupDescriptor cDesc;
    convertBindGroupDescriptor(descriptor, cEntries, cDesc);
    cDesc.layout = layoutImpl->getHandle();

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

    // Convert vertex state
    std::vector<std::vector<GfxVertexAttribute>> cAttributesPerBuffer;
    std::vector<GfxVertexBufferLayout> cVertexBuffers;
    GfxVertexState cVertexState;
    convertVertexState(descriptor.vertex, vertexShaderImpl->getHandle(), cAttributesPerBuffer, cVertexBuffers, cVertexState);

    // Convert fragment state (optional)
    std::optional<GfxFragmentState> cFragmentState;
    std::vector<GfxColorTargetState> cColorTargets;
    std::vector<GfxBlendState> cBlendStates;

    if (descriptor.fragment.has_value()) {
        const auto& fragment = *descriptor.fragment;
        auto fragmentShaderImpl = std::dynamic_pointer_cast<ShaderImpl>(fragment.module);
        if (!fragmentShaderImpl) {
            throw std::runtime_error("Invalid fragment shader type");
        }

        cFragmentState.emplace();
        convertFragmentState(fragment, fragmentShaderImpl->getHandle(), cColorTargets, cBlendStates, *cFragmentState);
    }

    // Convert primitive state
    GfxPrimitiveState cPrimitiveState;
    convertPrimitiveState(descriptor.primitive, cPrimitiveState);

    // Convert depth/stencil state (optional)
    std::optional<GfxDepthStencilState> cDepthStencilState;
    if (descriptor.depthStencil.has_value()) {
        cDepthStencilState.emplace();
        convertDepthStencilState(*descriptor.depthStencil, *cDepthStencilState);
    }

    // Extract render pass handle
    auto renderPassImpl = std::dynamic_pointer_cast<RenderPassImpl>(descriptor.renderPass);
    if (!renderPassImpl) {
        throw std::runtime_error("Invalid render pass type");
    }

    // Create pipeline descriptor
    std::vector<GfxBindGroupLayout> cBindGroupLayouts;
    GfxRenderPipelineDescriptor cDesc;
    convertRenderPipelineDescriptor(descriptor, renderPassImpl->getHandle(), cVertexState,
        cFragmentState, cPrimitiveState, cDepthStencilState,
        cBindGroupLayouts, cDesc);

    GfxRenderPipeline pipeline = nullptr;
    GfxResult result = gfxDeviceCreateRenderPipeline(m_handle, &cDesc, &pipeline);
    if (result != GFX_RESULT_SUCCESS || !pipeline) {
        throw std::runtime_error("Failed to create render pipeline");
    }
    return std::make_shared<RenderPipelineImpl>(pipeline);
}

std::shared_ptr<ComputePipeline> DeviceImpl::createComputePipeline(const ComputePipelineDescriptor& descriptor)
{
    if (!descriptor.compute) {
        throw std::invalid_argument("Compute shader cannot be null");
    }

    auto shaderImpl = std::dynamic_pointer_cast<ShaderImpl>(descriptor.compute);
    if (!shaderImpl) {
        throw std::runtime_error("Invalid shader type");
    }

    std::vector<GfxBindGroupLayout> bindGroupLayoutHandles;
    GfxComputePipelineDescriptor cDesc;
    convertComputePipelineDescriptor(descriptor, shaderImpl->getHandle(), bindGroupLayoutHandles, cDesc);

    GfxComputePipeline pipeline = nullptr;
    GfxResult result = gfxDeviceCreateComputePipeline(m_handle, &cDesc, &pipeline);
    if (result != GFX_RESULT_SUCCESS || !pipeline) {
        throw std::runtime_error("Failed to create compute pipeline");
    }
    return std::make_shared<ComputePipelineImpl>(pipeline);
}

std::shared_ptr<RenderPass> DeviceImpl::createRenderPass(const RenderPassCreateDescriptor& descriptor)
{
    std::vector<GfxRenderPassColorAttachment> cColorAttachments;
    std::vector<GfxRenderPassColorAttachmentTarget> cColorTargets;
    std::vector<GfxRenderPassColorAttachmentTarget> cColorResolveTargets;
    GfxRenderPassDepthStencilAttachment cDepthStencilAttachment;
    GfxRenderPassDepthStencilAttachmentTarget cDepthTarget;
    GfxRenderPassDepthStencilAttachmentTarget cDepthResolveTarget;
    GfxRenderPassDescriptor cDesc;
    convertRenderPassDescriptor(descriptor, cColorAttachments, cColorTargets, cColorResolveTargets, cDepthStencilAttachment, cDepthTarget, cDepthResolveTarget, cDesc);

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

    std::vector<GfxFramebufferAttachment> cColorAttachments;
    GfxFramebufferAttachment cDepthStencilAttachment;
    GfxFramebufferDescriptor cDesc;
    convertFramebufferDescriptor(descriptor, renderPassImpl->getHandle(), cColorAttachments, cDepthStencilAttachment, cDesc);

    GfxFramebuffer framebuffer = nullptr;
    GfxResult result = gfxDeviceCreateFramebuffer(m_handle, &cDesc, &framebuffer);
    if (result != GFX_RESULT_SUCCESS || !framebuffer) {
        throw std::runtime_error("Failed to create framebuffer");
    }
    return std::make_shared<FramebufferImpl>(framebuffer, renderPassImpl->getHandle());
}

std::shared_ptr<CommandEncoder> DeviceImpl::createCommandEncoder(const CommandEncoderDescriptor& descriptor)
{
    GfxCommandEncoderDescriptor cDesc;
    convertCommandEncoderDescriptor(descriptor, cDesc);

    GfxCommandEncoder encoder = nullptr;
    GfxResult result = gfxDeviceCreateCommandEncoder(m_handle, &cDesc, &encoder);
    if (result != GFX_RESULT_SUCCESS || !encoder) {
        throw std::runtime_error("Failed to create command encoder");
    }
    return std::make_shared<CommandEncoderImpl>(encoder);
}

std::shared_ptr<Fence> DeviceImpl::createFence(const FenceDescriptor& descriptor)
{
    GfxFenceDescriptor cDesc;
    convertFenceDescriptor(descriptor, cDesc);

    GfxFence fence = nullptr;
    GfxResult result = gfxDeviceCreateFence(m_handle, &cDesc, &fence);
    if (result != GFX_RESULT_SUCCESS || !fence) {
        throw std::runtime_error("Failed to create fence");
    }
    return std::make_shared<FenceImpl>(fence);
}

std::shared_ptr<Semaphore> DeviceImpl::createSemaphore(const SemaphoreDescriptor& descriptor)
{
    GfxSemaphoreDescriptor cDesc;
    convertSemaphoreDescriptor(descriptor, cDesc);

    GfxSemaphore semaphore = nullptr;
    GfxResult result = gfxDeviceCreateSemaphore(m_handle, &cDesc, &semaphore);
    if (result != GFX_RESULT_SUCCESS || !semaphore) {
        throw std::runtime_error("Failed to create semaphore");
    }
    return std::make_shared<SemaphoreImpl>(semaphore);
}

std::shared_ptr<QuerySet> DeviceImpl::createQuerySet(const QuerySetDescriptor& descriptor)
{
    GfxQuerySetDescriptor cDesc;
    convertQuerySetDescriptor(descriptor, cDesc);

    GfxQuerySet querySet = nullptr;
    GfxResult result = gfxDeviceCreateQuerySet(m_handle, &cDesc, &querySet);
    if (result != GFX_RESULT_SUCCESS || !querySet) {
        throw std::runtime_error("Failed to create query set");
    }
    return std::make_shared<QuerySetImpl>(querySet, descriptor.type, descriptor.count);
}

void DeviceImpl::waitIdle()
{
    gfxDeviceWaitIdle(m_handle);
}

DeviceLimits DeviceImpl::getLimits() const
{
    GfxDeviceLimits cLimits;
    gfxDeviceGetLimits(m_handle, &cLimits);
    return cDeviceLimitsToCppDeviceLimits(cLimits);
}

bool DeviceImpl::supportsShaderFormat(ShaderSourceType format) const
{
    bool supported = false;
    GfxShaderSourceType cFormat = cppShaderSourceTypeToCShaderSourceType(format);
    GfxResult result = gfxDeviceSupportsShaderFormat(m_handle, cFormat, &supported);
    if (result != GFX_RESULT_SUCCESS) {
        return false;
    }
    return supported;
}

} // namespace gfx
