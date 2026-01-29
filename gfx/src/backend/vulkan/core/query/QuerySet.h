#ifndef GFX_BACKEND_VULKAN_CORE_QUERY_SET_H
#define GFX_BACKEND_VULKAN_CORE_QUERY_SET_H

#include "../CoreTypes.h"

#include "../../common/Common.h"

namespace gfx::backend::vulkan::core {

class Device;

class QuerySet {
public:
    QuerySet(Device* device, const QuerySetCreateInfo& createInfo);
    ~QuerySet();

    QuerySet(const QuerySet&) = delete;
    QuerySet& operator=(const QuerySet&) = delete;

    VkQueryPool handle() const;
    Device* getDevice() const;
    VkQueryType getType() const;
    uint32_t getCount() const;

private:
    Device* m_device = nullptr;
    VkQueryPool m_queryPool = VK_NULL_HANDLE;
    VkQueryType m_type = VK_QUERY_TYPE_OCCLUSION;
    uint32_t m_count = 0;
};

} // namespace gfx::backend::vulkan::core

#endif // GFX_BACKEND_VULKAN_CORE_QUERY_SET_H
