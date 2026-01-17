#ifndef GFX_VULKAN_RENDERPIPELINE_H
#define GFX_VULKAN_RENDERPIPELINE_H

#include "../CoreTypes.h"

namespace gfx::backend::vulkan::core {

class Device;

class RenderPipeline {
public:
    RenderPipeline(const RenderPipeline&) = delete;
    RenderPipeline& operator=(const RenderPipeline&) = delete;

    RenderPipeline(Device* device, const RenderPipelineCreateInfo& createInfo);
    ~RenderPipeline();

    VkPipeline handle() const;
    VkPipelineLayout layout() const;

private:
    VkPipeline m_pipeline = VK_NULL_HANDLE;
    VkPipelineLayout m_pipelineLayout = VK_NULL_HANDLE;
    Device* m_device = nullptr;
};

} // namespace gfx::backend::vulkan::core

#endif // GFX_VULKAN_RENDERPIPELINE_H