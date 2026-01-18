#ifndef GFX_WEBGPU_RENDER_PIPELINE_H
#define GFX_WEBGPU_RENDER_PIPELINE_H

#include "../CoreTypes.h"

namespace gfx::backend::webgpu::core {

class Device;

class RenderPipeline {
public:
    // Prevent copying
    RenderPipeline(const RenderPipeline&) = delete;
    RenderPipeline& operator=(const RenderPipeline&) = delete;

    RenderPipeline(Device* device, const RenderPipelineCreateInfo& createInfo);
    ~RenderPipeline();

    WGPURenderPipeline handle() const;

private:
    WGPURenderPipeline m_pipeline = nullptr;
};

} // namespace gfx::backend::webgpu::core

#endif // GFX_WEBGPU_RENDER_PIPELINE_H