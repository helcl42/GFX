#pragma once

#include <gfx_cpp/gfx.hpp>

#include <gfx/gfx.h>

namespace gfx {

// Backend conversions
GfxBackend cppBackendToCBackend(Backend backend);
Backend cBackendToCppBackend(GfxBackend backend);

// Texture format conversions
GfxTextureFormat cppFormatToCFormat(TextureFormat format);
TextureFormat cFormatToCppFormat(GfxTextureFormat format);

// Texture layout conversions
GfxTextureLayout cppLayoutToCLayout(TextureLayout layout);
TextureLayout cLayoutToCppLayout(GfxTextureLayout layout);

// Present mode conversions
GfxPresentMode cppPresentModeToCPresentMode(PresentMode mode);
PresentMode cPresentModeToCppPresentMode(GfxPresentMode mode);

// Sample count conversions
GfxSampleCount cppSampleCountToCCount(SampleCount sampleCount);
SampleCount cSampleCountToCppCount(GfxSampleCount sampleCount);

// Buffer usage conversions
GfxBufferUsage cppBufferUsageToCUsage(BufferUsage usage);
BufferUsage cBufferUsageToCppUsage(GfxBufferUsage usage);

// Texture usage conversions
GfxTextureUsage cppTextureUsageToCUsage(TextureUsage usage);
TextureUsage cTextureUsageToCppUsage(GfxTextureUsage usage);

// Filter mode conversions
GfxFilterMode cppFilterModeToCFilterMode(FilterMode mode);

// Pipeline stage conversions
GfxPipelineStage cppPipelineStageToCPipelineStage(PipelineStage stage);

// Access flags conversions
GfxAccessFlags cppAccessFlagsToCAccessFlags(AccessFlags flags);
AccessFlags cAccessFlagsToCppAccessFlags(GfxAccessFlags flags);

// Address mode conversions
GfxAddressMode cppAddressModeToCAddressMode(AddressMode mode);

// Shader source type conversions
GfxShaderSourceType cppShaderSourceTypeToCShaderSourceType(ShaderSourceType type);

// Semaphore type conversions
SemaphoreType cSemaphoreTypeToCppSemaphoreType(GfxSemaphoreType type);
GfxSemaphoreType cppSemaphoreTypeToCSemaphoreType(SemaphoreType type);

// Blend operation conversions
GfxBlendOperation cppBlendOperationToCBlendOperation(BlendOperation op);

// Blend factor conversions
GfxBlendFactor cppBlendFactorToCBlendFactor(BlendFactor factor);

// Primitive topology conversions
GfxPrimitiveTopology cppPrimitiveTopologyToCPrimitiveTopology(PrimitiveTopology topology);

// Front face conversions
GfxFrontFace cppFrontFaceToCFrontFace(FrontFace frontFace);

// Cull mode conversions
GfxCullMode cppCullModeToCCullMode(CullMode cullMode);

// Polygon mode conversions
GfxPolygonMode cppPolygonModeToCPolygonMode(PolygonMode polygonMode);

// Compare function conversions
GfxCompareFunction cppCompareFunctionToCCompareFunction(CompareFunction func);

// Stencil operation conversions
GfxStencilOperation cppStencilOperationToCStencilOperation(StencilOperation op);

// Load/Store op conversions
GfxLoadOp cppLoadOpToCLoadOp(LoadOp op);
GfxStoreOp cppStoreOpToCStoreOp(StoreOp op);

// Feature type conversions
GfxDeviceFeatureType cppDeviceFeatureTypeToCDeviceFeatureType(DeviceFeatureType feature);
GfxInstanceFeatureType cppInstanceFeatureTypeToCInstanceFeatureType(InstanceFeatureType feature);

// Adapter preference conversions
GfxAdapterPreference cppAdapterPreferenceToCAdapterPreference(AdapterPreference preference);

// Shader stage conversions
GfxShaderStage cppShaderStageToCShaderStage(ShaderStage stage);

// Texture type conversions
GfxTextureType cppTextureTypeToCType(TextureType type);
TextureType cTextureTypeToCppType(GfxTextureType type);

// Texture view type conversions
GfxTextureViewType cppTextureViewTypeToCType(TextureViewType type);

// Windowing system conversions
GfxWindowingSystem cppWindowingSystemToC(WindowingSystem sys);

// Result conversions
Result cResultToCppResult(GfxResult result);

// Log level conversions
LogLevel cLogLevelToCppLogLevel(GfxLogLevel level);

// Platform window handle conversion
GfxPlatformWindowHandle cppHandleToCHandle(const PlatformWindowHandle& windowHandle);

} // namespace gfx
