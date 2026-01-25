#ifndef GFX_CPP_CONVERSIONS_H
#define GFX_CPP_CONVERSIONS_H

#include <gfx_cpp/gfx.hpp>

#include <gfx/gfx.h>

namespace gfx {

// Backend conversions
GfxBackend cppBackendToCBackend(Backend backend);
Backend cBackendToCppBackend(GfxBackend backend);

// Instance type conversions
GfxInstanceDescriptor cppInstanceDescriptorToCDescriptor(const InstanceDescriptor& descriptor, GfxBackend backend, std::vector<const char*>& extensionsStorage);

// Adapter type conversions
AdapterType cAdapterTypeToCppAdapterType(GfxAdapterType adapterType);

// Adapter info conversions
AdapterInfo cAdapterInfoToCppAdapterInfo(const GfxAdapterInfo& cInfo);

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
GfxBufferUsageFlags cppBufferUsageToCUsage(BufferUsage usage);
BufferUsage cBufferUsageToCppUsage(GfxBufferUsageFlags usage);

// Texture usage conversions
GfxTextureUsageFlags cppTextureUsageToCUsage(TextureUsage usage);
TextureUsage cTextureUsageToCppUsage(GfxTextureUsageFlags usage);

// Filter mode conversions
GfxFilterMode cppFilterModeToCFilterMode(FilterMode mode);

// Pipeline stage conversions
GfxPipelineStageFlags cppPipelineStageToCPipelineStage(PipelineStage stage);

// Access flags conversions
GfxAccessFlags cppAccessFlagsToCAccessFlags(AccessFlags flags);
AccessFlags cAccessFlagsToCppAccessFlags(GfxAccessFlags flags);

// Device limits conversions
DeviceLimits cDeviceLimitsToCppDeviceLimits(const GfxDeviceLimits& limits);

// Queue family conversions
QueueFamilyProperties cQueueFamilyPropertiesToCppQueueFamilyProperties(const GfxQueueFamilyProperties& props);
GfxQueueRequest cppQueueRequestToCQueueRequest(const QueueRequest& req);

// Device descriptor conversion
void convertDeviceDescriptor(const DeviceDescriptor& descriptor, std::vector<const char*>& outExtensions, std::vector<GfxQueueRequest>& outQueueRequests, GfxDeviceDescriptor& outDesc);

// Buffer info conversions
BufferInfo cBufferInfoToCppBufferInfo(const GfxBufferInfo& info);

// Texture info conversions
TextureInfo cTextureInfoToCppTextureInfo(const GfxTextureInfo& info);

// Swapchain info conversions
SwapchainInfo cSwapchainInfoToCppSwapchainInfo(const GfxSwapchainInfo& info);

// Address mode conversions
GfxAddressMode cppAddressModeToCAddressMode(AddressMode mode);

// Index format conversions
GfxIndexFormat cppIndexFormatToCIndexFormat(IndexFormat format);

// Adapter descriptor conversion
void convertAdapterDescriptor(const AdapterDescriptor& input, GfxAdapterDescriptor& output);

// Submit descriptor conversion
void convertSubmitDescriptor(const SubmitDescriptor& input, GfxSubmitDescriptor& output, std::vector<GfxCommandEncoder>& encoders, std::vector<GfxSemaphore>& waitSems, std::vector<GfxSemaphore>& signalSems);

// Barrier conversions
void convertMemoryBarrier(const MemoryBarrier& input, GfxMemoryBarrier& output);
void convertBufferBarrier(const BufferBarrier& input, GfxBufferBarrier& output);
void convertTextureBarrier(const TextureBarrier& input, GfxTextureBarrier& output);

// Copy/Blit descriptor conversions
void convertCopyBufferToBufferDescriptor(const CopyBufferToBufferDescriptor& input, GfxCopyBufferToBufferDescriptor& output);
void convertCopyBufferToTextureDescriptor(const CopyBufferToTextureDescriptor& input, GfxCopyBufferToTextureDescriptor& output);
void convertCopyTextureToBufferDescriptor(const CopyTextureToBufferDescriptor& input, GfxCopyTextureToBufferDescriptor& output);
void convertCopyTextureToTextureDescriptor(const CopyTextureToTextureDescriptor& input, GfxCopyTextureToTextureDescriptor& output);
void convertBlitTextureToTextureDescriptor(const BlitTextureToTextureDescriptor& input, GfxBlitTextureToTextureDescriptor& output);

// Pipeline barrier descriptor conversion
void convertPipelineBarrierDescriptor(const PipelineBarrierDescriptor& input, GfxPipelineBarrierDescriptor& output, std::vector<GfxMemoryBarrier>& memBarriers, std::vector<GfxBufferBarrier>& bufBarriers, std::vector<GfxTextureBarrier>& texBarriers);

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

// Adapter preference conversions
GfxAdapterPreference cppAdapterPreferenceToCAdapterPreference(AdapterPreference preference);

// Shader stage conversions
GfxShaderStageFlags cppShaderStageToCShaderStage(ShaderStage stage);

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

// Descriptor conversions - output parameter pattern
void convertSurfaceDescriptor(const SurfaceDescriptor& descriptor, GfxSurfaceDescriptor& outDesc);
void convertSwapchainDescriptor(const SwapchainDescriptor& descriptor, GfxSwapchainDescriptor& outDesc);
void convertBufferDescriptor(const BufferDescriptor& descriptor, GfxBufferDescriptor& outDesc);
void convertBufferImportDescriptor(const BufferImportDescriptor& descriptor, GfxBufferImportDescriptor& outDesc);
void convertTextureDescriptor(const TextureDescriptor& descriptor, GfxTextureDescriptor& outDesc);
void convertTextureImportDescriptor(const TextureImportDescriptor& descriptor, GfxTextureImportDescriptor& outDesc);
void convertTextureViewDescriptor(const TextureViewDescriptor& descriptor, GfxTextureViewDescriptor& outDesc);
void convertSamplerDescriptor(const SamplerDescriptor& descriptor, GfxSamplerDescriptor& outDesc);
void convertShaderDescriptor(const ShaderDescriptor& descriptor, GfxShaderDescriptor& outDesc);
void convertCommandEncoderDescriptor(const CommandEncoderDescriptor& descriptor, GfxCommandEncoderDescriptor& outDesc);
void convertFenceDescriptor(const FenceDescriptor& descriptor, GfxFenceDescriptor& outDesc);
void convertSemaphoreDescriptor(const SemaphoreDescriptor& descriptor, GfxSemaphoreDescriptor& outDesc);
void convertBindGroupLayoutDescriptor(const BindGroupLayoutDescriptor& descriptor, std::vector<GfxBindGroupLayoutEntry>& outEntries, GfxBindGroupLayoutDescriptor& outDesc);
void convertBindGroupDescriptor(const BindGroupDescriptor& descriptor, std::vector<GfxBindGroupEntry>& outEntries, GfxBindGroupDescriptor& outDesc);
void convertRenderPassDescriptor(const RenderPassCreateDescriptor& descriptor, std::vector<GfxRenderPassColorAttachment>& outColorAttachments, std::vector<GfxRenderPassColorAttachmentTarget>& outColorTargets, std::vector<GfxRenderPassColorAttachmentTarget>& outColorResolveTargets, GfxRenderPassDepthStencilAttachment& outDepthStencilAttachment, GfxRenderPassDepthStencilAttachmentTarget& outDepthTarget, GfxRenderPassDepthStencilAttachmentTarget& outDepthResolveTarget, GfxRenderPassDescriptor& outDesc);
void convertRenderPassBeginDescriptor(const RenderPassBeginDescriptor& descriptor, GfxRenderPass renderPassHandle, GfxFramebuffer framebufferHandle, std::vector<GfxColor>& outClearValues, GfxRenderPassBeginDescriptor& outDesc);
void convertComputePassBeginDescriptor(const ComputePassBeginDescriptor& descriptor, GfxComputePassBeginDescriptor& outDesc);
void convertPresentInfo(const PresentInfo& info, std::vector<GfxSemaphore>& outWaitSemaphores, GfxPresentInfo& outInfo);
void convertFramebufferDescriptor(const FramebufferDescriptor& descriptor, GfxRenderPass renderPassHandle, std::vector<GfxFramebufferAttachment>& outColorAttachments, GfxFramebufferAttachment& outDepthStencilAttachment, GfxFramebufferDescriptor& outDesc);

// RenderPipeline state conversions
void convertVertexState(const VertexState& input, GfxShader vertexShaderHandle, std::vector<std::vector<GfxVertexAttribute>>& outAttributesPerBuffer, std::vector<GfxVertexBufferLayout>& outVertexBuffers, GfxVertexState& out);
void convertFragmentState(const FragmentState& input, GfxShader fragmentShaderHandle, std::vector<GfxColorTargetState>& outColorTargets, std::vector<GfxBlendState>& outBlendStates, GfxFragmentState& out);
void convertPrimitiveState(const PrimitiveState& input, GfxPrimitiveState& out);
void convertDepthStencilState(const DepthStencilState& input, GfxDepthStencilState& out);
void convertRenderPipelineDescriptor(const RenderPipelineDescriptor& descriptor, GfxRenderPass renderPassHandle, const GfxVertexState& vertexState, const std::optional<GfxFragmentState>& fragmentState, const GfxPrimitiveState& primitiveState, const std::optional<GfxDepthStencilState>& depthStencilState, std::vector<GfxBindGroupLayout>& outBindGroupLayouts, GfxRenderPipelineDescriptor& out);
void convertComputePipelineDescriptor(const ComputePipelineDescriptor& descriptor, GfxShader computeShaderHandle, std::vector<GfxBindGroupLayout>& outBindGroupLayouts, GfxComputePipelineDescriptor& out);

} // namespace gfx

#endif // GFX_CPP_CONVERSIONS_H
