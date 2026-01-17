#ifndef GFX_VULKAN_BINDGROUPLAYOUT_H
#define GFX_VULKAN_BINDGROUPLAYOUT_H

#include "CoreTypes.h"

#include <unordered_map>

namespace gfx::backend::vulkan::core {

class Device;

class BindGroupLayout {
public:
    BindGroupLayout(const BindGroupLayout&) = delete;
    BindGroupLayout& operator=(const BindGroupLayout&) = delete;

    BindGroupLayout(Device* device, const BindGroupLayoutCreateInfo& createInfo);
    ~BindGroupLayout();

    VkDescriptorSetLayout handle() const;
    VkDescriptorType getBindingType(uint32_t binding) const;

private:
    VkDescriptorSetLayout m_layout = VK_NULL_HANDLE;
    Device* m_device = nullptr;
    std::unordered_map<uint32_t, VkDescriptorType> m_bindingTypes;
};

} // namespace gfx::backend::vulkan::core

#endif // GFX_VULKAN_BINDGROUPLAYOUT_H