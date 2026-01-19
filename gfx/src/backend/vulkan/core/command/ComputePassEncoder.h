#ifndef GFX_VULKAN_COMPUTEPASSSENCODER_H
#define GFX_VULKAN_COMPUTEPASSSENCODER_H

#include "../CoreTypes.h"

namespace gfx::backend::vulkan::core {

class Device;
class CommandEncoder;
class ComputePipeline;
class BindGroup;

class ComputePassEncoder {
public:
    ComputePassEncoder(const ComputePassEncoder&) = delete;
    ComputePassEncoder& operator=(const ComputePassEncoder&) = delete;

    ComputePassEncoder(CommandEncoder* commandEncoder, const ComputePassEncoderCreateInfo& createInfo);
    ~ComputePassEncoder() = default;

    VkCommandBuffer handle() const;
    Device* device() const;
    CommandEncoder* commandEncoder() const;

    void setPipeline(ComputePipeline* pipeline);
    void setBindGroup(uint32_t index, BindGroup* bindGroup, const uint32_t* dynamicOffsets, uint32_t dynamicOffsetCount);

    void dispatchWorkgroups(uint32_t workgroupCountX, uint32_t workgroupCountY, uint32_t workgroupCountZ);
    void dispatchIndirect(Buffer* buffer, uint64_t offset);

private:
    VkCommandBuffer m_commandBuffer = VK_NULL_HANDLE;
    Device* m_device = nullptr;
    CommandEncoder* m_commandEncoder = nullptr;
};

} // namespace gfx::backend::vulkan::core

#endif // GFX_VULKAN_COMPUTEPASSSENCODER_H