#pragma once

#include <gfx_cpp/gfx.hpp>

#include <gfx/gfx.h>

#include <memory>

namespace gfx {

class CComputePassEncoderImpl : public ComputePassEncoder {
public:
    explicit CComputePassEncoderImpl(GfxComputePassEncoder h);
    ~CComputePassEncoderImpl() override;

    void setPipeline(std::shared_ptr<ComputePipeline> pipeline) override;
    void setBindGroup(uint32_t index, std::shared_ptr<BindGroup> bindGroup, const uint32_t* dynamicOffsets = nullptr, uint32_t dynamicOffsetCount = 0) override;

    void dispatch(uint32_t workgroupCountX, uint32_t workgroupCountY = 1, uint32_t workgroupCountZ = 1) override;
    void dispatchIndirect(std::shared_ptr<Buffer> indirectBuffer, uint64_t indirectOffset) override;

private:
    GfxComputePassEncoder m_handle;
};

} // namespace gfx
