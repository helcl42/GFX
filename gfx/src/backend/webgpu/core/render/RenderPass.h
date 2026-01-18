#ifndef GFX_WEBGPU_RENDER_PASS_H
#define GFX_WEBGPU_RENDER_PASS_H

#include "../CoreTypes.h"

namespace gfx::backend::webgpu::core {

class Device;

class RenderPass {
public:
    // Prevent copying
    RenderPass(const RenderPass&) = delete;
    RenderPass& operator=(const RenderPass&) = delete;

    RenderPass(Device* device, const RenderPassCreateInfo& createInfo);
    ~RenderPass() = default;

    const RenderPassCreateInfo& getCreateInfo() const;
    Device* getDevice() const;

private:
    Device* m_device = nullptr; // Non-owning pointer
    RenderPassCreateInfo m_createInfo;
};

} // namespace gfx::backend::webgpu::core

#endif // GFX_WEBGPU_RENDER_PASS_H