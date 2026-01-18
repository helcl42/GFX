#include "RenderPipeline.h"

#include "../system/Device.h"

#include <stdexcept>

namespace gfx::backend::webgpu::core {

RenderPipeline::RenderPipeline(Device* device, const RenderPipelineCreateInfo& createInfo)
{
    WGPURenderPipelineDescriptor desc = WGPU_RENDER_PIPELINE_DESCRIPTOR_INIT;

    // Create pipeline layout if bind group layouts are provided
    WGPUPipelineLayout pipelineLayout = nullptr;
    if (!createInfo.bindGroupLayouts.empty()) {
        WGPUPipelineLayoutDescriptor layoutDesc = WGPU_PIPELINE_LAYOUT_DESCRIPTOR_INIT;
        layoutDesc.bindGroupLayouts = createInfo.bindGroupLayouts.data();
        layoutDesc.bindGroupLayoutCount = static_cast<uint32_t>(createInfo.bindGroupLayouts.size());
        pipelineLayout = wgpuDeviceCreatePipelineLayout(device->handle(), &layoutDesc);
        desc.layout = pipelineLayout;
    }

    // Vertex state
    WGPUVertexState vertexState = WGPU_VERTEX_STATE_INIT;
    vertexState.module = createInfo.vertex.module;
    vertexState.entryPoint = { createInfo.vertex.entryPoint, WGPU_STRLEN };

    // Convert vertex buffers
    std::vector<WGPUVertexBufferLayout> vertexBuffers;
    std::vector<std::vector<WGPUVertexAttribute>> allAttributes;

    if (!createInfo.vertex.buffers.empty()) {
        vertexBuffers.reserve(createInfo.vertex.buffers.size());
        allAttributes.reserve(createInfo.vertex.buffers.size());

        for (const auto& buffer : createInfo.vertex.buffers) {
            std::vector<WGPUVertexAttribute> attributes;
            attributes.reserve(buffer.attributes.size());

            for (const auto& attr : buffer.attributes) {
                WGPUVertexAttribute wgpuAttr = WGPU_VERTEX_ATTRIBUTE_INIT;
                wgpuAttr.format = attr.format;
                wgpuAttr.offset = attr.offset;
                wgpuAttr.shaderLocation = attr.shaderLocation;
                attributes.push_back(wgpuAttr);
            }

            allAttributes.push_back(std::move(attributes));

            WGPUVertexBufferLayout wgpuBuffer = WGPU_VERTEX_BUFFER_LAYOUT_INIT;
            wgpuBuffer.arrayStride = buffer.arrayStride;
            wgpuBuffer.stepMode = buffer.stepMode;
            wgpuBuffer.attributes = allAttributes.back().data();
            wgpuBuffer.attributeCount = static_cast<uint32_t>(allAttributes.back().size());
            vertexBuffers.push_back(wgpuBuffer);
        }

        vertexState.buffers = vertexBuffers.data();
        vertexState.bufferCount = static_cast<uint32_t>(vertexBuffers.size());
    }

    desc.vertex = vertexState;

    // Fragment state (optional)
    WGPUFragmentState fragmentState = WGPU_FRAGMENT_STATE_INIT;
    std::vector<WGPUColorTargetState> colorTargets;
    std::vector<WGPUBlendState> blendStates;

    if (createInfo.fragment.has_value()) {
        fragmentState.module = createInfo.fragment->module;
        fragmentState.entryPoint = { createInfo.fragment->entryPoint, WGPU_STRLEN };

        if (!createInfo.fragment->targets.empty()) {
            colorTargets.reserve(createInfo.fragment->targets.size());

            for (const auto& target : createInfo.fragment->targets) {
                WGPUColorTargetState wgpuTarget = WGPU_COLOR_TARGET_STATE_INIT;
                wgpuTarget.format = target.format;
                wgpuTarget.writeMask = target.writeMask;

                if (target.blend.has_value()) {
                    WGPUBlendState blend = WGPU_BLEND_STATE_INIT;
                    blend.color.operation = target.blend->color.operation;
                    blend.color.srcFactor = target.blend->color.srcFactor;
                    blend.color.dstFactor = target.blend->color.dstFactor;
                    blend.alpha.operation = target.blend->alpha.operation;
                    blend.alpha.srcFactor = target.blend->alpha.srcFactor;
                    blend.alpha.dstFactor = target.blend->alpha.dstFactor;
                    blendStates.push_back(blend);
                    wgpuTarget.blend = &blendStates.back();
                }

                colorTargets.push_back(wgpuTarget);
            }

            fragmentState.targets = colorTargets.data();
            fragmentState.targetCount = static_cast<uint32_t>(colorTargets.size());
        }

        desc.fragment = &fragmentState;
    }

    // Primitive state
    WGPUPrimitiveState primitiveState = WGPU_PRIMITIVE_STATE_INIT;
    primitiveState.topology = createInfo.primitive.topology;
    primitiveState.frontFace = createInfo.primitive.frontFace;
    primitiveState.cullMode = createInfo.primitive.cullMode;
    primitiveState.stripIndexFormat = createInfo.primitive.stripIndexFormat;
    desc.primitive = primitiveState;

    // Depth/stencil state (optional)
    WGPUDepthStencilState depthStencilState = WGPU_DEPTH_STENCIL_STATE_INIT;
    if (createInfo.depthStencil.has_value()) {
        depthStencilState.format = createInfo.depthStencil->format;
        depthStencilState.depthWriteEnabled = createInfo.depthStencil->depthWriteEnabled ? WGPUOptionalBool_True : WGPUOptionalBool_False;
        depthStencilState.depthCompare = createInfo.depthStencil->depthCompare;

        depthStencilState.stencilFront.compare = createInfo.depthStencil->stencilFront.compare;
        depthStencilState.stencilFront.failOp = createInfo.depthStencil->stencilFront.failOp;
        depthStencilState.stencilFront.depthFailOp = createInfo.depthStencil->stencilFront.depthFailOp;
        depthStencilState.stencilFront.passOp = createInfo.depthStencil->stencilFront.passOp;

        depthStencilState.stencilBack.compare = createInfo.depthStencil->stencilBack.compare;
        depthStencilState.stencilBack.failOp = createInfo.depthStencil->stencilBack.failOp;
        depthStencilState.stencilBack.depthFailOp = createInfo.depthStencil->stencilBack.depthFailOp;
        depthStencilState.stencilBack.passOp = createInfo.depthStencil->stencilBack.passOp;

        depthStencilState.stencilReadMask = createInfo.depthStencil->stencilReadMask;
        depthStencilState.stencilWriteMask = createInfo.depthStencil->stencilWriteMask;
        depthStencilState.depthBias = createInfo.depthStencil->depthBias;
        depthStencilState.depthBiasSlopeScale = createInfo.depthStencil->depthBiasSlopeScale;
        depthStencilState.depthBiasClamp = createInfo.depthStencil->depthBiasClamp;

        desc.depthStencil = &depthStencilState;
    }

    // Multisample state
    WGPUMultisampleState multisampleState = WGPU_MULTISAMPLE_STATE_INIT;
    // WebGPU requires sampleCount >= 1, clamp to valid range
    multisampleState.count = std::max<uint32_t>(1, createInfo.sampleCount);
    desc.multisample = multisampleState;

    m_pipeline = wgpuDeviceCreateRenderPipeline(device->handle(), &desc);

    // Release the pipeline layout if we created one (pipeline holds its own reference)
    if (pipelineLayout) {
        wgpuPipelineLayoutRelease(pipelineLayout);
    }

    if (!m_pipeline) {
        throw std::runtime_error("Failed to create WebGPU RenderPipeline");
    }
}

RenderPipeline::~RenderPipeline()
{
    if (m_pipeline) {
        wgpuRenderPipelineRelease(m_pipeline);
    }
}

WGPURenderPipeline RenderPipeline::handle() const
{
    return m_pipeline;
}

} // namespace gfx::backend::webgpu::core