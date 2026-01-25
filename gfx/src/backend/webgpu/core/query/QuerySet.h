#ifndef GFX_BACKEND_WEBGPU_CORE_QUERY_SET_H
#define GFX_BACKEND_WEBGPU_CORE_QUERY_SET_H

#include "../CoreTypes.h"

#include <webgpu/webgpu.h>

namespace gfx::backend::webgpu::core {

class Device;

class QuerySet {
public:
    QuerySet(Device* device, const QuerySetCreateInfo& createInfo);
    ~QuerySet();

    QuerySet(const QuerySet&) = delete;
    QuerySet& operator=(const QuerySet&) = delete;

    WGPUQuerySet handle() const;
    Device* getDevice() const;
    WGPUQueryType getType() const;
    uint32_t getCount() const;

private:
    Device* m_device = nullptr;
    WGPUQuerySet m_querySet = nullptr;
    WGPUQueryType m_type = WGPUQueryType_Occlusion;
    uint32_t m_count = 0;
};

} // namespace gfx::backend::webgpu::core

#endif // GFX_BACKEND_WEBGPU_CORE_QUERY_SET_H
