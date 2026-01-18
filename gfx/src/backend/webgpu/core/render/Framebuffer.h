#ifndef GFX_WEBGPU_FRAMEBUFFER_H
#define GFX_WEBGPU_FRAMEBUFFER_H

#include "../CoreTypes.h"

namespace gfx::backend::webgpu::core {

class Device;

class Framebuffer {
public:
    // Prevent copying
    Framebuffer(const Framebuffer&) = delete;
    Framebuffer& operator=(const Framebuffer&) = delete;

    Framebuffer(Device* device, const FramebufferCreateInfo& createInfo);
    ~Framebuffer() = default;

    const FramebufferCreateInfo& getCreateInfo() const;
    Device* getDevice() const;

private:
    Device* m_device = nullptr; // Non-owning pointer
    FramebufferCreateInfo m_createInfo;
};

} // namespace gfx::backend::webgpu::core

#endif // GFX_WEBGPU_FRAMEBUFFER_H