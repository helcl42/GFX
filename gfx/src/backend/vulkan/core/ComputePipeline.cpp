#include "ComputePipeline.h"

#include "Device.h"

#include <stdexcept>

namespace gfx::backend::vulkan::core {

ComputePipeline::ComputePipeline(Device* device, const ComputePipelineCreateInfo& createInfo)
    : m_device(device)
{
    // Create pipeline layout
    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(createInfo.bindGroupLayouts.size());
    pipelineLayoutInfo.pSetLayouts = createInfo.bindGroupLayouts.data();

    VkResult result = vkCreatePipelineLayout(m_device->handle(), &pipelineLayoutInfo, nullptr, &m_pipelineLayout);
    if (result != VK_SUCCESS) {
        throw std::runtime_error("Failed to create compute pipeline layout");
    }

    // Shader stage
    VkPipelineShaderStageCreateInfo computeShaderStageInfo{};
    computeShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    computeShaderStageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    computeShaderStageInfo.module = createInfo.module;
    computeShaderStageInfo.pName = createInfo.entryPoint;

    // Create compute pipeline
    VkComputePipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    pipelineInfo.stage = computeShaderStageInfo;
    pipelineInfo.layout = m_pipelineLayout;

    result = vkCreateComputePipelines(m_device->handle(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_pipeline);
    if (result != VK_SUCCESS) {
        vkDestroyPipelineLayout(m_device->handle(), m_pipelineLayout, nullptr);
        throw std::runtime_error("Failed to create compute pipeline");
    }
}

ComputePipeline::~ComputePipeline()
{
    if (m_pipeline != VK_NULL_HANDLE) {
        vkDestroyPipeline(m_device->handle(), m_pipeline, nullptr);
    }
    if (m_pipelineLayout != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(m_device->handle(), m_pipelineLayout, nullptr);
    }
}

VkPipeline ComputePipeline::handle() const
{
    return m_pipeline;
}

VkPipelineLayout ComputePipeline::layout() const
{
    return m_pipelineLayout;
}

} // namespace gfx::backend::vulkan::core