#ifndef GFX_VULKAN_RENDERPASS_H
#define GFX_VULKAN_RENDERPASS_H

#include "CoreTypes.h"

namespace gfx::backend::vulkan::core {

class Device;

class RenderPass {
public:
    RenderPass(const RenderPass&) = delete;
    RenderPass& operator=(const RenderPass&) = delete;

    RenderPass(Device* device, const RenderPassCreateInfo& createInfo);
    ~RenderPass();

    VkRenderPass handle() const;
    uint32_t colorAttachmentCount() const;
    bool hasDepthStencil() const;
    const std::vector<bool>& colorHasResolve() const;

private:
    VkRenderPass m_renderPass = VK_NULL_HANDLE;
    Device* m_device = nullptr;
    uint32_t m_colorAttachmentCount = 0;
    bool m_hasDepthStencil = false;
    std::vector<bool> m_colorHasResolve; // Track which color attachments have resolve targets
};

} // namespace gfx::backend::vulkan::core

#endif // GFX_VULKAN_RENDERPASS_H