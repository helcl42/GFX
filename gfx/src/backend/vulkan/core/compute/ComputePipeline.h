#ifndef GFX_VULKAN_COMPUTEPIPELINE_H
#define GFX_VULKAN_COMPUTEPIPELINE_H

#include "../CoreTypes.h"

namespace gfx::backend::vulkan::core {

class Device;

class ComputePipeline {
public:
    ComputePipeline(const ComputePipeline&) = delete;
    ComputePipeline& operator=(const ComputePipeline&) = delete;

    ComputePipeline(Device* device, const ComputePipelineCreateInfo& createInfo);
    ~ComputePipeline();

    VkPipeline handle() const;
    VkPipelineLayout layout() const;

private:
    VkPipeline m_pipeline = VK_NULL_HANDLE;
    VkPipelineLayout m_pipelineLayout = VK_NULL_HANDLE;
    Device* m_device = nullptr;
};

} // namespace gfx::backend::vulkan::core

#endif // GFX_VULKAN_COMPUTEPIPELINE_Hs