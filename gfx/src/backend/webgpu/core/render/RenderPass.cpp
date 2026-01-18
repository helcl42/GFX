#include "../render/RenderPass.h"

#include "../system/Device.h"

namespace gfx::backend::webgpu::core {

RenderPass::RenderPass(Device* device, const RenderPassCreateInfo& createInfo)
    : m_device(device)
    , m_createInfo(createInfo)
{
    // WebGPU doesn't create persistent render pass objects
    // Just store the configuration for later use
}

const RenderPassCreateInfo& RenderPass::getCreateInfo() const
{
    return m_createInfo;
}

Device* RenderPass::getDevice() const
{
    return m_device;
}

} // namespace gfx::backend::webgpu::core