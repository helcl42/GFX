#ifndef GFX_CPP_COMPUTE_PIPELINE_H
#define GFX_CPP_COMPUTE_PIPELINE_H

#include <gfx_cpp/gfx.hpp>

#include <gfx/gfx.h>

namespace gfx {

class ComputePipelineImpl : public ComputePipeline {
public:
    explicit ComputePipelineImpl(GfxComputePipeline h);
    ~ComputePipelineImpl() override;

    GfxComputePipeline getHandle() const;

private:
    GfxComputePipeline m_handle;
};

} // namespace gfx

#endif // GFX_CPP_COMPUTE_PIPELINE_H
