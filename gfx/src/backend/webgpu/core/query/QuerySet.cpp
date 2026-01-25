#include "QuerySet.h"

#include "../system/Device.h"

#include <stdexcept>

namespace gfx::backend::webgpu::core {

QuerySet::QuerySet(Device* device, const QuerySetCreateInfo& createInfo)
    : m_device(device)
    , m_type(createInfo.type)
    , m_count(createInfo.count)
{
    WGPUQuerySetDescriptor descriptor{};

    if (createInfo.label) {
        descriptor.label = WGPUStringView{ createInfo.label, WGPU_STRLEN };
    }

    descriptor.count = createInfo.count;
    descriptor.type = createInfo.type;

    m_querySet = wgpuDeviceCreateQuerySet(m_device->handle(), &descriptor);
    if (!m_querySet) {
        throw std::runtime_error("Failed to create WebGPU query set");
    }
}

QuerySet::~QuerySet()
{
    if (m_querySet) {
        wgpuQuerySetRelease(m_querySet);
    }
}

WGPUQuerySet QuerySet::handle() const
{
    return m_querySet;
}

Device* QuerySet::getDevice() const
{
    return m_device;
}

WGPUQueryType QuerySet::getType() const
{
    return m_type;
}

uint32_t QuerySet::getCount() const
{
    return m_count;
}

} // namespace gfx::backend::webgpu::core
