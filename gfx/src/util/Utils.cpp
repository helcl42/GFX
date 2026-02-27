#include "util/Utils.h"

#ifdef GFX_HAS_COCOA
#include <objc/message.h>
#include <objc/runtime.h>
#endif

namespace gfx::util {

uint64_t alignUp(uint64_t value, uint64_t alignment)
{
    if (alignment == 0) {
        return value;
    }
    return (value + alignment - 1) & ~(alignment - 1);
}

uint64_t alignDown(uint64_t value, uint64_t alignment)
{
    if (alignment == 0) {
        return value;
    }
    return value & ~(alignment - 1);
}

uint32_t getFormatBytesPerPixel(GfxFormat format)
{
    switch (format) {
    // 1 byte
    case GFX_FORMAT_R8_UNORM:
    case GFX_FORMAT_STENCIL8:
        return 1;
    // 2 bytes
    case GFX_FORMAT_R8G8_UNORM:
    case GFX_FORMAT_R16_FLOAT:
    case GFX_FORMAT_DEPTH16_UNORM:
        return 2;
    // 4 bytes
    case GFX_FORMAT_R8G8B8A8_UNORM:
    case GFX_FORMAT_R8G8B8A8_UNORM_SRGB:
    case GFX_FORMAT_B8G8R8A8_UNORM:
    case GFX_FORMAT_B8G8R8A8_UNORM_SRGB:
    case GFX_FORMAT_R16G16_FLOAT:
    case GFX_FORMAT_R32_FLOAT:
    case GFX_FORMAT_DEPTH24_PLUS:
    case GFX_FORMAT_DEPTH32_FLOAT:
    case GFX_FORMAT_DEPTH24_PLUS_STENCIL8:
        return 4;
    // 8 bytes
    case GFX_FORMAT_R16G16B16A16_FLOAT:
    case GFX_FORMAT_R32G32_FLOAT:
    case GFX_FORMAT_DEPTH32_FLOAT_STENCIL8:
        return 8;
    // 12 bytes
    case GFX_FORMAT_R32G32B32_FLOAT:
        return 12;
    // 16 bytes
    case GFX_FORMAT_R32G32B32A32_FLOAT:
        return 16;
    case GFX_FORMAT_UNDEFINED:
    default:
        return 0;
    }
}

const char* resultToString(GfxResult result)
{
    switch (result) {
    case GFX_RESULT_SUCCESS:
        return "GFX_RESULT_SUCCESS";
    case GFX_RESULT_TIMEOUT:
        return "GFX_RESULT_TIMEOUT";
    case GFX_RESULT_NOT_READY:
        return "GFX_RESULT_NOT_READY";
    case GFX_RESULT_ERROR_INVALID_ARGUMENT:
        return "GFX_RESULT_ERROR_INVALID_ARGUMENT";
    case GFX_RESULT_ERROR_NOT_FOUND:
        return "GFX_RESULT_ERROR_NOT_FOUND";
    case GFX_RESULT_ERROR_OUT_OF_MEMORY:
        return "GFX_RESULT_ERROR_OUT_OF_MEMORY";
    case GFX_RESULT_ERROR_DEVICE_LOST:
        return "GFX_RESULT_ERROR_DEVICE_LOST";
    case GFX_RESULT_ERROR_SURFACE_LOST:
        return "GFX_RESULT_ERROR_SURFACE_LOST";
    case GFX_RESULT_ERROR_OUT_OF_DATE:
        return "GFX_RESULT_ERROR_OUT_OF_DATE";
    case GFX_RESULT_ERROR_BACKEND_NOT_LOADED:
        return "GFX_RESULT_ERROR_BACKEND_NOT_LOADED";
    case GFX_RESULT_ERROR_FEATURE_NOT_SUPPORTED:
        return "GFX_RESULT_ERROR_FEATURE_NOT_SUPPORTED";
    case GFX_RESULT_ERROR_UNKNOWN:
        return "GFX_RESULT_ERROR_UNKNOWN";
    default:
        return "GFX_RESULT_UNKNOWN";
    }
}

void* getMetalLayerFromCocoaWindow(void* cocoaWindow)
{
#if defined(GFX_HAS_COCOA)
    // Cast the input parameter directly
    id nsWindow = (id)cocoaWindow;

    if (!nsWindow) {
        return nullptr;
    }

    // 1. Get contentView: [nsWindow contentView]
    auto getContentView = (id (*)(id, SEL))objc_msgSend;
    id nsView = getContentView(nsWindow, sel_getUid("contentView"));

    if (!nsView) {
        return nullptr;
    }

    // 2. Ensure it's layer-backed: [nsView setWantsLayer:YES]
    auto setWantsLayer = (void (*)(id, SEL, bool))objc_msgSend;
    setWantsLayer(nsView, sel_getUid("setWantsLayer:"), true);

    // 3. Create a CAMetalLayer explicitly
    // Note: Simply calling [nsView layer] after setWantsLayer:YES creates an NSViewBackingLayer,
    // which is NOT a CAMetalLayer. MoltenVK requires a proper CAMetalLayer.
    Class caMetalLayerClass = objc_getClass("CAMetalLayer");
    if (!caMetalLayerClass) {
        return nullptr;
    }
    
    // Create new CAMetalLayer: [CAMetalLayer layer]
    auto layerMethod = (id (*)(id, SEL))objc_msgSend;
    id metalLayer = layerMethod((id)caMetalLayerClass, sel_getUid("layer"));
    
    if (!metalLayer) {
        return nullptr;
    }
    
    // 4. Set this as the view's layer: [nsView setLayer:metalLayer]
    auto setLayer = (void (*)(id, SEL, id))objc_msgSend;
    setLayer(nsView, sel_getUid("setLayer:"), metalLayer);
    
    // 5. Get the backing scale factor and set contentsScale for retina support
    // [nsWindow backingScaleFactor]
    auto getBackingScaleFactor = (double (*)(id, SEL))objc_msgSend;
    double scaleFactor = getBackingScaleFactor(nsWindow, sel_getUid("backingScaleFactor"));
    
    // [metalLayer setContentsScale:scaleFactor]
    auto setContentsScale = (void (*)(id, SEL, double))objc_msgSend;
    setContentsScale(metalLayer, sel_getUid("setContentsScale:"), scaleFactor);
    
    return (void*)metalLayer;
#else
    (void)cocoaWindow;
    return nullptr;
#endif
}

} // namespace gfx::util
