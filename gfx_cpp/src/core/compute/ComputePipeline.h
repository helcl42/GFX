#pragma once

#include <gfx_cpp/gfx.hpp>

#include <gfx/gfx.h>

namespace gfx {

class CComputePipelineImpl : public ComputePipeline {
public:
    explicit CComputePipelineImpl(GfxComputePipeline h);
    ~CComputePipelineImpl() override;

    GfxComputePipeline getHandle() const;

private:
    GfxComputePipeline m_handle;
};

} // namespace gfx
