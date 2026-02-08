#ifndef GFX_VULKAN_SURFACE_H
#define GFX_VULKAN_SURFACE_H

#include "../CoreTypes.h"

namespace gfx::backend::vulkan::core {

class Adapter;
class Instance;

class Surface {
public:
    Surface(const Surface&) = delete;
    Surface& operator=(const Surface&) = delete;

    Surface(Adapter* adapter, const SurfaceCreateInfo& createInfo);
    ~Surface();

    VkInstance instance() const;
    VkPhysicalDevice physicalDevice() const;
    VkSurfaceKHR handle() const;

    std::vector<VkSurfaceFormatKHR> getSupportedFormats() const;
    std::vector<VkPresentModeKHR> getSupportedPresentModes() const;

    VkSurfaceCapabilitiesKHR getCapabilities() const;

private:
    Adapter* m_adapter = nullptr;
    VkSurfaceKHR m_surface = VK_NULL_HANDLE;
};

} // namespace gfx::backend::vulkan::core

#endif // GFX_VULKAN_SURFACE_H