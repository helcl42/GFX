#ifndef GFX_WEBGPU_INSTANCE_H
#define GFX_WEBGPU_INSTANCE_H

#include "../CoreTypes.h"

namespace gfx::backend::webgpu::core {

class Instance {
public:
    // Prevent copying
    Instance(const Instance&) = delete;
    Instance& operator=(const Instance&) = delete;

    Instance(const InstanceCreateInfo& createInfo);
    ~Instance();

    WGPUInstance handle() const;

private:
    WGPUInstance m_instance = nullptr;
};

} // namespace gfx::backend::webgpu::core

#endif // GFX_WEBGPU_INSTANCE_H