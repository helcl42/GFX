#include "ComputePipeline.h"

#include "../system/Device.h"

#include <stdexcept>

namespace gfx::backend::webgpu::core {

ComputePipeline::ComputePipeline(Device* device, const ComputePipelineCreateInfo& createInfo)
{
    WGPUComputePipelineDescriptor desc = WGPU_COMPUTE_PIPELINE_DESCRIPTOR_INIT;

    // Create pipeline layout if bind group layouts are provided
    WGPUPipelineLayout pipelineLayout = nullptr;
    if (!createInfo.bindGroupLayouts.empty()) {
        WGPUPipelineLayoutDescriptor layoutDesc = WGPU_PIPELINE_LAYOUT_DESCRIPTOR_INIT;
        layoutDesc.bindGroupLayouts = createInfo.bindGroupLayouts.data();
        layoutDesc.bindGroupLayoutCount = static_cast<uint32_t>(createInfo.bindGroupLayouts.size());
        pipelineLayout = wgpuDeviceCreatePipelineLayout(device->handle(), &layoutDesc);
        desc.layout = pipelineLayout;
    }

    desc.compute.module = createInfo.module;
    desc.compute.entryPoint = { createInfo.entryPoint, WGPU_STRLEN };

    m_pipeline = wgpuDeviceCreateComputePipeline(device->handle(), &desc);

    // Release the pipeline layout if we created one (pipeline holds its own reference)
    if (pipelineLayout) {
        wgpuPipelineLayoutRelease(pipelineLayout);
    }

    if (!m_pipeline) {
        throw std::runtime_error("Failed to create WebGPU ComputePipeline");
    }
}

ComputePipeline::~ComputePipeline()
{
    if (m_pipeline) {
        wgpuComputePipelineRelease(m_pipeline);
    }
}

WGPUComputePipeline ComputePipeline::handle() const
{
    return m_pipeline;
}

} // namespace gfx::backend::webgpu::core