#include "Blit.h"

#include "Utils.h"
#include "common/Logger.h"

namespace gfx::backend::webgpu::core {

Blit::Blit(WGPUDevice device)
    : m_device(device)
{
    // Create shader module for 2D textures with source region support
    const char* shader2DCode = R"(
            struct SourceRegion {
                uvMin: vec2f,
                uvMax: vec2f,
            }
            
            struct VertexOutput {
                @builtin(position) position: vec4f,
                @location(0) texCoord: vec2f,
            }
            
            @group(0) @binding(0) var srcTexture: texture_2d<f32>;
            @group(0) @binding(1) var srcSampler: sampler;
            @group(0) @binding(2) var<uniform> sourceRegion: SourceRegion;
            
            @vertex
            fn vs_main(@builtin(vertex_index) vertexIndex: u32) -> VertexOutput {
                var output: VertexOutput;
                let x = f32((vertexIndex & 1u) << 1u) - 1.0;
                let y = 1.0 - f32((vertexIndex & 2u));
                output.position = vec4f(x, y, 0.0, 1.0);
                // Map vertex coordinates [0,1] to source region
                let uv = vec2f((x + 1.0) * 0.5, (1.0 - y) * 0.5);
                output.texCoord = mix(sourceRegion.uvMin, sourceRegion.uvMax, uv);
                return output;
            }
            
            @fragment
            fn fs_main(input: VertexOutput) -> @location(0) vec4f {
                return textureSample(srcTexture, srcSampler, input.texCoord);
            }
        )";

    WGPUShaderSourceWGSL wgslSource = WGPU_SHADER_SOURCE_WGSL_INIT;
    wgslSource.chain.sType = WGPUSType_ShaderSourceWGSL;
    wgslSource.code = toStringView(shader2DCode);

    WGPUShaderModuleDescriptor shaderDesc = WGPU_SHADER_MODULE_DESCRIPTOR_INIT;
    shaderDesc.nextInChain = &wgslSource.chain;
    m_shaderModule = wgpuDeviceCreateShaderModule(m_device, &shaderDesc);

    // Create bind group layout with uniform buffer for source region
    WGPUBindGroupLayoutEntry bgLayoutEntries[3] = { WGPU_BIND_GROUP_LAYOUT_ENTRY_INIT, WGPU_BIND_GROUP_LAYOUT_ENTRY_INIT, WGPU_BIND_GROUP_LAYOUT_ENTRY_INIT };
    bgLayoutEntries[0].binding = 0;
    bgLayoutEntries[0].visibility = WGPUShaderStage_Fragment;
    bgLayoutEntries[0].texture.sampleType = WGPUTextureSampleType_Float;
    bgLayoutEntries[0].texture.viewDimension = WGPUTextureViewDimension_2D;

    bgLayoutEntries[1].binding = 1;
    bgLayoutEntries[1].visibility = WGPUShaderStage_Fragment;
    bgLayoutEntries[1].sampler.type = WGPUSamplerBindingType_Filtering;

    bgLayoutEntries[2].binding = 2;
    bgLayoutEntries[2].visibility = WGPUShaderStage_Vertex;
    bgLayoutEntries[2].buffer.type = WGPUBufferBindingType_Uniform;
    bgLayoutEntries[2].buffer.minBindingSize = 16; // vec2f + vec2f = 16 bytes

    WGPUBindGroupLayoutDescriptor bgLayoutDesc = WGPU_BIND_GROUP_LAYOUT_DESCRIPTOR_INIT;
    bgLayoutDesc.entryCount = 3;
    bgLayoutDesc.entries = bgLayoutEntries;
    m_bindGroupLayout = wgpuDeviceCreateBindGroupLayout(m_device, &bgLayoutDesc);

    // Create pipeline layout
    WGPUPipelineLayoutDescriptor pipelineLayoutDesc = WGPU_PIPELINE_LAYOUT_DESCRIPTOR_INIT;
    pipelineLayoutDesc.bindGroupLayoutCount = 1;
    pipelineLayoutDesc.bindGroupLayouts = &m_bindGroupLayout;
    m_pipelineLayout = wgpuDeviceCreatePipelineLayout(m_device, &pipelineLayoutDesc);
}

Blit::~Blit()
{
    // Release cached pipelines
    for (auto& pair : m_pipelines) {
        wgpuRenderPipelineRelease(pair.second);
    }
    // Release cached samplers
    for (auto& pair : m_samplers) {
        wgpuSamplerRelease(pair.second);
    }
    if (m_pipelineLayout) {
        wgpuPipelineLayoutRelease(m_pipelineLayout);
    }
    if (m_bindGroupLayout) {
        wgpuBindGroupLayoutRelease(m_bindGroupLayout);
    }
    if (m_shaderModule) {
        wgpuShaderModuleRelease(m_shaderModule);
    }
}

void Blit::execute(WGPUCommandEncoder commandEncoder, WGPUTexture srcTexture, const WGPUOrigin3D& srcOrigin, const WGPUExtent3D& srcExtent, uint32_t srcMipLevel, WGPUTexture dstTexture, const WGPUOrigin3D& dstOrigin, const WGPUExtent3D& dstExtent, uint32_t dstMipLevel, WGPUFilterMode filterMode)
{
    // Get or create sampler with requested filter mode
    WGPUSampler sampler = getOrCreateSampler(filterMode);

    // Query texture dimension - use 2D for now, can be extended for arrays/3D later
    WGPUTextureDimension srcDimension = wgpuTextureGetDimension(srcTexture);
    WGPUTextureViewDimension viewDimension = WGPUTextureViewDimension_2D;

    // Map texture dimension to view dimension
    // For now we only support 2D, but this can be extended
    if (srcDimension == WGPUTextureDimension_2D) {
        viewDimension = WGPUTextureViewDimension_2D;
    } else {
        // 1D and 3D textures not yet supported by the shader
        gfx::common::Logger::instance().logWarning("[WebGPU Blit] Only 2D textures are currently supported");
    }

    // Create texture view for source
    WGPUTextureViewDescriptor srcViewDesc = WGPU_TEXTURE_VIEW_DESCRIPTOR_INIT;
    srcViewDesc.format = wgpuTextureGetFormat(srcTexture);
    srcViewDesc.dimension = viewDimension;
    srcViewDesc.baseMipLevel = srcMipLevel;
    srcViewDesc.mipLevelCount = 1;
    srcViewDesc.baseArrayLayer = 0;
    srcViewDesc.arrayLayerCount = 1;
    WGPUTextureView srcView = wgpuTextureCreateView(srcTexture, &srcViewDesc);

    // Calculate source texture size at mip level
    uint32_t srcTexWidth = std::max(1u, wgpuTextureGetWidth(srcTexture) >> srcMipLevel);
    uint32_t srcTexHeight = std::max(1u, wgpuTextureGetHeight(srcTexture) >> srcMipLevel);

    // Calculate UV coordinates for source region
    struct SourceRegionData {
        float uvMinX, uvMinY;
        float uvMaxX, uvMaxY;
    } regionData;
    regionData.uvMinX = static_cast<float>(srcOrigin.x) / static_cast<float>(srcTexWidth);
    regionData.uvMinY = static_cast<float>(srcOrigin.y) / static_cast<float>(srcTexHeight);
    regionData.uvMaxX = static_cast<float>(srcOrigin.x + srcExtent.width) / static_cast<float>(srcTexWidth);
    regionData.uvMaxY = static_cast<float>(srcOrigin.y + srcExtent.height) / static_cast<float>(srcTexHeight);

    // Create uniform buffer for source region
    WGPUBufferDescriptor uniformBufferDesc = WGPU_BUFFER_DESCRIPTOR_INIT;
    uniformBufferDesc.size = sizeof(SourceRegionData);
    uniformBufferDesc.usage = WGPUBufferUsage_Uniform | WGPUBufferUsage_CopyDst;
    WGPUBuffer uniformBuffer = wgpuDeviceCreateBuffer(m_device, &uniformBufferDesc);
    wgpuQueueWriteBuffer(wgpuDeviceGetQueue(m_device), uniformBuffer, 0, &regionData, sizeof(SourceRegionData));

    // Create bind group
    WGPUBindGroupEntry bgEntries[3] = { WGPU_BIND_GROUP_ENTRY_INIT, WGPU_BIND_GROUP_ENTRY_INIT, WGPU_BIND_GROUP_ENTRY_INIT };
    bgEntries[0].binding = 0;
    bgEntries[0].textureView = srcView;

    bgEntries[1].binding = 1;
    bgEntries[1].sampler = sampler;

    bgEntries[2].binding = 2;
    bgEntries[2].buffer = uniformBuffer;
    bgEntries[2].size = sizeof(SourceRegionData);

    WGPUBindGroupDescriptor bgDesc = WGPU_BIND_GROUP_DESCRIPTOR_INIT;
    bgDesc.layout = m_bindGroupLayout;
    bgDesc.entryCount = 3;
    bgDesc.entries = bgEntries;
    WGPUBindGroup bindGroup = wgpuDeviceCreateBindGroup(m_device, &bgDesc);

    // Get or create render pipeline for this format
    WGPUTextureFormat dstFormat = wgpuTextureGetFormat(dstTexture);
    WGPURenderPipeline pipeline = getOrCreatePipeline(dstFormat);

    // Create texture view for destination
    WGPUTextureViewDescriptor dstViewDesc = WGPU_TEXTURE_VIEW_DESCRIPTOR_INIT;
    dstViewDesc.format = dstFormat;
    dstViewDesc.dimension = WGPUTextureViewDimension_2D;
    dstViewDesc.baseMipLevel = dstMipLevel;
    dstViewDesc.mipLevelCount = 1;
    dstViewDesc.baseArrayLayer = 0;
    dstViewDesc.arrayLayerCount = 1;
    WGPUTextureView dstView = wgpuTextureCreateView(dstTexture, &dstViewDesc);

    // Create render pass
    WGPURenderPassColorAttachment colorAttachment = WGPU_RENDER_PASS_COLOR_ATTACHMENT_INIT;
    colorAttachment.view = dstView;
    colorAttachment.loadOp = WGPULoadOp_Load;
    colorAttachment.storeOp = WGPUStoreOp_Store;

    WGPURenderPassDescriptor renderPassDesc = {};
    renderPassDesc.colorAttachmentCount = 1;
    renderPassDesc.colorAttachments = &colorAttachment;

    WGPURenderPassEncoder renderPass = wgpuCommandEncoderBeginRenderPass(commandEncoder, &renderPassDesc);
    wgpuRenderPassEncoderSetPipeline(renderPass, pipeline);
    wgpuRenderPassEncoderSetBindGroup(renderPass, 0, bindGroup, 0, nullptr);

    // Set viewport to destination region for proper scaling
    wgpuRenderPassEncoderSetViewport(renderPass,
        static_cast<float>(dstOrigin.x), static_cast<float>(dstOrigin.y),
        static_cast<float>(dstExtent.width), static_cast<float>(dstExtent.height),
        0.0f, 1.0f);

    // Set scissor to destination region
    wgpuRenderPassEncoderSetScissorRect(renderPass,
        dstOrigin.x, dstOrigin.y,
        dstExtent.width, dstExtent.height);

    wgpuRenderPassEncoderDraw(renderPass, 4, 1, 0, 0);
    wgpuRenderPassEncoderEnd(renderPass);

    // Cleanup temporary resources
    wgpuRenderPassEncoderRelease(renderPass);
    wgpuTextureViewRelease(dstView);
    wgpuBindGroupRelease(bindGroup);
    wgpuBufferRelease(uniformBuffer);
    wgpuTextureViewRelease(srcView);
    // Note: sampler is cached, don't release it here
}

WGPURenderPipeline Blit::getOrCreatePipeline(WGPUTextureFormat format)
{
    // Check cache
    auto it = m_pipelines.find(format);
    if (it != m_pipelines.end()) {
        return it->second;
    }

    // Create new pipeline for this format
    WGPUColorTargetState colorTarget = WGPU_COLOR_TARGET_STATE_INIT;
    colorTarget.format = format;
    colorTarget.writeMask = WGPUColorWriteMask_All;

    WGPUFragmentState fragmentState = WGPU_FRAGMENT_STATE_INIT;
    fragmentState.module = m_shaderModule;
    fragmentState.entryPoint = toStringView("fs_main");
    fragmentState.targetCount = 1;
    fragmentState.targets = &colorTarget;

    WGPURenderPipelineDescriptor pipelineDesc = WGPU_RENDER_PIPELINE_DESCRIPTOR_INIT;
    pipelineDesc.layout = m_pipelineLayout;
    pipelineDesc.vertex.module = m_shaderModule;
    pipelineDesc.vertex.entryPoint = toStringView("vs_main");
    pipelineDesc.primitive.topology = WGPUPrimitiveTopology_TriangleStrip;
    pipelineDesc.fragment = &fragmentState;
    pipelineDesc.multisample.count = 1;

    WGPURenderPipeline pipeline = wgpuDeviceCreateRenderPipeline(m_device, &pipelineDesc);
    m_pipelines[format] = pipeline;
    return pipeline;
}

WGPUSampler Blit::getOrCreateSampler(WGPUFilterMode filterMode)
{
    // Check cache
    auto it = m_samplers.find(filterMode);
    if (it != m_samplers.end()) {
        return it->second;
    }

    // Create new sampler for this filter mode
    WGPUSamplerDescriptor samplerDesc = WGPU_SAMPLER_DESCRIPTOR_INIT;
    samplerDesc.addressModeU = WGPUAddressMode_ClampToEdge;
    samplerDesc.addressModeV = WGPUAddressMode_ClampToEdge;
    samplerDesc.addressModeW = WGPUAddressMode_ClampToEdge;
    samplerDesc.magFilter = filterMode;
    samplerDesc.minFilter = filterMode;
    samplerDesc.mipmapFilter = WGPUMipmapFilterMode_Nearest;
    samplerDesc.maxAnisotropy = 1;

    WGPUSampler sampler = wgpuDeviceCreateSampler(m_device, &samplerDesc);
    m_samplers[filterMode] = sampler;
    return sampler;
}

} // namespace gfx::backend::webgpu::core