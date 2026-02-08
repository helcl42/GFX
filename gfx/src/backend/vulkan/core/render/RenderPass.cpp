#include "RenderPass.h"

#include "../system/Device.h"

#include <stdexcept>

namespace gfx::backend::vulkan::core {

RenderPass::RenderPass(Device* device, const RenderPassCreateInfo& createInfo)
    : m_device(device)
{
    // Store metadata
    m_colorAttachmentCount = static_cast<uint32_t>(createInfo.colorAttachments.size());
    m_hasDepthStencil = createInfo.depthStencilAttachment.has_value();

    // Build attachment descriptions and references
    std::vector<VkAttachmentDescription> attachments;
    std::vector<VkAttachmentReference> colorRefs;
    std::vector<VkAttachmentReference> resolveRefs;

    uint32_t attachmentIndex = 0;

    // Process color attachments
    for (const auto& colorAttachment : createInfo.colorAttachments) {
        const RenderPassColorAttachmentTarget& target = colorAttachment.target;
        bool isMSAA = (target.sampleCount > VK_SAMPLE_COUNT_1_BIT);

        // Add color attachment
        VkAttachmentDescription colorAttachmentDesc{};
        colorAttachmentDesc.format = target.format;
        colorAttachmentDesc.samples = target.sampleCount;
        colorAttachmentDesc.loadOp = target.loadOp;
        colorAttachmentDesc.storeOp = target.storeOp;
        colorAttachmentDesc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachmentDesc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAttachmentDesc.initialLayout = (target.loadOp == VK_ATTACHMENT_LOAD_OP_LOAD)
            ? VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
            : VK_IMAGE_LAYOUT_UNDEFINED;
        colorAttachmentDesc.finalLayout = target.finalLayout;
        attachments.push_back(colorAttachmentDesc);

        VkAttachmentReference colorRef{};
        colorRef.attachment = attachmentIndex++;
        colorRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        colorRefs.push_back(colorRef);

        // Check if this attachment has a resolve target
        if (colorAttachment.resolveTarget.has_value()) {
            const RenderPassColorAttachmentTarget& resolveTarget = colorAttachment.resolveTarget.value();

            VkAttachmentDescription resolveAttachmentDesc{};
            resolveAttachmentDesc.format = resolveTarget.format;
            resolveAttachmentDesc.samples = VK_SAMPLE_COUNT_1_BIT;
            resolveAttachmentDesc.loadOp = resolveTarget.loadOp;
            resolveAttachmentDesc.storeOp = resolveTarget.storeOp;
            resolveAttachmentDesc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            resolveAttachmentDesc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            resolveAttachmentDesc.initialLayout = (resolveTarget.loadOp == VK_ATTACHMENT_LOAD_OP_LOAD)
                ? VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
                : VK_IMAGE_LAYOUT_UNDEFINED;
            resolveAttachmentDesc.finalLayout = resolveTarget.finalLayout;
            attachments.push_back(resolveAttachmentDesc);

            VkAttachmentReference resolveRef{};
            resolveRef.attachment = attachmentIndex++;
            resolveRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            resolveRefs.push_back(resolveRef);

            m_colorHasResolve.push_back(true);
        } else if (isMSAA) {
            // MSAA without resolve needs unused reference
            VkAttachmentReference unusedRef{};
            unusedRef.attachment = VK_ATTACHMENT_UNUSED;
            unusedRef.layout = VK_IMAGE_LAYOUT_UNDEFINED;
            resolveRefs.push_back(unusedRef);

            m_colorHasResolve.push_back(false);
        } else {
            m_colorHasResolve.push_back(false);
        }
    }

    // Process depth/stencil attachment
    VkAttachmentReference depthRef{};
    bool hasDepth = false;

    // TODO - handle stencil-only attachments (we would need vulkan 1.2 for that)
    if (createInfo.depthStencilAttachment.has_value()) {
        const RenderPassDepthStencilAttachmentTarget& target = createInfo.depthStencilAttachment->target;

        VkAttachmentDescription depthAttachment{};
        depthAttachment.format = target.format;
        depthAttachment.samples = target.sampleCount;
        depthAttachment.loadOp = target.depthLoadOp;
        depthAttachment.storeOp = target.depthStoreOp;
        depthAttachment.stencilLoadOp = target.stencilLoadOp;
        depthAttachment.stencilStoreOp = target.stencilStoreOp;

        bool loadDepth = (target.depthLoadOp == VK_ATTACHMENT_LOAD_OP_LOAD);
        bool loadStencil = (target.stencilLoadOp == VK_ATTACHMENT_LOAD_OP_LOAD);
        depthAttachment.initialLayout = (loadDepth || loadStencil)
            ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
            : VK_IMAGE_LAYOUT_UNDEFINED;
        depthAttachment.finalLayout = target.finalLayout;
        attachments.push_back(depthAttachment);

        depthRef.attachment = attachmentIndex++;
        depthRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        hasDepth = true;
    }

    // Create subpass
    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = static_cast<uint32_t>(colorRefs.size());
    subpass.pColorAttachments = colorRefs.empty() ? nullptr : colorRefs.data();
    subpass.pResolveAttachments = resolveRefs.empty() ? nullptr : resolveRefs.data();
    subpass.pDepthStencilAttachment = hasDepth ? &depthRef : nullptr;

    // Create subpass dependency
    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = 0;
    dependency.dstStageMask = 0;
    dependency.srcAccessMask = 0;
    dependency.dstAccessMask = 0;

    // Add color attachment stages if present
    bool hasColor = !colorRefs.empty();
    if (hasColor) {
        dependency.srcStageMask |= VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.dstStageMask |= VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.srcAccessMask |= VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        dependency.dstAccessMask |= VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    }

    // Add depth/stencil stages if present
    if (hasDepth) {
        dependency.srcStageMask |= VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
        dependency.dstStageMask |= VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
        dependency.srcAccessMask |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        dependency.dstAccessMask |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    }

    dependency.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    // Create render pass
    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    renderPassInfo.pAttachments = attachments.data();
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    // Add multiview support if requested
    VkRenderPassMultiviewCreateInfo multiviewInfo{};
    if (createInfo.viewMask.has_value()) {
        multiviewInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_MULTIVIEW_CREATE_INFO;
        multiviewInfo.pNext = nullptr;
        multiviewInfo.subpassCount = 1;
        multiviewInfo.pViewMasks = &createInfo.viewMask.value();
        multiviewInfo.dependencyCount = 0;
        multiviewInfo.pViewOffsets = nullptr;
        multiviewInfo.correlationMaskCount = static_cast<uint32_t>(createInfo.correlationMasks.size());
        multiviewInfo.pCorrelationMasks = createInfo.correlationMasks.empty() ? nullptr : createInfo.correlationMasks.data();

        renderPassInfo.pNext = &multiviewInfo;
    }

    VkResult result = vkCreateRenderPass(m_device->handle(), &renderPassInfo, nullptr, &m_renderPass);
    if (result != VK_SUCCESS) {
        throw std::runtime_error("Failed to create render pass");
    }
}

RenderPass::~RenderPass()
{
    if (m_renderPass != VK_NULL_HANDLE) {
        vkDestroyRenderPass(m_device->handle(), m_renderPass, nullptr);
    }
}

VkRenderPass RenderPass::handle() const
{
    return m_renderPass;
}

uint32_t RenderPass::colorAttachmentCount() const
{
    return m_colorAttachmentCount;
}

bool RenderPass::hasDepthStencil() const
{
    return m_hasDepthStencil;
}

const std::vector<bool>& RenderPass::colorHasResolve() const
{
    return m_colorHasResolve;
}

} // namespace gfx::backend::vulkan::core