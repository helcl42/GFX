#ifndef GFX_VULKAN_BINDGROUP_H
#define GFX_VULKAN_BINDGROUP_H

#include "CoreTypes.h"

namespace gfx::backend::vulkan::core {

class Device;

class BindGroup {
public:
    BindGroup(const BindGroup&) = delete;
    BindGroup& operator=(const BindGroup&) = delete;

    BindGroup(Device* device, const BindGroupCreateInfo& createInfo);
    ~BindGroup();

    VkDescriptorSet handle() const;

private:
    VkDescriptorSet m_descriptorSet = VK_NULL_HANDLE;
    Device* m_device = nullptr;
    VkDescriptorPool m_pool = VK_NULL_HANDLE;
};

} // namespace gfx::backend::vulkan::core

#endif // GFX_VULKAN_BINDGROUP_H