#ifndef GFX_WEBGPU_SAMPLER_H
#define GFX_WEBGPU_SAMPLER_H

#include "../CoreTypes.h"

namespace gfx::backend::webgpu::core {

class Device;

class Sampler {
public:
    // Prevent copying
    Sampler(const Sampler&) = delete;
    Sampler& operator=(const Sampler&) = delete;

    Sampler(Device* device, const SamplerCreateInfo& createInfo);
    ~Sampler();

    WGPUSampler handle() const;

private:
    WGPUSampler m_sampler = nullptr;
    Device* m_device = nullptr; // Non-owning pointer for device operations
};

} // namespace gfx::backend::webgpu::core

#endif // GFX_WEBGPU_SAMPLER_H