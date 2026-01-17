#include "Sampler.h"

#include "Device.h"

#include <stdexcept>

namespace gfx::backend::vulkan::core {

Sampler::Sampler(Device* device, const SamplerCreateInfo& createInfo)
    : m_device(device)
{
    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;

    // Address modes
    samplerInfo.addressModeU = createInfo.addressModeU;
    samplerInfo.addressModeV = createInfo.addressModeV;
    samplerInfo.addressModeW = createInfo.addressModeW;

    // Filter modes
    samplerInfo.magFilter = createInfo.magFilter;
    samplerInfo.minFilter = createInfo.minFilter;
    samplerInfo.mipmapMode = createInfo.mipmapMode;

    // LOD
    samplerInfo.minLod = createInfo.lodMinClamp;
    samplerInfo.maxLod = createInfo.lodMaxClamp;

    // Anisotropy
    if (createInfo.maxAnisotropy > 1) {
        samplerInfo.anisotropyEnable = VK_TRUE;
        samplerInfo.maxAnisotropy = static_cast<float>(createInfo.maxAnisotropy);
    } else {
        samplerInfo.anisotropyEnable = VK_FALSE;
        samplerInfo.maxAnisotropy = 1.0f;
    }

    // Compare operation for depth textures
    if (createInfo.compareOp != VK_COMPARE_OP_MAX_ENUM) {
        samplerInfo.compareEnable = VK_TRUE;
        samplerInfo.compareOp = createInfo.compareOp;
    } else {
        samplerInfo.compareEnable = VK_FALSE;
    }

    VkResult result = vkCreateSampler(m_device->handle(), &samplerInfo, nullptr, &m_sampler);
    if (result != VK_SUCCESS) {
        throw std::runtime_error("Failed to create sampler");
    }
}

Sampler::~Sampler()
{
    if (m_sampler != VK_NULL_HANDLE) {
        vkDestroySampler(m_device->handle(), m_sampler, nullptr);
    }
}

VkSampler Sampler::handle() const
{
    return m_sampler;
}

} // namespace gfx::backend::vulkan::core
