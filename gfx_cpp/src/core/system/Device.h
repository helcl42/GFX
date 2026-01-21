#ifndef GFX_CPP_DEVICE_H
#define GFX_CPP_DEVICE_H

#include <gfx_cpp/gfx.hpp>

#include <gfx/gfx.h>

#include <memory>

namespace gfx {

class DeviceImpl : public Device {
public:
    explicit DeviceImpl(GfxDevice h);
    ~DeviceImpl() override;

    std::shared_ptr<Queue> getQueue() override;
    std::shared_ptr<Queue> getQueueByIndex(uint32_t queueFamilyIndex, uint32_t queueIndex) override;

    std::shared_ptr<Surface> createSurface(const SurfaceDescriptor& descriptor) override;

    std::shared_ptr<Swapchain> createSwapchain(std::shared_ptr<Surface> surface, const SwapchainDescriptor& descriptor) override;

    std::shared_ptr<Buffer> createBuffer(const BufferDescriptor& descriptor) override;

    std::shared_ptr<Buffer> importBuffer(const BufferImportDescriptor& descriptor) override;

    std::shared_ptr<Texture> createTexture(const TextureDescriptor& descriptor) override;

    std::shared_ptr<Texture> importTexture(const TextureImportDescriptor& descriptor) override;

    std::shared_ptr<Sampler> createSampler(const SamplerDescriptor& descriptor = {}) override;

    std::shared_ptr<Shader> createShader(const ShaderDescriptor& descriptor) override;

    std::shared_ptr<BindGroupLayout> createBindGroupLayout(const BindGroupLayoutDescriptor& descriptor) override;

    std::shared_ptr<BindGroup> createBindGroup(const BindGroupDescriptor& descriptor) override;

    std::shared_ptr<RenderPipeline> createRenderPipeline(const RenderPipelineDescriptor& descriptor) override;

    std::shared_ptr<ComputePipeline> createComputePipeline(const ComputePipelineDescriptor& descriptor) override;

    std::shared_ptr<RenderPass> createRenderPass(const RenderPassCreateDescriptor& descriptor) override;

    std::shared_ptr<Framebuffer> createFramebuffer(const FramebufferDescriptor& descriptor) override;

    std::shared_ptr<CommandEncoder> createCommandEncoder(const CommandEncoderDescriptor& descriptor = {}) override;

    std::shared_ptr<Fence> createFence(const FenceDescriptor& descriptor = {}) override;

    std::shared_ptr<Semaphore> createSemaphore(const SemaphoreDescriptor& descriptor = {}) override;

    void waitIdle() override;

    DeviceLimits getLimits() const override;

private:
    GfxDevice m_handle;
    std::shared_ptr<class QueueImpl> m_queue;
};

} // namespace gfx

#endif // GFX_CPP_DEVICE_H
