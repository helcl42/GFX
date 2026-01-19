#include "Conversions.h"

namespace gfx {

GfxBackend cppBackendToCBackend(Backend backend)
{
    return static_cast<GfxBackend>(backend);
}

Backend cBackendToCppBackend(GfxBackend backend)
{
    return static_cast<Backend>(backend);
}

GfxTextureFormat cppFormatToCFormat(TextureFormat format)
{
    return static_cast<GfxTextureFormat>(format);
}

TextureFormat cFormatToCppFormat(GfxTextureFormat format)
{
    return static_cast<TextureFormat>(format);
}

GfxTextureLayout cppLayoutToCLayout(TextureLayout layout)
{
    return static_cast<GfxTextureLayout>(layout);
}

TextureLayout cLayoutToCppLayout(GfxTextureLayout layout)
{
    return static_cast<TextureLayout>(layout);
}

GfxPresentMode cppPresentModeToCPresentMode(PresentMode mode)
{
    return static_cast<GfxPresentMode>(mode);
}

PresentMode cPresentModeToCppPresentMode(GfxPresentMode mode)
{
    return static_cast<PresentMode>(mode);
}

GfxSampleCount cppSampleCountToCCount(SampleCount sampleCount)
{
    return static_cast<GfxSampleCount>(sampleCount);
}

SampleCount cSampleCountToCppCount(GfxSampleCount sampleCount)
{
    return static_cast<SampleCount>(sampleCount);
}

GfxBufferUsage cppBufferUsageToCUsage(BufferUsage usage)
{
    return static_cast<GfxBufferUsage>(static_cast<uint32_t>(usage));
}

BufferUsage cBufferUsageToCppUsage(GfxBufferUsage usage)
{
    return static_cast<BufferUsage>(usage);
}

GfxTextureUsage cppTextureUsageToCUsage(TextureUsage usage)
{
    return static_cast<GfxTextureUsage>(static_cast<uint32_t>(usage));
}

TextureUsage cTextureUsageToCppUsage(GfxTextureUsage usage)
{
    return static_cast<TextureUsage>(usage);
}

GfxFilterMode cppFilterModeToCFilterMode(FilterMode mode)
{
    return static_cast<GfxFilterMode>(mode);
}

GfxPipelineStage cppPipelineStageToCPipelineStage(PipelineStage stage)
{
    return static_cast<GfxPipelineStage>(stage);
}

GfxAccessFlags cppAccessFlagsToCAccessFlags(AccessFlags flags)
{
    return static_cast<GfxAccessFlags>(flags);
}

AccessFlags cAccessFlagsToCppAccessFlags(GfxAccessFlags flags)
{
    return static_cast<AccessFlags>(flags);
}

GfxAddressMode cppAddressModeToCAddressMode(AddressMode mode)
{
    return static_cast<GfxAddressMode>(mode);
}

GfxShaderSourceType cppShaderSourceTypeToCShaderSourceType(ShaderSourceType type)
{
    return static_cast<GfxShaderSourceType>(type);
}

SemaphoreType cSemaphoreTypeToCppSemaphoreType(GfxSemaphoreType type)
{
    return static_cast<SemaphoreType>(type);
}

GfxSemaphoreType cppSemaphoreTypeToCSemaphoreType(SemaphoreType type)
{
    return static_cast<GfxSemaphoreType>(type);
}

GfxBlendOperation cppBlendOperationToCBlendOperation(BlendOperation op)
{
    return static_cast<GfxBlendOperation>(op);
}

GfxBlendFactor cppBlendFactorToCBlendFactor(BlendFactor factor)
{
    return static_cast<GfxBlendFactor>(factor);
}

GfxPrimitiveTopology cppPrimitiveTopologyToCPrimitiveTopology(PrimitiveTopology topology)
{
    return static_cast<GfxPrimitiveTopology>(topology);
}

GfxFrontFace cppFrontFaceToCFrontFace(FrontFace frontFace)
{
    return static_cast<GfxFrontFace>(frontFace);
}

GfxCullMode cppCullModeToCCullMode(CullMode cullMode)
{
    return static_cast<GfxCullMode>(cullMode);
}

GfxPolygonMode cppPolygonModeToCPolygonMode(PolygonMode polygonMode)
{
    return static_cast<GfxPolygonMode>(polygonMode);
}

GfxCompareFunction cppCompareFunctionToCCompareFunction(CompareFunction func)
{
    return static_cast<GfxCompareFunction>(func);
}

GfxStencilOperation cppStencilOperationToCStencilOperation(StencilOperation op)
{
    return static_cast<GfxStencilOperation>(op);
}

GfxLoadOp cppLoadOpToCLoadOp(LoadOp op)
{
    return static_cast<GfxLoadOp>(op);
}

GfxStoreOp cppStoreOpToCStoreOp(StoreOp op)
{
    return static_cast<GfxStoreOp>(op);
}

GfxDeviceFeatureType cppDeviceFeatureTypeToCDeviceFeatureType(DeviceFeatureType feature)
{
    return static_cast<GfxDeviceFeatureType>(feature);
}

GfxInstanceFeatureType cppInstanceFeatureTypeToCInstanceFeatureType(InstanceFeatureType feature)
{
    return static_cast<GfxInstanceFeatureType>(feature);
}

GfxAdapterPreference cppAdapterPreferenceToCAdapterPreference(AdapterPreference preference)
{
    return static_cast<GfxAdapterPreference>(preference);
}

GfxShaderStage cppShaderStageToCShaderStage(ShaderStage stage)
{
    return static_cast<GfxShaderStage>(static_cast<uint32_t>(stage));
}

GfxTextureType cppTextureTypeToCType(TextureType type)
{
    return static_cast<GfxTextureType>(type);
}

TextureType cTextureTypeToCppType(GfxTextureType type)
{
    return static_cast<TextureType>(type);
}

GfxTextureViewType cppTextureViewTypeToCType(TextureViewType type)
{
    return static_cast<GfxTextureViewType>(type);
}

GfxWindowingSystem cppWindowingSystemToC(WindowingSystem sys)
{
    return static_cast<GfxWindowingSystem>(sys);
}

Result cResultToCppResult(GfxResult result)
{
    return static_cast<Result>(result);
}

LogLevel cLogLevelToCppLogLevel(GfxLogLevel level)
{
    return static_cast<LogLevel>(level);
}

GfxPlatformWindowHandle cppHandleToCHandle(const PlatformWindowHandle& windowHandle)
{
    GfxPlatformWindowHandle cHandle = {};
    cHandle.windowingSystem = cppWindowingSystemToC(windowHandle.windowingSystem);

    switch (windowHandle.windowingSystem) {
    case WindowingSystem::Win32:
        cHandle.win32.hwnd = windowHandle.handle.win32.hwnd;
        cHandle.win32.hinstance = windowHandle.handle.win32.hinstance;
        break;
    case WindowingSystem::Xlib:
        cHandle.xlib.window = windowHandle.handle.xlib.window;
        cHandle.xlib.display = windowHandle.handle.xlib.display;
        break;
    case WindowingSystem::Wayland:
        cHandle.wayland.surface = windowHandle.handle.wayland.surface;
        cHandle.wayland.display = windowHandle.handle.wayland.display;
        break;
    case WindowingSystem::XCB:
        cHandle.xcb.connection = windowHandle.handle.xcb.connection;
        cHandle.xcb.window = windowHandle.handle.xcb.window;
        break;
    case WindowingSystem::Metal:
        cHandle.metal.layer = windowHandle.handle.metal.layer;
        break;
    case WindowingSystem::Emscripten:
        cHandle.emscripten.canvasSelector = windowHandle.handle.emscripten.canvasSelector;
        break;
    case WindowingSystem::Android:
        cHandle.android.window = windowHandle.handle.android.window;
        break;
    case WindowingSystem::Unknown:
    default:
        // Unknown platform - leave handles null
        break;
    }

    return cHandle;
}

} // namespace gfx
