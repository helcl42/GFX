#include "ComputePassEncoder.h"

#include "../render/BindGroup.h"
#include "CommandEncoder.h"
#include "../compute/ComputePipeline.h"
#include "../system/Device.h"

namespace gfx::backend::vulkan::core {

ComputePassEncoder::ComputePassEncoder(CommandEncoder* commandEncoder, const ComputePassEncoderCreateInfo& createInfo)
    : m_commandBuffer(commandEncoder->handle())
    , m_device(commandEncoder->getDevice())
    , m_commandEncoder(commandEncoder)
{
    (void)createInfo; // Label unused for now
}

VkCommandBuffer ComputePassEncoder::handle() const
{
    return m_commandBuffer;
}

Device* ComputePassEncoder::device() const
{
    return m_device;
}

CommandEncoder* ComputePassEncoder::commandEncoder() const
{
    return m_commandEncoder;
}

void ComputePassEncoder::setPipeline(ComputePipeline* pipeline)
{
    vkCmdBindPipeline(m_commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline->handle());
    m_commandEncoder->setCurrentPipelineLayout(pipeline->layout());
}

void ComputePassEncoder::setBindGroup(uint32_t index, BindGroup* bindGroup, const uint32_t* dynamicOffsets, uint32_t dynamicOffsetCount)
{
    VkPipelineLayout layout = m_commandEncoder->currentPipelineLayout();
    if (layout != VK_NULL_HANDLE) {
        VkDescriptorSet set = bindGroup->handle();
        vkCmdBindDescriptorSets(m_commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, layout, index, 1, &set, dynamicOffsetCount, dynamicOffsets);
    }
}

void ComputePassEncoder::dispatchWorkgroups(uint32_t workgroupCountX, uint32_t workgroupCountY, uint32_t workgroupCountZ)
{
    vkCmdDispatch(m_commandBuffer, workgroupCountX, workgroupCountY, workgroupCountZ);
}

} // namespace gfx::backend::vulkan::core