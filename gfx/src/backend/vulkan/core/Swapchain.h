#ifndef GFX_VULKAN_SWAPCHAIN_H
#define GFX_VULKAN_SWAPCHAIN_H

#include "CoreTypes.h"

#include <memory>
#include <vector>

namespace gfx::backend::vulkan::core {

class Surface;
class Device;
class Texture;
class TextureView;

class Swapchain {
public:
    Swapchain(const Swapchain&) = delete;
    Swapchain& operator=(const Swapchain&) = delete;

    Swapchain(Device* device, Surface* surface, const SwapchainCreateInfo& createInfo);
    ~Swapchain();

    VkSwapchainKHR handle() const;
    uint32_t getImageCount() const;
    Texture* getTexture(uint32_t index) const;
    Texture* getCurrentTexture() const;
    TextureView* getTextureView(uint32_t index) const;
    TextureView* getCurrentTextureView() const;
    VkFormat getFormat() const;
    uint32_t getWidth() const;
    uint32_t getHeight() const;
    uint32_t getCurrentImageIndex() const;
    VkPresentModeKHR getPresentMode() const;
    const SwapchainInfo& getInfo() const;

    VkResult acquireNextImage(uint64_t timeoutNs, VkSemaphore semaphore, VkFence fence, uint32_t* outImageIndex);
    VkResult present(const std::vector<VkSemaphore>& waitSemaphores);

private:
    VkSwapchainKHR m_swapchain = VK_NULL_HANDLE;
    Device* m_device = nullptr;
    Surface* m_surface = nullptr;
    VkQueue m_presentQueue = VK_NULL_HANDLE;
    std::vector<VkImage> m_images;
    std::vector<std::unique_ptr<Texture>> m_textures;
    std::vector<std::unique_ptr<TextureView>> m_textureViews;
    SwapchainInfo m_info{};
    uint32_t m_currentImageIndex = 0;
};

} // namespace gfx::backend::vulkan::core

#endif // GFX_VULKAN_SWAPCHAIN_H