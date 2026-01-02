#include "common/WebGPUCommon.h"

#include "WebGPUBackend.h"
#include "converter/GfxWebGPUConverter.h"
#include "entity/Entities.h"
#include "surface/SurfaceFactory.h"

#include <cassert>
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>
#include <vector>

// ============================================================================
// WebGPU Callback Functions
// ============================================================================

extern "C" {
// Callback for adapter request
static void onAdapterRequested(WGPURequestAdapterStatus status, WGPUAdapter adapter,
    WGPUStringView message, void* userdata1, void* userdata2)
{
    struct AdapterRequestContext {
        GfxAdapter* outAdapter;
        gfx::webgpu::Instance* instance;
        bool completed;
        WGPURequestAdapterStatus status;
    };

    auto* ctx = static_cast<AdapterRequestContext*>(userdata1);
    ctx->status = status;
    ctx->completed = true;

    if (status == WGPURequestAdapterStatus_Success && adapter) {
        auto* adapterObj = new gfx::webgpu::Adapter(adapter, ctx->instance);
        *ctx->outAdapter = reinterpret_cast<GfxAdapter>(adapterObj);
    } else if (message.data) {
        fprintf(stderr, "Error: Failed to request adapter: %.*s\n",
            (int)message.length, message.data);
    }
    (void)userdata2; // Unused
}

// Callback for device request
static void onDeviceRequested(WGPURequestDeviceStatus status, WGPUDevice device,
    WGPUStringView message, void* userdata1, void* userdata2)
{
    struct DeviceRequestContext {
        GfxDevice* outDevice;
        gfx::webgpu::Adapter* adapter;
        bool completed;
        WGPURequestDeviceStatus status;
    };

    auto* ctx = static_cast<DeviceRequestContext*>(userdata1);
    ctx->status = status;
    ctx->completed = true;

    if (status == WGPURequestDeviceStatus_Success && device) {
        auto* deviceObj = new gfx::webgpu::Device(device, ctx->adapter);
        *ctx->outDevice = reinterpret_cast<GfxDevice>(deviceObj);
    } else if (message.data) {
        fprintf(stderr, "Error: Failed to request device: %.*s\n",
            (int)message.length, message.data);
    }
    (void)userdata2; // Unused
}

static void uncapturedErrorCallback(WGPUDevice const*, WGPUErrorType type, WGPUStringView message, void*, void*)
{
    const char* errorType = "Unknown";
    switch (type) {
    case WGPUErrorType_Validation:
        errorType = "Validation";
        break;
    case WGPUErrorType_OutOfMemory:
        errorType = "OutOfMemory";
        break;
    case WGPUErrorType_Internal:
        errorType = "Internal";
        break;
    default:
        break;
    }
    fprintf(stderr, "[WebGPU ERROR - %s]: %.*s\n",
        errorType, (int)message.length, message.data);
}

static void deviceLostCallback(WGPUDevice const*, WGPUDeviceLostReason reason, WGPUStringView message, void*, void*)
{
    const char* reasonStr = "Unknown";
    switch (reason) {
    case WGPUDeviceLostReason_Unknown:
        reasonStr = "Unknown";
        break;
    case WGPUDeviceLostReason_Destroyed:
        reasonStr = "Destroyed";
        break;
    case WGPUDeviceLostReason_CallbackCancelled:
        reasonStr = "CallbackCancelled";
        break;
    case WGPUDeviceLostReason_FailedCreation:
        reasonStr = "FailedCreation";
        break;
    default:
        break;
    }
    if (message.data && message.length > 0) {
        fprintf(stderr, "[WebGPU Device Lost - %s]: %.*s\n",
            reasonStr, (int)message.length, message.data);
    }
}

static void queueWorkDoneCallback(WGPUQueueWorkDoneStatus status, WGPUStringView message, void* userdata1, void* userdata2)
{
    (void)status;
    (void)message;
    (void)userdata1;
    (void)userdata2;
    // Nothing to do - we just needed a valid callback for Dawn
}

struct MapCallbackData {
    WGPUMapAsyncStatus status = WGPUMapAsyncStatus_Error;
    bool completed = false;
};

static void bufferMapCallback(WGPUMapAsyncStatus status, WGPUStringView, void* userdata1, void*)
{
    auto* data = static_cast<MapCallbackData*>(userdata1);
    data->status = status;
    data->completed = true;
}
} // extern "C"

namespace gfx::webgpu {

// ============================================================================
// Backend C++ Class Export
// ============================================================================

// Instance functions
GfxResult WebGPUBackend::createInstance(const GfxInstanceDescriptor* descriptor, GfxInstance* outInstance) const
{
    if (!outInstance) {
        return GFX_RESULT_ERROR_INVALID_PARAMETER;
    }

    try {
        auto createInfo = gfx::convertor::gfxDescriptorToWebGPUInstanceCreateInfo(descriptor);
        auto* instance = new gfx::webgpu::Instance(createInfo);
        *outInstance = reinterpret_cast<GfxInstance>(instance);
        return GFX_RESULT_SUCCESS;
    } catch (...) {
        return GFX_RESULT_ERROR_UNKNOWN;
    }
}

void WebGPUBackend::instanceDestroy(GfxInstance instance) const
{
    if (!instance) {
        return;
    }

    // Process any remaining events before destroying the instance
    // This ensures all pending callbacks are completed
    auto* inst = reinterpret_cast<gfx::webgpu::Instance*>(instance);
    if (inst->handle()) {
        wgpuInstanceProcessEvents(inst->handle());
    }

    delete inst;
}

void WebGPUBackend::instanceSetDebugCallback(GfxInstance instance, GfxDebugCallback callback, void* userData) const
{
    // TODO: Implement debug callback using WebGPU error handling
    (void)instance;
    (void)callback;
    (void)userData;
}

GfxResult WebGPUBackend::instanceRequestAdapter(GfxInstance instance, const GfxAdapterDescriptor* descriptor, GfxAdapter* outAdapter) const
{
    if (!instance || !outAdapter) {
        return GFX_RESULT_ERROR_INVALID_PARAMETER;
    }

    auto* inst = reinterpret_cast<gfx::webgpu::Instance*>(instance);

    WGPURequestAdapterOptions options = WGPU_REQUEST_ADAPTER_OPTIONS_INIT;
    if (descriptor) {
        switch (descriptor->powerPreference) {
        case GFX_POWER_PREFERENCE_LOW_POWER:
            options.powerPreference = WGPUPowerPreference_LowPower;
            break;
        case GFX_POWER_PREFERENCE_HIGH_PERFORMANCE:
            options.powerPreference = WGPUPowerPreference_HighPerformance;
            break;
        default:
            options.powerPreference = WGPUPowerPreference_Undefined;
            break;
        }
        options.forceFallbackAdapter = descriptor->forceFallbackAdapter ? WGPU_TRUE : WGPU_FALSE;
    }

    // Use a struct to track callback completion
    struct AdapterRequestContext {
        GfxAdapter* outAdapter;
        gfx::webgpu::Instance* instance;
        bool completed;
        WGPURequestAdapterStatus status;
    } context = { outAdapter, inst, false, WGPURequestAdapterStatus_Error };

    WGPURequestAdapterCallbackInfo callbackInfo = WGPU_REQUEST_ADAPTER_CALLBACK_INFO_INIT;
    callbackInfo.mode = WGPUCallbackMode_WaitAnyOnly;
    callbackInfo.callback = onAdapterRequested;
    callbackInfo.userdata1 = &context;

    WGPUFuture future = wgpuInstanceRequestAdapter(inst->handle(), &options, callbackInfo);

    // Use WaitAny to properly wait for the callback
    WGPUFutureWaitInfo waitInfo = WGPU_FUTURE_WAIT_INFO_INIT;
    waitInfo.future = future;
    wgpuInstanceWaitAny(inst->handle(), 1, &waitInfo, UINT64_MAX);

    if (!context.completed) {
        fprintf(stderr, "Error: Adapter request timed out\n");
        return GFX_RESULT_ERROR_UNKNOWN;
    }

    return *outAdapter ? GFX_RESULT_SUCCESS : GFX_RESULT_ERROR_UNKNOWN;
}

uint32_t WebGPUBackend::instanceEnumerateAdapters(GfxInstance instance, GfxAdapter* adapters, uint32_t maxAdapters) const
{
    if (!instance || maxAdapters == 0) {
        return 0;
    }

    GfxAdapter adapter = nullptr;
    if (instanceRequestAdapter(instance, nullptr, &adapter) == GFX_RESULT_SUCCESS && adapter) {
        if (adapters) {
            adapters[0] = adapter;
        }
        return 1;
    }
    return 0;
}

// Adapter functions
void WebGPUBackend::adapterDestroy(GfxAdapter adapter) const
{
    if (!adapter) {
        return;
    }
    delete reinterpret_cast<gfx::webgpu::Adapter*>(adapter);
}

GfxResult WebGPUBackend::adapterCreateDevice(GfxAdapter adapter, const GfxDeviceDescriptor* descriptor, GfxDevice* outDevice) const
{
    if (!adapter || !outDevice) {
        return GFX_RESULT_ERROR_INVALID_PARAMETER;
    }

    auto* adapterPtr = reinterpret_cast<gfx::webgpu::Adapter*>(adapter);

    WGPUUncapturedErrorCallbackInfo errorCallbackInfo = WGPU_UNCAPTURED_ERROR_CALLBACK_INFO_INIT;
    errorCallbackInfo.callback = uncapturedErrorCallback;

    WGPUDeviceLostCallbackInfo deviceLostCallbackInfo = WGPU_DEVICE_LOST_CALLBACK_INFO_INIT;
    deviceLostCallbackInfo.mode = WGPUCallbackMode_AllowSpontaneous;
    deviceLostCallbackInfo.callback = deviceLostCallback;

    WGPUDeviceDescriptor wgpuDesc = WGPU_DEVICE_DESCRIPTOR_INIT;
    wgpuDesc.uncapturedErrorCallbackInfo = errorCallbackInfo;
    wgpuDesc.deviceLostCallbackInfo = deviceLostCallbackInfo;
    if (descriptor && descriptor->label) {
        wgpuDesc.label = gfx::convertor::gfxStringView(descriptor->label);
    }

    struct DeviceRequestContext {
        GfxDevice* outDevice;
        gfx::webgpu::Adapter* adapter;
        bool completed;
        WGPURequestDeviceStatus status;
    } context = { outDevice, adapterPtr, false, WGPURequestDeviceStatus_Error };

    WGPURequestDeviceCallbackInfo callbackInfo = WGPU_REQUEST_DEVICE_CALLBACK_INFO_INIT;
    callbackInfo.mode = WGPUCallbackMode_WaitAnyOnly;
    callbackInfo.callback = onDeviceRequested;
    callbackInfo.userdata1 = &context;

    WGPUFuture future = wgpuAdapterRequestDevice(adapterPtr->handle(), &wgpuDesc, callbackInfo);

    // Use WaitAny to properly wait for the callback
    if (adapterPtr->getInstance()) {
        WGPUFutureWaitInfo waitInfo = WGPU_FUTURE_WAIT_INFO_INIT;
        waitInfo.future = future;
        wgpuInstanceWaitAny(adapterPtr->getInstance()->handle(), 1, &waitInfo, UINT64_MAX);
    }

    if (!context.completed) {
        fprintf(stderr, "Error: Device request timed out\n");
        return GFX_RESULT_ERROR_UNKNOWN;
    }

    return *outDevice ? GFX_RESULT_SUCCESS : GFX_RESULT_ERROR_UNKNOWN;
}

const char* WebGPUBackend::adapterGetName(GfxAdapter adapter) const
{
    if (!adapter) {
        return nullptr;
    }
    auto* adapterPtr = reinterpret_cast<gfx::webgpu::Adapter*>(adapter);
    return adapterPtr->getName();
}

GfxBackend WebGPUBackend::adapterGetBackend(GfxAdapter adapter) const
{
    return adapter ? GFX_BACKEND_WEBGPU : GFX_BACKEND_AUTO;
}

void WebGPUBackend::adapterGetLimits(GfxAdapter adapter, GfxDeviceLimits* outLimits) const
{
    if (!adapter || !outLimits) {
        return;
    }

    auto* adapterPtr = reinterpret_cast<gfx::webgpu::Adapter*>(adapter);

    WGPULimits limits = WGPU_LIMITS_INIT;
    WGPUStatus status = wgpuAdapterGetLimits(adapterPtr->handle(), &limits);
    if (status == WGPUStatus_Success) {
        outLimits->minUniformBufferOffsetAlignment = limits.minUniformBufferOffsetAlignment;
        outLimits->minStorageBufferOffsetAlignment = limits.minStorageBufferOffsetAlignment;
        outLimits->maxUniformBufferBindingSize = static_cast<uint32_t>(limits.maxUniformBufferBindingSize);
        outLimits->maxStorageBufferBindingSize = static_cast<uint32_t>(limits.maxStorageBufferBindingSize);
        outLimits->maxBufferSize = limits.maxBufferSize;
        outLimits->maxTextureDimension1D = limits.maxTextureDimension1D;
        outLimits->maxTextureDimension2D = limits.maxTextureDimension2D;
        outLimits->maxTextureDimension3D = limits.maxTextureDimension3D;
        outLimits->maxTextureArrayLayers = limits.maxTextureArrayLayers;
    } else {
        // Fallback to reasonable defaults if query fails
        outLimits->minUniformBufferOffsetAlignment = 256;
        outLimits->minStorageBufferOffsetAlignment = 256;
        outLimits->maxUniformBufferBindingSize = 65536;
        outLimits->maxStorageBufferBindingSize = 134217728;
        outLimits->maxBufferSize = 268435456;
        outLimits->maxTextureDimension1D = 8192;
        outLimits->maxTextureDimension2D = 8192;
        outLimits->maxTextureDimension3D = 2048;
        outLimits->maxTextureArrayLayers = 256;
    }
}

// Device functions
void WebGPUBackend::deviceDestroy(GfxDevice device) const
{
    if (!device) {
        return;
    }
    delete reinterpret_cast<gfx::webgpu::Device*>(device);
}

GfxQueue WebGPUBackend::deviceGetQueue(GfxDevice device) const
{
    if (!device) {
        return nullptr;
    }
    auto* devicePtr = reinterpret_cast<gfx::webgpu::Device*>(device);
    return reinterpret_cast<GfxQueue>(devicePtr->getQueue());
}

GfxResult WebGPUBackend::deviceCreateSurface(GfxDevice device, const GfxSurfaceDescriptor* descriptor, GfxSurface* outSurface) const
{
    if (!device || !descriptor || !outSurface) {
        return GFX_RESULT_ERROR_INVALID_PARAMETER;
    }

    auto* devicePtr = reinterpret_cast<gfx::webgpu::Device*>(device);

    // Get the instance from the device's adapter
    WGPUInstance inst = nullptr;
    if (devicePtr->getAdapter() && devicePtr->getAdapter()->getInstance()) {
        inst = devicePtr->getAdapter()->getInstance()->handle();
    }

    if (!inst) {
        fprintf(stderr, "Error: Cannot create surface - no instance available\\n");
        return GFX_RESULT_ERROR_UNKNOWN;
    }

    WGPUSurface wgpuSurface = gfx::webgpu::surface::SurfaceFactory{}.createFromNativeWindow(inst, descriptor->windowHandle);
    if (!wgpuSurface) {
        return GFX_RESULT_ERROR_UNKNOWN;
    }

    auto* surface = new gfx::webgpu::Surface(wgpuSurface, devicePtr->getAdapter()->handle());
    *outSurface = reinterpret_cast<GfxSurface>(surface);
    return GFX_RESULT_SUCCESS;
}

GfxResult WebGPUBackend::deviceCreateSwapchain(GfxDevice device, GfxSurface surface, const GfxSwapchainDescriptor* descriptor, GfxSwapchain* outSwapchain) const
{
    if (!device || !surface || !descriptor || !outSwapchain) {
        return GFX_RESULT_ERROR_INVALID_PARAMETER;
    }

    auto* devicePtr = reinterpret_cast<gfx::webgpu::Device*>(device);
    auto* surfacePtr = reinterpret_cast<gfx::webgpu::Surface*>(surface);

    // Get surface capabilities
    WGPUSurfaceCapabilities capabilities = WGPU_SURFACE_CAPABILITIES_INIT;
    wgpuSurfaceGetCapabilities(surfacePtr->handle(), devicePtr->getAdapter()->handle(), &capabilities);

    WGPUTextureFormat format = gfx::convertor::gfxFormatToWGPUFormat(descriptor->format);
    bool formatSupported = false;
    for (size_t i = 0; i < capabilities.formatCount; ++i) {
        if (capabilities.formats[i] == format) {
            formatSupported = true;
            break;
        }
    }
    if (!formatSupported && capabilities.formatCount > 0) {
        format = capabilities.formats[0];
    }

    WGPUPresentMode presentMode = gfx::convertor::gfxPresentModeToWGPU(descriptor->presentMode);
    bool presentModeSupported = false;
    for (size_t i = 0; i < capabilities.presentModeCount; ++i) {
        if (capabilities.presentModes[i] == presentMode) {
            presentModeSupported = true;
            break;
        }
    }
    if (!presentModeSupported && capabilities.presentModeCount > 0) {
        presentMode = capabilities.presentModes[0];
    }

    // Configure surface
    WGPUSurfaceConfiguration config = WGPU_SURFACE_CONFIGURATION_INIT;
    config.device = devicePtr->handle();
    config.format = format;
    config.usage = gfx::convertor::gfxTextureUsageToWGPU(descriptor->usage);
    config.width = descriptor->width;
    config.height = descriptor->height;
    config.presentMode = presentMode;
    config.alphaMode = WGPUCompositeAlphaMode_Auto;

    wgpuSurfaceConfigure(surfacePtr->handle(), &config);

    auto* swapchain = new gfx::webgpu::Swapchain(surfacePtr->handle(), devicePtr->handle(),
        descriptor->width, descriptor->height,
        format, presentMode, descriptor->bufferCount);
    *outSwapchain = reinterpret_cast<GfxSwapchain>(swapchain);

    // Free capabilities using the proper WebGPU function
    wgpuSurfaceCapabilitiesFreeMembers(capabilities);

    return GFX_RESULT_SUCCESS;
}

GfxResult WebGPUBackend::deviceCreateBuffer(GfxDevice device, const GfxBufferDescriptor* descriptor, GfxBuffer* outBuffer) const
{
    if (!device || !descriptor || !outBuffer) {
        return GFX_RESULT_ERROR_INVALID_PARAMETER;
    }

    auto* devicePtr = reinterpret_cast<gfx::webgpu::Device*>(device);

    WGPUBufferDescriptor wgpuDesc = WGPU_BUFFER_DESCRIPTOR_INIT;
    if (descriptor->label) {
        wgpuDesc.label = gfx::convertor::gfxStringView(descriptor->label);
    }
    wgpuDesc.size = descriptor->size;
    wgpuDesc.usage = gfx::convertor::gfxBufferUsageToWGPU(descriptor->usage);
    wgpuDesc.mappedAtCreation = descriptor->mappedAtCreation ? WGPU_TRUE : WGPU_FALSE;

    WGPUBuffer wgpuBuffer = wgpuDeviceCreateBuffer(devicePtr->handle(), &wgpuDesc);
    if (!wgpuBuffer) {
        return GFX_RESULT_ERROR_UNKNOWN;
    }

    auto* buffer = new gfx::webgpu::Buffer(wgpuBuffer, descriptor->size, static_cast<WGPUBufferUsage>(descriptor->usage), devicePtr);
    *outBuffer = reinterpret_cast<GfxBuffer>(buffer);
    return GFX_RESULT_SUCCESS;
}

GfxResult WebGPUBackend::deviceCreateTexture(GfxDevice device, const GfxTextureDescriptor* descriptor, GfxTexture* outTexture) const
{
    if (!device || !descriptor || !outTexture) {
        return GFX_RESULT_ERROR_INVALID_PARAMETER;
    }

    auto* devicePtr = reinterpret_cast<gfx::webgpu::Device*>(device);

    WGPUTextureDescriptor wgpuDesc = WGPU_TEXTURE_DESCRIPTOR_INIT;
    if (descriptor->label) {
        wgpuDesc.label = gfx::convertor::gfxStringView(descriptor->label);
    }
    wgpuDesc.dimension = gfx::convertor::gfxTextureTypeToWGPU(descriptor->type);

    // Set size based on texture type
    uint32_t arrayLayers = descriptor->arrayLayerCount > 0 ? descriptor->arrayLayerCount : 1;
    if (descriptor->type == GFX_TEXTURE_TYPE_CUBE) {
        // Cube maps need 6 or 6*N layers
        if (arrayLayers < 6) {
            arrayLayers = 6;
        }
    }

    wgpuDesc.size = { descriptor->size.width, descriptor->size.height,
        descriptor->type == GFX_TEXTURE_TYPE_3D ? descriptor->size.depth : arrayLayers };
    wgpuDesc.mipLevelCount = descriptor->mipLevelCount;
    wgpuDesc.sampleCount = descriptor->sampleCount;
    wgpuDesc.format = gfx::convertor::gfxFormatToWGPUFormat(descriptor->format);
    wgpuDesc.usage = gfx::convertor::gfxTextureUsageToWGPU(descriptor->usage);

    WGPUTexture wgpuTexture = wgpuDeviceCreateTexture(devicePtr->handle(), &wgpuDesc);
    if (!wgpuTexture) {
        return GFX_RESULT_ERROR_UNKNOWN;
    }

    auto* texture = new gfx::webgpu::Texture(wgpuTexture, wgpuDesc.size, wgpuDesc.format,
        descriptor->mipLevelCount, descriptor->sampleCount,
        wgpuDesc.usage);
    *outTexture = reinterpret_cast<GfxTexture>(texture);
    return GFX_RESULT_SUCCESS;
}

GfxResult WebGPUBackend::deviceCreateSampler(GfxDevice device, const GfxSamplerDescriptor* descriptor, GfxSampler* outSampler) const
{
    if (!device || !descriptor || !outSampler) {
        return GFX_RESULT_ERROR_INVALID_PARAMETER;
    }

    auto* devicePtr = reinterpret_cast<gfx::webgpu::Device*>(device);

    WGPUSamplerDescriptor wgpuDesc = WGPU_SAMPLER_DESCRIPTOR_INIT;
    if (descriptor->label) {
        wgpuDesc.label = gfx::convertor::gfxStringView(descriptor->label);
    }

    // Convert address modes
    wgpuDesc.addressModeU = gfx::convertor::gfxAddressModeToWGPU(descriptor->addressModeU);
    wgpuDesc.addressModeV = gfx::convertor::gfxAddressModeToWGPU(descriptor->addressModeV);
    wgpuDesc.addressModeW = gfx::convertor::gfxAddressModeToWGPU(descriptor->addressModeW);

    // Convert filter modes
    wgpuDesc.magFilter = gfx::convertor::gfxFilterModeToWGPU(descriptor->magFilter);
    wgpuDesc.minFilter = gfx::convertor::gfxFilterModeToWGPU(descriptor->minFilter);
    wgpuDesc.mipmapFilter = gfx::convertor::gfxMipmapFilterModeToWGPU(descriptor->mipmapFilter);

    wgpuDesc.lodMinClamp = descriptor->lodMinClamp;
    wgpuDesc.lodMaxClamp = descriptor->lodMaxClamp;
    wgpuDesc.maxAnisotropy = descriptor->maxAnisotropy;

    if (descriptor->compare) {
        wgpuDesc.compare = gfx::convertor::gfxCompareFunctionToWGPU(*descriptor->compare);
    }

    WGPUSampler wgpuSampler = wgpuDeviceCreateSampler(devicePtr->handle(), &wgpuDesc);
    if (!wgpuSampler) {
        return GFX_RESULT_ERROR_UNKNOWN;
    }

    auto* sampler = new gfx::webgpu::Sampler(wgpuSampler);
    *outSampler = reinterpret_cast<GfxSampler>(sampler);
    return GFX_RESULT_SUCCESS;
}

GfxResult WebGPUBackend::deviceCreateShader(GfxDevice device, const GfxShaderDescriptor* descriptor, GfxShader* outShader) const
{
    if (!device || !descriptor || !descriptor->code || !outShader) {
        return GFX_RESULT_ERROR_INVALID_PARAMETER;
    }

    auto* devicePtr = reinterpret_cast<gfx::webgpu::Device*>(device);

    WGPUShaderModuleDescriptor wgpuDesc = WGPU_SHADER_MODULE_DESCRIPTOR_INIT;
    if (descriptor->label) {
        wgpuDesc.label = gfx::convertor::gfxStringView(descriptor->label);
    }

    // Use explicit source type from descriptor
    if (descriptor->sourceType == GFX_SHADER_SOURCE_SPIRV) {
        // SPIR-V shader (binary)
        const uint32_t* codeU32 = static_cast<const uint32_t*>(descriptor->code);
        WGPUShaderSourceSPIRV spirvSource = WGPU_SHADER_SOURCE_SPIRV_INIT;
        spirvSource.code = codeU32;
        spirvSource.codeSize = descriptor->codeSize / sizeof(uint32_t);
        wgpuDesc.nextInChain = (WGPUChainedStruct*)&spirvSource;
    } else {
        // WGSL shader (text)
        const char* wgslCode = static_cast<const char*>(descriptor->code);
        WGPUShaderSourceWGSL wgslSource = WGPU_SHADER_SOURCE_WGSL_INIT;
        wgslSource.code = gfx::convertor::gfxStringView(wgslCode);
        wgpuDesc.nextInChain = (WGPUChainedStruct*)&wgslSource;
    }

    WGPUShaderModule wgpuModule = wgpuDeviceCreateShaderModule(devicePtr->handle(), &wgpuDesc);
    if (!wgpuModule) {
        return GFX_RESULT_ERROR_UNKNOWN;
    }

    auto* shader = new gfx::webgpu::Shader(wgpuModule);
    *outShader = reinterpret_cast<GfxShader>(shader);
    return GFX_RESULT_SUCCESS;
}

GfxResult WebGPUBackend::deviceCreateBindGroupLayout(GfxDevice device, const GfxBindGroupLayoutDescriptor* descriptor, GfxBindGroupLayout* outLayout) const
{
    if (!device || !descriptor || !outLayout) {
        return GFX_RESULT_ERROR_INVALID_PARAMETER;
    }

    auto* devicePtr = reinterpret_cast<gfx::webgpu::Device*>(device);

    WGPUBindGroupLayoutDescriptor wgpuDesc = WGPU_BIND_GROUP_LAYOUT_DESCRIPTOR_INIT;
    if (descriptor->label) {
        wgpuDesc.label = gfx::convertor::gfxStringView(descriptor->label);
    }

    std::vector<WGPUBindGroupLayoutEntry> entries;
    if (descriptor->entryCount > 0 && descriptor->entries) {
        entries.reserve(descriptor->entryCount);

        for (uint32_t i = 0; i < descriptor->entryCount; ++i) {
            const auto& entry = descriptor->entries[i];
            WGPUBindGroupLayoutEntry wgpuEntry = WGPU_BIND_GROUP_LAYOUT_ENTRY_INIT;

            wgpuEntry.binding = entry.binding;
            wgpuEntry.visibility = WGPUShaderStage_None;
            if (entry.visibility & GFX_SHADER_STAGE_VERTEX) {
                wgpuEntry.visibility |= WGPUShaderStage_Vertex;
            }
            if (entry.visibility & GFX_SHADER_STAGE_FRAGMENT) {
                wgpuEntry.visibility |= WGPUShaderStage_Fragment;
            }
            if (entry.visibility & GFX_SHADER_STAGE_COMPUTE) {
                wgpuEntry.visibility |= WGPUShaderStage_Compute;
            }

            switch (entry.type) {
            case GFX_BINDING_TYPE_BUFFER:
                wgpuEntry.buffer.type = WGPUBufferBindingType_Uniform;
                wgpuEntry.buffer.hasDynamicOffset = entry.buffer.hasDynamicOffset ? WGPU_TRUE : WGPU_FALSE;
                wgpuEntry.buffer.minBindingSize = entry.buffer.minBindingSize;
                break;
            case GFX_BINDING_TYPE_SAMPLER:
                wgpuEntry.sampler.type = entry.sampler.comparison ? WGPUSamplerBindingType_Comparison : WGPUSamplerBindingType_Filtering;
                break;
            case GFX_BINDING_TYPE_TEXTURE:
                wgpuEntry.texture.sampleType = gfx::convertor::gfxTextureSampleTypeToWGPU(entry.texture.sampleType);
                wgpuEntry.texture.viewDimension = gfx::convertor::gfxTextureViewTypeToWGPU(entry.texture.viewDimension);
                wgpuEntry.texture.multisampled = entry.texture.multisampled ? WGPU_TRUE : WGPU_FALSE;
                break;
            case GFX_BINDING_TYPE_STORAGE_TEXTURE:
                wgpuEntry.storageTexture.access = entry.storageTexture.writeOnly ? WGPUStorageTextureAccess_WriteOnly : WGPUStorageTextureAccess_ReadOnly;
                wgpuEntry.storageTexture.format = gfx::convertor::gfxFormatToWGPUFormat(entry.storageTexture.format);
                wgpuEntry.storageTexture.viewDimension = gfx::convertor::gfxTextureViewTypeToWGPU(entry.storageTexture.viewDimension);
                break;
            }

            entries.push_back(wgpuEntry);
        }

        wgpuDesc.entries = entries.data();
        wgpuDesc.entryCount = static_cast<uint32_t>(entries.size());
    }

    WGPUBindGroupLayout wgpuLayout = wgpuDeviceCreateBindGroupLayout(devicePtr->handle(), &wgpuDesc);
    if (!wgpuLayout) {
        return GFX_RESULT_ERROR_UNKNOWN;
    }

    auto* layout = new gfx::webgpu::BindGroupLayout(wgpuLayout);
    *outLayout = reinterpret_cast<GfxBindGroupLayout>(layout);
    return GFX_RESULT_SUCCESS;
}

GfxResult WebGPUBackend::deviceCreateBindGroup(GfxDevice device, const GfxBindGroupDescriptor* descriptor, GfxBindGroup* outBindGroup) const
{
    if (!device || !descriptor || !descriptor->layout || !outBindGroup) {
        return GFX_RESULT_ERROR_INVALID_PARAMETER;
    }

    auto* devicePtr = reinterpret_cast<gfx::webgpu::Device*>(device);
    auto* layoutPtr = reinterpret_cast<gfx::webgpu::BindGroupLayout*>(descriptor->layout);

    WGPUBindGroupDescriptor wgpuDesc = WGPU_BIND_GROUP_DESCRIPTOR_INIT;
    if (descriptor->label) {
        wgpuDesc.label = gfx::convertor::gfxStringView(descriptor->label);
    }
    wgpuDesc.layout = layoutPtr->handle();

    std::vector<WGPUBindGroupEntry> entries;
    if (descriptor->entryCount > 0 && descriptor->entries) {
        entries.reserve(descriptor->entryCount);

        for (uint32_t i = 0; i < descriptor->entryCount; ++i) {
            const auto& entry = descriptor->entries[i];
            WGPUBindGroupEntry wgpuEntry = WGPU_BIND_GROUP_ENTRY_INIT;

            wgpuEntry.binding = entry.binding;

            switch (entry.type) {
            case GFX_BIND_GROUP_ENTRY_TYPE_BUFFER: {
                auto* buffer = reinterpret_cast<gfx::webgpu::Buffer*>(entry.resource.buffer.buffer);
                wgpuEntry.buffer = buffer->handle();
                wgpuEntry.offset = entry.resource.buffer.offset;
                wgpuEntry.size = entry.resource.buffer.size;
                break;
            }
            case GFX_BIND_GROUP_ENTRY_TYPE_SAMPLER: {
                auto* sampler = reinterpret_cast<gfx::webgpu::Sampler*>(entry.resource.sampler);
                wgpuEntry.sampler = sampler->handle();
                break;
            }
            case GFX_BIND_GROUP_ENTRY_TYPE_TEXTURE_VIEW: {
                auto* textureView = reinterpret_cast<gfx::webgpu::TextureView*>(entry.resource.textureView);
                wgpuEntry.textureView = textureView->handle();
                break;
            }
            }

            entries.push_back(wgpuEntry);
        }

        wgpuDesc.entries = entries.data();
        wgpuDesc.entryCount = static_cast<uint32_t>(entries.size());
    }

    WGPUBindGroup wgpuBindGroup = wgpuDeviceCreateBindGroup(devicePtr->handle(), &wgpuDesc);
    if (!wgpuBindGroup) {
        return GFX_RESULT_ERROR_UNKNOWN;
    }

    auto* bindGroup = new gfx::webgpu::BindGroup(wgpuBindGroup);
    *outBindGroup = reinterpret_cast<GfxBindGroup>(bindGroup);
    return GFX_RESULT_SUCCESS;
}

GfxResult WebGPUBackend::deviceCreateRenderPipeline(GfxDevice device, const GfxRenderPipelineDescriptor* descriptor, GfxRenderPipeline* outPipeline) const
{
    if (!device || !descriptor || !outPipeline) {
        return GFX_RESULT_ERROR_INVALID_PARAMETER;
    }

    auto* devicePtr = reinterpret_cast<gfx::webgpu::Device*>(device);

    WGPURenderPipelineDescriptor wgpuDesc = WGPU_RENDER_PIPELINE_DESCRIPTOR_INIT;
    if (descriptor->label) {
        wgpuDesc.label = gfx::convertor::gfxStringView(descriptor->label);
    }

    // Create pipeline layout if bind group layouts are provided
    WGPUPipelineLayout pipelineLayout = nullptr;
    if (descriptor->bindGroupLayoutCount > 0 && descriptor->bindGroupLayouts) {
        std::vector<WGPUBindGroupLayout> wgpuBindGroupLayouts;
        wgpuBindGroupLayouts.reserve(descriptor->bindGroupLayoutCount);

        for (uint32_t i = 0; i < descriptor->bindGroupLayoutCount; ++i) {
            auto* layout = reinterpret_cast<gfx::webgpu::BindGroupLayout*>(descriptor->bindGroupLayouts[i]);
            wgpuBindGroupLayouts.push_back(layout->handle());
        }

        WGPUPipelineLayoutDescriptor layoutDesc = WGPU_PIPELINE_LAYOUT_DESCRIPTOR_INIT;
        layoutDesc.bindGroupLayouts = wgpuBindGroupLayouts.data();
        layoutDesc.bindGroupLayoutCount = static_cast<uint32_t>(wgpuBindGroupLayouts.size());

        pipelineLayout = wgpuDeviceCreatePipelineLayout(devicePtr->handle(), &layoutDesc);
        wgpuDesc.layout = pipelineLayout;
    }

    // Vertex state
    auto* vertexShader = reinterpret_cast<gfx::webgpu::Shader*>(descriptor->vertex->module);
    WGPUVertexState vertexState = WGPU_VERTEX_STATE_INIT;
    vertexState.module = vertexShader->handle();
    vertexState.entryPoint = gfx::convertor::gfxStringView(descriptor->vertex->entryPoint);

    fprintf(stderr, "[WebGPU] Creating pipeline - vertex entry: '%s'\n", descriptor->vertex->entryPoint ? descriptor->vertex->entryPoint : "NULL");

    // Convert vertex buffers
    std::vector<WGPUVertexBufferLayout> vertexBuffers;
    std::vector<std::vector<WGPUVertexAttribute>> allAttributes;

    if (descriptor->vertex->bufferCount > 0) {
        vertexBuffers.reserve(descriptor->vertex->bufferCount);
        allAttributes.reserve(descriptor->vertex->bufferCount);

        for (uint32_t i = 0; i < descriptor->vertex->bufferCount; ++i) {
            const auto& buffer = descriptor->vertex->buffers[i];

            std::vector<WGPUVertexAttribute> attributes;
            attributes.reserve(buffer.attributeCount);

            for (uint32_t j = 0; j < buffer.attributeCount; ++j) {
                const auto& attr = buffer.attributes[j];
                WGPUVertexAttribute wgpuAttr = WGPU_VERTEX_ATTRIBUTE_INIT;
                wgpuAttr.format = gfx::convertor::gfxFormatToWGPUVertexFormat(attr.format);
                wgpuAttr.offset = attr.offset;
                wgpuAttr.shaderLocation = attr.shaderLocation;
                attributes.push_back(wgpuAttr);
            }

            allAttributes.push_back(std::move(attributes));

            WGPUVertexBufferLayout wgpuBuffer = WGPU_VERTEX_BUFFER_LAYOUT_INIT;
            wgpuBuffer.arrayStride = buffer.arrayStride;
            wgpuBuffer.stepMode = buffer.stepModeInstance ? WGPUVertexStepMode_Instance : WGPUVertexStepMode_Vertex;
            wgpuBuffer.attributes = allAttributes.back().data();
            wgpuBuffer.attributeCount = static_cast<uint32_t>(allAttributes.back().size());
            vertexBuffers.push_back(wgpuBuffer);
        }

        vertexState.buffers = vertexBuffers.data();
        vertexState.bufferCount = static_cast<uint32_t>(vertexBuffers.size());
    }

    wgpuDesc.vertex = vertexState;

    // Fragment state (optional)
    WGPUFragmentState fragmentState = WGPU_FRAGMENT_STATE_INIT;
    std::vector<WGPUColorTargetState> colorTargets;
    std::vector<WGPUBlendState> blendStates;

    if (descriptor->fragment) {
        auto* fragmentShader = reinterpret_cast<gfx::webgpu::Shader*>(descriptor->fragment->module);
        fragmentState.module = fragmentShader->handle();
        fragmentState.entryPoint = gfx::convertor::gfxStringView(descriptor->fragment->entryPoint);

        fprintf(stderr, "[WebGPU] Creating pipeline - fragment entry: '%s'\n", descriptor->fragment->entryPoint ? descriptor->fragment->entryPoint : "NULL");

        if (descriptor->fragment->targetCount > 0) {
            colorTargets.reserve(descriptor->fragment->targetCount);

            for (uint32_t i = 0; i < descriptor->fragment->targetCount; ++i) {
                const auto& target = descriptor->fragment->targets[i];
                WGPUColorTargetState wgpuTarget = WGPU_COLOR_TARGET_STATE_INIT;
                wgpuTarget.format = gfx::convertor::gfxFormatToWGPUFormat(target.format);
                wgpuTarget.writeMask = target.writeMask;

                if (target.blend) {
                    WGPUBlendState blend = WGPU_BLEND_STATE_INIT;

                    // Color blend
                    blend.color.operation = gfx::convertor::gfxBlendOperationToWGPU(target.blend->color.operation);
                    blend.color.srcFactor = gfx::convertor::gfxBlendFactorToWGPU(target.blend->color.srcFactor);
                    blend.color.dstFactor = gfx::convertor::gfxBlendFactorToWGPU(target.blend->color.dstFactor);

                    // Alpha blend
                    blend.alpha.operation = gfx::convertor::gfxBlendOperationToWGPU(target.blend->alpha.operation);
                    blend.alpha.srcFactor = gfx::convertor::gfxBlendFactorToWGPU(target.blend->alpha.srcFactor);
                    blend.alpha.dstFactor = gfx::convertor::gfxBlendFactorToWGPU(target.blend->alpha.dstFactor);

                    blendStates.push_back(blend);
                    wgpuTarget.blend = &blendStates.back();
                }

                colorTargets.push_back(wgpuTarget);
            }

            fragmentState.targets = colorTargets.data();
            fragmentState.targetCount = static_cast<uint32_t>(colorTargets.size());
        }

        wgpuDesc.fragment = &fragmentState;
    }

    // Primitive state
    WGPUPrimitiveState primitiveState = WGPU_PRIMITIVE_STATE_INIT;
    primitiveState.topology = gfx::convertor::gfxPrimitiveTopologyToWGPU(descriptor->primitive->topology);
    primitiveState.frontFace = gfx::convertor::gfxFrontFaceToWGPU(descriptor->primitive->frontFace);
    primitiveState.cullMode = gfx::convertor::gfxCullModeToWGPU(descriptor->primitive->cullMode);

    if (descriptor->primitive->stripIndexFormat) {
        primitiveState.stripIndexFormat = gfx::convertor::gfxIndexFormatToWGPU(*descriptor->primitive->stripIndexFormat);
    }

    wgpuDesc.primitive = primitiveState;

    // Depth/stencil state (optional)
    WGPUDepthStencilState depthStencilState = WGPU_DEPTH_STENCIL_STATE_INIT;
    if (descriptor->depthStencil) {
        depthStencilState.format = gfx::convertor::gfxFormatToWGPUFormat(descriptor->depthStencil->format);
        depthStencilState.depthWriteEnabled = descriptor->depthStencil->depthWriteEnabled ? WGPUOptionalBool_True : WGPUOptionalBool_False;
        depthStencilState.depthCompare = gfx::convertor::gfxCompareFunctionToWGPU(descriptor->depthStencil->depthCompare);

        // Only configure stencil ops for formats that have a stencil aspect
        // For depth-only formats, the INIT macro sets sensible defaults (Always/Keep)
        if (gfx::convertor::formatHasStencil(descriptor->depthStencil->format)) {
            // Stencil front
            depthStencilState.stencilFront.compare = gfx::convertor::gfxCompareFunctionToWGPU(descriptor->depthStencil->stencilFront.compare);
            depthStencilState.stencilFront.failOp = gfx::convertor::gfxStencilOperationToWGPU(descriptor->depthStencil->stencilFront.failOp);
            depthStencilState.stencilFront.depthFailOp = gfx::convertor::gfxStencilOperationToWGPU(descriptor->depthStencil->stencilFront.depthFailOp);
            depthStencilState.stencilFront.passOp = gfx::convertor::gfxStencilOperationToWGPU(descriptor->depthStencil->stencilFront.passOp);

            // Stencil back
            depthStencilState.stencilBack.compare = gfx::convertor::gfxCompareFunctionToWGPU(descriptor->depthStencil->stencilBack.compare);
            depthStencilState.stencilBack.failOp = gfx::convertor::gfxStencilOperationToWGPU(descriptor->depthStencil->stencilBack.failOp);
            depthStencilState.stencilBack.depthFailOp = gfx::convertor::gfxStencilOperationToWGPU(descriptor->depthStencil->stencilBack.depthFailOp);
            depthStencilState.stencilBack.passOp = gfx::convertor::gfxStencilOperationToWGPU(descriptor->depthStencil->stencilBack.passOp);

            depthStencilState.stencilReadMask = descriptor->depthStencil->stencilReadMask;
            depthStencilState.stencilWriteMask = descriptor->depthStencil->stencilWriteMask;
        }

        depthStencilState.depthBias = descriptor->depthStencil->depthBias;
        depthStencilState.depthBiasSlopeScale = descriptor->depthStencil->depthBiasSlopeScale;
        depthStencilState.depthBiasClamp = descriptor->depthStencil->depthBiasClamp;

        wgpuDesc.depthStencil = &depthStencilState;
    }

    // Multisample state
    WGPUMultisampleState multisampleState = WGPU_MULTISAMPLE_STATE_INIT;
    multisampleState.count = descriptor->sampleCount;
    wgpuDesc.multisample = multisampleState;

    WGPURenderPipeline wgpuPipeline = wgpuDeviceCreateRenderPipeline(devicePtr->handle(), &wgpuDesc);

    // Release the pipeline layout if we created one (pipeline holds its own reference)
    if (pipelineLayout) {
        wgpuPipelineLayoutRelease(pipelineLayout);
    }

    if (!wgpuPipeline) {
        return GFX_RESULT_ERROR_UNKNOWN;
    }

    auto* pipeline = new gfx::webgpu::RenderPipeline(wgpuPipeline);
    *outPipeline = reinterpret_cast<GfxRenderPipeline>(pipeline);
    return GFX_RESULT_SUCCESS;
}

GfxResult WebGPUBackend::deviceCreateComputePipeline(GfxDevice device, const GfxComputePipelineDescriptor* descriptor, GfxComputePipeline* outPipeline) const
{
    if (!device || !descriptor || !descriptor->compute || !outPipeline) {
        return GFX_RESULT_ERROR_INVALID_PARAMETER;
    }

    auto* devicePtr = reinterpret_cast<gfx::webgpu::Device*>(device);
    auto* shader = reinterpret_cast<gfx::webgpu::Shader*>(descriptor->compute);

    WGPUComputePipelineDescriptor wgpuDesc = WGPU_COMPUTE_PIPELINE_DESCRIPTOR_INIT;
    if (descriptor->label) {
        wgpuDesc.label = gfx::convertor::gfxStringView(descriptor->label);
    }

    // Create pipeline layout if bind group layouts are provided
    WGPUPipelineLayout pipelineLayout = nullptr;
    if (descriptor->bindGroupLayoutCount > 0 && descriptor->bindGroupLayouts) {
        std::vector<WGPUBindGroupLayout> wgpuBindGroupLayouts;
        wgpuBindGroupLayouts.reserve(descriptor->bindGroupLayoutCount);

        for (uint32_t i = 0; i < descriptor->bindGroupLayoutCount; ++i) {
            auto* layout = reinterpret_cast<gfx::webgpu::BindGroupLayout*>(descriptor->bindGroupLayouts[i]);
            wgpuBindGroupLayouts.push_back(layout->handle());
        }

        WGPUPipelineLayoutDescriptor layoutDesc = WGPU_PIPELINE_LAYOUT_DESCRIPTOR_INIT;
        layoutDesc.bindGroupLayouts = wgpuBindGroupLayouts.data();
        layoutDesc.bindGroupLayoutCount = static_cast<uint32_t>(wgpuBindGroupLayouts.size());

        pipelineLayout = wgpuDeviceCreatePipelineLayout(devicePtr->handle(), &layoutDesc);
        wgpuDesc.layout = pipelineLayout;
    }

    wgpuDesc.compute.module = shader->handle();
    wgpuDesc.compute.entryPoint = gfx::convertor::gfxStringView(descriptor->entryPoint);

    WGPUComputePipeline wgpuPipeline = wgpuDeviceCreateComputePipeline(devicePtr->handle(), &wgpuDesc);

    // Release the pipeline layout if we created one (pipeline holds its own reference)
    if (pipelineLayout) {
        wgpuPipelineLayoutRelease(pipelineLayout);
    }

    if (!wgpuPipeline) {
        return GFX_RESULT_ERROR_UNKNOWN;
    }

    auto* pipeline = new gfx::webgpu::ComputePipeline(wgpuPipeline);
    *outPipeline = reinterpret_cast<GfxComputePipeline>(pipeline);
    return GFX_RESULT_SUCCESS;
}

GfxResult WebGPUBackend::deviceCreateCommandEncoder(GfxDevice device, const GfxCommandEncoderDescriptor* descriptor, GfxCommandEncoder* outEncoder) const
{
    if (!device || !descriptor || !outEncoder) {
        return GFX_RESULT_ERROR_INVALID_PARAMETER;
    }

    auto* devicePtr = reinterpret_cast<gfx::webgpu::Device*>(device);

    WGPUCommandEncoderDescriptor wgpuDesc = WGPU_COMMAND_ENCODER_DESCRIPTOR_INIT;
    if (descriptor->label) {
        wgpuDesc.label = gfx::convertor::gfxStringView(descriptor->label);
    }

    WGPUCommandEncoder wgpuEncoder = wgpuDeviceCreateCommandEncoder(devicePtr->handle(), &wgpuDesc);
    if (!wgpuEncoder) {
        return GFX_RESULT_ERROR_UNKNOWN;
    }

    auto* encoder = new gfx::webgpu::CommandEncoder(devicePtr->handle(), wgpuEncoder);
    *outEncoder = reinterpret_cast<GfxCommandEncoder>(encoder);
    return GFX_RESULT_SUCCESS;
}

GfxResult WebGPUBackend::deviceCreateFence(GfxDevice device, const GfxFenceDescriptor* descriptor, GfxFence* outFence) const
{
    if (!device || !descriptor || !outFence) {
        return GFX_RESULT_ERROR_INVALID_PARAMETER;
    }

    auto* fence = new gfx::webgpu::Fence(descriptor->signaled);
    *outFence = reinterpret_cast<GfxFence>(fence);
    return GFX_RESULT_SUCCESS;
}

GfxResult WebGPUBackend::deviceCreateSemaphore(GfxDevice device, const GfxSemaphoreDescriptor* descriptor, GfxSemaphore* outSemaphore) const
{
    if (!device || !descriptor || !outSemaphore) {
        return GFX_RESULT_ERROR_INVALID_PARAMETER;
    }

    auto semaphoreType = gfx::convertor::gfxSemaphoreTypeToWebGPUSemaphoreType(descriptor->type);
    auto* semaphore = new gfx::webgpu::Semaphore(semaphoreType, descriptor->initialValue);
    *outSemaphore = reinterpret_cast<GfxSemaphore>(semaphore);
    return GFX_RESULT_SUCCESS;
}

void WebGPUBackend::deviceWaitIdle(GfxDevice device) const
{
    if (!device) {
        return;
    }
    auto* devicePtr = reinterpret_cast<gfx::webgpu::Device*>(device);

    // Device always has valid queue, adapter, and instance
    WGPUQueueWorkDoneCallbackInfo callbackInfo = WGPU_QUEUE_WORK_DONE_CALLBACK_INFO_INIT;
    callbackInfo.mode = WGPUCallbackMode_WaitAnyOnly;
    callbackInfo.callback = queueWorkDoneCallback;
    WGPUFuture future = wgpuQueueOnSubmittedWorkDone(devicePtr->getQueue()->handle(), callbackInfo);

    // Wait for the work to complete
    WGPUInstance instance = devicePtr->getAdapter()->getInstance()->handle();
    WGPUFutureWaitInfo waitInfo = WGPU_FUTURE_WAIT_INFO_INIT;
    waitInfo.future = future;
    wgpuInstanceWaitAny(instance, 1, &waitInfo, UINT64_MAX);
}

void WebGPUBackend::devicePoll(GfxDevice device) const
{
    if (!device) {
        return;
    }
    auto* devicePtr = reinterpret_cast<gfx::webgpu::Device*>(device);

    // Call ProcessEvents on the instance to handle async operations
    wgpuInstanceProcessEvents(devicePtr->getAdapter()->getInstance()->handle());
}

void WebGPUBackend::deviceGetLimits(GfxDevice device, GfxDeviceLimits* outLimits) const
{
    if (!device || !outLimits) {
        return;
    }

    auto* devicePtr = reinterpret_cast<gfx::webgpu::Device*>(device);

    WGPULimits limits = WGPU_LIMITS_INIT;
    WGPUStatus status = wgpuDeviceGetLimits(devicePtr->handle(), &limits);
    if (status == WGPUStatus_Success) {
        outLimits->minUniformBufferOffsetAlignment = limits.minUniformBufferOffsetAlignment;
        outLimits->minStorageBufferOffsetAlignment = limits.minStorageBufferOffsetAlignment;
        outLimits->maxUniformBufferBindingSize = static_cast<uint32_t>(limits.maxUniformBufferBindingSize);
        outLimits->maxStorageBufferBindingSize = static_cast<uint32_t>(limits.maxStorageBufferBindingSize);
        outLimits->maxBufferSize = limits.maxBufferSize;
        outLimits->maxTextureDimension1D = limits.maxTextureDimension1D;
        outLimits->maxTextureDimension2D = limits.maxTextureDimension2D;
        outLimits->maxTextureDimension3D = limits.maxTextureDimension3D;
        outLimits->maxTextureArrayLayers = limits.maxTextureArrayLayers;
    } else {
        // Fallback to reasonable defaults if query fails
        outLimits->minUniformBufferOffsetAlignment = 256;
        outLimits->minStorageBufferOffsetAlignment = 256;
        outLimits->maxUniformBufferBindingSize = 65536;
        outLimits->maxStorageBufferBindingSize = 134217728;
        outLimits->maxBufferSize = 268435456;
        outLimits->maxTextureDimension1D = 8192;
        outLimits->maxTextureDimension2D = 8192;
        outLimits->maxTextureDimension3D = 2048;
        outLimits->maxTextureArrayLayers = 256;
    }
}

// Surface functions
void WebGPUBackend::surfaceDestroy(GfxSurface surface) const
{
    if (!surface) {
        return;
    }
    delete reinterpret_cast<gfx::webgpu::Surface*>(surface);
}

uint32_t WebGPUBackend::surfaceGetSupportedFormats(GfxSurface surface, GfxTextureFormat* formats, uint32_t maxFormats) const
{
    if (!surface) {
        return 0;
    }

    auto* surf = reinterpret_cast<gfx::webgpu::Surface*>(surface);

    // Query surface capabilities
    WGPUSurfaceCapabilities capabilities = WGPU_SURFACE_CAPABILITIES_INIT;
    wgpuSurfaceGetCapabilities(surf->handle(), surf->adapter(), &capabilities);

    uint32_t formatCount = static_cast<uint32_t>(capabilities.formatCount);

    if (formatCount == 0) {
        wgpuSurfaceCapabilitiesFreeMembers(capabilities);
        return 0;
    }

    // Convert to GfxTextureFormat
    if (formats && maxFormats > 0) {
        uint32_t copyCount = std::min(formatCount, maxFormats);
        for (uint32_t i = 0; i < copyCount; ++i) {
            formats[i] = gfx::convertor::wgpuFormatToGfxFormat(capabilities.formats[i]);
        }
    }

    wgpuSurfaceCapabilitiesFreeMembers(capabilities);
    return formatCount;
}

uint32_t WebGPUBackend::surfaceGetSupportedPresentModes(GfxSurface surface, GfxPresentMode* presentModes, uint32_t maxModes) const
{
    if (!surface) {
        return 0;
    }

    auto* surf = reinterpret_cast<gfx::webgpu::Surface*>(surface);

    // Query surface capabilities
    WGPUSurfaceCapabilities capabilities = WGPU_SURFACE_CAPABILITIES_INIT;
    wgpuSurfaceGetCapabilities(surf->handle(), surf->adapter(), &capabilities);

    uint32_t modeCount = static_cast<uint32_t>(capabilities.presentModeCount);

    if (modeCount == 0) {
        wgpuSurfaceCapabilitiesFreeMembers(capabilities);
        return 0;
    }

    // Convert to GfxPresentMode
    if (presentModes && maxModes > 0) {
        uint32_t copyCount = std::min(modeCount, maxModes);
        for (uint32_t i = 0; i < copyCount; ++i) {
            presentModes[i] = gfx::convertor::wgpuPresentModeToGfxPresentMode(capabilities.presentModes[i]);
        }
    }

    wgpuSurfaceCapabilitiesFreeMembers(capabilities);
    return modeCount;
}

// Swapchain functions
void WebGPUBackend::swapchainDestroy(GfxSwapchain swapchain) const
{
    if (!swapchain) {
        return;
    }
    delete reinterpret_cast<gfx::webgpu::Swapchain*>(swapchain);
}

uint32_t WebGPUBackend::swapchainGetWidth(GfxSwapchain swapchain) const
{
    if (!swapchain) {
        return 0;
    }
    return reinterpret_cast<gfx::webgpu::Swapchain*>(swapchain)->getWidth();
}

uint32_t WebGPUBackend::swapchainGetHeight(GfxSwapchain swapchain) const
{
    if (!swapchain) {
        return 0;
    }
    return reinterpret_cast<gfx::webgpu::Swapchain*>(swapchain)->getHeight();
}

GfxTextureFormat WebGPUBackend::swapchainGetFormat(GfxSwapchain swapchain) const
{
    if (!swapchain) {
        return GFX_TEXTURE_FORMAT_UNDEFINED;
    }
    auto* swapchainPtr = reinterpret_cast<gfx::webgpu::Swapchain*>(swapchain);
    return gfx::convertor::wgpuFormatToGfxFormat(swapchainPtr->getFormat());
}

uint32_t WebGPUBackend::swapchainGetBufferCount(GfxSwapchain swapchain) const
{
    if (!swapchain) {
        return 0;
    }
    return reinterpret_cast<gfx::webgpu::Swapchain*>(swapchain)->getBufferCount();
}

GfxResult WebGPUBackend::swapchainAcquireNextImage(GfxSwapchain swapchain, uint64_t timeoutNs, GfxSemaphore imageAvailableSemaphore, GfxFence fence, uint32_t* outImageIndex) const
{
    if (!swapchain || !outImageIndex) {
        return GFX_RESULT_ERROR_INVALID_PARAMETER;
    }

    // WebGPU doesn't have explicit acquire semantics with semaphores
    // The surface texture is acquired implicitly when we call wgpuSurfaceGetCurrentTexture
    // For now, we just return image index 0 (WebGPU doesn't expose multiple image indices)
    // The semaphore and fence are noted but WebGPU doesn't support explicit synchronization primitives

    (void)timeoutNs;
    (void)imageAvailableSemaphore; // WebGPU doesn't support explicit semaphore signaling

    auto* swapchainPtr = reinterpret_cast<gfx::webgpu::Swapchain*>(swapchain);

    // Get current texture - cache it for the frame
    WGPUSurfaceTexture surfaceTexture = WGPU_SURFACE_TEXTURE_INIT;
    wgpuSurfaceGetCurrentTexture(swapchainPtr->surface(), &surfaceTexture);

    GfxResult result = GFX_RESULT_SUCCESS;
    switch (surfaceTexture.status) {
    case WGPUSurfaceGetCurrentTextureStatus_SuccessOptimal:
    case WGPUSurfaceGetCurrentTextureStatus_SuccessSuboptimal:
        *outImageIndex = 0; // WebGPU only exposes current image
        // Cache the texture - it will be released after present
        swapchainPtr->setCurrentTexture(surfaceTexture.texture);
        result = GFX_RESULT_SUCCESS;
        break;
    case WGPUSurfaceGetCurrentTextureStatus_Timeout:
        result = GFX_RESULT_TIMEOUT;
        if (surfaceTexture.texture) {
            wgpuTextureRelease(surfaceTexture.texture);
        }
        break;
    case WGPUSurfaceGetCurrentTextureStatus_Outdated:
        result = GFX_RESULT_ERROR_OUT_OF_DATE;
        if (surfaceTexture.texture) {
            wgpuTextureRelease(surfaceTexture.texture);
        }
        break;
    case WGPUSurfaceGetCurrentTextureStatus_Lost:
        result = GFX_RESULT_ERROR_SURFACE_LOST;
        if (surfaceTexture.texture) {
            wgpuTextureRelease(surfaceTexture.texture);
        }
        break;
    default:
        result = GFX_RESULT_ERROR_UNKNOWN;
        if (surfaceTexture.texture) {
            wgpuTextureRelease(surfaceTexture.texture);
        }
        break;
    }

    // Signal fence if provided (even though WebGPU doesn't truly have fences)
    if (fence && result == GFX_RESULT_SUCCESS) {
        auto* fencePtr = reinterpret_cast<gfx::webgpu::Fence*>(fence);
        fencePtr->setSignaled(true);
    }

    return result;
}

GfxTextureView WebGPUBackend::swapchainGetImageView(GfxSwapchain swapchain, uint32_t imageIndex) const
{
    if (!swapchain) {
        return nullptr;
    }

    // WebGPU doesn't expose multiple swapchain images by index
    // Always return the current texture view regardless of index
    (void)imageIndex;

    return swapchainGetCurrentTextureView(swapchain);
}

GfxTextureView WebGPUBackend::swapchainGetCurrentTextureView(GfxSwapchain swapchain) const
{
    if (!swapchain) {
        return nullptr;
    }

    auto* swapchainPtr = reinterpret_cast<gfx::webgpu::Swapchain*>(swapchain);

    // Return cached view if already created
    if (swapchainPtr->getCurrentView()) {
        return reinterpret_cast<GfxTextureView>(swapchainPtr->getCurrentView());
    }

    // Use the cached texture from acquire - don't call wgpuSurfaceGetCurrentTexture again!
    // Dawn expects GetCurrentTexture to be called only ONCE per frame
    WGPUTexture texture = swapchainPtr->getCurrentTexture();
    if (!texture) {
        fprintf(stderr, "[WebGPU] No cached texture available! Call AcquireNextImage first.\\n");
        return nullptr;
    }

    // Create texture view with nullptr descriptor - let Dawn infer the format from texture
    // This matches the working Dawn app behavior
    WGPUTextureView wgpuView = wgpuTextureCreateView(texture, nullptr);
    if (!wgpuView) {
        fprintf(stderr, "[WebGPU] Failed to create texture view\\n");
        return nullptr;
    }

    // Cache the view in the swapchain - it will be destroyed on next acquire or swapchain destruction
    auto* view = new gfx::webgpu::TextureView(wgpuView, nullptr);
    swapchainPtr->setCurrentView(view);
    return reinterpret_cast<GfxTextureView>(view);
}

GfxResult WebGPUBackend::swapchainPresent(GfxSwapchain swapchain, const GfxPresentInfo* presentInfo) const
{
    if (!swapchain) {
        return GFX_RESULT_ERROR_INVALID_PARAMETER;
    }

    // WebGPU doesn't support explicit wait semaphores for present
    // The queue submission already ensures ordering, so we just present
    (void)presentInfo; // Wait semaphores are noted but not used in WebGPU

    auto* swapchainPtr = reinterpret_cast<gfx::webgpu::Swapchain*>(swapchain);

#ifndef __EMSCRIPTEN__
    // Present the surface - Dawn will present the texture we have been rendering to
    // Note: In Emscripten, presentation is automatic via requestAnimationFrame
    wgpuSurfacePresent(swapchainPtr->surface());
#endif

    // Release the cached texture after presenting
    swapchainPtr->setCurrentTexture(nullptr);

    return GFX_RESULT_SUCCESS;
}

// Buffer functions
void WebGPUBackend::bufferDestroy(GfxBuffer buffer) const
{
    if (!buffer) {
        return;
    }
    delete reinterpret_cast<gfx::webgpu::Buffer*>(buffer);
}

uint64_t WebGPUBackend::bufferGetSize(GfxBuffer buffer) const
{
    if (!buffer) {
        return 0;
    }
    return reinterpret_cast<gfx::webgpu::Buffer*>(buffer)->getSize();
}

GfxBufferUsage WebGPUBackend::bufferGetUsage(GfxBuffer buffer) const
{
    if (!buffer) {
        return GFX_BUFFER_USAGE_NONE;
    }
    auto usage = reinterpret_cast<gfx::webgpu::Buffer*>(buffer)->getUsage();
    return gfx::convertor::webgpuBufferUsageToGfxBufferUsage(usage);
}

GfxResult WebGPUBackend::bufferMap(GfxBuffer buffer, uint64_t offset, uint64_t size, void** outMappedPointer) const
{
    if (!buffer || !outMappedPointer) {
        return GFX_RESULT_ERROR_INVALID_PARAMETER;
    }

    auto* bufferPtr = reinterpret_cast<gfx::webgpu::Buffer*>(buffer);

    // If size is 0, map the entire buffer from offset
    uint64_t mapSize = size;
    if (mapSize == 0) {
        mapSize = bufferPtr->getSize() - offset;
    }

    // Determine map mode based on buffer usage
    WGPUMapMode mapMode = WGPUMapMode_None;
    if (bufferPtr->getUsage() & GFX_BUFFER_USAGE_MAP_READ) {
        mapMode |= WGPUMapMode_Read;
    }
    if (bufferPtr->getUsage() & GFX_BUFFER_USAGE_MAP_WRITE) {
        mapMode |= WGPUMapMode_Write;
    }

    if (mapMode == WGPUMapMode_None) {
        return GFX_RESULT_ERROR_INVALID_PARAMETER;
    }

    // Set up async mapping with synchronous wait
    MapCallbackData callbackData;

    WGPUBufferMapCallbackInfo callbackInfo = WGPU_BUFFER_MAP_CALLBACK_INFO_INIT;
    callbackInfo.mode = WGPUCallbackMode_WaitAnyOnly;
    callbackInfo.callback = bufferMapCallback;
    callbackInfo.userdata1 = &callbackData;

    WGPUFuture future = wgpuBufferMapAsync(bufferPtr->handle(), mapMode, offset, mapSize, callbackInfo);

    // Properly wait for the mapping to complete
    if (bufferPtr->getDevice() && bufferPtr->getDevice()->getAdapter() && bufferPtr->getDevice()->getAdapter()->getInstance()) {
        WGPUFutureWaitInfo waitInfo = WGPU_FUTURE_WAIT_INFO_INIT;
        waitInfo.future = future;
        wgpuInstanceWaitAny(bufferPtr->getDevice()->getAdapter()->getInstance()->handle(), 1, &waitInfo, UINT64_MAX);
    }

    if (!callbackData.completed || callbackData.status != WGPUMapAsyncStatus_Success) {
        return GFX_RESULT_ERROR_UNKNOWN;
    }

    // Get the mapped range
    void* mappedData = wgpuBufferGetMappedRange(bufferPtr->handle(), offset, mapSize);
    if (!mappedData) {
        wgpuBufferUnmap(bufferPtr->handle());
        return GFX_RESULT_ERROR_UNKNOWN;
    }

    *outMappedPointer = mappedData;
    return GFX_RESULT_SUCCESS;
}

void WebGPUBackend::bufferUnmap(GfxBuffer buffer) const
{
    if (!buffer) {
        return;
    }
    auto* bufferPtr = reinterpret_cast<gfx::webgpu::Buffer*>(buffer);
    wgpuBufferUnmap(bufferPtr->handle());
}

// Texture functions
void WebGPUBackend::textureDestroy(GfxTexture texture) const
{
    if (!texture) {
        return;
    }
    delete reinterpret_cast<gfx::webgpu::Texture*>(texture);
}

GfxExtent3D WebGPUBackend::textureGetSize(GfxTexture texture) const
{
    if (!texture) {
        return { 0, 0, 0 };
    }
    auto* texturePtr = reinterpret_cast<gfx::webgpu::Texture*>(texture);
    WGPUExtent3D size = texturePtr->getSize();
    return { size.width, size.height, size.depthOrArrayLayers };
}

GfxTextureFormat WebGPUBackend::textureGetFormat(GfxTexture texture) const
{
    if (!texture) {
        return GFX_TEXTURE_FORMAT_UNDEFINED;
    }
    auto* texturePtr = reinterpret_cast<gfx::webgpu::Texture*>(texture);
    return gfx::convertor::wgpuFormatToGfxFormat(texturePtr->getFormat());
}

uint32_t WebGPUBackend::textureGetMipLevelCount(GfxTexture texture) const
{
    if (!texture) {
        return 0;
    }
    return reinterpret_cast<gfx::webgpu::Texture*>(texture)->getMipLevels();
}

GfxSampleCount WebGPUBackend::textureGetSampleCount(GfxTexture texture) const
{
    if (!texture) {
        return GFX_SAMPLE_COUNT_1;
    }
    return static_cast<GfxSampleCount>(reinterpret_cast<gfx::webgpu::Texture*>(texture)->getSampleCount());
}

GfxTextureUsage WebGPUBackend::textureGetUsage(GfxTexture texture) const
{
    if (!texture) {
        return GFX_TEXTURE_USAGE_NONE;
    }

    auto* texturePtr = reinterpret_cast<gfx::webgpu::Texture*>(texture);
    WGPUTextureUsage wgpuUsage = texturePtr->getUsage();

    uint32_t usage = GFX_TEXTURE_USAGE_NONE;
    if (wgpuUsage & WGPUTextureUsage_CopySrc) {
        usage |= GFX_TEXTURE_USAGE_COPY_SRC;
    }
    if (wgpuUsage & WGPUTextureUsage_CopyDst) {
        usage |= GFX_TEXTURE_USAGE_COPY_DST;
    }
    if (wgpuUsage & WGPUTextureUsage_TextureBinding) {
        usage |= GFX_TEXTURE_USAGE_TEXTURE_BINDING;
    }
    if (wgpuUsage & WGPUTextureUsage_StorageBinding) {
        usage |= GFX_TEXTURE_USAGE_STORAGE_BINDING;
    }
    if (wgpuUsage & WGPUTextureUsage_RenderAttachment) {
        usage |= GFX_TEXTURE_USAGE_RENDER_ATTACHMENT;
    }

    return static_cast<GfxTextureUsage>(usage);
}

GfxTextureLayout WebGPUBackend::textureGetLayout(GfxTexture texture) const
{
    // WebGPU doesn't have explicit layouts, return GENERAL as a reasonable default
    if (!texture) {
        return GFX_TEXTURE_LAYOUT_UNDEFINED;
    }
    return GFX_TEXTURE_LAYOUT_GENERAL;
}

GfxResult WebGPUBackend::textureCreateView(GfxTexture texture, const GfxTextureViewDescriptor* descriptor, GfxTextureView* outView) const
{
    if (!texture || !outView) {
        return GFX_RESULT_ERROR_INVALID_PARAMETER;
    }

    auto* texturePtr = reinterpret_cast<gfx::webgpu::Texture*>(texture);

    WGPUTextureViewDescriptor wgpuDesc = WGPU_TEXTURE_VIEW_DESCRIPTOR_INIT;
    if (descriptor) {
        if (descriptor->label) {
            wgpuDesc.label = gfx::convertor::gfxStringView(descriptor->label);
        }
        wgpuDesc.dimension = gfx::convertor::gfxTextureViewTypeToWGPU(descriptor->viewType);
        wgpuDesc.format = gfx::convertor::gfxFormatToWGPUFormat(descriptor->format);
        wgpuDesc.baseMipLevel = descriptor->baseMipLevel;
        wgpuDesc.mipLevelCount = descriptor->mipLevelCount;
        wgpuDesc.baseArrayLayer = descriptor->baseArrayLayer;
        wgpuDesc.arrayLayerCount = descriptor->arrayLayerCount;
    }

    WGPUTextureView wgpuView = wgpuTextureCreateView(texturePtr->handle(), &wgpuDesc);
    if (!wgpuView) {
        return GFX_RESULT_ERROR_UNKNOWN;
    }

    auto* view = new gfx::webgpu::TextureView(wgpuView, texturePtr);
    *outView = reinterpret_cast<GfxTextureView>(view);
    return GFX_RESULT_SUCCESS;
}

// TextureView functions
void WebGPUBackend::textureViewDestroy(GfxTextureView textureView) const
{
    if (!textureView) {
        return;
    }
    delete reinterpret_cast<gfx::webgpu::TextureView*>(textureView);
}

// Sampler functions
void WebGPUBackend::samplerDestroy(GfxSampler sampler) const
{
    if (!sampler) {
        return;
    }
    delete reinterpret_cast<gfx::webgpu::Sampler*>(sampler);
}

// Shader functions
void WebGPUBackend::shaderDestroy(GfxShader shader) const
{
    if (!shader) {
        return;
    }
    delete reinterpret_cast<gfx::webgpu::Shader*>(shader);
}

// BindGroupLayout functions
void WebGPUBackend::bindGroupLayoutDestroy(GfxBindGroupLayout bindGroupLayout) const
{
    if (!bindGroupLayout) {
        return;
    }
    delete reinterpret_cast<gfx::webgpu::BindGroupLayout*>(bindGroupLayout);
}

// BindGroup functions
void WebGPUBackend::bindGroupDestroy(GfxBindGroup bindGroup) const
{
    if (!bindGroup) {
        return;
    }
    delete reinterpret_cast<gfx::webgpu::BindGroup*>(bindGroup);
}

// RenderPipeline functions
void WebGPUBackend::renderPipelineDestroy(GfxRenderPipeline renderPipeline) const
{
    if (!renderPipeline) {
        return;
    }
    delete reinterpret_cast<gfx::webgpu::RenderPipeline*>(renderPipeline);
}

// ComputePipeline functions
void WebGPUBackend::computePipelineDestroy(GfxComputePipeline computePipeline) const
{
    if (!computePipeline) {
        return;
    }
    delete reinterpret_cast<gfx::webgpu::ComputePipeline*>(computePipeline);
}

// Queue functions
GfxResult WebGPUBackend::queueSubmit(GfxQueue queue, const GfxSubmitInfo* submitInfo) const
{
    if (!queue || !submitInfo) {
        return GFX_RESULT_ERROR_INVALID_PARAMETER;
    }

    auto* queuePtr = reinterpret_cast<gfx::webgpu::Queue*>(queue);

    // WebGPU doesn't support semaphore-based sync - just submit command buffers
    for (uint32_t i = 0; i < submitInfo->commandEncoderCount; ++i) {
        if (submitInfo->commandEncoders[i]) {
            auto* encoderPtr = reinterpret_cast<gfx::webgpu::CommandEncoder*>(submitInfo->commandEncoders[i]);

            WGPUCommandBufferDescriptor cmdDesc = WGPU_COMMAND_BUFFER_DESCRIPTOR_INIT;
            WGPUCommandBuffer cmdBuffer = wgpuCommandEncoderFinish(encoderPtr->handle(), &cmdDesc);

            if (cmdBuffer) {
                wgpuQueueSubmit(queuePtr->handle(), 1, &cmdBuffer);
                wgpuCommandBufferRelease(cmdBuffer);

                // Mark encoder as finished so it will be recreated on next Begin()
                encoderPtr->markFinished();
            } else {
                return GFX_RESULT_ERROR_UNKNOWN;
            }
        }
    }

    // Signal fence if provided - use queue work done to wait for GPU completion
    if (submitInfo->signalFence) {
        auto* fencePtr = reinterpret_cast<gfx::webgpu::Fence*>(submitInfo->signalFence);

        static auto fenceSignalCallback = [](WGPUQueueWorkDoneStatus status, WGPUStringView, void* userdata1, void*) {
            auto* fence = static_cast<gfx::webgpu::Fence*>(userdata1);
            if (status == WGPUQueueWorkDoneStatus_Success) {
                fence->setSignaled(true);
            }
        };

        WGPUQueueWorkDoneCallbackInfo callbackInfo = WGPU_QUEUE_WORK_DONE_CALLBACK_INFO_INIT;
        callbackInfo.mode = WGPUCallbackMode_WaitAnyOnly;
        callbackInfo.callback = fenceSignalCallback;
        callbackInfo.userdata1 = fencePtr;

        WGPUFuture future = wgpuQueueOnSubmittedWorkDone(queuePtr->handle(), callbackInfo);

        // Wait for the fence to be signaled (GPU work done)
        if (queuePtr->getDevice() && queuePtr->getDevice()->getAdapter() && queuePtr->getDevice()->getAdapter()->getInstance()) {
            WGPUFutureWaitInfo waitInfo = WGPU_FUTURE_WAIT_INFO_INIT;
            waitInfo.future = future;
            wgpuInstanceWaitAny(queuePtr->getDevice()->getAdapter()->getInstance()->handle(), 1, &waitInfo, UINT64_MAX);
        }
    }

    return GFX_RESULT_SUCCESS;
}

void WebGPUBackend::queueWriteBuffer(GfxQueue queue, GfxBuffer buffer, uint64_t offset, const void* data, uint64_t size) const
{
    if (!queue || !buffer || !data) {
        return;
    }

    auto* queuePtr = reinterpret_cast<gfx::webgpu::Queue*>(queue);
    auto* bufferPtr = reinterpret_cast<gfx::webgpu::Buffer*>(buffer);

    wgpuQueueWriteBuffer(queuePtr->handle(), bufferPtr->handle(), offset, data, size);
}

void WebGPUBackend::queueWriteTexture(GfxQueue queue, GfxTexture texture, const GfxOrigin3D* origin, uint32_t mipLevel,
    const void* data, uint64_t dataSize, uint32_t bytesPerRow, const GfxExtent3D* extent, GfxTextureLayout finalLayout) const
{
    if (!queue || !texture || !origin || !extent || !data) {
        return;
    }

    auto* queuePtr = reinterpret_cast<gfx::webgpu::Queue*>(queue);
    auto* texturePtr = reinterpret_cast<gfx::webgpu::Texture*>(texture);

    WGPUTexelCopyTextureInfo dest = WGPU_TEXEL_COPY_TEXTURE_INFO_INIT;
    dest.texture = texturePtr->handle();
    dest.mipLevel = mipLevel;
    dest.origin = { static_cast<uint32_t>(origin->x), static_cast<uint32_t>(origin->y), static_cast<uint32_t>(origin->z) };

    WGPUTexelCopyBufferLayout layout = WGPU_TEXEL_COPY_BUFFER_LAYOUT_INIT;
    layout.bytesPerRow = bytesPerRow;

    WGPUExtent3D wgpuExtent = { extent->width, extent->height, extent->depth };

    wgpuQueueWriteTexture(queuePtr->handle(), &dest, data, dataSize, &layout, &wgpuExtent);

    (void)finalLayout; // WebGPU handles layout transitions automatically
}

GfxResult WebGPUBackend::queueWaitIdle(GfxQueue queue) const
{
    if (!queue) {
        return GFX_RESULT_ERROR_INVALID_PARAMETER;
    }

    auto* queuePtr = reinterpret_cast<gfx::webgpu::Queue*>(queue);

    // Submit empty command to ensure all previous work is queued
    static auto queueWorkDoneCallback = [](WGPUQueueWorkDoneStatus status, WGPUStringView, void* userdata1, void*) {
        bool* done = static_cast<bool*>(userdata1);
        if (status == WGPUQueueWorkDoneStatus_Success) {
            *done = true;
        }
    };

    bool workDone = false;
    WGPUQueueWorkDoneCallbackInfo callbackInfo = WGPU_QUEUE_WORK_DONE_CALLBACK_INFO_INIT;
    callbackInfo.mode = WGPUCallbackMode_WaitAnyOnly;
    callbackInfo.callback = queueWorkDoneCallback;
    callbackInfo.userdata1 = &workDone;

    WGPUFuture future = wgpuQueueOnSubmittedWorkDone(queuePtr->handle(), callbackInfo);

    // Properly wait for the queue work to complete
    if (queuePtr->getDevice() && queuePtr->getDevice()->getAdapter() && queuePtr->getDevice()->getAdapter()->getInstance()) {
        WGPUFutureWaitInfo waitInfo = WGPU_FUTURE_WAIT_INFO_INIT;
        waitInfo.future = future;
        wgpuInstanceWaitAny(queuePtr->getDevice()->getAdapter()->getInstance()->handle(), 1, &waitInfo, UINT64_MAX);
    }

    return workDone ? GFX_RESULT_SUCCESS : GFX_RESULT_ERROR_UNKNOWN;
}

// CommandEncoder functions
void WebGPUBackend::commandEncoderDestroy(GfxCommandEncoder commandEncoder) const
{
    if (!commandEncoder) {
        return;
    }
    delete reinterpret_cast<gfx::webgpu::CommandEncoder*>(commandEncoder);
}

GfxResult WebGPUBackend::commandEncoderBeginRenderPass(GfxCommandEncoder commandEncoder,
    const GfxRenderPassDescriptor* descriptor,
    GfxRenderPassEncoder* outRenderPass) const
{
    if (!commandEncoder || !descriptor || !outRenderPass) {
        return GFX_RESULT_ERROR_INVALID_PARAMETER;
    }

    auto* encoderPtr = reinterpret_cast<gfx::webgpu::CommandEncoder*>(commandEncoder);

    // Extract parameters from descriptor
    const GfxColorAttachment* colorAttachments = descriptor->colorAttachments;
    uint32_t colorAttachmentCount = descriptor->colorAttachmentCount;
    const GfxDepthStencilAttachment* depthStencilAttachment = descriptor->depthStencilAttachment;

    WGPURenderPassDescriptor wgpuDesc = WGPU_RENDER_PASS_DESCRIPTOR_INIT;

    std::vector<WGPURenderPassColorAttachment> wgpuColorAttachments;
    if (colorAttachmentCount > 0 && colorAttachments) {
        wgpuColorAttachments.reserve(colorAttachmentCount);

        for (uint32_t i = 0; i < colorAttachmentCount; ++i) {
            WGPURenderPassColorAttachment attachment = WGPU_RENDER_PASS_COLOR_ATTACHMENT_INIT;
            if (colorAttachments[i].target.view) {
                auto* viewPtr = reinterpret_cast<gfx::webgpu::TextureView*>(colorAttachments[i].target.view);
                attachment.view = viewPtr->handle();
                attachment.loadOp = gfx::convertor::gfxLoadOpToWGPULoadOp(colorAttachments[i].target.ops.loadOp);
                attachment.storeOp = gfx::convertor::gfxStoreOpToWGPUStoreOp(colorAttachments[i].target.ops.storeOp);

                const GfxColor& color = colorAttachments[i].target.ops.clearColor;
                attachment.clearValue = { color.r, color.g, color.b, color.a };

                // WebGPU MSAA handling: check if this attachment has a resolve target
                if (colorAttachments[i].resolveTarget && colorAttachments[i].resolveTarget->view) {
                    auto* resolveViewPtr = reinterpret_cast<gfx::webgpu::TextureView*>(colorAttachments[i].resolveTarget->view);
                    attachment.resolveTarget = resolveViewPtr->handle();
                }
            }
            wgpuColorAttachments.push_back(attachment);
        }

        wgpuDesc.colorAttachments = wgpuColorAttachments.data();
        wgpuDesc.colorAttachmentCount = static_cast<uint32_t>(wgpuColorAttachments.size());
    }

    WGPURenderPassDepthStencilAttachment wgpuDepthStencil = WGPU_RENDER_PASS_DEPTH_STENCIL_ATTACHMENT_INIT;
    if (depthStencilAttachment) {
        const GfxDepthStencilAttachmentTarget* target = &depthStencilAttachment->target;
        auto* viewPtr = reinterpret_cast<gfx::webgpu::TextureView*>(target->view);
        wgpuDepthStencil.view = viewPtr->handle();

        // Handle depth operations if depth pointer is set
        if (target->depthOps) {
            wgpuDepthStencil.depthLoadOp = gfx::convertor::gfxLoadOpToWGPULoadOp(target->depthOps->loadOp);
            wgpuDepthStencil.depthStoreOp = gfx::convertor::gfxStoreOpToWGPUStoreOp(target->depthOps->storeOp);
            wgpuDepthStencil.depthClearValue = target->depthOps->clearValue;
        } else {
            wgpuDepthStencil.depthLoadOp = WGPULoadOp_Undefined;
            wgpuDepthStencil.depthStoreOp = WGPUStoreOp_Undefined;
            wgpuDepthStencil.depthClearValue = 1.0f;
        }

        // Only set stencil operations for formats that have a stencil aspect
        // For depth-only formats (Depth16Unorm, Depth24Plus, Depth32Float), we must set to Undefined
        WGPUTextureFormat wgpuFormat = viewPtr->getTexture()->getFormat();
        GfxTextureFormat format = gfx::convertor::wgpuFormatToGfxFormat(wgpuFormat);
        if (gfx::convertor::formatHasStencil(format)) {
            if (target->stencilOps) {
                wgpuDepthStencil.stencilLoadOp = gfx::convertor::gfxLoadOpToWGPULoadOp(target->stencilOps->loadOp);
                wgpuDepthStencil.stencilStoreOp = gfx::convertor::gfxStoreOpToWGPUStoreOp(target->stencilOps->storeOp);
                wgpuDepthStencil.stencilClearValue = target->stencilOps->clearValue;
            } else {
                wgpuDepthStencil.stencilLoadOp = WGPULoadOp_Undefined;
                wgpuDepthStencil.stencilStoreOp = WGPUStoreOp_Undefined;
                wgpuDepthStencil.stencilClearValue = 0;
            }
        } else {
            wgpuDepthStencil.stencilLoadOp = WGPULoadOp_Undefined;
            wgpuDepthStencil.stencilStoreOp = WGPUStoreOp_Undefined;
            wgpuDepthStencil.stencilClearValue = 0;
        }

        wgpuDesc.depthStencilAttachment = &wgpuDepthStencil;
    }

    WGPURenderPassEncoder wgpuEncoder = wgpuCommandEncoderBeginRenderPass(encoderPtr->handle(), &wgpuDesc);
    if (!wgpuEncoder) {
        return GFX_RESULT_ERROR_UNKNOWN;
    }

    auto* renderPassEncoder = new gfx::webgpu::RenderPassEncoder(wgpuEncoder);
    *outRenderPass = reinterpret_cast<GfxRenderPassEncoder>(renderPassEncoder);
    return GFX_RESULT_SUCCESS;
}

GfxResult WebGPUBackend::commandEncoderBeginComputePass(GfxCommandEncoder commandEncoder, const GfxComputePassDescriptor* descriptor, GfxComputePassEncoder* outComputePass) const
{
    if (!commandEncoder || !descriptor || !outComputePass) {
        return GFX_RESULT_ERROR_INVALID_PARAMETER;
    }

    auto* encoderPtr = reinterpret_cast<gfx::webgpu::CommandEncoder*>(commandEncoder);

    WGPUComputePassDescriptor wgpuDesc = WGPU_COMPUTE_PASS_DESCRIPTOR_INIT;
    if (descriptor->label) {
        wgpuDesc.label = gfx::convertor::gfxStringView(descriptor->label);
    }

    WGPUComputePassEncoder wgpuEncoder = wgpuCommandEncoderBeginComputePass(encoderPtr->handle(), &wgpuDesc);
    if (!wgpuEncoder) {
        return GFX_RESULT_ERROR_UNKNOWN;
    }

    auto* computePassEncoder = new gfx::webgpu::ComputePassEncoder(wgpuEncoder);
    *outComputePass = reinterpret_cast<GfxComputePassEncoder>(computePassEncoder);
    return GFX_RESULT_SUCCESS;
}

void WebGPUBackend::commandEncoderCopyBufferToBuffer(GfxCommandEncoder commandEncoder, GfxBuffer source, uint64_t sourceOffset,
    GfxBuffer destination, uint64_t destinationOffset, uint64_t size) const
{
    if (!commandEncoder || !source || !destination) {
        return;
    }

    auto* encoderPtr = reinterpret_cast<gfx::webgpu::CommandEncoder*>(commandEncoder);
    auto* srcPtr = reinterpret_cast<gfx::webgpu::Buffer*>(source);
    auto* dstPtr = reinterpret_cast<gfx::webgpu::Buffer*>(destination);

    wgpuCommandEncoderCopyBufferToBuffer(encoderPtr->handle(),
        srcPtr->handle(), sourceOffset,
        dstPtr->handle(), destinationOffset,
        size);
}

void WebGPUBackend::commandEncoderCopyBufferToTexture(GfxCommandEncoder commandEncoder, GfxBuffer source, uint64_t sourceOffset, uint32_t bytesPerRow,
    GfxTexture destination, const GfxOrigin3D* origin, const GfxExtent3D* extent, uint32_t mipLevel, GfxTextureLayout finalLayout) const
{
    if (!commandEncoder || !source || !destination || !origin || !extent) {
        return;
    }

    auto* encoderPtr = reinterpret_cast<gfx::webgpu::CommandEncoder*>(commandEncoder);
    auto* srcPtr = reinterpret_cast<gfx::webgpu::Buffer*>(source);
    auto* dstPtr = reinterpret_cast<gfx::webgpu::Texture*>(destination);

    WGPUTexelCopyBufferInfo sourceInfo = WGPU_TEXEL_COPY_BUFFER_INFO_INIT;
    sourceInfo.buffer = srcPtr->handle();
    sourceInfo.layout.offset = sourceOffset;
    sourceInfo.layout.bytesPerRow = bytesPerRow;

    WGPUTexelCopyTextureInfo destInfo = WGPU_TEXEL_COPY_TEXTURE_INFO_INIT;
    destInfo.texture = dstPtr->handle();
    destInfo.mipLevel = mipLevel;
    destInfo.origin = { static_cast<uint32_t>(origin->x), static_cast<uint32_t>(origin->y), static_cast<uint32_t>(origin->z) };

    WGPUExtent3D wgpuExtent = { extent->width, extent->height, extent->depth };

    wgpuCommandEncoderCopyBufferToTexture(encoderPtr->handle(), &sourceInfo, &destInfo, &wgpuExtent);

    (void)finalLayout; // WebGPU handles layout transitions automatically
}

void WebGPUBackend::commandEncoderCopyTextureToBuffer(GfxCommandEncoder commandEncoder, GfxTexture source, const GfxOrigin3D* origin, uint32_t mipLevel,
    GfxBuffer destination, uint64_t destinationOffset, uint32_t bytesPerRow, const GfxExtent3D* extent, GfxTextureLayout finalLayout) const
{
    if (!commandEncoder || !source || !destination || !origin || !extent) {
        return;
    }

    auto* encoderPtr = reinterpret_cast<gfx::webgpu::CommandEncoder*>(commandEncoder);
    auto* srcPtr = reinterpret_cast<gfx::webgpu::Texture*>(source);
    auto* dstPtr = reinterpret_cast<gfx::webgpu::Buffer*>(destination);

    WGPUTexelCopyTextureInfo sourceInfo = WGPU_TEXEL_COPY_TEXTURE_INFO_INIT;
    sourceInfo.texture = srcPtr->handle();
    sourceInfo.mipLevel = mipLevel;
    sourceInfo.origin = { static_cast<uint32_t>(origin->x), static_cast<uint32_t>(origin->y), static_cast<uint32_t>(origin->z) };

    WGPUTexelCopyBufferInfo destInfo = WGPU_TEXEL_COPY_BUFFER_INFO_INIT;
    destInfo.buffer = dstPtr->handle();
    destInfo.layout.offset = destinationOffset;
    destInfo.layout.bytesPerRow = bytesPerRow;

    WGPUExtent3D wgpuExtent = { extent->width, extent->height, extent->depth };

    wgpuCommandEncoderCopyTextureToBuffer(encoderPtr->handle(), &sourceInfo, &destInfo, &wgpuExtent);

    (void)finalLayout; // WebGPU handles layout transitions automatically
}

void WebGPUBackend::commandEncoderCopyTextureToTexture(GfxCommandEncoder commandEncoder, GfxTexture source, const GfxOrigin3D* sourceOrigin, uint32_t sourceMipLevel,
    GfxTexture destination, const GfxOrigin3D* destinationOrigin, uint32_t destinationMipLevel, const GfxExtent3D* extent,
    GfxTextureLayout srcFinalLayout, GfxTextureLayout dstFinalLayout) const
{
    if (!commandEncoder || !source || !destination || !sourceOrigin || !destinationOrigin || !extent) {
        return;
    }

    auto* encoderPtr = reinterpret_cast<gfx::webgpu::CommandEncoder*>(commandEncoder);
    auto* srcPtr = reinterpret_cast<gfx::webgpu::Texture*>(source);
    auto* dstPtr = reinterpret_cast<gfx::webgpu::Texture*>(destination);

    // For 2D textures and arrays, extent->depth represents layer count
    // For 3D textures, it represents actual depth
    WGPUExtent3D srcSize = srcPtr->getSize();
    bool is3DTexture = (srcSize.depthOrArrayLayers > 1 && srcSize.height > 1);

    WGPUTexelCopyTextureInfo sourceInfo = WGPU_TEXEL_COPY_TEXTURE_INFO_INIT;
    sourceInfo.texture = srcPtr->handle();
    sourceInfo.mipLevel = sourceMipLevel;
    sourceInfo.origin = { static_cast<uint32_t>(sourceOrigin->x), static_cast<uint32_t>(sourceOrigin->y), static_cast<uint32_t>(is3DTexture ? sourceOrigin->z : 0) };

    WGPUTexelCopyTextureInfo destInfo = WGPU_TEXEL_COPY_TEXTURE_INFO_INIT;
    destInfo.texture = dstPtr->handle();
    destInfo.mipLevel = destinationMipLevel;
    destInfo.origin = { static_cast<uint32_t>(destinationOrigin->x), static_cast<uint32_t>(destinationOrigin->y), static_cast<uint32_t>(is3DTexture ? destinationOrigin->z : 0) };

    WGPUExtent3D wgpuExtent = { extent->width, extent->height, extent->depth };

    wgpuCommandEncoderCopyTextureToTexture(encoderPtr->handle(), &sourceInfo, &destInfo, &wgpuExtent);

    (void)srcFinalLayout; // WebGPU handles layout transitions automatically
    (void)dstFinalLayout;
}

void WebGPUBackend::commandEncoderPipelineBarrier(GfxCommandEncoder commandEncoder,
    const GfxMemoryBarrier* memoryBarriers, uint32_t memoryBarrierCount,
    const GfxBufferBarrier* bufferBarriers, uint32_t bufferBarrierCount,
    const GfxTextureBarrier* textureBarriers, uint32_t textureBarrierCount) const
{
    // WebGPU handles synchronization and layout transitions automatically
    // This is a no-op for WebGPU backend
    (void)commandEncoder;
    (void)memoryBarriers;
    (void)memoryBarrierCount;
    (void)bufferBarriers;
    (void)bufferBarrierCount;
    (void)textureBarriers;
    (void)textureBarrierCount;
}

void WebGPUBackend::commandEncoderEnd(GfxCommandEncoder commandEncoder) const
{
    (void)commandEncoder; // Parameter unused - handled in queueSubmit
}

void WebGPUBackend::commandEncoderBegin(GfxCommandEncoder commandEncoder) const
{
    if (!commandEncoder) {
        return;
    }

    auto* encoderPtr = reinterpret_cast<gfx::webgpu::CommandEncoder*>(commandEncoder);

    // WebGPU encoders can't be reused after Finish() - recreate if needed
    if (!encoderPtr->recreateIfNeeded()) {
        fprintf(stderr, "[WebGPU ERROR] Failed to recreate command encoder\n");
    }
}

// RenderPassEncoder functions
void WebGPUBackend::renderPassEncoderDestroy(GfxRenderPassEncoder renderPassEncoder) const
{
    if (!renderPassEncoder) {
        return;
    }
    delete reinterpret_cast<gfx::webgpu::RenderPassEncoder*>(renderPassEncoder);
}

void WebGPUBackend::renderPassEncoderSetPipeline(GfxRenderPassEncoder renderPassEncoder, GfxRenderPipeline pipeline) const
{
    if (!renderPassEncoder || !pipeline) {
        return;
    }

    auto* encoderPtr = reinterpret_cast<gfx::webgpu::RenderPassEncoder*>(renderPassEncoder);
    auto* pipelinePtr = reinterpret_cast<gfx::webgpu::RenderPipeline*>(pipeline);

    wgpuRenderPassEncoderSetPipeline(encoderPtr->handle(), pipelinePtr->handle());
}

void WebGPUBackend::renderPassEncoderSetBindGroup(GfxRenderPassEncoder renderPassEncoder, uint32_t index, GfxBindGroup bindGroup, const uint32_t* dynamicOffsets, uint32_t dynamicOffsetCount) const
{
    if (!renderPassEncoder || !bindGroup) {
        return;
    }

    auto* encoderPtr = reinterpret_cast<gfx::webgpu::RenderPassEncoder*>(renderPassEncoder);
    auto* bindGroupPtr = reinterpret_cast<gfx::webgpu::BindGroup*>(bindGroup);

    wgpuRenderPassEncoderSetBindGroup(encoderPtr->handle(), index, bindGroupPtr->handle(), dynamicOffsetCount, dynamicOffsets);
}

void WebGPUBackend::renderPassEncoderSetVertexBuffer(GfxRenderPassEncoder renderPassEncoder, uint32_t slot, GfxBuffer buffer, uint64_t offset, uint64_t size) const
{
    if (!renderPassEncoder || !buffer) {
        return;
    }

    auto* encoderPtr = reinterpret_cast<gfx::webgpu::RenderPassEncoder*>(renderPassEncoder);
    auto* bufferPtr = reinterpret_cast<gfx::webgpu::Buffer*>(buffer);

    wgpuRenderPassEncoderSetVertexBuffer(encoderPtr->handle(), slot, bufferPtr->handle(), offset, size);
}

void WebGPUBackend::renderPassEncoderSetIndexBuffer(GfxRenderPassEncoder renderPassEncoder, GfxBuffer buffer, GfxIndexFormat format, uint64_t offset, uint64_t size) const
{
    if (!renderPassEncoder || !buffer) {
        return;
    }

    auto* encoderPtr = reinterpret_cast<gfx::webgpu::RenderPassEncoder*>(renderPassEncoder);
    auto* bufferPtr = reinterpret_cast<gfx::webgpu::Buffer*>(buffer);

    wgpuRenderPassEncoderSetIndexBuffer(encoderPtr->handle(), bufferPtr->handle(),
        gfx::convertor::gfxIndexFormatToWGPU(format), offset, size);
}

void WebGPUBackend::renderPassEncoderSetViewport(GfxRenderPassEncoder renderPassEncoder, const GfxViewport* viewport) const
{
    if (!renderPassEncoder || !viewport) {
        return;
    }

    auto* encoderPtr = reinterpret_cast<gfx::webgpu::RenderPassEncoder*>(renderPassEncoder);
    wgpuRenderPassEncoderSetViewport(encoderPtr->handle(),
        viewport->x, viewport->y, viewport->width, viewport->height,
        viewport->minDepth, viewport->maxDepth);
}

void WebGPUBackend::renderPassEncoderSetScissorRect(GfxRenderPassEncoder renderPassEncoder, const GfxScissorRect* scissor) const
{
    if (!renderPassEncoder || !scissor) {
        return;
    }

    auto* encoderPtr = reinterpret_cast<gfx::webgpu::RenderPassEncoder*>(renderPassEncoder);
    wgpuRenderPassEncoderSetScissorRect(encoderPtr->handle(),
        scissor->x, scissor->y, scissor->width, scissor->height);
}

void WebGPUBackend::renderPassEncoderDraw(GfxRenderPassEncoder renderPassEncoder, uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance) const
{
    if (!renderPassEncoder) {
        return;
    }

    auto* encoderPtr = reinterpret_cast<gfx::webgpu::RenderPassEncoder*>(renderPassEncoder);
    wgpuRenderPassEncoderDraw(encoderPtr->handle(), vertexCount, instanceCount, firstVertex, firstInstance);
}

void WebGPUBackend::renderPassEncoderDrawIndexed(GfxRenderPassEncoder renderPassEncoder, uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t baseVertex, uint32_t firstInstance) const
{
    if (!renderPassEncoder) {
        return;
    }

    auto* encoderPtr = reinterpret_cast<gfx::webgpu::RenderPassEncoder*>(renderPassEncoder);
    wgpuRenderPassEncoderDrawIndexed(encoderPtr->handle(), indexCount, instanceCount,
        firstIndex, baseVertex, firstInstance);
}

void WebGPUBackend::renderPassEncoderEnd(GfxRenderPassEncoder renderPassEncoder) const
{
    if (!renderPassEncoder) {
        return;
    }

    auto* encoderPtr = reinterpret_cast<gfx::webgpu::RenderPassEncoder*>(renderPassEncoder);
    wgpuRenderPassEncoderEnd(encoderPtr->handle());
}

// ComputePassEncoder functions
void WebGPUBackend::computePassEncoderDestroy(GfxComputePassEncoder computePassEncoder) const
{
    if (!computePassEncoder) {
        return;
    }
    delete reinterpret_cast<gfx::webgpu::ComputePassEncoder*>(computePassEncoder);
}

void WebGPUBackend::computePassEncoderSetPipeline(GfxComputePassEncoder computePassEncoder, GfxComputePipeline pipeline) const
{
    if (!computePassEncoder || !pipeline) {
        return;
    }

    auto* encoderPtr = reinterpret_cast<gfx::webgpu::ComputePassEncoder*>(computePassEncoder);
    auto* pipelinePtr = reinterpret_cast<gfx::webgpu::ComputePipeline*>(pipeline);

    wgpuComputePassEncoderSetPipeline(encoderPtr->handle(), pipelinePtr->handle());
}

void WebGPUBackend::computePassEncoderSetBindGroup(GfxComputePassEncoder computePassEncoder, uint32_t index, GfxBindGroup bindGroup, const uint32_t* dynamicOffsets, uint32_t dynamicOffsetCount) const
{
    if (!computePassEncoder || !bindGroup) {
        return;
    }

    auto* encoderPtr = reinterpret_cast<gfx::webgpu::ComputePassEncoder*>(computePassEncoder);
    auto* bindGroupPtr = reinterpret_cast<gfx::webgpu::BindGroup*>(bindGroup);

    wgpuComputePassEncoderSetBindGroup(encoderPtr->handle(), index, bindGroupPtr->handle(), dynamicOffsetCount, dynamicOffsets);
}

void WebGPUBackend::computePassEncoderDispatchWorkgroups(GfxComputePassEncoder computePassEncoder, uint32_t workgroupCountX, uint32_t workgroupCountY, uint32_t workgroupCountZ) const
{
    if (!computePassEncoder) {
        return;
    }

    auto* encoderPtr = reinterpret_cast<gfx::webgpu::ComputePassEncoder*>(computePassEncoder);
    wgpuComputePassEncoderDispatchWorkgroups(encoderPtr->handle(), workgroupCountX, workgroupCountY, workgroupCountZ);
}

void WebGPUBackend::computePassEncoderEnd(GfxComputePassEncoder computePassEncoder) const
{
    if (!computePassEncoder) {
        return;
    }

    auto* encoderPtr = reinterpret_cast<gfx::webgpu::ComputePassEncoder*>(computePassEncoder);
    wgpuComputePassEncoderEnd(encoderPtr->handle());
}

// Fence functions
void WebGPUBackend::fenceDestroy(GfxFence fence) const
{
    if (!fence) {
        return;
    }
    delete reinterpret_cast<gfx::webgpu::Fence*>(fence);
}

GfxResult WebGPUBackend::fenceGetStatus(GfxFence fence, bool* isSignaled) const
{
    if (!fence || !isSignaled) {
        return GFX_RESULT_ERROR_INVALID_PARAMETER;
    }

    auto* fencePtr = reinterpret_cast<gfx::webgpu::Fence*>(fence);
    *isSignaled = fencePtr->isSignaled();
    return GFX_RESULT_SUCCESS;
}

GfxResult WebGPUBackend::fenceWait(GfxFence fence, uint64_t timeoutNs) const
{
    if (!fence) {
        return GFX_RESULT_ERROR_INVALID_PARAMETER;
    }

    auto* fencePtr = reinterpret_cast<gfx::webgpu::Fence*>(fence);

    // Fence is already properly signaled by queueSubmit after GPU work completes
    // Just check the status
    (void)timeoutNs; // Timeout is handled in queueSubmit

    return fencePtr->isSignaled() ? GFX_RESULT_SUCCESS : GFX_RESULT_TIMEOUT;
}

void WebGPUBackend::fenceReset(GfxFence fence) const
{
    if (!fence) {
        return;
    }

    auto* fencePtr = reinterpret_cast<gfx::webgpu::Fence*>(fence);
    fencePtr->setSignaled(false);
}

// Semaphore functions
void WebGPUBackend::semaphoreDestroy(GfxSemaphore semaphore) const
{
    if (!semaphore) {
        return;
    }
    delete reinterpret_cast<gfx::webgpu::Semaphore*>(semaphore);
}

GfxSemaphoreType WebGPUBackend::semaphoreGetType(GfxSemaphore semaphore) const
{
    if (!semaphore) {
        return GFX_SEMAPHORE_TYPE_BINARY;
    }
    auto type = reinterpret_cast<gfx::webgpu::Semaphore*>(semaphore)->getType();
    return gfx::convertor::webgpuSemaphoreTypeToGfxSemaphoreType(type);
}

GfxResult WebGPUBackend::semaphoreSignal(GfxSemaphore semaphore, uint64_t value) const
{
    if (!semaphore) {
        return GFX_RESULT_ERROR_INVALID_PARAMETER;
    }

    auto* semaphorePtr = reinterpret_cast<gfx::webgpu::Semaphore*>(semaphore);
    if (semaphorePtr->getType() == gfx::webgpu::SemaphoreType::Timeline) {
        semaphorePtr->setValue(value);
    }
    return GFX_RESULT_SUCCESS;
}

GfxResult WebGPUBackend::semaphoreWait(GfxSemaphore semaphore, uint64_t value, uint64_t timeoutNs) const
{
    if (!semaphore) {
        return GFX_RESULT_ERROR_INVALID_PARAMETER;
    }
    (void)timeoutNs; // WebGPU doesn't support timeout for semaphore waits

    auto* semaphorePtr = reinterpret_cast<gfx::webgpu::Semaphore*>(semaphore);
    if (semaphorePtr->getType() == gfx::webgpu::SemaphoreType::Timeline) {
        return (semaphorePtr->getValue() >= value) ? GFX_RESULT_SUCCESS : GFX_RESULT_TIMEOUT;
    }
    return GFX_RESULT_SUCCESS;
}

uint64_t WebGPUBackend::semaphoreGetValue(GfxSemaphore semaphore) const
{
    if (!semaphore) {
        return 0;
    }
    return reinterpret_cast<gfx::webgpu::Semaphore*>(semaphore)->getValue();
}

const IBackend* WebGPUBackend::create()
{
    static WebGPUBackend webgpuBackend;
    return &webgpuBackend;
}

} // namespace gfx::webgpu