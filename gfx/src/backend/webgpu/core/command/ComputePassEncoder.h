#ifndef GFX_WEBGPU_COMPUTE_PASS_ENCODER_H
#define GFX_WEBGPU_COMPUTE_PASS_ENCODER_H

#include "../CoreTypes.h"

namespace gfx::backend::webgpu::core {

class ComputePassEncoder {
public:
    // Prevent copying
    ComputePassEncoder(const ComputePassEncoder&) = delete;
    ComputePassEncoder& operator=(const ComputePassEncoder&) = delete;

    ComputePassEncoder(CommandEncoder* commandEncoder, const ComputePassEncoderCreateInfo& createInfo);
    ~ComputePassEncoder();

    void setPipeline(WGPUComputePipeline pipeline);
    void setBindGroup(uint32_t index, WGPUBindGroup bindGroup, const uint32_t* dynamicOffsets, uint32_t dynamicOffsetCount);

    void dispatchWorkgroups(uint32_t workgroupCountX, uint32_t workgroupCountY, uint32_t workgroupCountZ);
    void dispatchIndirect(WGPUBuffer buffer, uint64_t offset);

    WGPUComputePassEncoder handle() const;

private:
    WGPUComputePassEncoder m_encoder = nullptr;
    bool m_ended = false;
};

} // namespace gfx::backend::webgpu::core

#endif // GFX_WEBGPU_COMPUTE_PASS_ENCODER_H