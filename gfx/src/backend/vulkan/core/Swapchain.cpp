#include "Swapchain.h"

#include "Adapter.h"
#include "Device.h"
#include "Surface.h"
#include "Texture.h"
#include "TextureView.h"

namespace gfx::backend::vulkan::core {

Swapchain::Swapchain(Device* device, Surface* surface, const SwapchainCreateInfo& createInfo)
    : m_device(device)
    , m_surface(surface)
{
    uint32_t queueFamily = device->getAdapter()->getGraphicsQueueFamily();

    // Check if queue family supports presentation
    VkBool32 presentSupport = VK_FALSE;
    vkGetPhysicalDeviceSurfaceSupportKHR(device->getAdapter()->handle(), queueFamily, surface->handle(), &presentSupport);
    if (presentSupport != VK_TRUE) {
        throw std::runtime_error("Selected queue family does not support presentation");
    }

    // Query and choose format
    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(m_surface->physicalDevice(), m_surface->handle(), &formatCount, nullptr);
    if (formatCount == 0) {
        throw std::runtime_error("No surface formats available for swapchain");
    }
    std::vector<VkSurfaceFormatKHR> formats(formatCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(m_surface->physicalDevice(), m_surface->handle(), &formatCount, formats.data());

    VkSurfaceFormatKHR selectedFormat = formats[0];
    for (const auto& availableFormat : formats) {
        if (availableFormat.format == createInfo.format) {
            selectedFormat = availableFormat;
            break;
        }
    }
    m_info.format = selectedFormat.format;

    // Query and choose present mode
    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(m_surface->physicalDevice(), m_surface->handle(), &presentModeCount, nullptr);
    if (presentModeCount == 0) {
        throw std::runtime_error("No present modes available for swapchain");
    }
    std::vector<VkPresentModeKHR> presentModes(presentModeCount);
    vkGetPhysicalDeviceSurfacePresentModesKHR(m_surface->physicalDevice(), m_surface->handle(), &presentModeCount, presentModes.data());

    m_info.presentMode = VK_PRESENT_MODE_FIFO_KHR;
    for (const auto& availableMode : presentModes) {
        if (availableMode == createInfo.presentMode) {
            m_info.presentMode = availableMode;
            break;
        }
    }

    // Query surface capabilities
    VkSurfaceCapabilitiesKHR capabilities;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_surface->physicalDevice(), m_surface->handle(), &capabilities);

    // Determine actual swapchain extent and store directly in m_info
    // If currentExtent is defined, we MUST use it. Otherwise, we can choose within min/max bounds.
    if (capabilities.currentExtent.width != UINT32_MAX) {
        // Window manager is telling us the size - we must use it
        m_info.width = capabilities.currentExtent.width;
        m_info.height = capabilities.currentExtent.height;
    } else {
        // We can choose the extent within bounds
        m_info.width = std::max(capabilities.minImageExtent.width,
            std::min(createInfo.width, capabilities.maxImageExtent.width));
        m_info.height = std::max(capabilities.minImageExtent.height,
            std::min(createInfo.height, capabilities.maxImageExtent.height));
    }

    // Create swapchain
    VkSwapchainCreateInfoKHR vkCreateInfo{};
    vkCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    vkCreateInfo.surface = m_surface->handle();
    vkCreateInfo.minImageCount = std::min(createInfo.imageCount, capabilities.minImageCount + 1);
    if (capabilities.maxImageCount > 0) {
        vkCreateInfo.minImageCount = std::min(vkCreateInfo.minImageCount, capabilities.maxImageCount);
    }
    vkCreateInfo.imageFormat = m_info.format;
    vkCreateInfo.imageColorSpace = selectedFormat.colorSpace;
    vkCreateInfo.imageExtent = { m_info.width, m_info.height };
    vkCreateInfo.imageArrayLayers = 1;
    vkCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    vkCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    vkCreateInfo.preTransform = capabilities.currentTransform;
    vkCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    vkCreateInfo.presentMode = m_info.presentMode;
    vkCreateInfo.clipped = VK_TRUE;

    VkResult result = vkCreateSwapchainKHR(m_device->handle(), &vkCreateInfo, nullptr, &m_swapchain);
    if (result != VK_SUCCESS) {
        throw std::runtime_error("Failed to create swapchain");
    }

    // Get swapchain images
    uint32_t imageCount;
    vkGetSwapchainImagesKHR(m_device->handle(), m_swapchain, &imageCount, nullptr);
    m_images.resize(imageCount);
    vkGetSwapchainImagesKHR(m_device->handle(), m_swapchain, &imageCount, m_images.data());

    // Update SwapchainInfo with final imageCount (width/height/format/presentMode already set)
    m_info.imageCount = imageCount;

    m_textures.reserve(imageCount);
    m_textureViews.reserve(imageCount);
    for (size_t i = 0; i < imageCount; ++i) {
        // Create non-owning Texture wrapper for swapchain image
        TextureCreateInfo textureCreateInfo{};
        textureCreateInfo.format = m_info.format;
        textureCreateInfo.size = { m_info.width, m_info.height, 1 };
        textureCreateInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        textureCreateInfo.sampleCount = VK_SAMPLE_COUNT_1_BIT;
        textureCreateInfo.mipLevelCount = 1;
        textureCreateInfo.imageType = VK_IMAGE_TYPE_2D;
        textureCreateInfo.arrayLayers = 1;
        textureCreateInfo.flags = 0;
        m_textures.push_back(std::make_unique<Texture>(m_device, m_images[i], textureCreateInfo));

        // Create TextureView for the texture
        TextureViewCreateInfo viewCreateInfo{};
        viewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewCreateInfo.format = VK_FORMAT_UNDEFINED; // Use texture's format
        viewCreateInfo.baseMipLevel = 0;
        viewCreateInfo.mipLevelCount = 1;
        viewCreateInfo.baseArrayLayer = 0;
        viewCreateInfo.arrayLayerCount = 1;
        m_textureViews.push_back(std::make_unique<TextureView>(m_textures[i].get(), viewCreateInfo));
    }

    // Get present queue (assume queue family 0)
    vkGetDeviceQueue(m_device->handle(), 0, 0, &m_presentQueue);

    // Don't pre-acquire an image - let explicit acquire handle it
    m_currentImageIndex = 0;
}

Swapchain::~Swapchain()
{
    // Explicitly destroy TextureViews and Textures before destroying the swapchain
    // This ensures VkImageViews are destroyed before the swapchain's VkImages
    m_textureViews.clear();
    m_textures.clear();

    if (m_swapchain != VK_NULL_HANDLE) {
        vkDestroySwapchainKHR(m_device->handle(), m_swapchain, nullptr);
    }
}

VkSwapchainKHR Swapchain::handle() const
{
    return m_swapchain;
}

uint32_t Swapchain::getImageCount() const
{
    return m_info.imageCount;
}

Texture* Swapchain::getTexture(uint32_t index) const
{
    return m_textures[index].get();
}

Texture* Swapchain::getCurrentTexture() const
{
    return m_textures[m_currentImageIndex].get();
}

TextureView* Swapchain::getTextureView(uint32_t index) const
{
    return m_textureViews[index].get();
}

TextureView* Swapchain::getCurrentTextureView() const
{
    return m_textureViews[m_currentImageIndex].get();
}

VkFormat Swapchain::getFormat() const
{
    return m_info.format;
}

uint32_t Swapchain::getWidth() const
{
    return m_info.width;
}

uint32_t Swapchain::getHeight() const
{
    return m_info.height;
}

uint32_t Swapchain::getCurrentImageIndex() const
{
    return m_currentImageIndex;
}

VkPresentModeKHR Swapchain::getPresentMode() const
{
    return m_info.presentMode;
}

const SwapchainInfo& Swapchain::getInfo() const
{
    return m_info;
}

VkResult Swapchain::acquireNextImage(uint64_t timeoutNs, VkSemaphore semaphore, VkFence fence, uint32_t* outImageIndex)
{
    VkResult result = vkAcquireNextImageKHR(m_device->handle(), m_swapchain, timeoutNs, semaphore, fence, outImageIndex);
    if (result == VK_SUCCESS || result == VK_SUBOPTIMAL_KHR) {
        m_currentImageIndex = *outImageIndex;
    }
    return result;
}

VkResult Swapchain::present(const std::vector<VkSemaphore>& waitSemaphores)
{
    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = static_cast<uint32_t>(waitSemaphores.size());
    presentInfo.pWaitSemaphores = waitSemaphores.empty() ? nullptr : waitSemaphores.data();
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &m_swapchain;
    presentInfo.pImageIndices = &m_currentImageIndex;

    return vkQueuePresentKHR(m_presentQueue, &presentInfo);
}

} // namespace gfx::backend::vulkan::core