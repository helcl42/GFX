#include "../render/Framebuffer.h"

#include "../system/Device.h"

namespace gfx::backend::webgpu::core {

Framebuffer::Framebuffer(Device* device, const FramebufferCreateInfo& createInfo)
    : m_device(device)
    , m_createInfo(createInfo)
{
    // WebGPU doesn't have framebuffer objects
    // Just store the views for use in beginRenderPass
}

const FramebufferCreateInfo& Framebuffer::getCreateInfo() const
{
    return m_createInfo;
}

Device* Framebuffer::getDevice() const
{
    return m_device;
}

} // namespace gfx::backend::webgpu::core