#include "Surface.h"

#include "Adapter.h"
#include "Instance.h"

#include <stdexcept>

namespace gfx::backend::vulkan::core {

namespace {

#ifndef GFX_HEADLESS_BUILD
#ifdef GFX_HAS_WIN32
    static VkSurfaceKHR createSurfaceWin32(VkInstance instance, const PlatformWindowHandle& windowHandle)
    {
        if (!windowHandle.handle.win32.hwnd || !windowHandle.handle.win32.hinstance) {
            throw std::runtime_error("Invalid Win32 window or instance handle");
        }

        VkWin32SurfaceCreateInfoKHR vkCreateInfo{};
        vkCreateInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
        vkCreateInfo.hwnd = windowHandle.handle.win32.hwnd;
        vkCreateInfo.hinstance = windowHandle.handle.win32.hinstance;

        VkSurfaceKHR surface;
        if (vkCreateWin32SurfaceKHR(instance, &vkCreateInfo, nullptr, &surface) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create Win32 surface");
        }
        return surface;
    }
#endif
#ifdef GFX_HAS_ANDROID
    static VkSurfaceKHR createSurfaceAndroid(VkInstance instance, const PlatformWindowHandle& windowHandle)
    {
        if (!windowHandle.handle.android.window) {
            throw std::runtime_error("Invalid Android window handle");
        }

        VkAndroidSurfaceCreateInfoKHR vkCreateInfo{};
        vkCreateInfo.sType = VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR;
        vkCreateInfo.window = windowHandle.handle.android.window;

        VkSurfaceKHR surface;
        if (vkCreateAndroidSurfaceKHR(instance, &vkCreateInfo, nullptr, &surface) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create Android surface");
        }
        return surface;
    }
#endif
#ifdef GFX_HAS_X11
    static VkSurfaceKHR createSurfaceXlib(VkInstance instance, const PlatformWindowHandle& windowHandle)
    {
        if (!windowHandle.handle.xlib.display || !windowHandle.handle.xlib.window) {
            throw std::runtime_error("Invalid Xlib display handle or window handle");
        }

        VkXlibSurfaceCreateInfoKHR vkCreateInfo{};
        vkCreateInfo.sType = VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR;
        vkCreateInfo.dpy = static_cast<Display*>(windowHandle.handle.xlib.display);
        vkCreateInfo.window = static_cast<Window>(windowHandle.handle.xlib.window);

        VkSurfaceKHR surface;
        if (vkCreateXlibSurfaceKHR(instance, &vkCreateInfo, nullptr, &surface) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create Xlib surface");
        }
        return surface;
    }
#endif
#ifdef GFX_HAS_XCB
    static VkSurfaceKHR createSurfaceXCB(VkInstance instance, const PlatformWindowHandle& windowHandle)
    {
        if (!windowHandle.handle.xcb.window || !windowHandle.handle.xcb.connection) {
            throw std::runtime_error("Invalid XCB window or connection handle");
        }

        VkXcbSurfaceCreateInfoKHR vkCreateInfo{};
        vkCreateInfo.sType = VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR;
        vkCreateInfo.connection = static_cast<xcb_connection_t*>(windowHandle.handle.xcb.connection);
        vkCreateInfo.window = static_cast<xcb_window_t>(windowHandle.handle.xcb.window);

        VkSurfaceKHR surface;
        if (vkCreateXcbSurfaceKHR(instance, &vkCreateInfo, nullptr, &surface) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create XCB surface");
        }
        return surface;
    }
#endif
#ifdef GFX_HAS_WAYLAND
    static VkSurfaceKHR createSurfaceWayland(VkInstance instance, const PlatformWindowHandle& windowHandle)
    {
        if (!windowHandle.handle.wayland.surface || !windowHandle.handle.wayland.display) {
            throw std::runtime_error("Invalid Wayland surface or display handle");
        }

        VkWaylandSurfaceCreateInfoKHR vkCreateInfo{};
        vkCreateInfo.sType = VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR;
        vkCreateInfo.display = static_cast<wl_display*>(windowHandle.handle.wayland.display);
        vkCreateInfo.surface = static_cast<wl_surface*>(windowHandle.handle.wayland.surface);

        VkSurfaceKHR surface;
        if (vkCreateWaylandSurfaceKHR(instance, &vkCreateInfo, nullptr, &surface) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create Wayland surface");
        }
        return surface;
    }
#endif
#if defined(GFX_HAS_COCOA) || defined(GFX_HAS_UIKIT)
    static VkSurfaceKHR createSurfaceMetal(VkInstance instance, const PlatformWindowHandle& windowHandle)
    {
        if (!windowHandle.handle.metalLayer) {
            throw std::runtime_error("Invalid Metal layer handle");
        }

        VkMetalSurfaceCreateInfoEXT metalCreateInfo{};
        metalCreateInfo.sType = VK_STRUCTURE_TYPE_METAL_SURFACE_CREATE_INFO_EXT;
        metalCreateInfo.pLayer = windowHandle.handle.metalLayer;

        VkSurfaceKHR surface;
        if (vkCreateMetalSurfaceEXT(instance, &metalCreateInfo, nullptr, &surface) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create Metal surface");
        }
        return surface;
    }
#endif
#endif // GFX_HEADLESS_BUILD

} // namespace

Surface::Surface(Adapter* adapter, const SurfaceCreateInfo& createInfo)
    : m_adapter(adapter)
{
#ifdef GFX_HEADLESS_BUILD
    (void)createInfo;
    throw std::runtime_error("Surface creation is not available in headless builds");
#else
    switch (createInfo.windowHandle.platform) {
#ifdef GFX_HAS_WIN32
    case PlatformWindowHandle::Platform::Win32:
        m_surface = createSurfaceWin32(m_adapter->getInstance()->handle(), createInfo.windowHandle);
        break;
#endif
#ifdef GFX_HAS_ANDROID
    case PlatformWindowHandle::Platform::Android:
        m_surface = createSurfaceAndroid(m_adapter->getInstance()->handle(), createInfo.windowHandle);
        break;
#endif
#ifdef GFX_HAS_X11
    case PlatformWindowHandle::Platform::Xlib:
        m_surface = createSurfaceXlib(m_adapter->getInstance()->handle(), createInfo.windowHandle);
        break;
#endif
#ifdef GFX_HAS_XCB
    case PlatformWindowHandle::Platform::Xcb:
        m_surface = createSurfaceXCB(m_adapter->getInstance()->handle(), createInfo.windowHandle);
        break;
#endif
#ifdef GFX_HAS_WAYLAND
    case PlatformWindowHandle::Platform::Wayland:
        m_surface = createSurfaceWayland(m_adapter->getInstance()->handle(), createInfo.windowHandle);
        break;
#endif
#if defined(GFX_HAS_COCOA) || defined(GFX_HAS_UIKIT)
    case PlatformWindowHandle::Platform::Metal:
        m_surface = createSurfaceMetal(m_adapter->getInstance()->handle(), createInfo.windowHandle);
        break;
#endif
    // Other platforms can be added here
    default:
        throw std::runtime_error("Unsupported windowing platform");
    }
#endif // GFX_HEADLESS_BUILD
}

Surface::~Surface()
{
    if (m_surface != VK_NULL_HANDLE) {
        vkDestroySurfaceKHR(m_adapter->getInstance()->handle(), m_surface, nullptr);
    }
}

VkInstance Surface::instance() const
{
    return m_adapter->getInstance()->handle();
}

VkPhysicalDevice Surface::physicalDevice() const
{
    return m_adapter->handle();
}

VkSurfaceKHR Surface::handle() const
{
    return m_surface;
}

std::vector<VkSurfaceFormatKHR> Surface::getSupportedFormats() const
{
    uint32_t formatCount = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(m_adapter->handle(), m_surface, &formatCount, nullptr);

    if (formatCount == 0) {
        return {};
    }

    std::vector<VkSurfaceFormatKHR> formats(formatCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(m_adapter->handle(), m_surface, &formatCount, formats.data());
    return formats;
}

std::vector<VkPresentModeKHR> Surface::getSupportedPresentModes() const
{
    uint32_t modeCount = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(m_adapter->handle(), m_surface, &modeCount, nullptr);

    if (modeCount == 0) {
        return {};
    }

    std::vector<VkPresentModeKHR> presentModes(modeCount);
    vkGetPhysicalDeviceSurfacePresentModesKHR(m_adapter->handle(), m_surface, &modeCount, presentModes.data());
    return presentModes;
}

} // namespace gfx::backend::vulkan::core