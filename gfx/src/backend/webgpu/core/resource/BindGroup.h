#ifndef GFX_WEBGPU_BIND_GROUP_H
#define GFX_WEBGPU_BIND_GROUP_H

#include "../CoreTypes.h"

namespace gfx::backend::webgpu::core {

class Device;

class BindGroup {
public:
    // Prevent copying
    BindGroup(const BindGroup&) = delete;
    BindGroup& operator=(const BindGroup&) = delete;

    BindGroup(Device* device, const BindGroupCreateInfo& createInfo);
    ~BindGroup();

    WGPUBindGroup handle() const;

private:
    WGPUBindGroup m_bindGroup = nullptr;
    Device* m_device = nullptr; // Non-owning pointer for device operations
};

} // namespace gfx::backend::webgpu::core

#endif // GFX_WEBGPU_BIND_GROUP_H