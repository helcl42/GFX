#include "Surface.h"

#include "../util/Utils.h"

#include <stdexcept>

namespace gfx::backend::webgpu::core {

namespace {
#ifdef GFX_HAS_WIN32
    WGPUSurface createSurfaceWin32(WGPUInstance instance, const PlatformWindowHandle& windowHandle)
    {
        if (!windowHandle.handle.win32.hwnd || !windowHandle.handle.win32.hinstance) {
            throw std::runtime_error("Invalid Win32 window or instance handle");
        }

        WGPUSurfaceSourceWindowsHWND source = WGPU_SURFACE_SOURCE_WINDOWS_HWND_INIT;
        source.hwnd = windowHandle.handle.win32.hwnd;
        source.hinstance = windowHandle.handle.win32.hinstance;

        WGPUSurfaceDescriptor surfaceDesc = WGPU_SURFACE_DESCRIPTOR_INIT;
        surfaceDesc.label = toStringView("Win32 Surface");
        surfaceDesc.nextInChain = (WGPUChainedStruct*)&source;

        return wgpuInstanceCreateSurface(instance, &surfaceDesc);
    }
#endif
#ifdef GFX_HAS_ANDROID
    WGPUSurface createSurfaceAndroid(WGPUInstance instance, const PlatformWindowHandle& windowHandle)
    {
        if (!windowHandle.handle.android.window) {
            throw std::runtime_error("Invalid Android window handle");
        }

        WGPUSurfaceSourceAndroidNativeWindow source = WGPU_SURFACE_SOURCE_ANDROID_NATIVE_WINDOW_INIT;
        source.window = windowHandle.android.window;

        WGPUSurfaceDescriptor surfaceDesc = WGPU_SURFACE_DESCRIPTOR_INIT;
        surfaceDesc.label = toStringView("Android Surface");
        surfaceDesc.nextInChain = (WGPUChainedStruct*)&source;

        return wgpuInstanceCreateSurface(instance, &surfaceDesc);
    }
#endif
#ifdef GFX_HAS_X11
    WGPUSurface createSurfaceXlib(WGPUInstance instance, const PlatformWindowHandle& windowHandle)
    {
        if (!windowHandle.handle.xlib.window || !windowHandle.handle.xlib.display) {
            throw std::runtime_error("Invalid Xlib window or display handle");
        }

        WGPUSurfaceSourceXlibWindow source = WGPU_SURFACE_SOURCE_XLIB_WINDOW_INIT;
        source.display = windowHandle.handle.xlib.display;
        source.window = windowHandle.handle.xlib.window;

        WGPUSurfaceDescriptor surfaceDesc = WGPU_SURFACE_DESCRIPTOR_INIT;
        surfaceDesc.label = toStringView("X11 Surface");
        surfaceDesc.nextInChain = (WGPUChainedStruct*)&source;

        return wgpuInstanceCreateSurface(instance, &surfaceDesc);
    }
#endif
#ifdef GFX_HAS_XCB
    WGPUSurface createSurfaceXCB(WGPUInstance instance, const PlatformWindowHandle& windowHandle)
    {
        if (!windowHandle.handle.xcb.window || !windowHandle.handle.xcb.connection) {
            throw std::runtime_error("Invalid XCB window or connection handle");
        }

        WGPUSurfaceSourceXCBWindow source = WGPU_SURFACE_SOURCE_XCB_WINDOW_INIT;
        source.connection = windowHandle.handle.xcb.connection;
        source.window = windowHandle.handle.xcb.window;

        WGPUSurfaceDescriptor surfaceDesc = WGPU_SURFACE_DESCRIPTOR_INIT;
        surfaceDesc.label = toStringView("XCB Surface");
        surfaceDesc.nextInChain = (WGPUChainedStruct*)&source;

        return wgpuInstanceCreateSurface(instance, &surfaceDesc);
    }
#endif
#ifdef GFX_HAS_WAYLAND
    WGPUSurface createSurfaceWayland(WGPUInstance instance, const PlatformWindowHandle& windowHandle)
    {
        if (!windowHandle.handle.wayland.surface || !windowHandle.handle.wayland.display) {
            throw std::runtime_error("Invalid Wayland surface or display handle");
        }

        WGPUSurfaceSourceWaylandSurface source = WGPU_SURFACE_SOURCE_WAYLAND_SURFACE_INIT;
        source.display = windowHandle.handle.wayland.display;
        source.surface = windowHandle.handle.wayland.surface;

        WGPUSurfaceDescriptor surfaceDesc = WGPU_SURFACE_DESCRIPTOR_INIT;
        surfaceDesc.label = toStringView("Wayland Surface");
        surfaceDesc.nextInChain = (WGPUChainedStruct*)&source;

        return wgpuInstanceCreateSurface(instance, &surfaceDesc);
    }
#endif
#if defined(GFX_HAS_COCOA) || defined(GFX_HAS_UIKIT)
    WGPUSurface createSurfaceMetal(WGPUInstance instance, const PlatformWindowHandle& windowHandle)
    {
        if (!windowHandle.handle.metalLayer) {
            throw std::runtime_error("Invalid Metal layer handle");
        }

        WGPUSurfaceSourceMetalLayer source = WGPU_SURFACE_SOURCE_METAL_LAYER_INIT;
        source.layer = windowHandle.handle.metalLayer;

        WGPUSurfaceDescriptor surfaceDesc = WGPU_SURFACE_DESCRIPTOR_INIT;
        surfaceDesc.label = toStringView("Metal Surface");
        surfaceDesc.nextInChain = (WGPUChainedStruct*)&source;

        return wgpuInstanceCreateSurface(instance, &surfaceDesc);
    }
#endif
#ifdef GFX_HAS_EMSCRIPTEN
    WGPUSurface createSurfaceEmscripten(WGPUInstance instance, const PlatformWindowHandle& windowHandle)
    {
        if (!windowHandle.handle.emscripten.canvasSelector) {
            throw std::runtime_error("Invalid Emscripten canvas selector");
        }

        WGPUEmscriptenSurfaceSourceCanvasHTMLSelector canvasDesc = WGPU_EMSCRIPTEN_SURFACE_SOURCE_CANVAS_HTML_SELECTOR_INIT;
        canvasDesc.selector = toStringView(windowHandle.handle.emscripten.canvasSelector);

        WGPUSurfaceDescriptor surfaceDesc = WGPU_SURFACE_DESCRIPTOR_INIT;
        surfaceDesc.nextInChain = (WGPUChainedStruct*)&canvasDesc;

        return wgpuInstanceCreateSurface(instance, &surfaceDesc);
    }
#endif

} // namespace

Surface::Surface(WGPUInstance instance, WGPUAdapter adapter, const SurfaceCreateInfo& createInfo)
    : m_adapter(adapter)
{
    switch (createInfo.windowHandle.platform) {
#ifdef GFX_HAS_WIN32
    case PlatformWindowHandle::Platform::Win32:
        m_surface = createSurfaceWin32(instance, createInfo.windowHandle);
        break;
#endif
#ifdef GFX_HAS_ANDROID
    case PlatformWindowHandle::Platform::Android:
        m_surface = createSurfaceAndroid(instance, createInfo.windowHandle);
        break;
#endif
#ifdef GFX_HAS_X11
    case PlatformWindowHandle::Platform::Xlib:
        m_surface = createSurfaceXlib(instance, createInfo.windowHandle);
        break;
#endif
#ifdef GFX_HAS_XCB
    case PlatformWindowHandle::Platform::Xcb:
        m_surface = createSurfaceXCB(instance, createInfo.windowHandle);
        break;
#endif
#ifdef GFX_HAS_WAYLAND
    case PlatformWindowHandle::Platform::Wayland:
        m_surface = createSurfaceWayland(instance, createInfo.windowHandle);
        break;
#endif
#if defined(GFX_HAS_COCOA) || defined(GFX_HAS_UIKIT)
    case PlatformWindowHandle::Platform::Cocoa:
        m_surface = createSurfaceMetal(instance, createInfo.windowHandle);
        break;
#endif
#ifdef GFX_HAS_EMSCRIPTEN
    case PlatformWindowHandle::Platform::Emscripten:
        m_surface = createSurfaceEmscripten(instance, createInfo.windowHandle);
        break;
#endif
    default:
        throw std::runtime_error("Unsupported windowing system for WebGPU surface creation");
    }
}

Surface::~Surface()
{
    if (m_surface) {
        wgpuSurfaceRelease(m_surface);
    }
}

WGPUAdapter Surface::adapter() const
{
    return m_adapter;
}

WGPUSurface Surface::handle() const
{
    return m_surface;
}

// Query surface capabilities and return them
// Caller is responsible for calling wgpuSurfaceCapabilitiesFreeMembers
WGPUSurfaceCapabilities Surface::getCapabilities() const
{
    WGPUSurfaceCapabilities capabilities = WGPU_SURFACE_CAPABILITIES_INIT;
    wgpuSurfaceGetCapabilities(m_surface, m_adapter, &capabilities);
    return capabilities;
}

} // namespace gfx::backend::webgpu::core