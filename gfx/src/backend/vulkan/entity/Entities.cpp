#include "Entities.h"

namespace gfx::vulkan {

// ============================================================================
// Queue Implementation
// ============================================================================

Queue::Queue(Device* device, uint32_t queueFamily)
    : m_device(device)
    , m_queueFamily(queueFamily)
{
    vkGetDeviceQueue(device->handle(), queueFamily, 0, &m_queue);
}

VkDevice Queue::device() const
{
    return m_device->handle();
}

VkPhysicalDevice Queue::physicalDevice() const
{
    return m_device->getAdapter()->handle();
}

VkResult Queue::submit(const SubmitInfo& submitInfo)
{
    // Convert command encoders to command buffers
    std::vector<VkCommandBuffer> commandBuffers;
    commandBuffers.reserve(submitInfo.commandEncoderCount);
    for (uint32_t i = 0; i < submitInfo.commandEncoderCount; ++i) {
        commandBuffers.push_back(submitInfo.commandEncoders[i]->handle());
    }

    // Convert wait semaphores
    std::vector<VkSemaphore> waitSemaphores;
    std::vector<uint64_t> waitValues;
    std::vector<VkPipelineStageFlags> waitStages;
    waitSemaphores.reserve(submitInfo.waitSemaphoreCount);
    waitStages.reserve(submitInfo.waitSemaphoreCount);

    bool hasTimelineWait = false;
    for (uint32_t i = 0; i < submitInfo.waitSemaphoreCount; ++i) {
        waitSemaphores.push_back(submitInfo.waitSemaphores[i]->handle());
        waitStages.push_back(VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);

        if (submitInfo.waitSemaphores[i]->getType() == SemaphoreType::Timeline) {
            hasTimelineWait = true;
            uint64_t value = submitInfo.waitValues ? submitInfo.waitValues[i] : 0;
            waitValues.push_back(value);
        } else {
            waitValues.push_back(0);
        }
    }

    // Convert signal semaphores
    std::vector<VkSemaphore> signalSemaphores;
    std::vector<uint64_t> signalValues;
    signalSemaphores.reserve(submitInfo.signalSemaphoreCount);

    bool hasTimelineSignal = false;
    for (uint32_t i = 0; i < submitInfo.signalSemaphoreCount; ++i) {
        signalSemaphores.push_back(submitInfo.signalSemaphores[i]->handle());

        if (submitInfo.signalSemaphores[i]->getType() == SemaphoreType::Timeline) {
            hasTimelineSignal = true;
            uint64_t value = submitInfo.signalValues ? submitInfo.signalValues[i] : 0;
            signalValues.push_back(value);
        } else {
            signalValues.push_back(0);
        }
    }

    // Timeline semaphore info (if needed)
    VkTimelineSemaphoreSubmitInfo timelineInfo{};
    if (hasTimelineWait || hasTimelineSignal) {
        timelineInfo.sType = VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO;
        timelineInfo.waitSemaphoreValueCount = static_cast<uint32_t>(waitValues.size());
        timelineInfo.pWaitSemaphoreValues = waitValues.empty() ? nullptr : waitValues.data();
        timelineInfo.signalSemaphoreValueCount = static_cast<uint32_t>(signalValues.size());
        timelineInfo.pSignalSemaphoreValues = signalValues.empty() ? nullptr : signalValues.data();
    }

    // Build submit info
    VkSubmitInfo vkSubmitInfo{};
    vkSubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    if (hasTimelineWait || hasTimelineSignal) {
        vkSubmitInfo.pNext = &timelineInfo;
    }
    vkSubmitInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());
    vkSubmitInfo.pCommandBuffers = commandBuffers.empty() ? nullptr : commandBuffers.data();
    vkSubmitInfo.waitSemaphoreCount = static_cast<uint32_t>(waitSemaphores.size());
    vkSubmitInfo.pWaitSemaphores = waitSemaphores.empty() ? nullptr : waitSemaphores.data();
    vkSubmitInfo.pWaitDstStageMask = waitStages.empty() ? nullptr : waitStages.data();
    vkSubmitInfo.signalSemaphoreCount = static_cast<uint32_t>(signalSemaphores.size());
    vkSubmitInfo.pSignalSemaphores = signalSemaphores.empty() ? nullptr : signalSemaphores.data();

    // Get fence if provided
    VkFence fence = VK_NULL_HANDLE;
    if (submitInfo.signalFence) {
        fence = submitInfo.signalFence->handle();
    }

    return vkQueueSubmit(m_queue, 1, &vkSubmitInfo, fence);
}

void Queue::writeTexture(Texture* texture, const VkOffset3D& origin, uint32_t mipLevel,
    const void* data, uint64_t dataSize,
    const VkExtent3D& extent, VkImageLayout finalLayout)
{
    VkDevice device = texture->device();

    // Create staging buffer
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = dataSize;
    bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VkBuffer stagingBuffer = VK_NULL_HANDLE;
    VkResult result = vkCreateBuffer(device, &bufferInfo, nullptr, &stagingBuffer);
    if (result != VK_SUCCESS) {
        fprintf(stderr, "Failed to create staging buffer for texture upload\n");
        return;
    }

    // Get memory requirements and allocate
    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(device, stagingBuffer, &memRequirements);

    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice(), &memProperties);

    uint32_t memoryTypeIndex = UINT32_MAX;
    VkMemoryPropertyFlags properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; ++i) {
        if ((memRequirements.memoryTypeBits & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            memoryTypeIndex = i;
            break;
        }
    }

    if (memoryTypeIndex == UINT32_MAX) {
        fprintf(stderr, "Failed to find suitable memory type for staging buffer\n");
        vkDestroyBuffer(device, stagingBuffer, nullptr);
        return;
    }

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = memoryTypeIndex;

    VkDeviceMemory stagingMemory = VK_NULL_HANDLE;
    result = vkAllocateMemory(device, &allocInfo, nullptr, &stagingMemory);
    if (result != VK_SUCCESS) {
        fprintf(stderr, "Failed to allocate staging buffer memory\n");
        vkDestroyBuffer(device, stagingBuffer, nullptr);
        return;
    }

    vkBindBufferMemory(device, stagingBuffer, stagingMemory, 0);

    // Copy data to staging buffer
    void* mappedData = nullptr;
    vkMapMemory(device, stagingMemory, 0, dataSize, 0, &mappedData);
    memcpy(mappedData, data, dataSize);
    vkUnmapMemory(device, stagingMemory);

    // Create temporary command buffer
    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
    poolInfo.queueFamilyIndex = m_queueFamily;

    VkCommandPool commandPool = VK_NULL_HANDLE;
    vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool);

    VkCommandBufferAllocateInfo allocCmdInfo{};
    allocCmdInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocCmdInfo.commandPool = commandPool;
    allocCmdInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocCmdInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
    vkAllocateCommandBuffers(device, &allocCmdInfo, &commandBuffer);

    // Begin command buffer
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    // Transition image to transfer dst optimal
    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = texture->getLayout();
    barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = texture->handle();
    barrier.subresourceRange.aspectMask = converter::getImageAspectMask(texture->getFormat());
    barrier.subresourceRange.baseMipLevel = mipLevel;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

    vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
        VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

    // Copy buffer to image
    VkBufferImageCopy region{};
    region.bufferOffset = 0;
    region.bufferRowLength = 0; // Tightly packed
    region.bufferImageHeight = 0;
    region.imageSubresource.aspectMask = converter::getImageAspectMask(texture->getFormat());
    region.imageSubresource.mipLevel = mipLevel;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    region.imageOffset = origin;
    region.imageExtent = extent;

    vkCmdCopyBufferToImage(commandBuffer, stagingBuffer, texture->handle(),
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    // Transition image to final layout
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = finalLayout;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = getVkAccessFlagsForLayout(finalLayout);

    vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

    // Update tracked layout
    texture->setLayout(finalLayout);

    // End and submit
    vkEndCommandBuffer(commandBuffer);

    // Create fence for synchronization
    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    VkFence fence = VK_NULL_HANDLE;
    vkCreateFence(device, &fenceInfo, nullptr, &fence);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    vkQueueSubmit(m_queue, 1, &submitInfo, fence);
    vkWaitForFences(device, 1, &fence, VK_TRUE, UINT64_MAX);
    vkDestroyFence(device, fence, nullptr);

    // Cleanup
    vkDestroyCommandPool(device, commandPool, nullptr);
    vkDestroyBuffer(device, stagingBuffer, nullptr);
    vkFreeMemory(device, stagingMemory, nullptr);
}

void Queue::waitIdle()
{
    vkQueueWaitIdle(m_queue);
}

// ============================================================================
// RenderPassEncoder Implementation
// ============================================================================

RenderPassEncoder::RenderPassEncoder(CommandEncoder* commandEncoder, const RenderPassEncoderCreateInfo& createInfo)
    : m_commandBuffer(commandEncoder->handle())
    , m_device(commandEncoder->getDevice())
    , m_commandEncoder(commandEncoder)
{
    // Determine framebuffer dimensions from first available attachment
    uint32_t width = 0;
    uint32_t height = 0;

    // Try to get dimensions from color attachments
    for (const auto& colorAttachment : createInfo.colorAttachments) {
        if (width == 0 || height == 0) {
            width = colorAttachment.target.width;
            height = colorAttachment.target.height;
            break;
        }
    }

    // Fall back to depth attachment if no color attachment had valid dimensions
    if ((width == 0 || height == 0) && createInfo.depthStencilAttachment.has_value()) {
        width = createInfo.depthStencilAttachment->target.width;
        height = createInfo.depthStencilAttachment->target.height;
    }

    // Build Vulkan attachments and references
    std::vector<VkAttachmentDescription> attachments;
    std::vector<VkAttachmentReference> colorRefs;
    std::vector<VkAttachmentReference> resolveRefs;
    std::vector<VkImageView> fbAttachments;

    uint32_t attachmentIndex = 0;
    uint32_t numColorRefs = 0;

    // Process color attachments
    for (const auto& colorAttachment : createInfo.colorAttachments) {
        const ColorAttachmentTarget& target = colorAttachment.target;
        bool isMSAA = (target.sampleCount > VK_SAMPLE_COUNT_1_BIT);

        // Add color attachment
        VkAttachmentDescription colorAttachmentDesc{};
        colorAttachmentDesc.format = target.format;
        colorAttachmentDesc.samples = target.sampleCount;
        colorAttachmentDesc.loadOp = target.ops.loadOp;
        colorAttachmentDesc.storeOp = target.ops.storeOp;
        colorAttachmentDesc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachmentDesc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAttachmentDesc.initialLayout = (target.ops.loadOp == VK_ATTACHMENT_LOAD_OP_LOAD)
            ? VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
            : VK_IMAGE_LAYOUT_UNDEFINED;
        colorAttachmentDesc.finalLayout = target.finalLayout;
        attachments.push_back(colorAttachmentDesc);

        VkAttachmentReference colorRef{};
        colorRef.attachment = attachmentIndex++;
        colorRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        colorRefs.push_back(colorRef);
        ++numColorRefs;

        fbAttachments.push_back(target.view);

        // Check if this attachment has a resolve target
        if (colorAttachment.resolveTarget.has_value()) {
            const ColorAttachmentTarget& resolveTarget = colorAttachment.resolveTarget.value();

            VkAttachmentDescription resolveAttachmentDesc{};
            resolveAttachmentDesc.format = resolveTarget.format;
            resolveAttachmentDesc.samples = VK_SAMPLE_COUNT_1_BIT;
            resolveAttachmentDesc.loadOp = resolveTarget.ops.loadOp;
            resolveAttachmentDesc.storeOp = resolveTarget.ops.storeOp;
            resolveAttachmentDesc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            resolveAttachmentDesc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            resolveAttachmentDesc.initialLayout = (resolveTarget.ops.loadOp == VK_ATTACHMENT_LOAD_OP_LOAD)
                ? VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
                : VK_IMAGE_LAYOUT_UNDEFINED;
            resolveAttachmentDesc.finalLayout = resolveTarget.finalLayout;
            attachments.push_back(resolveAttachmentDesc);

            VkAttachmentReference resolveRef{};
            resolveRef.attachment = attachmentIndex++;
            resolveRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            resolveRefs.push_back(resolveRef);

            fbAttachments.push_back(resolveTarget.view);
        } else if (isMSAA) {
            // MSAA without resolve needs unused reference
            VkAttachmentReference unusedRef{};
            unusedRef.attachment = VK_ATTACHMENT_UNUSED;
            unusedRef.layout = VK_IMAGE_LAYOUT_UNDEFINED;
            resolveRefs.push_back(unusedRef);
        }
    }

    // Add depth/stencil attachment if provided
    VkAttachmentReference depthRef{};
    bool hasDepth = false;

    if (createInfo.depthStencilAttachment.has_value()) {
        const DepthStencilAttachmentTarget& target = createInfo.depthStencilAttachment->target;

        VkAttachmentDescription depthAttachment{};
        depthAttachment.format = target.format;
        depthAttachment.samples = target.sampleCount;

        // Handle depth operations
        if (target.depthOps.has_value()) {
            depthAttachment.loadOp = target.depthOps->loadOp;
            depthAttachment.storeOp = target.depthOps->storeOp;
        } else {
            depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        }

        // Handle stencil operations
        if (target.stencilOps.has_value()) {
            depthAttachment.stencilLoadOp = target.stencilOps->loadOp;
            depthAttachment.stencilStoreOp = target.stencilOps->storeOp;
        } else {
            depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        }

        bool loadDepth = (target.depthOps.has_value() && target.depthOps->loadOp == VK_ATTACHMENT_LOAD_OP_LOAD);
        bool loadStencil = (target.stencilOps.has_value() && target.stencilOps->loadOp == VK_ATTACHMENT_LOAD_OP_LOAD);
        depthAttachment.initialLayout = (loadDepth || loadStencil)
            ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
            : VK_IMAGE_LAYOUT_UNDEFINED;
        depthAttachment.finalLayout = target.finalLayout;
        attachments.push_back(depthAttachment);

        depthRef.attachment = attachmentIndex++;
        depthRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        hasDepth = true;

        fbAttachments.push_back(target.view);
    }

    // Create subpass
    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = static_cast<uint32_t>(colorRefs.size());
    subpass.pColorAttachments = colorRefs.empty() ? nullptr : colorRefs.data();
    subpass.pResolveAttachments = resolveRefs.empty() ? nullptr : resolveRefs.data();
    subpass.pDepthStencilAttachment = hasDepth ? &depthRef : nullptr;

    // Create render pass
    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    renderPassInfo.pAttachments = attachments.data();
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;

    VkRenderPass renderPass;
    VkResult result = vkCreateRenderPass(m_device->handle(), &renderPassInfo, nullptr, &renderPass);
    if (result != VK_SUCCESS) {
        throw std::runtime_error("Failed to create render pass");
    }

    // Create framebuffer
    VkFramebufferCreateInfo framebufferInfo{};
    framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferInfo.renderPass = renderPass;
    framebufferInfo.attachmentCount = static_cast<uint32_t>(fbAttachments.size());
    framebufferInfo.pAttachments = fbAttachments.data();
    framebufferInfo.width = width;
    framebufferInfo.height = height;
    framebufferInfo.layers = 1;

    VkFramebuffer framebuffer;
    result = vkCreateFramebuffer(m_device->handle(), &framebufferInfo, nullptr, &framebuffer);
    if (result != VK_SUCCESS) {
        vkDestroyRenderPass(m_device->handle(), renderPass, nullptr);
        throw std::runtime_error("Failed to create framebuffer");
    }

    // Track for cleanup
    commandEncoder->trackRenderPass(renderPass, framebuffer);

    // Build clear values
    std::vector<VkClearValue> clearValues;
    clearValues.reserve(attachments.size());

    uint32_t clearColorIdx = 0;
    for (size_t i = 0; i < attachments.size(); ++i) {
        VkClearValue clearValue{};

        if (converter::isDepthFormat(attachments[i].format)) {
            // Depth/stencil attachment
            float depthClear = (createInfo.depthStencilAttachment.has_value() && createInfo.depthStencilAttachment->target.depthOps.has_value())
                ? createInfo.depthStencilAttachment->target.depthOps->clearValue
                : 1.0f;
            uint32_t stencilClear = (createInfo.depthStencilAttachment.has_value() && createInfo.depthStencilAttachment->target.stencilOps.has_value())
                ? createInfo.depthStencilAttachment->target.stencilOps->clearValue
                : 0;
            clearValue.depthStencil = { depthClear, stencilClear };
        } else {
            // Color attachment
            bool isPrevMSAA = (i > 0 && attachments[i - 1].samples > VK_SAMPLE_COUNT_1_BIT);
            bool isResolve = (attachments[i].samples == VK_SAMPLE_COUNT_1_BIT && isPrevMSAA);

            if (isResolve) {
                clearValue.color = { { 0.0f, 0.0f, 0.0f, 0.0f } };
            } else {
                if (clearColorIdx < numColorRefs) {
                    clearValue.color = createInfo.colorAttachments[clearColorIdx].target.ops.clearColor;
                    clearColorIdx++;
                } else {
                    clearValue.color = { { 0.0f, 0.0f, 0.0f, 0.0f } };
                }
            }
        }

        clearValues.push_back(clearValue);
    }

    // Begin render pass
    VkRenderPassBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    beginInfo.renderPass = renderPass;
    beginInfo.framebuffer = framebuffer;
    beginInfo.renderArea.offset = { 0, 0 };
    beginInfo.renderArea.extent = { width, height };
    beginInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    beginInfo.pClearValues = clearValues.data();

    vkCmdBeginRenderPass(m_commandBuffer, &beginInfo, VK_SUBPASS_CONTENTS_INLINE);
}

// ============================================================================
// ComputePassEncoder Implementation
// ============================================================================

ComputePassEncoder::ComputePassEncoder(CommandEncoder* commandEncoder, const ComputePassEncoderCreateInfo& createInfo)
    : m_commandBuffer(commandEncoder->handle())
    , m_device(commandEncoder->getDevice())
    , m_commandEncoder(commandEncoder)
{
    (void)createInfo; // Label unused for now
}

} // namespace gfx::vulkan
