#include "BindGroup.h"

#include "../system/Device.h"

#include <stdexcept>

namespace gfx::backend::webgpu::core {

BindGroup::BindGroup(Device* device, const BindGroupCreateInfo& createInfo)
    : m_device(device)
{
    WGPUBindGroupDescriptor desc = WGPU_BIND_GROUP_DESCRIPTOR_INIT;
    desc.layout = createInfo.layout;

    std::vector<WGPUBindGroupEntry> wgpuEntries;
    wgpuEntries.reserve(createInfo.entries.size());

    for (const auto& entry : createInfo.entries) {
        WGPUBindGroupEntry wgpuEntry = WGPU_BIND_GROUP_ENTRY_INIT;
        wgpuEntry.binding = entry.binding;
        wgpuEntry.buffer = entry.buffer;
        wgpuEntry.offset = entry.bufferOffset;
        wgpuEntry.size = entry.bufferSize;
        wgpuEntry.sampler = entry.sampler;
        wgpuEntry.textureView = entry.textureView;
        wgpuEntries.push_back(wgpuEntry);
    }

    desc.entries = wgpuEntries.data();
    desc.entryCount = static_cast<uint32_t>(wgpuEntries.size());

    m_bindGroup = wgpuDeviceCreateBindGroup(m_device->handle(), &desc);
    if (!m_bindGroup) {
        throw std::runtime_error("Failed to create WebGPU BindGroup");
    }
}

BindGroup::~BindGroup()
{
    if (m_bindGroup) {
        wgpuBindGroupRelease(m_bindGroup);
    }
}

WGPUBindGroup BindGroup::handle() const
{
    return m_bindGroup;
}

} // namespace gfx::backend::webgpu::core