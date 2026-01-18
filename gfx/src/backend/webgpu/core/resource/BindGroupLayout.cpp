#include "BindGroupLayout.h"

#include "../system/Device.h"

#include <stdexcept>

namespace gfx::backend::webgpu::core {

BindGroupLayout::BindGroupLayout(Device* device, const BindGroupLayoutCreateInfo& createInfo)
{
    WGPUBindGroupLayoutDescriptor desc = WGPU_BIND_GROUP_LAYOUT_DESCRIPTOR_INIT;

    std::vector<WGPUBindGroupLayoutEntry> wgpuEntries;
    wgpuEntries.reserve(createInfo.entries.size());

    for (const auto& entry : createInfo.entries) {
        WGPUBindGroupLayoutEntry wgpuEntry = WGPU_BIND_GROUP_LAYOUT_ENTRY_INIT;
        wgpuEntry.binding = entry.binding;
        wgpuEntry.visibility = entry.visibility;

        // Set binding type based on which field is used
        if (entry.bufferType != WGPUBufferBindingType_Undefined) {
            wgpuEntry.buffer.type = entry.bufferType;
            wgpuEntry.buffer.hasDynamicOffset = entry.bufferHasDynamicOffset;
            wgpuEntry.buffer.minBindingSize = entry.bufferMinBindingSize;
        }
        if (entry.samplerType != WGPUSamplerBindingType_Undefined) {
            wgpuEntry.sampler.type = entry.samplerType;
        }
        if (entry.textureSampleType != WGPUTextureSampleType_Undefined) {
            wgpuEntry.texture.sampleType = entry.textureSampleType;
            wgpuEntry.texture.viewDimension = entry.textureViewDimension;
            wgpuEntry.texture.multisampled = entry.textureMultisampled;
        }
        if (entry.storageTextureAccess != WGPUStorageTextureAccess_Undefined) {
            wgpuEntry.storageTexture.access = entry.storageTextureAccess;
            wgpuEntry.storageTexture.format = entry.storageTextureFormat;
            wgpuEntry.storageTexture.viewDimension = entry.storageTextureViewDimension;
        }

        wgpuEntries.push_back(wgpuEntry);
    }

    desc.entryCount = static_cast<uint32_t>(wgpuEntries.size());
    desc.entries = wgpuEntries.data();

    m_layout = wgpuDeviceCreateBindGroupLayout(device->handle(), &desc);
    if (!m_layout) {
        throw std::runtime_error("Failed to create WebGPU BindGroupLayout");
    }
}

BindGroupLayout::~BindGroupLayout()
{
    if (m_layout) {
        wgpuBindGroupLayoutRelease(m_layout);
    }
}

WGPUBindGroupLayout BindGroupLayout::handle() const
{
    return m_layout;
}

} // namespace gfx::backend::webgpu::core