#ifndef GFX_WEBGPU_COMPUTE_PIPELINE_H
#define GFX_WEBGPU_COMPUTE_PIPELINE_H

#include "../CoreTypes.h"

namespace gfx::backend::webgpu::core {

class Device;

class ComputePipeline {
public:
    // Prevent copying
    ComputePipeline(const ComputePipeline&) = delete;
    ComputePipeline& operator=(const ComputePipeline&) = delete;

    ComputePipeline(Device* device, const ComputePipelineCreateInfo& createInfo);
    ~ComputePipeline();

    WGPUComputePipeline handle() const;

private:
    WGPUComputePipeline m_pipeline = nullptr;
};

} // namespace gfx::backend::webgpu::core

#endif // GFX_WEBGPU_COMPUTE_PIPELINE_H