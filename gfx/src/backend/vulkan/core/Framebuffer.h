#ifndef GFX_VULKAN_FRAMEBUFFER_H
#define GFX_VULKAN_FRAMEBUFFER_H

#include "CoreTypes.h"

namespace gfx::backend::vulkan::core {

class Device;

class Framebuffer {
public:
    Framebuffer(const Framebuffer&) = delete;
    Framebuffer& operator=(const Framebuffer&) = delete;

    Framebuffer(Device* device, const FramebufferCreateInfo& createInfo);
    ~Framebuffer();

    VkFramebuffer handle() const;
    uint32_t width() const;
    uint32_t height() const;

private:
    VkFramebuffer m_framebuffer = VK_NULL_HANDLE;
    Device* m_device = nullptr;
    uint32_t m_width = 0;
    uint32_t m_height = 0;
};

} // namespace gfx::backend::vulkan::core

#endif // GFX_VULKAN_FRAMEBUFFER_H