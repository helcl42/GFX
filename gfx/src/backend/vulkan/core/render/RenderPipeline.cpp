#include "RenderPipeline.h"

#include "../system/Device.h"

#include <stdexcept>

namespace gfx::backend::vulkan::core {

RenderPipeline::RenderPipeline(Device* device, const RenderPipelineCreateInfo& createInfo)
    : m_device(device)
{
    // Create pipeline layout
    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(createInfo.bindGroupLayouts.size());
    pipelineLayoutInfo.pSetLayouts = createInfo.bindGroupLayouts.data();

    VkResult result = vkCreatePipelineLayout(m_device->handle(), &pipelineLayoutInfo, nullptr, &m_pipelineLayout);
    if (result != VK_SUCCESS) {
        throw std::runtime_error("Failed to create pipeline layout");
    }

    // Shader stages
    VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
    vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module = createInfo.vertex.module;
    vertShaderStageInfo.pName = createInfo.vertex.entryPoint;

    VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
    uint32_t stageCount = 1;
    if (createInfo.fragment.module != VK_NULL_HANDLE) {
        fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        fragShaderStageInfo.module = createInfo.fragment.module;
        fragShaderStageInfo.pName = createInfo.fragment.entryPoint;
        stageCount = 2;
    }

    VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

    // Process vertex input
    std::vector<VkVertexInputBindingDescription> bindings;
    std::vector<VkVertexInputAttributeDescription> attributes;

    for (size_t i = 0; i < createInfo.vertex.buffers.size(); ++i) {
        const auto& bufferLayout = createInfo.vertex.buffers[i];

        VkVertexInputBindingDescription binding{};
        binding.binding = static_cast<uint32_t>(i);
        binding.stride = static_cast<uint32_t>(bufferLayout.arrayStride);
        binding.inputRate = bufferLayout.inputRate;
        bindings.push_back(binding);

        attributes.insert(attributes.end(), bufferLayout.attributes.begin(), bufferLayout.attributes.end());
    }

    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = static_cast<uint32_t>(bindings.size());
    vertexInputInfo.pVertexBindingDescriptions = bindings.data();
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributes.size());
    vertexInputInfo.pVertexAttributeDescriptions = attributes.data();

    // Input assembly
    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = createInfo.primitive.topology;

    // Viewport
    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = 800.0f; // Placeholder, dynamic state will be used
    viewport.height = 600.0f; // Placeholder, dynamic state will be used
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissorRect{};
    scissorRect.offset = { 0, 0 };
    scissorRect.extent = { 800, 600 }; // Placeholder, dynamic state will be used

    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.pViewports = &viewport;
    viewportState.pScissors = &scissorRect;
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;

    // Rasterizer
    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.polygonMode = createInfo.primitive.polygonMode;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = createInfo.primitive.cullMode;
    rasterizer.frontFace = createInfo.primitive.frontFace;

    // Multisampling
    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.rasterizationSamples = createInfo.sampleCount;

    // Color blending
    std::vector<VkPipelineColorBlendAttachmentState> colorBlendAttachments;
    if (!createInfo.fragment.targets.empty()) {
        for (const auto& target : createInfo.fragment.targets) {
            colorBlendAttachments.push_back(target.blendState);
        }
    } else {
        VkPipelineColorBlendAttachmentState blendAttachment{};
        blendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        blendAttachment.blendEnable = VK_FALSE;
        colorBlendAttachments.push_back(blendAttachment);
    }

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.attachmentCount = static_cast<uint32_t>(colorBlendAttachments.size());
    colorBlending.pAttachments = colorBlendAttachments.data();

    // Dynamic state
    VkDynamicState dynamicStates[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = 2;
    dynamicState.pDynamicStates = dynamicStates;

    // Create depth stencil state if provided
    VkPipelineDepthStencilStateCreateInfo depthStencil{};
    if (createInfo.depthStencil.has_value()) {
        depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        depthStencil.depthTestEnable = VK_TRUE;
        depthStencil.depthWriteEnable = createInfo.depthStencil->depthWriteEnabled ? VK_TRUE : VK_FALSE;
        depthStencil.depthCompareOp = createInfo.depthStencil->depthCompareOp;
        depthStencil.depthBoundsTestEnable = VK_FALSE;
        depthStencil.stencilTestEnable = VK_FALSE;
    }

    // Create graphics pipeline
    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = stageCount;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = &dynamicState;
    if (createInfo.depthStencil.has_value()) {
        pipelineInfo.pDepthStencilState = &depthStencil;
    }
    pipelineInfo.layout = m_pipelineLayout;
    pipelineInfo.renderPass = createInfo.renderPass;
    pipelineInfo.subpass = 0;

    result = vkCreateGraphicsPipelines(m_device->handle(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_pipeline);

    if (result != VK_SUCCESS) {
        vkDestroyPipelineLayout(m_device->handle(), m_pipelineLayout, nullptr);
        throw std::runtime_error("Failed to create graphics pipeline");
    }
}

RenderPipeline::~RenderPipeline()
{
    if (m_pipeline != VK_NULL_HANDLE) {
        vkDestroyPipeline(m_device->handle(), m_pipeline, nullptr);
    }
    if (m_pipelineLayout != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(m_device->handle(), m_pipelineLayout, nullptr);
    }
}

VkPipeline RenderPipeline::handle() const
{
    return m_pipeline;
}

VkPipelineLayout RenderPipeline::layout() const
{
    return m_pipelineLayout;
}

} // namespace gfx::backend::vulkan::core