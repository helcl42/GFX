#include "SurfaceFactory.h"
#include "../converter/GfxWebGPUConverter.h"

#if defined(__EMSCRIPTEN__)
#include <emscripten.h>
// Platform-specific includes for surface creation
#elif defined(_WIN32)
#include <windows.h>
#elif defined(__linux__)
#include <X11/Xlib.h>
// For Wayland support
struct wl_display;
struct wl_surface;
#elif defined(__APPLE__)
#include <objc/objc.h>
#include <objc/runtime.h>
#endif

#include <stdexcept>

namespace gfx::webgpu::surface {
namespace {

#ifdef _WIN32
    static WGPUSurface createSurfaceWin32(WGPUInstance instance, const GfxPlatformWindowHandle& handle)
    {
        if (!handle.hwnd || !handle.hinstance) {
            return nullptr;
        }

        WGPUSurfaceSourceWindowsHWND source = WGPU_SURFACE_SOURCE_WINDOWS_HWND_INIT;
        source.hwnd = handle.hwnd;
        source.hinstance = handle.hinstance;

        WGPUSurfaceDescriptor surface_desc = WGPU_SURFACE_DESCRIPTOR_INIT;
        surface_desc.label = gfx::convertor::gfxStringView("Win32 Surface");
        surface_desc.nextInChain = (WGPUChainedStruct*)&source;

        return wgpuInstanceCreateSurface(instance, &surface_desc);
    }
#endif

#ifdef __linux__
    static WGPUSurface createSurfaceX11(WGPUInstance instance, const GfxPlatformWindowHandle& handle)
    {
        if (!handle.x11.window || !handle.x11.display) {
            return nullptr;
        }

        WGPUSurfaceSourceXlibWindow source = WGPU_SURFACE_SOURCE_XLIB_WINDOW_INIT;
        source.display = handle.x11.display;
        source.window = (uint64_t)(uintptr_t)handle.x11.window;

        WGPUSurfaceDescriptor surface_desc = WGPU_SURFACE_DESCRIPTOR_INIT;
        surface_desc.label = gfx::convertor::gfxStringView("X11 Surface");
        surface_desc.nextInChain = (WGPUChainedStruct*)&source;

        return wgpuInstanceCreateSurface(instance, &surface_desc);
    }

    static WGPUSurface createSurfaceWayland(WGPUInstance instance, const GfxPlatformWindowHandle& handle)
    {
        if (!handle.wayland.surface || !handle.wayland.display) {
            return nullptr;
        }

        WGPUSurfaceSourceWaylandSurface source = WGPU_SURFACE_SOURCE_WAYLAND_SURFACE_INIT;
        source.display = handle.wayland.display;
        source.surface = handle.wayland.surface;

        WGPUSurfaceDescriptor surface_desc = WGPU_SURFACE_DESCRIPTOR_INIT;
        surface_desc.label = gfx::convertor::gfxStringView("Wayland Surface");
        surface_desc.nextInChain = (WGPUChainedStruct*)&source;

        return wgpuInstanceCreateSurface(instance, &surface_desc);
    }
#endif

#ifdef __APPLE__
    static WGPUSurface createSurfaceMetal(WGPUInstance instance, const GfxPlatformWindowHandle& handle)
    {
        void* metal_layer = handle.metalLayer;

        // If no metal layer provided, try to get it from the NSWindow
        if (!metal_layer && handle.nsWindow) {
            id nsWindow = (id)handle.nsWindow;
            SEL contentViewSel = sel_registerName("contentView");
            id contentView = ((id (*)(id, SEL))objc_msgSend)(nsWindow, contentViewSel);

            if (contentView) {
                SEL layerSel = sel_registerName("layer");
                metal_layer = ((void* (*)(id, SEL))objc_msgSend)(contentView, layerSel);

                if (metal_layer) {
                    Class metalLayerClass = objc_getClass("CAMetalLayer");
                    if (metalLayerClass && !object_isKindOfClass((id)metal_layer, metalLayerClass)) {
                        id newMetalLayer = ((id (*)(Class, SEL))objc_msgSend)(metalLayerClass, sel_registerName("new"));
                        SEL setLayerSel = sel_registerName("setLayer:");
                        ((void (*)(id, SEL, id))objc_msgSend)(contentView, setLayerSel, newMetalLayer);
                        SEL setWantsLayerSel = sel_registerName("setWantsLayer:");
                        ((void (*)(id, SEL, BOOL))objc_msgSend)(contentView, setWantsLayerSel, YES);
                        metal_layer = newMetalLayer;
                    }
                }
            }
        }

        if (!metal_layer) {
            return nullptr;
        }

        WGPUSurfaceSourceMetalLayer source = WGPU_SURFACE_SOURCE_METAL_LAYER_INIT;
        source.layer = metal_layer;

        WGPUSurfaceDescriptor surface_desc = WGPU_SURFACE_DESCRIPTOR_INIT;
        surface_desc.label = gfx::convertor::gfxStringView("Metal Surface");
        surface_desc.nextInChain = (WGPUChainedStruct*)&source;

        return wgpuInstanceCreateSurface(instance, &surface_desc);
    }
#endif

#ifdef __EMSCRIPTEN__
    static WGPUSurface createSurfaceEmscripten(WGPUInstance instance, const GfxPlatformWindowHandle& handle)
    {
        if (!handle.emscripten.canvasSelector) {
            return nullptr;
        }

        WGPUEmscriptenSurfaceSourceCanvasHTMLSelector canvasDesc = WGPU_EMSCRIPTEN_SURFACE_SOURCE_CANVAS_HTML_SELECTOR_INIT;
        canvasDesc.selector = gfx::convertor::gfxStringView(handle.emscripten.canvasSelector);

        WGPUSurfaceDescriptor surfaceDesc = WGPU_SURFACE_DESCRIPTOR_INIT;
        surfaceDesc.nextInChain = (WGPUChainedStruct*)&canvasDesc;

        return wgpuInstanceCreateSurface(instance, &surfaceDesc);
    }
#endif

} // namespace

WGPUSurface SurfaceFactory::createFromNativeWindow(WGPUInstance instance, const GfxPlatformWindowHandle& platformHandle)
{
    if (!instance) {
        return nullptr;
    }

    switch (platformHandle.windowingSystem) {
#ifdef __EMSCRIPTEN__
    case GFX_WINDOWING_SYSTEM_EMSCRIPTEN:
        return createSurfaceEmscripten(instance, platformHandle);
#endif
#ifdef _WIN32
    case GFX_WINDOWING_SYSTEM_WIN32:
        return createSurfaceWin32(instance, platformHandle);
#endif
#ifdef __linux__
    case GFX_WINDOWING_SYSTEM_WAYLAND:
        return createSurfaceWayland(instance, platformHandle);
    case GFX_WINDOWING_SYSTEM_X11:
    case GFX_WINDOWING_SYSTEM_XCB:
        return createSurfaceX11(instance, platformHandle);
#endif
#ifdef __APPLE__
    case GFX_WINDOWING_SYSTEM_COCOA:
        return createSurfaceMetal(instance, platformHandle);
#endif
    default:
        throw std::runtime_error("Unsupported windowing system for WebGPU surface creation");
    }
}
} // namespace gfx::webgpu::surface