#ifndef GFX_WEBGPU_BLIT_H
#define GFX_WEBGPU_BLIT_H

#include "../../common/Common.h"

#include <unordered_map>

namespace gfx::backend::webgpu::core {

class Blit {
public:
    // Prevent copying
    Blit(const Blit&) = delete;
    Blit& operator=(const Blit&) = delete;

    Blit(WGPUDevice device);
    ~Blit();

    void execute(WGPUCommandEncoder commandEncoder, WGPUTexture srcTexture, const WGPUOrigin3D& srcOrigin, const WGPUExtent3D& srcExtent, uint32_t srcMipLevel, WGPUTexture dstTexture, const WGPUOrigin3D& dstOrigin, const WGPUExtent3D& dstExtent, uint32_t dstMipLevel, WGPUFilterMode filterMode);

private:
    WGPURenderPipeline getOrCreatePipeline(WGPUTextureFormat format);
    WGPUSampler getOrCreateSampler(WGPUFilterMode filterMode);

    WGPUDevice m_device = nullptr;
    WGPUShaderModule m_shaderModule = nullptr;
    WGPUBindGroupLayout m_bindGroupLayout = nullptr;
    WGPUPipelineLayout m_pipelineLayout = nullptr;
    std::unordered_map<WGPUTextureFormat, WGPURenderPipeline> m_pipelines;
    std::unordered_map<WGPUFilterMode, WGPUSampler> m_samplers;
};

} // namespace gfx::backend::webgpu::core

#endif // GFX_WEBGPU_BLIT_H
