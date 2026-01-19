#include "ComputePassEncoder.h"

#include "../compute/ComputePipeline.h"
#include "../resource/BindGroup.h"
#include "../resource/Buffer.h"

#include <stdexcept>

namespace gfx {

ComputePassEncoderImpl::ComputePassEncoderImpl(GfxComputePassEncoder h)
    : m_handle(h)
{
}

ComputePassEncoderImpl::~ComputePassEncoderImpl()
{
    if (m_handle) {
        GfxResult result = gfxComputePassEncoderEnd(m_handle);
        if (result != GFX_RESULT_SUCCESS) {
            // Can't throw from destructor, just log or ignore
        }
    }
}

void ComputePassEncoderImpl::setPipeline(std::shared_ptr<ComputePipeline> pipeline)
{
    auto impl = std::dynamic_pointer_cast<ComputePipelineImpl>(pipeline);
    if (impl) {
        GfxResult result = gfxComputePassEncoderSetPipeline(m_handle, impl->getHandle());
        if (result != GFX_RESULT_SUCCESS) {
            throw std::runtime_error("Failed to set compute pipeline");
        }
    }
}

void ComputePassEncoderImpl::setBindGroup(uint32_t index, std::shared_ptr<BindGroup> bindGroup, const uint32_t* dynamicOffsets, uint32_t dynamicOffsetCount)
{
    auto impl = std::dynamic_pointer_cast<BindGroupImpl>(bindGroup);
    if (impl) {
        GfxResult result = gfxComputePassEncoderSetBindGroup(m_handle, index, impl->getHandle(), dynamicOffsets, dynamicOffsetCount);
        if (result != GFX_RESULT_SUCCESS) {
            throw std::runtime_error("Failed to set compute bind group");
        }
    }
}

void ComputePassEncoderImpl::dispatch(uint32_t workgroupCountX, uint32_t workgroupCountY, uint32_t workgroupCountZ)
{
    GfxResult result = gfxComputePassEncoderDispatch(m_handle, workgroupCountX, workgroupCountY, workgroupCountZ);
    if (result != GFX_RESULT_SUCCESS) {
        throw std::runtime_error("Failed to dispatch workgroups");
    }
}

void ComputePassEncoderImpl::dispatchIndirect(std::shared_ptr<Buffer> indirectBuffer, uint64_t indirectOffset)
{
    if (!indirectBuffer) {
        throw std::invalid_argument("Indirect buffer cannot be null");
    }
    auto* bufferImpl = dynamic_cast<BufferImpl*>(indirectBuffer.get());
    if (!bufferImpl) {
        throw std::runtime_error("Invalid buffer implementation");
    }
    GfxResult result = gfxComputePassEncoderDispatchIndirect(m_handle, bufferImpl->getHandle(), indirectOffset);
    if (result != GFX_RESULT_SUCCESS) {
        throw std::runtime_error("Failed to dispatch indirect");
    }
}

} // namespace gfx
