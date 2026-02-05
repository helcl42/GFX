#include "Conversions.h"

#include "../core/command/CommandEncoder.h"
#include "../core/resource/BindGroupLayout.h"
#include "../core/resource/Buffer.h"
#include "../core/resource/Sampler.h"
#include "../core/resource/Texture.h"
#include "../core/resource/TextureView.h"
#include "../core/sync/Fence.h"
#include "../core/sync/Semaphore.h"
#include "../core/util/HandleExtractor.h"

#include <stdexcept>

namespace gfx {

GfxBackend cppBackendToCBackend(Backend backend)
{
    return static_cast<GfxBackend>(backend);
}

Backend cBackendToCppBackend(GfxBackend backend)
{
    return static_cast<Backend>(backend);
}

std::vector<std::string> cStringArrayToCppStringVector(const char** strings, uint32_t count)
{
    std::vector<std::string> result;
    result.reserve(count);
    for (uint32_t i = 0; i < count; ++i) {
        if (strings[i]) {
            result.emplace_back(strings[i]);
        }
    }
    return result;
}

GfxInstanceDescriptor cppInstanceDescriptorToCDescriptor(const InstanceDescriptor& descriptor, GfxBackend backend, std::vector<const char*>& extensionsStorage)
{
    // Convert enabled extensions
    extensionsStorage.clear();
    extensionsStorage.reserve(descriptor.enabledExtensions.size());
    for (const auto& ext : descriptor.enabledExtensions) {
        extensionsStorage.push_back(ext.c_str());
    }

    GfxInstanceDescriptor cDesc = {};
    cDesc.sType = GFX_STRUCTURE_TYPE_INSTANCE_DESCRIPTOR;
    cDesc.pNext = NULL;
    cDesc.backend = backend;
    cDesc.applicationName = descriptor.applicationName.c_str();
    cDesc.applicationVersion = descriptor.applicationVersion;
    cDesc.enabledExtensions = extensionsStorage.empty() ? nullptr : extensionsStorage.data();
    cDesc.enabledExtensionCount = static_cast<uint32_t>(extensionsStorage.size());

    return cDesc;
}

AdapterType cAdapterTypeToCppAdapterType(GfxAdapterType adapterType)
{
    switch (adapterType) {
    case GFX_ADAPTER_TYPE_DISCRETE_GPU:
        return AdapterType::DiscreteGPU;
    case GFX_ADAPTER_TYPE_INTEGRATED_GPU:
        return AdapterType::IntegratedGPU;
    case GFX_ADAPTER_TYPE_CPU:
        return AdapterType::CPU;
    default:
        return AdapterType::Unknown;
    }
}

AdapterInfo cAdapterInfoToCppAdapterInfo(const GfxAdapterInfo& cInfo)
{
    AdapterInfo info;
    info.name = cInfo.name ? cInfo.name : "Unknown";
    info.driverDescription = cInfo.driverDescription ? cInfo.driverDescription : "";
    info.vendorID = cInfo.vendorID;
    info.deviceID = cInfo.deviceID;
    info.adapterType = cAdapterTypeToCppAdapterType(cInfo.adapterType);
    info.backend = cBackendToCppBackend(cInfo.backend);
    return info;
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

GfxBufferUsageFlags cppBufferUsageToCUsage(BufferUsage usage)
{
    return static_cast<GfxBufferUsageFlags>(static_cast<uint32_t>(usage));
}

BufferUsage cBufferUsageToCppUsage(GfxBufferUsageFlags usage)
{
    return static_cast<BufferUsage>(usage);
}

GfxMemoryPropertyFlags cppMemoryPropertyToCMemoryProperty(MemoryProperty property)
{
    return static_cast<GfxMemoryPropertyFlags>(static_cast<uint32_t>(property));
}

MemoryProperty cMemoryPropertyToCppMemoryProperty(GfxMemoryPropertyFlags property)
{
    return static_cast<MemoryProperty>(property);
}

GfxTextureUsageFlags cppTextureUsageToCUsage(TextureUsage usage)
{
    return static_cast<GfxTextureUsageFlags>(static_cast<uint32_t>(usage));
}

TextureUsage cTextureUsageToCppUsage(GfxTextureUsageFlags usage)
{
    return static_cast<TextureUsage>(usage);
}

GfxFilterMode cppFilterModeToCFilterMode(FilterMode mode)
{
    return static_cast<GfxFilterMode>(mode);
}

GfxIndexFormat cppIndexFormatToCIndexFormat(IndexFormat format)
{
    return static_cast<GfxIndexFormat>(format);
}

GfxVertexStepMode cppVertexStepModeToCVertexStepMode(VertexStepMode mode)
{
    return static_cast<GfxVertexStepMode>(mode);
}

GfxPipelineStageFlags cppPipelineStageToCPipelineStage(PipelineStage stage)
{
    return static_cast<GfxPipelineStageFlags>(stage);
}

GfxAccessFlags cppAccessFlagsToCAccessFlags(AccessFlags flags)
{
    return static_cast<GfxAccessFlags>(flags);
}

AccessFlags cAccessFlagsToCppAccessFlags(GfxAccessFlags flags)
{
    return static_cast<AccessFlags>(flags);
}

DeviceLimits cDeviceLimitsToCppDeviceLimits(const GfxDeviceLimits& cLimits)
{
    DeviceLimits limits;
    limits.minUniformBufferOffsetAlignment = cLimits.minUniformBufferOffsetAlignment;
    limits.minStorageBufferOffsetAlignment = cLimits.minStorageBufferOffsetAlignment;
    limits.maxUniformBufferBindingSize = cLimits.maxUniformBufferBindingSize;
    limits.maxStorageBufferBindingSize = cLimits.maxStorageBufferBindingSize;
    limits.maxBufferSize = cLimits.maxBufferSize;
    limits.maxTextureDimension1D = cLimits.maxTextureDimension1D;
    limits.maxTextureDimension2D = cLimits.maxTextureDimension2D;
    limits.maxTextureDimension3D = cLimits.maxTextureDimension3D;
    limits.maxTextureArrayLayers = cLimits.maxTextureArrayLayers;
    return limits;
}

QueueFamilyProperties cQueueFamilyPropertiesToCppQueueFamilyProperties(const GfxQueueFamilyProperties& props)
{
    QueueFamilyProperties result{};
    result.flags = static_cast<QueueFlags>(props.flags);
    result.queueCount = props.queueCount;
    return result;
}

GfxQueueRequest cppQueueRequestToCQueueRequest(const QueueRequest& req)
{
    GfxQueueRequest cReq{};
    cReq.queueFamilyIndex = req.queueFamilyIndex;
    cReq.queueIndex = req.queueIndex;
    cReq.priority = req.priority;
    return cReq;
}

void convertDeviceDescriptor(const DeviceDescriptor& descriptor, std::vector<const char*>& outExtensions, std::vector<GfxQueueRequest>& outQueueRequests, GfxDeviceDescriptor& outDesc)
{
    // Convert enabled extensions
    outExtensions.clear();
    outExtensions.reserve(descriptor.enabledExtensions.size());
    for (const auto& ext : descriptor.enabledExtensions) {
        outExtensions.push_back(ext.c_str());
    }

    // Convert queue requests
    outQueueRequests.clear();
    outQueueRequests.reserve(descriptor.queueRequests.size());
    for (const auto& req : descriptor.queueRequests) {
        outQueueRequests.push_back(cppQueueRequestToCQueueRequest(req));
    }

    // Build C descriptor
    outDesc = {};
    outDesc.sType = GFX_STRUCTURE_TYPE_DEVICE_DESCRIPTOR;
    outDesc.pNext = NULL;
    outDesc.label = descriptor.label.c_str();
    outDesc.enabledExtensions = outExtensions.empty() ? nullptr : outExtensions.data();
    outDesc.enabledExtensionCount = static_cast<uint32_t>(outExtensions.size());
    outDesc.queueRequests = outQueueRequests.empty() ? nullptr : outQueueRequests.data();
    outDesc.queueRequestCount = static_cast<uint32_t>(outQueueRequests.size());
}

BufferInfo cBufferInfoToCppBufferInfo(const GfxBufferInfo& cInfo)
{
    BufferInfo info;
    info.size = cInfo.size;
    info.usage = cBufferUsageToCppUsage(cInfo.usage);
    return info;
}

TextureInfo cTextureInfoToCppTextureInfo(const GfxTextureInfo& cInfo)
{
    TextureInfo info;
    info.type = cTextureTypeToCppType(cInfo.type);
    info.size = Extent3D(cInfo.size.width, cInfo.size.height, cInfo.size.depth);
    info.arrayLayerCount = cInfo.arrayLayerCount;
    info.mipLevelCount = cInfo.mipLevelCount;
    info.sampleCount = cSampleCountToCppCount(cInfo.sampleCount);
    info.format = cFormatToCppFormat(cInfo.format);
    info.usage = cTextureUsageToCppUsage(cInfo.usage);
    return info;
}

SwapchainInfo cSwapchainInfoToCppSwapchainInfo(const GfxSwapchainInfo& cInfo)
{
    SwapchainInfo info;
    info.width = cInfo.width;
    info.height = cInfo.height;
    info.format = cFormatToCppFormat(cInfo.format);
    info.presentMode = cPresentModeToCppPresentMode(cInfo.presentMode);
    info.imageCount = cInfo.imageCount;
    return info;
}

GfxAddressMode cppAddressModeToCAddressMode(AddressMode mode)
{
    return static_cast<GfxAddressMode>(mode);
}

void convertAdapterDescriptor(const AdapterDescriptor& input, GfxAdapterDescriptor& output)
{
    output.adapterIndex = UINT32_MAX; // Use preference-based selection
    output.preference = cppAdapterPreferenceToCAdapterPreference(input.preference);
}

void convertSubmitDescriptor(const SubmitDescriptor& input, GfxSubmitDescriptor& output, std::vector<GfxCommandEncoder>& encoders, std::vector<GfxSemaphore>& waitSems, std::vector<GfxSemaphore>& signalSems)
{
    // Convert command encoders
    encoders.clear();
    for (auto& encoder : input.commandEncoders) {
        auto impl = std::dynamic_pointer_cast<CommandEncoderImpl>(encoder);
        if (!impl) {
            throw std::runtime_error("Invalid command encoder type");
        }
        encoders.push_back(impl->getHandle());
    }

    // Convert wait semaphores
    waitSems.clear();
    for (auto& sem : input.waitSemaphores) {
        auto impl = std::dynamic_pointer_cast<SemaphoreImpl>(sem);
        if (!impl) {
            throw std::runtime_error("Invalid wait semaphore type");
        }
        waitSems.push_back(impl->getHandle());
    }

    // Convert signal semaphores
    signalSems.clear();
    for (auto& sem : input.signalSemaphores) {
        auto impl = std::dynamic_pointer_cast<SemaphoreImpl>(sem);
        if (!impl) {
            throw std::runtime_error("Invalid signal semaphore type");
        }
        signalSems.push_back(impl->getHandle());
    }

    // Populate output descriptor
    output = {};
    output.sType = GFX_STRUCTURE_TYPE_SUBMIT_DESCRIPTOR;
    output.pNext = NULL;
    output.commandEncoders = encoders.data();
    output.commandEncoderCount = static_cast<uint32_t>(encoders.size());
    output.waitSemaphores = waitSems.data();
    output.waitSemaphoreCount = static_cast<uint32_t>(waitSems.size());
    output.signalSemaphores = signalSems.data();
    output.signalSemaphoreCount = static_cast<uint32_t>(signalSems.size());

    // Convert fence if present
    if (input.signalFence) {
        auto fenceImpl = std::dynamic_pointer_cast<FenceImpl>(input.signalFence);
        if (!fenceImpl) {
            throw std::runtime_error("Invalid fence type");
        }
        output.signalFence = fenceImpl->getHandle();
    } else {
        output.signalFence = nullptr;
    }

    // Convert timeline semaphore values (empty vectors for binary semaphores)
    output.waitValues = input.waitValues.empty() ? nullptr : const_cast<uint64_t*>(input.waitValues.data());
    output.signalValues = input.signalValues.empty() ? nullptr : const_cast<uint64_t*>(input.signalValues.data());
}

void convertMemoryBarrier(const MemoryBarrier& input, GfxMemoryBarrier& output)
{
    output = {};
    output.srcStageMask = cppPipelineStageToCPipelineStage(input.srcStageMask);
    output.dstStageMask = cppPipelineStageToCPipelineStage(input.dstStageMask);
    output.srcAccessMask = cppAccessFlagsToCAccessFlags(input.srcAccessMask);
    output.dstAccessMask = cppAccessFlagsToCAccessFlags(input.dstAccessMask);
}

void convertBufferBarrier(const BufferBarrier& input, GfxBufferBarrier& output)
{
    auto bufferImpl = std::dynamic_pointer_cast<BufferImpl>(input.buffer);
    if (!bufferImpl) {
        throw std::runtime_error("Invalid buffer type");
    }

    output = {};
    output.buffer = bufferImpl->getHandle();
    output.srcStageMask = cppPipelineStageToCPipelineStage(input.srcStageMask);
    output.dstStageMask = cppPipelineStageToCPipelineStage(input.dstStageMask);
    output.srcAccessMask = cppAccessFlagsToCAccessFlags(input.srcAccessMask);
    output.dstAccessMask = cppAccessFlagsToCAccessFlags(input.dstAccessMask);
    output.offset = input.offset;
    output.size = input.size;
}

void convertTextureBarrier(const TextureBarrier& input, GfxTextureBarrier& output)
{
    auto textureImpl = std::dynamic_pointer_cast<TextureImpl>(input.texture);
    if (!textureImpl) {
        throw std::runtime_error("Invalid texture type");
    }

    output = {};
    output.texture = textureImpl->getHandle();
    output.oldLayout = cppLayoutToCLayout(input.oldLayout);
    output.newLayout = cppLayoutToCLayout(input.newLayout);
    output.srcStageMask = cppPipelineStageToCPipelineStage(input.srcStageMask);
    output.dstStageMask = cppPipelineStageToCPipelineStage(input.dstStageMask);

    // Auto-deduce access masks if not explicitly set
    output.srcAccessMask = (input.srcAccessMask == AccessFlags::None)
        ? gfxGetAccessFlagsForLayout(output.oldLayout)
        : cppAccessFlagsToCAccessFlags(input.srcAccessMask);
    output.dstAccessMask = (input.dstAccessMask == AccessFlags::None)
        ? gfxGetAccessFlagsForLayout(output.newLayout)
        : cppAccessFlagsToCAccessFlags(input.dstAccessMask);

    output.baseMipLevel = input.baseMipLevel;
    output.mipLevelCount = input.mipLevelCount;
    output.baseArrayLayer = input.baseArrayLayer;
    output.arrayLayerCount = input.arrayLayerCount;
}

void convertCopyBufferToBufferDescriptor(const CopyBufferToBufferDescriptor& input, GfxCopyBufferToBufferDescriptor& output)
{
    auto srcImpl = std::dynamic_pointer_cast<BufferImpl>(input.source);
    if (!srcImpl) {
        throw std::runtime_error("Invalid source buffer type");
    }
    auto dstImpl = std::dynamic_pointer_cast<BufferImpl>(input.destination);
    if (!dstImpl) {
        throw std::runtime_error("Invalid destination buffer type");
    }

    output.source = srcImpl->getHandle();
    output.sourceOffset = input.sourceOffset;
    output.destination = dstImpl->getHandle();
    output.destinationOffset = input.destinationOffset;
    output.size = input.size;
}

void convertCopyBufferToTextureDescriptor(const CopyBufferToTextureDescriptor& input, GfxCopyBufferToTextureDescriptor& output)
{
    auto srcImpl = std::dynamic_pointer_cast<BufferImpl>(input.source);
    if (!srcImpl) {
        throw std::runtime_error("Invalid source buffer type");
    }
    auto dstImpl = std::dynamic_pointer_cast<TextureImpl>(input.destination);
    if (!dstImpl) {
        throw std::runtime_error("Invalid destination texture type");
    }

    output.source = srcImpl->getHandle();
    output.sourceOffset = input.sourceOffset;
    output.destination = dstImpl->getHandle();
    output.origin = { input.origin.x, input.origin.y, input.origin.z };
    output.extent = { input.extent.width, input.extent.height, input.extent.depth };
    output.mipLevel = input.mipLevel;
    output.finalLayout = cppLayoutToCLayout(input.finalLayout);
}

void convertCopyTextureToBufferDescriptor(const CopyTextureToBufferDescriptor& input, GfxCopyTextureToBufferDescriptor& output)
{
    auto srcImpl = std::dynamic_pointer_cast<TextureImpl>(input.source);
    if (!srcImpl) {
        throw std::runtime_error("Invalid source texture type");
    }
    auto dstImpl = std::dynamic_pointer_cast<BufferImpl>(input.destination);
    if (!dstImpl) {
        throw std::runtime_error("Invalid destination buffer type");
    }

    output.source = srcImpl->getHandle();
    output.origin = { input.origin.x, input.origin.y, input.origin.z };
    output.mipLevel = input.mipLevel;
    output.destination = dstImpl->getHandle();
    output.destinationOffset = input.destinationOffset;
    output.extent = { input.extent.width, input.extent.height, input.extent.depth };
    output.finalLayout = cppLayoutToCLayout(input.finalLayout);
}

void convertCopyTextureToTextureDescriptor(const CopyTextureToTextureDescriptor& input, GfxCopyTextureToTextureDescriptor& output)
{
    auto srcImpl = std::dynamic_pointer_cast<TextureImpl>(input.source);
    if (!srcImpl) {
        throw std::runtime_error("Invalid source texture type");
    }
    auto dstImpl = std::dynamic_pointer_cast<TextureImpl>(input.destination);
    if (!dstImpl) {
        throw std::runtime_error("Invalid destination texture type");
    }

    output.source = srcImpl->getHandle();
    output.sourceOrigin = { input.sourceOrigin.x, input.sourceOrigin.y, input.sourceOrigin.z };
    output.sourceMipLevel = input.sourceMipLevel;
    output.sourceFinalLayout = cppLayoutToCLayout(input.sourceFinalLayout);
    output.destination = dstImpl->getHandle();
    output.destinationOrigin = { input.destinationOrigin.x, input.destinationOrigin.y, input.destinationOrigin.z };
    output.destinationMipLevel = input.destinationMipLevel;
    output.destinationFinalLayout = cppLayoutToCLayout(input.destinationFinalLayout);
    output.extent = { input.extent.width, input.extent.height, input.extent.depth };
}

void convertBlitTextureToTextureDescriptor(const BlitTextureToTextureDescriptor& input, GfxBlitTextureToTextureDescriptor& output)
{
    auto srcImpl = std::dynamic_pointer_cast<TextureImpl>(input.source);
    if (!srcImpl) {
        throw std::runtime_error("Invalid source texture type");
    }
    auto dstImpl = std::dynamic_pointer_cast<TextureImpl>(input.destination);
    if (!dstImpl) {
        throw std::runtime_error("Invalid destination texture type");
    }

    output.source = srcImpl->getHandle();
    output.sourceOrigin = { input.sourceOrigin.x, input.sourceOrigin.y, input.sourceOrigin.z };
    output.sourceExtent = { input.sourceExtent.width, input.sourceExtent.height, input.sourceExtent.depth };
    output.sourceMipLevel = input.sourceMipLevel;
    output.sourceFinalLayout = cppLayoutToCLayout(input.sourceFinalLayout);
    output.destination = dstImpl->getHandle();
    output.destinationOrigin = { input.destinationOrigin.x, input.destinationOrigin.y, input.destinationOrigin.z };
    output.destinationExtent = { input.destinationExtent.width, input.destinationExtent.height, input.destinationExtent.depth };
    output.destinationMipLevel = input.destinationMipLevel;
    output.destinationFinalLayout = cppLayoutToCLayout(input.destinationFinalLayout);
    output.filter = cppFilterModeToCFilterMode(input.filter);
}

void convertPipelineBarrierDescriptor(const PipelineBarrierDescriptor& input, GfxPipelineBarrierDescriptor& output,
    std::vector<GfxMemoryBarrier>& memBarriers, std::vector<GfxBufferBarrier>& bufBarriers, std::vector<GfxTextureBarrier>& texBarriers)
{
    // Convert memory barriers
    memBarriers.clear();
    memBarriers.reserve(input.memoryBarriers.size());
    for (const auto& barrier : input.memoryBarriers) {
        GfxMemoryBarrier gfxBarrier;
        convertMemoryBarrier(barrier, gfxBarrier);
        memBarriers.push_back(gfxBarrier);
    }

    // Convert buffer barriers
    bufBarriers.clear();
    bufBarriers.reserve(input.bufferBarriers.size());
    for (const auto& barrier : input.bufferBarriers) {
        GfxBufferBarrier gfxBarrier;
        convertBufferBarrier(barrier, gfxBarrier);
        bufBarriers.push_back(gfxBarrier);
    }

    // Convert texture barriers
    texBarriers.clear();
    texBarriers.reserve(input.textureBarriers.size());
    for (const auto& barrier : input.textureBarriers) {
        GfxTextureBarrier gfxBarrier;
        convertTextureBarrier(barrier, gfxBarrier);
        texBarriers.push_back(gfxBarrier);
    }

    // Set pointers in output descriptor
    output.memoryBarriers = memBarriers.empty() ? nullptr : memBarriers.data();
    output.memoryBarrierCount = static_cast<uint32_t>(memBarriers.size());
    output.bufferBarriers = bufBarriers.empty() ? nullptr : bufBarriers.data();
    output.bufferBarrierCount = static_cast<uint32_t>(bufBarriers.size());
    output.textureBarriers = texBarriers.empty() ? nullptr : texBarriers.data();
    output.textureBarrierCount = static_cast<uint32_t>(texBarriers.size());
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

QueryType cQueryTypeToCppQueryType(GfxQueryType type)
{
    return static_cast<QueryType>(type);
}

GfxQueryType cppQueryTypeToCQueryType(QueryType type)
{
    return static_cast<GfxQueryType>(type);
}

GfxBlendOperation cppBlendOperationToCBlendOperation(BlendOperation op)
{
    return static_cast<GfxBlendOperation>(op);
}

GfxBlendFactor cppBlendFactorToCBlendFactor(BlendFactor factor)
{
    return static_cast<GfxBlendFactor>(factor);
}

GfxColorWriteMask cppColorWriteMaskToCColorWriteMask(ColorWriteMask mask)
{
    return static_cast<GfxColorWriteMask>(static_cast<uint32_t>(mask));
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

GfxAdapterPreference cppAdapterPreferenceToCAdapterPreference(AdapterPreference preference)
{
    return static_cast<GfxAdapterPreference>(preference);
}

GfxShaderStageFlags cppShaderStageToCShaderStage(ShaderStage stage)
{
    return static_cast<GfxShaderStageFlags>(static_cast<uint32_t>(stage));
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
    default:
        // Unknown or unsupported platform - leave handles null
        break;
    }

    return cHandle;
}

void convertSurfaceDescriptor(const SurfaceDescriptor& descriptor, GfxSurfaceDescriptor& outDesc)
{
    outDesc = {};
    outDesc.sType = GFX_STRUCTURE_TYPE_SURFACE_DESCRIPTOR;
    outDesc.pNext = NULL;
    outDesc.label = descriptor.label.c_str();
    outDesc.windowHandle = cppHandleToCHandle(descriptor.windowHandle);
}

void convertSwapchainDescriptor(const SwapchainDescriptor& descriptor, GfxSwapchainDescriptor& outDesc, GfxSurface cSurface)
{
    outDesc = {};
    outDesc.sType = GFX_STRUCTURE_TYPE_SWAPCHAIN_DESCRIPTOR;
    outDesc.pNext = NULL;
    outDesc.label = descriptor.label.c_str();
    outDesc.surface = cSurface;
    outDesc.width = descriptor.width;
    outDesc.height = descriptor.height;
    outDesc.format = cppFormatToCFormat(descriptor.format);
    outDesc.usage = cppTextureUsageToCUsage(descriptor.usage);
    outDesc.presentMode = cppPresentModeToCPresentMode(descriptor.presentMode);
    outDesc.imageCount = descriptor.imageCount;
}

void convertBufferDescriptor(const BufferDescriptor& descriptor, GfxBufferDescriptor& outDesc)
{
    outDesc = {};
    outDesc.sType = GFX_STRUCTURE_TYPE_BUFFER_DESCRIPTOR;
    outDesc.pNext = NULL;
    outDesc.label = descriptor.label.c_str();
    outDesc.size = descriptor.size;
    outDesc.usage = cppBufferUsageToCUsage(descriptor.usage);
    outDesc.memoryProperties = cppMemoryPropertyToCMemoryProperty(descriptor.memoryProperties);
}

void convertBufferImportDescriptor(const BufferImportDescriptor& descriptor, GfxBufferImportDescriptor& outDesc)
{
    outDesc = {};
    outDesc.sType = GFX_STRUCTURE_TYPE_BUFFER_IMPORT_DESCRIPTOR;
    outDesc.pNext = NULL;
    outDesc.label = descriptor.label.c_str();
    outDesc.nativeHandle = descriptor.nativeHandle;
    outDesc.size = descriptor.size;
    outDesc.usage = cppBufferUsageToCUsage(descriptor.usage);
}

void convertTextureDescriptor(const TextureDescriptor& descriptor, GfxTextureDescriptor& outDesc)
{
    outDesc = {};
    outDesc.sType = GFX_STRUCTURE_TYPE_TEXTURE_DESCRIPTOR;
    outDesc.pNext = NULL;
    outDesc.label = descriptor.label.c_str();
    outDesc.type = cppTextureTypeToCType(descriptor.type);
    outDesc.size = { descriptor.size.width, descriptor.size.height, descriptor.size.depth };
    outDesc.arrayLayerCount = descriptor.arrayLayerCount;
    outDesc.mipLevelCount = descriptor.mipLevelCount;
    outDesc.sampleCount = cppSampleCountToCCount(descriptor.sampleCount);
    outDesc.format = cppFormatToCFormat(descriptor.format);
    outDesc.usage = cppTextureUsageToCUsage(descriptor.usage);
}

void convertTextureImportDescriptor(const TextureImportDescriptor& descriptor, GfxTextureImportDescriptor& outDesc)
{
    outDesc = {};
    outDesc.sType = GFX_STRUCTURE_TYPE_TEXTURE_IMPORT_DESCRIPTOR;
    outDesc.pNext = NULL;
    outDesc.label = descriptor.label.c_str();
    outDesc.nativeHandle = descriptor.nativeHandle;
    outDesc.type = cppTextureTypeToCType(descriptor.type);
    outDesc.size = { descriptor.size.width, descriptor.size.height, descriptor.size.depth };
    outDesc.arrayLayerCount = descriptor.arrayLayerCount;
    outDesc.mipLevelCount = descriptor.mipLevelCount;
    outDesc.sampleCount = cppSampleCountToCCount(descriptor.sampleCount);
    outDesc.format = cppFormatToCFormat(descriptor.format);
    outDesc.usage = cppTextureUsageToCUsage(descriptor.usage);
    outDesc.currentLayout = cppLayoutToCLayout(descriptor.currentLayout);
}

void convertTextureViewDescriptor(const TextureViewDescriptor& descriptor, GfxTextureViewDescriptor& outDesc)
{
    outDesc = {};
    outDesc.sType = GFX_STRUCTURE_TYPE_TEXTURE_VIEW_DESCRIPTOR;
    outDesc.pNext = NULL;
    outDesc.label = descriptor.label.c_str();
    outDesc.viewType = cppTextureViewTypeToCType(descriptor.viewType);
    outDesc.format = cppFormatToCFormat(descriptor.format);
    outDesc.baseMipLevel = descriptor.baseMipLevel;
    outDesc.mipLevelCount = descriptor.mipLevelCount;
    outDesc.baseArrayLayer = descriptor.baseArrayLayer;
    outDesc.arrayLayerCount = descriptor.arrayLayerCount;
}

void convertSamplerDescriptor(const SamplerDescriptor& descriptor, GfxSamplerDescriptor& outDesc)
{
    outDesc = {};
    outDesc.sType = GFX_STRUCTURE_TYPE_SAMPLER_DESCRIPTOR;
    outDesc.pNext = NULL;
    outDesc.label = descriptor.label.c_str();
    outDesc.addressModeU = cppAddressModeToCAddressMode(descriptor.addressModeU);
    outDesc.addressModeV = cppAddressModeToCAddressMode(descriptor.addressModeV);
    outDesc.addressModeW = cppAddressModeToCAddressMode(descriptor.addressModeW);
    outDesc.magFilter = cppFilterModeToCFilterMode(descriptor.magFilter);
    outDesc.minFilter = cppFilterModeToCFilterMode(descriptor.minFilter);
    outDesc.mipmapFilter = cppFilterModeToCFilterMode(descriptor.mipmapFilter);
    outDesc.lodMinClamp = descriptor.lodMinClamp;
    outDesc.lodMaxClamp = descriptor.lodMaxClamp;
    outDesc.maxAnisotropy = descriptor.maxAnisotropy;
    outDesc.compare = cppCompareFunctionToCCompareFunction(descriptor.compare);
}

void convertShaderDescriptor(const ShaderDescriptor& descriptor, GfxShaderDescriptor& outDesc)
{
    outDesc = {};
    outDesc.sType = GFX_STRUCTURE_TYPE_SHADER_DESCRIPTOR;
    outDesc.pNext = NULL;
    outDesc.label = descriptor.label.c_str();
    outDesc.sourceType = cppShaderSourceTypeToCShaderSourceType(descriptor.sourceType);
    outDesc.code = descriptor.code.data();
    outDesc.codeSize = descriptor.code.size();
    outDesc.entryPoint = descriptor.entryPoint.c_str();
}

void convertCommandEncoderDescriptor(const CommandEncoderDescriptor& descriptor, GfxCommandEncoderDescriptor& outDesc)
{
    outDesc = {};
    outDesc.sType = GFX_STRUCTURE_TYPE_COMMAND_ENCODER_DESCRIPTOR;
    outDesc.pNext = NULL;
    outDesc.label = descriptor.label.c_str();
}

void convertFenceDescriptor(const FenceDescriptor& descriptor, GfxFenceDescriptor& outDesc)
{
    outDesc = {};
    outDesc.sType = GFX_STRUCTURE_TYPE_FENCE_DESCRIPTOR;
    outDesc.pNext = NULL;
    outDesc.label = descriptor.label.c_str();
    outDesc.signaled = descriptor.signaled;
}

void convertSemaphoreDescriptor(const SemaphoreDescriptor& descriptor, GfxSemaphoreDescriptor& outDesc)
{
    outDesc = {};
    outDesc.sType = GFX_STRUCTURE_TYPE_SEMAPHORE_DESCRIPTOR;
    outDesc.pNext = NULL;
    outDesc.label = descriptor.label.c_str();
    outDesc.type = cppSemaphoreTypeToCSemaphoreType(descriptor.type);
    outDesc.initialValue = descriptor.initialValue;
}

void convertQuerySetDescriptor(const QuerySetDescriptor& descriptor, GfxQuerySetDescriptor& outDesc)
{
    outDesc = {};
    outDesc.sType = GFX_STRUCTURE_TYPE_QUERY_SET_DESCRIPTOR;
    outDesc.pNext = NULL;
    outDesc.label = descriptor.label.c_str();
    outDesc.type = cppQueryTypeToCQueryType(descriptor.type);
    outDesc.count = descriptor.count;
}

void convertBindGroupLayoutDescriptor(const BindGroupLayoutDescriptor& descriptor, std::vector<GfxBindGroupLayoutEntry>& outEntries, GfxBindGroupLayoutDescriptor& outDesc)
{
    outEntries.resize(descriptor.entries.size());

    for (size_t i = 0; i < descriptor.entries.size(); ++i) {
        const auto& entry = descriptor.entries[i];
        outEntries[i].binding = entry.binding;
        outEntries[i].visibility = cppShaderStageToCShaderStage(entry.visibility);

        if (entry.resource.index() == 0) {
            outEntries[i].type = GFX_BINDING_TYPE_BUFFER;
            const auto& buffer = std::get<BindGroupLayoutEntry::BufferBinding>(entry.resource);
            outEntries[i].buffer.hasDynamicOffset = buffer.hasDynamicOffset;
            outEntries[i].buffer.minBindingSize = buffer.minBindingSize;
        } else if (entry.resource.index() == 1) {
            outEntries[i].type = GFX_BINDING_TYPE_SAMPLER;
            const auto& sampler = std::get<BindGroupLayoutEntry::SamplerBinding>(entry.resource);
            outEntries[i].sampler.comparison = sampler.comparison;
        } else if (entry.resource.index() == 2) {
            outEntries[i].type = GFX_BINDING_TYPE_TEXTURE;
            const auto& texture = std::get<BindGroupLayoutEntry::TextureBinding>(entry.resource);
            outEntries[i].texture.multisampled = texture.multisampled;
            outEntries[i].texture.viewDimension = cppTextureViewTypeToCType(texture.viewDimension);
        } else if (entry.resource.index() == 3) {
            outEntries[i].type = GFX_BINDING_TYPE_STORAGE_TEXTURE;
            const auto& storageTexture = std::get<BindGroupLayoutEntry::StorageTextureBinding>(entry.resource);
            outEntries[i].storageTexture.format = cppFormatToCFormat(storageTexture.format);
            outEntries[i].storageTexture.writeOnly = storageTexture.writeOnly;
            outEntries[i].storageTexture.viewDimension = cppTextureViewTypeToCType(storageTexture.viewDimension);
        }
    }

    outDesc = {};
    outDesc.sType = GFX_STRUCTURE_TYPE_BIND_GROUP_LAYOUT_DESCRIPTOR;
    outDesc.pNext = NULL;
    outDesc.label = descriptor.label.c_str();
    outDesc.entries = outEntries.data();
    outDesc.entryCount = static_cast<uint32_t>(outEntries.size());
}

void convertBindGroupDescriptor(const BindGroupDescriptor& descriptor, std::vector<GfxBindGroupEntry>& outEntries, GfxBindGroupDescriptor& outDesc)
{
    outEntries.resize(descriptor.entries.size());

    for (size_t i = 0; i < descriptor.entries.size(); ++i) {
        const auto& entry = descriptor.entries[i];
        outEntries[i].binding = entry.binding;

        if (entry.resource.index() == 0) {
            outEntries[i].type = GFX_BIND_GROUP_ENTRY_TYPE_BUFFER;
            auto buffer = std::get<std::shared_ptr<Buffer>>(entry.resource);
            auto bufferImpl = std::dynamic_pointer_cast<BufferImpl>(buffer);
            if (bufferImpl) {
                outEntries[i].resource.buffer.buffer = bufferImpl->getHandle();
                outEntries[i].resource.buffer.offset = entry.offset;
                outEntries[i].resource.buffer.size = entry.size;
            }
        } else if (entry.resource.index() == 1) {
            outEntries[i].type = GFX_BIND_GROUP_ENTRY_TYPE_SAMPLER;
            auto sampler = std::get<std::shared_ptr<Sampler>>(entry.resource);
            auto samplerImpl = std::dynamic_pointer_cast<SamplerImpl>(sampler);
            if (samplerImpl) {
                outEntries[i].resource.sampler = samplerImpl->getHandle();
            }
        } else if (entry.resource.index() == 2) {
            outEntries[i].type = GFX_BIND_GROUP_ENTRY_TYPE_TEXTURE_VIEW;
            auto textureView = std::get<std::shared_ptr<TextureView>>(entry.resource);
            auto textureViewImpl = std::dynamic_pointer_cast<TextureViewImpl>(textureView);
            if (textureViewImpl) {
                outEntries[i].resource.textureView = textureViewImpl->getHandle();
            }
        }
    }

    outDesc = {};
    outDesc.sType = GFX_STRUCTURE_TYPE_BIND_GROUP_DESCRIPTOR;
    outDesc.pNext = NULL;
    outDesc.label = descriptor.label.c_str();
    outDesc.entries = outEntries.data();
    outDesc.entryCount = static_cast<uint32_t>(outEntries.size());
}

void convertRenderPassDescriptor(const RenderPassCreateDescriptor& descriptor, std::vector<GfxRenderPassColorAttachment>& outColorAttachments, std::vector<GfxRenderPassColorAttachmentTarget>& outColorTargets, std::vector<GfxRenderPassColorAttachmentTarget>& outColorResolveTargets, GfxRenderPassDepthStencilAttachment& outDepthStencilAttachment, GfxRenderPassDepthStencilAttachmentTarget& outDepthTarget, GfxRenderPassDepthStencilAttachmentTarget& outDepthResolveTarget, GfxRenderPassMultiviewInfo& outMultiviewInfo, std::vector<uint32_t>& outCorrelationMasks, GfxRenderPassDescriptor& outDesc)
{
    outColorAttachments.clear();
    outColorTargets.clear();
    outColorResolveTargets.clear();
    outCorrelationMasks.clear();

    // Convert color attachments
    for (const auto& attachment : descriptor.colorAttachments) {
        GfxRenderPassColorAttachment cAttachment = {};

        GfxRenderPassColorAttachmentTarget cTarget = {};
        cTarget.format = cppFormatToCFormat(attachment.target.format);
        cTarget.sampleCount = cppSampleCountToCCount(attachment.target.sampleCount);
        cTarget.ops.loadOp = cppLoadOpToCLoadOp(attachment.target.loadOp);
        cTarget.ops.storeOp = cppStoreOpToCStoreOp(attachment.target.storeOp);
        cTarget.finalLayout = cppLayoutToCLayout(attachment.target.finalLayout);
        outColorTargets.push_back(cTarget);
        cAttachment.target = outColorTargets.back();

        if (attachment.resolveTarget.has_value()) {
            GfxRenderPassColorAttachmentTarget cResolveTarget = {};
            cResolveTarget.format = cppFormatToCFormat(attachment.resolveTarget->format);
            cResolveTarget.sampleCount = cppSampleCountToCCount(attachment.resolveTarget->sampleCount);
            cResolveTarget.ops.loadOp = cppLoadOpToCLoadOp(attachment.resolveTarget->loadOp);
            cResolveTarget.ops.storeOp = cppStoreOpToCStoreOp(attachment.resolveTarget->storeOp);
            cResolveTarget.finalLayout = cppLayoutToCLayout(attachment.resolveTarget->finalLayout);
            outColorResolveTargets.push_back(cResolveTarget);
            cAttachment.resolveTarget = &outColorResolveTargets.back();
        } else {
            cAttachment.resolveTarget = nullptr;
        }

        outColorAttachments.push_back(cAttachment);
    }

    // Convert depth/stencil attachment if present
    outDepthStencilAttachment = {};
    outDepthTarget = {};
    outDepthResolveTarget = {};
    const GfxRenderPassDepthStencilAttachment* cDepthStencilPtr = nullptr;

    if (descriptor.depthStencilAttachment.has_value()) {
        outDepthTarget.format = cppFormatToCFormat(descriptor.depthStencilAttachment->target.format);
        outDepthTarget.sampleCount = cppSampleCountToCCount(descriptor.depthStencilAttachment->target.sampleCount);
        outDepthTarget.depthOps.loadOp = cppLoadOpToCLoadOp(descriptor.depthStencilAttachment->target.depthLoadOp);
        outDepthTarget.depthOps.storeOp = cppStoreOpToCStoreOp(descriptor.depthStencilAttachment->target.depthStoreOp);
        outDepthTarget.stencilOps.loadOp = cppLoadOpToCLoadOp(descriptor.depthStencilAttachment->target.stencilLoadOp);
        outDepthTarget.stencilOps.storeOp = cppStoreOpToCStoreOp(descriptor.depthStencilAttachment->target.stencilStoreOp);
        outDepthTarget.finalLayout = cppLayoutToCLayout(descriptor.depthStencilAttachment->target.finalLayout);
        outDepthStencilAttachment.target = outDepthTarget;

        if (descriptor.depthStencilAttachment->resolveTarget.has_value()) {
            outDepthResolveTarget.format = cppFormatToCFormat(descriptor.depthStencilAttachment->resolveTarget->format);
            outDepthResolveTarget.sampleCount = cppSampleCountToCCount(descriptor.depthStencilAttachment->resolveTarget->sampleCount);
            outDepthResolveTarget.depthOps.loadOp = cppLoadOpToCLoadOp(descriptor.depthStencilAttachment->resolveTarget->depthLoadOp);
            outDepthResolveTarget.depthOps.storeOp = cppStoreOpToCStoreOp(descriptor.depthStencilAttachment->resolveTarget->depthStoreOp);
            outDepthResolveTarget.stencilOps.loadOp = cppLoadOpToCLoadOp(descriptor.depthStencilAttachment->resolveTarget->stencilLoadOp);
            outDepthResolveTarget.stencilOps.storeOp = cppStoreOpToCStoreOp(descriptor.depthStencilAttachment->resolveTarget->stencilStoreOp);
            outDepthResolveTarget.finalLayout = cppLayoutToCLayout(descriptor.depthStencilAttachment->resolveTarget->finalLayout);
            outDepthStencilAttachment.resolveTarget = &outDepthResolveTarget;
        } else {
            outDepthStencilAttachment.resolveTarget = nullptr;
        }

        cDepthStencilPtr = &outDepthStencilAttachment;
    }

    outDesc = {};
    outDesc.sType = GFX_STRUCTURE_TYPE_RENDER_PASS_DESCRIPTOR;
    outDesc.pNext = nullptr;
    outDesc.label = descriptor.label.c_str();
    outDesc.colorAttachments = outColorAttachments.empty() ? nullptr : outColorAttachments.data();
    outDesc.colorAttachmentCount = static_cast<uint32_t>(outColorAttachments.size());
    outDesc.depthStencilAttachment = cDepthStencilPtr;

    // Handle extension chain - convert C++ ChainedStruct to C pNext chain
    const ChainedStruct* chainNode = descriptor.next;
    while (chainNode) {
        // Check for multiview extension
        if (const auto* multiview = dynamic_cast<const RenderPassMultiviewInfo*>(chainNode)) {
            outMultiviewInfo = {};
            outMultiviewInfo.sType = GFX_STRUCTURE_TYPE_RENDER_PASS_MULTIVIEW_INFO;
            outMultiviewInfo.pNext = nullptr;
            outMultiviewInfo.viewMask = multiview->viewMask;
            outMultiviewInfo.correlationMaskCount = static_cast<uint32_t>(multiview->correlationMasks.size());
            
            // Copy correlation masks to output vector
            outCorrelationMasks = multiview->correlationMasks;
            outMultiviewInfo.correlationMasks = outCorrelationMasks.empty() ? nullptr : outCorrelationMasks.data();
            
            // Link to pNext chain
            outDesc.pNext = &outMultiviewInfo;
        }
        
        chainNode = chainNode->next;
    }
}

void convertRenderPassBeginDescriptor(const RenderPassBeginDescriptor& descriptor, GfxRenderPass renderPassHandle, GfxFramebuffer framebufferHandle, std::vector<GfxColor>& outClearValues, GfxRenderPassBeginDescriptor& outDesc)
{
    outClearValues.clear();
    outClearValues.reserve(descriptor.colorClearValues.size());
    for (const auto& color : descriptor.colorClearValues) {
        outClearValues.push_back({ color.r, color.g, color.b, color.a });
    }

    outDesc = {};
    outDesc.sType = GFX_STRUCTURE_TYPE_RENDER_PASS_BEGIN_DESCRIPTOR;
    outDesc.pNext = NULL;
    outDesc.label = nullptr;
    outDesc.renderPass = renderPassHandle;
    outDesc.framebuffer = framebufferHandle;
    outDesc.colorClearValues = outClearValues.empty() ? nullptr : outClearValues.data();
    outDesc.colorClearValueCount = static_cast<uint32_t>(outClearValues.size());
    outDesc.depthClearValue = descriptor.depthClearValue;
    outDesc.stencilClearValue = descriptor.stencilClearValue;
}

void convertComputePassBeginDescriptor(const ComputePassBeginDescriptor& descriptor, GfxComputePassBeginDescriptor& outDesc)
{
    outDesc = {};
    outDesc.sType = GFX_STRUCTURE_TYPE_COMPUTE_PASS_BEGIN_DESCRIPTOR;
    outDesc.pNext = NULL;
    outDesc.label = descriptor.label.c_str();
}

void convertPresentDescriptor(const PresentDescriptor& descriptor, std::vector<GfxSemaphore>& outWaitSemaphores, GfxPresentDescriptor& outDescriptor)
{
    outWaitSemaphores.clear();
    outWaitSemaphores.reserve(descriptor.waitSemaphores.size());

    for (const auto& sem : descriptor.waitSemaphores) {
        outWaitSemaphores.push_back(extractNativeHandle<GfxSemaphore>(sem));
    }

    outDescriptor = {};
    outDescriptor.sType = GFX_STRUCTURE_TYPE_PRESENT_DESCRIPTOR;
    outDescriptor.pNext = NULL;
    outDescriptor.waitSemaphores = outWaitSemaphores.empty() ? nullptr : outWaitSemaphores.data();
    outDescriptor.waitSemaphoreCount = static_cast<uint32_t>(outWaitSemaphores.size());
}

void convertFramebufferDescriptor(const FramebufferDescriptor& descriptor, GfxRenderPass renderPassHandle, std::vector<GfxFramebufferAttachment>& outColorAttachments, GfxFramebufferAttachment& outDepthStencilAttachment, GfxFramebufferDescriptor& outDesc)
{
    outColorAttachments.clear();

    if (!renderPassHandle) {
        throw std::runtime_error("Invalid render pass handle");
    }

    // Convert color attachments
    for (const auto& attachment : descriptor.colorAttachments) {
        GfxFramebufferAttachment cAttachment = {};

        auto viewImpl = std::dynamic_pointer_cast<TextureViewImpl>(attachment.view);
        if (!viewImpl) {
            throw std::runtime_error("Invalid texture view type");
        }
        cAttachment.view = viewImpl->getHandle();

        if (attachment.resolveTarget) {
            auto resolveImpl = std::dynamic_pointer_cast<TextureViewImpl>(attachment.resolveTarget);
            if (!resolveImpl) {
                throw std::runtime_error("Invalid resolve target texture view type");
            }
            cAttachment.resolveTarget = resolveImpl->getHandle();
        } else {
            cAttachment.resolveTarget = nullptr;
        }

        outColorAttachments.push_back(cAttachment);
    }

    // Convert depth/stencil attachment if present
    outDepthStencilAttachment = { nullptr, nullptr };

    if (descriptor.depthStencilAttachment.has_value()) {
        auto viewImpl = std::dynamic_pointer_cast<TextureViewImpl>(descriptor.depthStencilAttachment->view);
        if (!viewImpl) {
            throw std::runtime_error("Invalid depth/stencil texture view type");
        }
        outDepthStencilAttachment.view = viewImpl->getHandle();

        if (descriptor.depthStencilAttachment->resolveTarget) {
            auto resolveImpl = std::dynamic_pointer_cast<TextureViewImpl>(descriptor.depthStencilAttachment->resolveTarget);
            if (!resolveImpl) {
                throw std::runtime_error("Invalid depth/stencil resolve target texture view type");
            }
            outDepthStencilAttachment.resolveTarget = resolveImpl->getHandle();
        }
    }

    outDesc = {};
    outDesc.sType = GFX_STRUCTURE_TYPE_FRAMEBUFFER_DESCRIPTOR;
    outDesc.pNext = NULL;
    outDesc.label = descriptor.label.c_str();
    outDesc.renderPass = renderPassHandle;
    outDesc.colorAttachments = outColorAttachments.empty() ? nullptr : outColorAttachments.data();
    outDesc.colorAttachmentCount = static_cast<uint32_t>(outColorAttachments.size());
    outDesc.depthStencilAttachment = outDepthStencilAttachment;
    outDesc.width = descriptor.width;
    outDesc.height = descriptor.height;
}

void convertVertexState(const VertexState& input, GfxShader vertexShaderHandle, std::vector<std::vector<GfxVertexAttribute>>& outAttributesPerBuffer, std::vector<GfxVertexBufferLayout>& outVertexBuffers, GfxVertexState& out)
{
    outAttributesPerBuffer.clear();
    outVertexBuffers.clear();

    for (const auto& buffer : input.buffers) {
        std::vector<GfxVertexAttribute> cAttributes;
        for (const auto& attr : buffer.attributes) {
            GfxVertexAttribute cAttr = {};
            cAttr.format = cppFormatToCFormat(attr.format);
            cAttr.offset = attr.offset;
            cAttr.shaderLocation = attr.shaderLocation;
            cAttributes.push_back(cAttr);
        }
        outAttributesPerBuffer.push_back(std::move(cAttributes));

        GfxVertexBufferLayout cBuffer = {};
        cBuffer.arrayStride = buffer.arrayStride;
        cBuffer.attributes = outAttributesPerBuffer.back().data();
        cBuffer.attributeCount = static_cast<uint32_t>(outAttributesPerBuffer.back().size());
        cBuffer.stepMode = cppVertexStepModeToCVertexStepMode(buffer.stepMode);
        outVertexBuffers.push_back(cBuffer);
    }

    out = {};
    out.module = vertexShaderHandle;
    out.entryPoint = input.entryPoint.c_str();
    out.buffers = outVertexBuffers.empty() ? nullptr : outVertexBuffers.data();
    out.bufferCount = static_cast<uint32_t>(outVertexBuffers.size());
}

void convertFragmentState(const FragmentState& input, GfxShader fragmentShaderHandle, std::vector<GfxColorTargetState>& outColorTargets, std::vector<GfxBlendState>& outBlendStates, GfxFragmentState& out)
{
    outColorTargets.clear();
    outBlendStates.clear();

    for (const auto& target : input.targets) {
        GfxColorTargetState cTarget = {};
        cTarget.format = cppFormatToCFormat(target.format);
        cTarget.writeMask = cppColorWriteMaskToCColorWriteMask(target.writeMask);

        if (target.blend.has_value()) {
            GfxBlendState cBlend = {};
            cBlend.color.operation = cppBlendOperationToCBlendOperation(target.blend->color.operation);
            cBlend.color.srcFactor = cppBlendFactorToCBlendFactor(target.blend->color.srcFactor);
            cBlend.color.dstFactor = cppBlendFactorToCBlendFactor(target.blend->color.dstFactor);
            cBlend.alpha.operation = cppBlendOperationToCBlendOperation(target.blend->alpha.operation);
            cBlend.alpha.srcFactor = cppBlendFactorToCBlendFactor(target.blend->alpha.srcFactor);
            cBlend.alpha.dstFactor = cppBlendFactorToCBlendFactor(target.blend->alpha.dstFactor);
            outBlendStates.push_back(cBlend);
            cTarget.blend = &outBlendStates.back();
        } else {
            cTarget.blend = nullptr;
        }

        outColorTargets.push_back(cTarget);
    }

    out = {};
    out.module = fragmentShaderHandle;
    out.entryPoint = input.entryPoint.c_str();
    out.targets = outColorTargets.data();
    out.targetCount = static_cast<uint32_t>(outColorTargets.size());
}

void convertPrimitiveState(const PrimitiveState& input, GfxPrimitiveState& out)
{
    out = {};
    out.topology = cppPrimitiveTopologyToCPrimitiveTopology(input.topology);
    out.stripIndexFormat = cppIndexFormatToCIndexFormat(input.stripIndexFormat);
    out.frontFace = cppFrontFaceToCFrontFace(input.frontFace);
    out.cullMode = cppCullModeToCCullMode(input.cullMode);
    out.polygonMode = cppPolygonModeToCPolygonMode(input.polygonMode);
}

void convertDepthStencilState(const DepthStencilState& input, GfxDepthStencilState& out)
{
    out = {};
    out.format = cppFormatToCFormat(input.format);
    out.depthWriteEnabled = input.depthWriteEnabled;
    out.depthCompare = cppCompareFunctionToCCompareFunction(input.depthCompare);

    out.stencilFront.compare = cppCompareFunctionToCCompareFunction(input.stencilFront.compare);
    out.stencilFront.failOp = cppStencilOperationToCStencilOperation(input.stencilFront.failOp);
    out.stencilFront.depthFailOp = cppStencilOperationToCStencilOperation(input.stencilFront.depthFailOp);
    out.stencilFront.passOp = cppStencilOperationToCStencilOperation(input.stencilFront.passOp);

    out.stencilBack.compare = cppCompareFunctionToCCompareFunction(input.stencilBack.compare);
    out.stencilBack.failOp = cppStencilOperationToCStencilOperation(input.stencilBack.failOp);
    out.stencilBack.depthFailOp = cppStencilOperationToCStencilOperation(input.stencilBack.depthFailOp);
    out.stencilBack.passOp = cppStencilOperationToCStencilOperation(input.stencilBack.passOp);

    out.stencilReadMask = input.stencilReadMask;
    out.stencilWriteMask = input.stencilWriteMask;
    out.depthBias = input.depthBias;
    out.depthBiasSlopeScale = input.depthBiasSlopeScale;
    out.depthBiasClamp = input.depthBiasClamp;
}

void convertRenderPipelineDescriptor(const RenderPipelineDescriptor& descriptor, GfxRenderPass renderPassHandle, const GfxVertexState& vertexState, const std::optional<GfxFragmentState>& fragmentState, const GfxPrimitiveState& primitiveState, const std::optional<GfxDepthStencilState>& depthStencilState, std::vector<GfxBindGroupLayout>& outBindGroupLayouts, GfxRenderPipelineDescriptor& out)
{
    outBindGroupLayouts.clear();
    for (const auto& layout : descriptor.bindGroupLayouts) {
        auto layoutImpl = std::dynamic_pointer_cast<BindGroupLayoutImpl>(layout);
        if (layoutImpl) {
            outBindGroupLayouts.push_back(layoutImpl->getHandle());
        }
    }

    out = {};
    out.sType = GFX_STRUCTURE_TYPE_RENDER_PIPELINE_DESCRIPTOR;
    out.pNext = NULL;
    out.label = descriptor.label.c_str();
    out.renderPass = renderPassHandle;
    out.vertex = const_cast<GfxVertexState*>(&vertexState);
    out.fragment = fragmentState.has_value() ? const_cast<GfxFragmentState*>(&(*fragmentState)) : nullptr;
    out.primitive = const_cast<GfxPrimitiveState*>(&primitiveState);
    out.depthStencil = depthStencilState.has_value() ? const_cast<GfxDepthStencilState*>(&(*depthStencilState)) : nullptr;
    out.sampleCount = cppSampleCountToCCount(descriptor.sampleCount);
    out.bindGroupLayouts = outBindGroupLayouts.empty() ? nullptr : outBindGroupLayouts.data();
    out.bindGroupLayoutCount = static_cast<uint32_t>(outBindGroupLayouts.size());
}

void convertComputePipelineDescriptor(const ComputePipelineDescriptor& descriptor, GfxShader computeShaderHandle, std::vector<GfxBindGroupLayout>& outBindGroupLayouts, GfxComputePipelineDescriptor& out)
{
    outBindGroupLayouts.clear();
    for (const auto& layout : descriptor.bindGroupLayouts) {
        auto layoutImpl = std::dynamic_pointer_cast<BindGroupLayoutImpl>(layout);
        if (layoutImpl) {
            outBindGroupLayouts.push_back(layoutImpl->getHandle());
        }
    }

    out = {};
    out.sType = GFX_STRUCTURE_TYPE_COMPUTE_PIPELINE_DESCRIPTOR;
    out.pNext = NULL;
    out.label = descriptor.label.c_str();
    out.compute = computeShaderHandle;
    out.entryPoint = descriptor.entryPoint.c_str();
    out.bindGroupLayouts = outBindGroupLayouts.empty() ? nullptr : outBindGroupLayouts.data();
    out.bindGroupLayoutCount = static_cast<uint32_t>(outBindGroupLayouts.size());
}

} // namespace gfx
