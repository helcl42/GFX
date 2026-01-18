#ifndef GFX_WEBGPU_BIND_GROUP_LAYOUT_H
#define GFX_WEBGPU_BIND_GROUP_LAYOUT_H

#include "../CoreTypes.h"

namespace gfx::backend::webgpu::core {

class Device;

class BindGroupLayout {
public:
    // Prevent copying
    BindGroupLayout(const BindGroupLayout&) = delete;
    BindGroupLayout& operator=(const BindGroupLayout&) = delete;

    BindGroupLayout(Device* device, const BindGroupLayoutCreateInfo& createInfo);
    ~BindGroupLayout();

    WGPUBindGroupLayout handle() const;

private:
    WGPUBindGroupLayout m_layout = nullptr;
};

} // namespace gfx::backend::webgpu::core

#endif // GFX_WEBGPU_BIND_GROUP_LAYOUT_H