#include "GfxWebGPUConverter.h"
#include "../entity/CreateInfo.h"

namespace gfx::convertor {

// ============================================================================
// Internal Type Conversions
// ============================================================================

gfx::webgpu::InstanceCreateInfo gfxDescriptorToWebGPUInstanceCreateInfo(const GfxInstanceDescriptor* descriptor)
{
    gfx::webgpu::InstanceCreateInfo createInfo{};
    createInfo.enableValidation = descriptor ? descriptor->enableValidation : false;
    return createInfo;
}

gfx::webgpu::PlatformWindowHandle gfxWindowHandleToWebGPUPlatformWindowHandle(const GfxPlatformWindowHandle& gfxHandle)
{
    gfx::webgpu::PlatformWindowHandle handle{};
    
    switch (gfxHandle.windowingSystem) {
    case GFX_WINDOWING_SYSTEM_XCB:
        handle.platform = gfx::webgpu::PlatformWindowHandle::Platform::XCB;
        handle.handle.xcb.connection = gfxHandle.xcb.connection;
        handle.handle.xcb.window = gfxHandle.xcb.window;
        break;
    case GFX_WINDOWING_SYSTEM_X11:
        handle.platform = gfx::webgpu::PlatformWindowHandle::Platform::Xlib;
        handle.handle.xlib.display = gfxHandle.x11.display;
        handle.handle.xlib.window = reinterpret_cast<unsigned long>(gfxHandle.x11.window);
        break;
    case GFX_WINDOWING_SYSTEM_WAYLAND:
        handle.platform = gfx::webgpu::PlatformWindowHandle::Platform::Wayland;
        handle.handle.wayland.display = gfxHandle.wayland.display;
        handle.handle.wayland.surface = gfxHandle.wayland.surface;
        break;
    case GFX_WINDOWING_SYSTEM_WIN32:
        handle.platform = gfx::webgpu::PlatformWindowHandle::Platform::Win32;
        handle.handle.win32.hinstance = gfxHandle.win32.hinstance;
        handle.handle.win32.hwnd = gfxHandle.win32.hwnd;
        break;
    case GFX_WINDOWING_SYSTEM_COCOA:
        handle.platform = gfx::webgpu::PlatformWindowHandle::Platform::MacOS;
        handle.handle.macos.layer = gfxHandle.cocoa.nsWindow;
        break;
    case GFX_WINDOWING_SYSTEM_EMSCRIPTEN:
        handle.platform = gfx::webgpu::PlatformWindowHandle::Platform::Unknown;
        break;
    default:
        handle.platform = gfx::webgpu::PlatformWindowHandle::Platform::Unknown;
        break;
    }
    
    return handle;
}

gfx::webgpu::SemaphoreType gfxSemaphoreTypeToWebGPUSemaphoreType(GfxSemaphoreType gfxType)
{
    switch (gfxType) {
    case GFX_SEMAPHORE_TYPE_BINARY:
        return gfx::webgpu::SemaphoreType::Binary;
    case GFX_SEMAPHORE_TYPE_TIMELINE:
        return gfx::webgpu::SemaphoreType::Timeline;
    default:
        return gfx::webgpu::SemaphoreType::Binary;
    }
}

// ============================================================================
// Reverse Conversions - Internal to Gfx API types
// ============================================================================

GfxBufferUsage webgpuBufferUsageToGfxBufferUsage(WGPUBufferUsage usage)
{
    return static_cast<GfxBufferUsage>(usage);
}

GfxSemaphoreType webgpuSemaphoreTypeToGfxSemaphoreType(gfx::webgpu::SemaphoreType type)
{
    switch (type) {
    case gfx::webgpu::SemaphoreType::Binary:
        return GFX_SEMAPHORE_TYPE_BINARY;
    case gfx::webgpu::SemaphoreType::Timeline:
        return GFX_SEMAPHORE_TYPE_TIMELINE;
    default:
        return GFX_SEMAPHORE_TYPE_BINARY;
    }
}

} // namespace gfx::convertor
