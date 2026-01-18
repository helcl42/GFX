#include "Sampler.h"

#include "../system/Device.h"

#include <algorithm>
#include <stdexcept>

namespace gfx::backend::webgpu::core {

Sampler::Sampler(Device* device, const SamplerCreateInfo& createInfo)
    : m_device(device)
{
    WGPUSamplerDescriptor desc = WGPU_SAMPLER_DESCRIPTOR_INIT;
    desc.addressModeU = createInfo.addressModeU;
    desc.addressModeV = createInfo.addressModeV;
    desc.addressModeW = createInfo.addressModeW;
    desc.magFilter = createInfo.magFilter;
    desc.minFilter = createInfo.minFilter;
    desc.mipmapFilter = createInfo.mipmapFilter;
    desc.lodMinClamp = createInfo.lodMinClamp;
    desc.lodMaxClamp = createInfo.lodMaxClamp;
    desc.maxAnisotropy = std::max<uint16_t>(1, createInfo.maxAnisotropy);
    desc.compare = createInfo.compareFunction;

    m_sampler = wgpuDeviceCreateSampler(m_device->handle(), &desc);
    if (!m_sampler) {
        throw std::runtime_error("Failed to create WebGPU sampler");
    }
}

Sampler::~Sampler()
{
    if (m_sampler) {
        wgpuSamplerRelease(m_sampler);
    }
}

WGPUSampler Sampler::handle() const
{
    return m_sampler;
}

} // namespace gfx::backend::webgpu::core