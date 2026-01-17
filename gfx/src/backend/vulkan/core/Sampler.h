#ifndef GFX_VULKAN_SAMPLER_H
#define GFX_VULKAN_SAMPLER_H

#include "CoreTypes.h"

namespace gfx::backend::vulkan::core {

class Device;

class Sampler {
public:
    Sampler(const Sampler&) = delete;
    Sampler& operator=(const Sampler&) = delete;

    Sampler(Device* device, const SamplerCreateInfo& createInfo);
    ~Sampler();

    VkSampler handle() const;

private:
    VkSampler m_sampler = VK_NULL_HANDLE;
    Device* m_device = nullptr;
};

} // namespace gfx::backend::vulkan::core

#endif // GFX_VULKAN_SAMPLER_H