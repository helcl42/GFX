#include "QuerySet.h"

#include "../system/Device.h"

#include <stdexcept>

namespace gfx::backend::vulkan::core {

QuerySet::QuerySet(Device* device, const QuerySetCreateInfo& createInfo)
    : m_device(device)
    , m_type(createInfo.type)
    , m_count(createInfo.count)
{
    VkQueryPoolCreateInfo poolCreateInfo{};
    poolCreateInfo.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
    poolCreateInfo.queryType = createInfo.type;
    poolCreateInfo.queryCount = createInfo.count;

    VkResult result = vkCreateQueryPool(m_device->handle(), &poolCreateInfo, nullptr, &m_queryPool);
    if (result != VK_SUCCESS) {
        throw std::runtime_error("Failed to create Vulkan query pool");
    }
}

QuerySet::~QuerySet()
{
    if (m_queryPool != VK_NULL_HANDLE) {
        vkDestroyQueryPool(m_device->handle(), m_queryPool, nullptr);
    }
}

VkQueryPool QuerySet::handle() const
{
    return m_queryPool;
}

Device* QuerySet::getDevice() const
{
    return m_device;
}

VkQueryType QuerySet::getType() const
{
    return m_type;
}

uint32_t QuerySet::getCount() const
{
    return m_count;
}

} // namespace gfx::backend::vulkan::core
