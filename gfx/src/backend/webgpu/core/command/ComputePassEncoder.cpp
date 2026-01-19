#include "ComputePassEncoder.h"

#include "../command/CommandEncoder.h"
#include "../util/Utils.h"

#include <stdexcept>

namespace gfx::backend::webgpu::core {

ComputePassEncoder::ComputePassEncoder(CommandEncoder* commandEncoder, const ComputePassEncoderCreateInfo& createInfo)
{
    WGPUComputePassDescriptor wgpuDesc = WGPU_COMPUTE_PASS_DESCRIPTOR_INIT;
    if (createInfo.label) {
        wgpuDesc.label = toStringView(createInfo.label);
    }

    m_encoder = wgpuCommandEncoderBeginComputePass(commandEncoder->handle(), &wgpuDesc);
    if (!m_encoder) {
        throw std::runtime_error("Failed to create compute pass encoder");
    }
}

ComputePassEncoder::~ComputePassEncoder()
{
    if (m_encoder) {
        if (!m_ended) {
            wgpuComputePassEncoderEnd(m_encoder);
        }
        wgpuComputePassEncoderRelease(m_encoder);
    }
}

void ComputePassEncoder::setPipeline(WGPUComputePipeline pipeline)
{
    wgpuComputePassEncoderSetPipeline(m_encoder, pipeline);
}

void ComputePassEncoder::setBindGroup(uint32_t index, WGPUBindGroup bindGroup, const uint32_t* dynamicOffsets, uint32_t dynamicOffsetCount)
{
    wgpuComputePassEncoderSetBindGroup(m_encoder, index, bindGroup, dynamicOffsetCount, dynamicOffsets);
}

void ComputePassEncoder::dispatchWorkgroups(uint32_t workgroupCountX, uint32_t workgroupCountY, uint32_t workgroupCountZ)
{
    wgpuComputePassEncoderDispatchWorkgroups(m_encoder, workgroupCountX, workgroupCountY, workgroupCountZ);
}

void ComputePassEncoder::dispatchIndirect(WGPUBuffer buffer, uint64_t offset)
{
    wgpuComputePassEncoderDispatchWorkgroupsIndirect(m_encoder, buffer, offset);
}

WGPUComputePassEncoder ComputePassEncoder::handle() const
{
    return m_encoder;
}

} // namespace gfx::backend::webgpu::core