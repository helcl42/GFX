#include "backend/Factory.h"
#include "backend/Manager.h"
#include "common/Logger.h"

#include <gfx/gfx.h>

#include <vector>

// ============================================================================
// C API Implementation
// ============================================================================

extern "C" {

GfxResult gfxLoadBackend(GfxBackend backend)
{
    if (backend == GFX_BACKEND_AUTO) {
#ifdef GFX_ENABLE_VULKAN
        if (gfxLoadBackend(GFX_BACKEND_VULKAN) == GFX_RESULT_SUCCESS) {
            return GFX_RESULT_SUCCESS;
        }
#endif
#ifdef GFX_ENABLE_WEBGPU
        if (gfxLoadBackend(GFX_BACKEND_WEBGPU) == GFX_RESULT_SUCCESS) {
            return GFX_RESULT_SUCCESS;
        }
#endif
        return GFX_RESULT_ERROR_BACKEND_NOT_LOADED;
    }

    if (backend < 0 || backend >= GFX_BACKEND_AUTO) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    auto& manager = gfx::backend::BackendManager::instance();

    // Check if backend needs to be created
    if (!manager.getBackend(backend)) {
        auto backendImpl = gfx::backend::BackendFactory::create(backend);
        if (!backendImpl) {
            return GFX_RESULT_ERROR_BACKEND_NOT_LOADED;
        }

        return manager.loadBackend(backend, std::move(backendImpl)) ? GFX_RESULT_SUCCESS : GFX_RESULT_ERROR_UNKNOWN;
    }

    // Backend already loaded - shared_ptr handles ref counting automatically
    return GFX_RESULT_SUCCESS;
}

GfxResult gfxUnloadBackend(GfxBackend backend)
{
    if (backend == GFX_BACKEND_AUTO) {
        // Unload the first loaded backend
        auto& manager = gfx::backend::BackendManager::instance();
#ifdef GFX_ENABLE_VULKAN
        if (manager.getBackend(GFX_BACKEND_VULKAN)) {
            return gfxUnloadBackend(GFX_BACKEND_VULKAN);
        }
#endif
#ifdef GFX_ENABLE_WEBGPU
        if (manager.getBackend(GFX_BACKEND_WEBGPU)) {
            return gfxUnloadBackend(GFX_BACKEND_WEBGPU);
        }
#endif
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    if (backend >= 0 && backend < GFX_BACKEND_AUTO) {
        gfx::backend::BackendManager::instance().unloadBackend(backend);
        return GFX_RESULT_SUCCESS;
    }
    return GFX_RESULT_ERROR_INVALID_ARGUMENT;
}

GfxResult gfxLoadAllBackends(void)
{
    bool loadedAny = false;
#ifdef GFX_ENABLE_VULKAN
    if (gfxLoadBackend(GFX_BACKEND_VULKAN) == GFX_RESULT_SUCCESS) {
        loadedAny = true;
    }
#endif
#ifdef GFX_ENABLE_WEBGPU
    if (gfxLoadBackend(GFX_BACKEND_WEBGPU) == GFX_RESULT_SUCCESS) {
        loadedAny = true;
    }
#endif
    return loadedAny ? GFX_RESULT_SUCCESS : GFX_RESULT_ERROR_BACKEND_NOT_LOADED;
}

GfxResult gfxUnloadAllBackends(void)
{
    auto& manager = gfx::backend::BackendManager::instance();
#ifdef GFX_ENABLE_VULKAN
    while (manager.getBackend(GFX_BACKEND_VULKAN)) {
        gfxUnloadBackend(GFX_BACKEND_VULKAN);
    }
#endif
#ifdef GFX_ENABLE_WEBGPU
    while (manager.getBackend(GFX_BACKEND_WEBGPU)) {
        gfxUnloadBackend(GFX_BACKEND_WEBGPU);
    }
#endif
    return GFX_RESULT_SUCCESS;
}

// Instance Functions
GfxResult gfxCreateInstance(const GfxInstanceDescriptor* descriptor, GfxInstance* outInstance)
{
    if (!descriptor || !outInstance) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    GfxBackend backend = descriptor->backend;
    auto& manager = gfx::backend::BackendManager::instance();

    if (backend == GFX_BACKEND_AUTO) {
#ifdef GFX_ENABLE_VULKAN
        if (manager.getBackend(GFX_BACKEND_VULKAN)) {
            backend = GFX_BACKEND_VULKAN;
        }
#endif
#ifdef GFX_ENABLE_WEBGPU
        if (backend == GFX_BACKEND_AUTO && manager.getBackend(GFX_BACKEND_WEBGPU)) {
            backend = GFX_BACKEND_WEBGPU;
        }
#endif
        if (backend == GFX_BACKEND_AUTO) {
            return GFX_RESULT_ERROR_BACKEND_NOT_LOADED;
        }
    }

    auto backendImpl = gfx::backend::BackendManager::instance().getBackend(backend);
    if (!backendImpl) {
        return GFX_RESULT_ERROR_BACKEND_NOT_LOADED;
    }

    GfxInstance nativeInstance = nullptr;
    GfxResult result = backendImpl->createInstance(descriptor, &nativeInstance);
    if (result != GFX_RESULT_SUCCESS) {
        return result;
    }

    *outInstance = gfx::backend::BackendManager::instance().wrap(backend, nativeInstance);
    return GFX_RESULT_SUCCESS;
}

GfxResult gfxInstanceDestroy(GfxInstance instance)
{
    if (!instance) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }
    auto backend = gfx::backend::BackendManager::instance().getBackend(instance);
    if (!backend) {
        return GFX_RESULT_ERROR_NOT_FOUND;
    }

    GfxResult result = backend->instanceDestroy(instance);
    gfx::backend::BackendManager::instance().unwrap(instance);
    return result;
}

GfxResult gfxInstanceRequestAdapter(GfxInstance instance, const GfxAdapterDescriptor* descriptor, GfxAdapter* outAdapter)
{
    if (!instance || !descriptor || !outAdapter) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }
    auto backend = gfx::backend::BackendManager::instance().getBackend(instance);
    if (!backend) {
        return GFX_RESULT_ERROR_NOT_FOUND;
    }

    GfxBackend backendType = gfx::backend::BackendManager::instance().getBackendType(instance);
    GfxAdapter nativeAdapter = nullptr;
    GfxResult result = backend->instanceRequestAdapter(instance, descriptor, &nativeAdapter);
    if (result != GFX_RESULT_SUCCESS) {
        return result;
    }

    *outAdapter = gfx::backend::BackendManager::instance().wrap(backendType, nativeAdapter);
    return GFX_RESULT_SUCCESS;
}

GfxResult gfxInstanceEnumerateAdapters(GfxInstance instance, uint32_t* adapterCount, GfxAdapter* adapters)
{
    if (!instance || !adapterCount) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }
    auto backend = gfx::backend::BackendManager::instance().getBackend(instance);
    if (!backend) {
        return GFX_RESULT_ERROR_NOT_FOUND;
    }

    GfxBackend backendType = gfx::backend::BackendManager::instance().getBackendType(instance);
    GfxResult result = backend->instanceEnumerateAdapters(instance, adapterCount, adapters);

    // Wrap adapters for backend tracking
    if (result == GFX_RESULT_SUCCESS && adapters && *adapterCount > 0) {
        for (uint32_t i = 0; i < *adapterCount; ++i) {
            if (adapters[i]) {
                adapters[i] = gfx::backend::BackendManager::instance().wrap(backendType, adapters[i]);
            }
        }
    }

    return result;
}

GfxResult gfxEnumerateInstanceExtensions(GfxBackend backend, uint32_t* extensionCount, const char** extensionNames)
{
    if (!extensionCount) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }
    auto backendImpl = gfx::backend::BackendManager::instance().getBackend(backend);
    if (!backendImpl) {
        return GFX_RESULT_ERROR_NOT_FOUND;
    }
    return backendImpl->enumerateInstanceExtensions(extensionCount, extensionNames);
}

// Adapter Functions
GfxResult gfxAdapterCreateDevice(GfxAdapter adapter, const GfxDeviceDescriptor* descriptor, GfxDevice* outDevice)
{
    if (!adapter || !descriptor || !outDevice) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }
    auto backend = gfx::backend::BackendManager::instance().getBackend(adapter);
    if (!backend) {
        return GFX_RESULT_ERROR_NOT_FOUND;
    }

    GfxBackend backendType = gfx::backend::BackendManager::instance().getBackendType(adapter);
    GfxDevice nativeDevice = nullptr;
    GfxResult result = backend->adapterCreateDevice(adapter, descriptor, &nativeDevice);
    if (result != GFX_RESULT_SUCCESS) {
        return result;
    }

    *outDevice = gfx::backend::BackendManager::instance().wrap(backendType, nativeDevice);
    return GFX_RESULT_SUCCESS;
}

GfxResult gfxAdapterGetInfo(GfxAdapter adapter, GfxAdapterInfo* outInfo)
{
    if (!adapter || !outInfo) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }
    auto backend = gfx::backend::BackendManager::instance().getBackend(adapter);
    if (!backend) {
        return GFX_RESULT_ERROR_NOT_FOUND;
    }
    return backend->adapterGetInfo(adapter, outInfo);
}

GfxResult gfxAdapterGetLimits(GfxAdapter adapter, GfxDeviceLimits* outLimits)
{
    if (!adapter || !outLimits) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }
    auto backend = gfx::backend::BackendManager::instance().getBackend(adapter);
    if (!backend) {
        return GFX_RESULT_ERROR_NOT_FOUND;
    }
    return backend->adapterGetLimits(adapter, outLimits);
}

GfxResult gfxAdapterEnumerateQueueFamilies(GfxAdapter adapter, uint32_t* queueFamilyCount, GfxQueueFamilyProperties* queueFamilies)
{
    if (!adapter || !queueFamilyCount) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }
    auto backend = gfx::backend::BackendManager::instance().getBackend(adapter);
    if (!backend) {
        return GFX_RESULT_ERROR_NOT_FOUND;
    }
    return backend->adapterEnumerateQueueFamilies(adapter, queueFamilyCount, queueFamilies);
}

GfxResult gfxAdapterGetQueueFamilySurfaceSupport(GfxAdapter adapter, uint32_t queueFamilyIndex, GfxSurface surface, bool* outSupported)
{
    if (!adapter || !outSupported) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }
    auto backend = gfx::backend::BackendManager::instance().getBackend(adapter);
    if (!backend) {
        return GFX_RESULT_ERROR_NOT_FOUND;
    }
    return backend->adapterGetQueueFamilySurfaceSupport(adapter, queueFamilyIndex, surface, outSupported);
}

GfxResult gfxAdapterEnumerateDeviceExtensions(GfxAdapter adapter, uint32_t* extensionCount, const char** extensionNames)
{
    if (!adapter || !extensionCount) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }
    auto backend = gfx::backend::BackendManager::instance().getBackend(adapter);
    if (!backend) {
        return GFX_RESULT_ERROR_NOT_FOUND;
    }
    return backend->adapterEnumerateDeviceExtensions(adapter, extensionCount, extensionNames);
}

// Device Functions
GfxResult gfxDeviceDestroy(GfxDevice device)
{
    if (!device) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }
    auto backend = gfx::backend::BackendManager::instance().getBackend(device);
    if (!backend) {
        return GFX_RESULT_ERROR_NOT_FOUND;
    }
    GfxResult result = backend->deviceDestroy(device);
    gfx::backend::BackendManager::instance().unwrap(device);
    return result;
}

GfxResult gfxDeviceGetQueue(GfxDevice device, GfxQueue* outQueue)
{
    if (!device || !outQueue) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }
    auto backend = gfx::backend::BackendManager::instance().getBackend(device);
    if (!backend) {
        return GFX_RESULT_ERROR_NOT_FOUND;
    }

    GfxBackend backendType = gfx::backend::BackendManager::instance().getBackendType(device);
    GfxQueue nativeQueue = nullptr;
    GfxResult result = backend->deviceGetQueue(device, &nativeQueue);
    if (result != GFX_RESULT_SUCCESS) {
        return result;
    }

    *outQueue = gfx::backend::BackendManager::instance().wrap(backendType, nativeQueue);
    return GFX_RESULT_SUCCESS;
}

GfxResult gfxDeviceGetQueueByIndex(GfxDevice device, uint32_t queueFamilyIndex, uint32_t queueIndex, GfxQueue* outQueue)
{
    if (!device || !outQueue) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }
    auto backend = gfx::backend::BackendManager::instance().getBackend(device);
    if (!backend) {
        return GFX_RESULT_ERROR_NOT_FOUND;
    }

    GfxBackend backendType = gfx::backend::BackendManager::instance().getBackendType(device);
    GfxQueue nativeQueue = nullptr;
    GfxResult result = backend->deviceGetQueueByIndex(device, queueFamilyIndex, queueIndex, &nativeQueue);
    if (result != GFX_RESULT_SUCCESS) {
        return result;
    }

    *outQueue = gfx::backend::BackendManager::instance().wrap(backendType, nativeQueue);
    return GFX_RESULT_SUCCESS;
}

// Macro to generate device create functions with less code duplication
#define DEVICE_CREATE_FUNC(TypeName, funcName)                                                                                       \
    GfxResult gfxDeviceCreate##funcName(GfxDevice device, const Gfx##funcName##Descriptor* descriptor, Gfx##TypeName* out##TypeName) \
    {                                                                                                                                \
        if (!device || !descriptor || !out##TypeName) {                                                                              \
            return GFX_RESULT_ERROR_INVALID_ARGUMENT;                                                                                \
        }                                                                                                                            \
        auto backend = gfx::backend::BackendManager::instance().getBackend(device);                                                  \
        if (!backend) {                                                                                                              \
            return GFX_RESULT_ERROR_NOT_FOUND;                                                                                       \
        }                                                                                                                            \
        GfxBackend backendType = gfx::backend::BackendManager::instance().getBackendType(device);                                    \
        Gfx##TypeName native##TypeName = nullptr;                                                                                    \
        GfxResult result = backend->deviceCreate##funcName(device, descriptor, &native##TypeName);                                   \
        if (result != GFX_RESULT_SUCCESS) {                                                                                          \
            return result;                                                                                                           \
        }                                                                                                                            \
        *out##TypeName = gfx::backend::BackendManager::instance().wrap(backendType, native##TypeName);                               \
        return GFX_RESULT_SUCCESS;                                                                                                   \
    }

DEVICE_CREATE_FUNC(Surface, Surface)
DEVICE_CREATE_FUNC(Buffer, Buffer)
DEVICE_CREATE_FUNC(Texture, Texture)
DEVICE_CREATE_FUNC(Sampler, Sampler)
DEVICE_CREATE_FUNC(Shader, Shader)
DEVICE_CREATE_FUNC(BindGroupLayout, BindGroupLayout)
DEVICE_CREATE_FUNC(BindGroup, BindGroup)
DEVICE_CREATE_FUNC(RenderPipeline, RenderPipeline)
DEVICE_CREATE_FUNC(ComputePipeline, ComputePipeline)
DEVICE_CREATE_FUNC(Fence, Fence)
DEVICE_CREATE_FUNC(Semaphore, Semaphore)
DEVICE_CREATE_FUNC(QuerySet, QuerySet)

// Import functions for external resources
GfxResult gfxDeviceImportBuffer(GfxDevice device, const GfxBufferImportDescriptor* descriptor, GfxBuffer* outBuffer)
{
    if (!device || !outBuffer) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }
    auto backend = gfx::backend::BackendManager::instance().getBackend(device);
    if (!backend) {
        return GFX_RESULT_ERROR_NOT_FOUND;
    }
    GfxBackend backendType = gfx::backend::BackendManager::instance().getBackendType(device);
    GfxBuffer nativeBuffer = nullptr;
    GfxResult result = backend->deviceImportBuffer(device, descriptor, &nativeBuffer);
    if (result != GFX_RESULT_SUCCESS) {
        return result;
    }
    *outBuffer = gfx::backend::BackendManager::instance().wrap(backendType, nativeBuffer);
    return GFX_RESULT_SUCCESS;
}

GfxResult gfxDeviceImportTexture(GfxDevice device, const GfxTextureImportDescriptor* descriptor, GfxTexture* outTexture)
{
    if (!device || !outTexture) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }
    auto backend = gfx::backend::BackendManager::instance().getBackend(device);
    if (!backend) {
        return GFX_RESULT_ERROR_NOT_FOUND;
    }
    GfxBackend backendType = gfx::backend::BackendManager::instance().getBackendType(device);
    GfxTexture nativeTexture = nullptr;
    GfxResult result = backend->deviceImportTexture(device, descriptor, &nativeTexture);
    if (result != GFX_RESULT_SUCCESS) {
        return result;
    }
    *outTexture = gfx::backend::BackendManager::instance().wrap(backendType, nativeTexture);
    return GFX_RESULT_SUCCESS;
}

GfxResult gfxDeviceCreateSwapchain(GfxDevice device, GfxSurface surface, const GfxSwapchainDescriptor* descriptor, GfxSwapchain* outSwapchain)
{
    if (!device || !surface || !outSwapchain) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }
    auto backend = gfx::backend::BackendManager::instance().getBackend(device);
    if (!backend) {
        return GFX_RESULT_ERROR_NOT_FOUND;
    }

    GfxBackend backendType = gfx::backend::BackendManager::instance().getBackendType(device);
    GfxSwapchain nativeSwapchain = nullptr;
    GfxResult result = backend->deviceCreateSwapchain(
        device,
        surface,
        descriptor,
        &nativeSwapchain);

    if (result != GFX_RESULT_SUCCESS) {
        return result;
    }
    *outSwapchain = gfx::backend::BackendManager::instance().wrap(backendType, nativeSwapchain);
    return GFX_RESULT_SUCCESS;
}

GfxResult gfxDeviceCreateCommandEncoder(GfxDevice device, const GfxCommandEncoderDescriptor* descriptor, GfxCommandEncoder* outEncoder)
{
    if (!device || !outEncoder) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }
    auto backend = gfx::backend::BackendManager::instance().getBackend(device);
    if (!backend) {
        return GFX_RESULT_ERROR_NOT_FOUND;
    }

    GfxBackend backendType = gfx::backend::BackendManager::instance().getBackendType(device);
    GfxCommandEncoder nativeEncoder = nullptr;
    GfxResult result = backend->deviceCreateCommandEncoder(device, descriptor, &nativeEncoder);
    if (result != GFX_RESULT_SUCCESS) {
        return result;
    }

    *outEncoder = gfx::backend::BackendManager::instance().wrap(backendType, nativeEncoder);
    return GFX_RESULT_SUCCESS;
}

GfxResult gfxDeviceCreateRenderPass(GfxDevice device, const GfxRenderPassDescriptor* descriptor, GfxRenderPass* outRenderPass)
{
    if (!device || !outRenderPass) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }
    auto backend = gfx::backend::BackendManager::instance().getBackend(device);
    if (!backend) {
        return GFX_RESULT_ERROR_NOT_FOUND;
    }

    GfxBackend backendType = gfx::backend::BackendManager::instance().getBackendType(device);
    GfxRenderPass nativeRenderPass = nullptr;
    GfxResult result = backend->deviceCreateRenderPass(device, descriptor, &nativeRenderPass);
    if (result != GFX_RESULT_SUCCESS) {
        return result;
    }

    *outRenderPass = gfx::backend::BackendManager::instance().wrap(backendType, nativeRenderPass);
    return GFX_RESULT_SUCCESS;
}

GfxResult gfxDeviceCreateFramebuffer(GfxDevice device, const GfxFramebufferDescriptor* descriptor, GfxFramebuffer* outFramebuffer)
{
    if (!device || !outFramebuffer) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }
    auto backend = gfx::backend::BackendManager::instance().getBackend(device);
    if (!backend) {
        return GFX_RESULT_ERROR_NOT_FOUND;
    }

    GfxBackend backendType = gfx::backend::BackendManager::instance().getBackendType(device);
    GfxFramebuffer nativeFramebuffer = nullptr;
    GfxResult result = backend->deviceCreateFramebuffer(device, descriptor, &nativeFramebuffer);
    if (result != GFX_RESULT_SUCCESS) {
        return result;
    }

    *outFramebuffer = gfx::backend::BackendManager::instance().wrap(backendType, nativeFramebuffer);
    return GFX_RESULT_SUCCESS;
}

GfxResult gfxDeviceWaitIdle(GfxDevice device)
{
    if (!device) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }
    auto backend = gfx::backend::BackendManager::instance().getBackend(device);
    if (!backend) {
        return GFX_RESULT_ERROR_NOT_FOUND;
    }
    return backend->deviceWaitIdle(device);
}

GfxResult gfxDeviceGetLimits(GfxDevice device, GfxDeviceLimits* outLimits)
{
    if (!device || !outLimits) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }
    auto backend = gfx::backend::BackendManager::instance().getBackend(device);
    if (!backend) {
        return GFX_RESULT_ERROR_NOT_FOUND;
    }
    return backend->deviceGetLimits(device, outLimits);
}

// Macro for simple destroy functions
#define DESTROY_FUNC(TypeName, typeName)                                              \
    GfxResult gfx##TypeName##Destroy(Gfx##TypeName typeName)                          \
    {                                                                                 \
        if (!typeName) {                                                              \
            return GFX_RESULT_ERROR_INVALID_ARGUMENT;                                 \
        }                                                                             \
        auto backend = gfx::backend::BackendManager::instance().getBackend(typeName); \
        if (!backend) {                                                               \
            return GFX_RESULT_ERROR_NOT_FOUND;                                        \
        }                                                                             \
        GfxResult result = backend->typeName##Destroy(typeName);                      \
        gfx::backend::BackendManager::instance().unwrap(typeName);                    \
        return result;                                                                \
    }
DESTROY_FUNC(Surface, surface)
DESTROY_FUNC(Swapchain, swapchain)
DESTROY_FUNC(Buffer, buffer)
DESTROY_FUNC(Texture, texture)
DESTROY_FUNC(TextureView, textureView)
DESTROY_FUNC(Sampler, sampler)
DESTROY_FUNC(Shader, shader)
DESTROY_FUNC(BindGroupLayout, bindGroupLayout)
DESTROY_FUNC(BindGroup, bindGroup)
DESTROY_FUNC(RenderPipeline, renderPipeline)
DESTROY_FUNC(ComputePipeline, computePipeline)
DESTROY_FUNC(RenderPass, renderPass)
DESTROY_FUNC(Framebuffer, framebuffer)
DESTROY_FUNC(CommandEncoder, commandEncoder)
DESTROY_FUNC(Fence, fence)
DESTROY_FUNC(Semaphore, semaphore)
DESTROY_FUNC(QuerySet, querySet)

// Surface Functions
GfxResult gfxSurfaceEnumerateSupportedFormats(GfxSurface surface, uint32_t* formatCount, GfxTextureFormat* formats)
{
    if (!surface || !formatCount) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }
    auto backend = gfx::backend::BackendManager::instance().getBackend(surface);
    if (!backend) {
        return GFX_RESULT_ERROR_NOT_FOUND;
    }
    return backend->surfaceEnumerateSupportedFormats(surface, formatCount, formats);
}

GfxResult gfxSurfaceEnumerateSupportedPresentModes(GfxSurface surface, uint32_t* presentModeCount, GfxPresentMode* presentModes)
{
    if (!surface || !presentModeCount) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }
    auto backend = gfx::backend::BackendManager::instance().getBackend(surface);
    if (!backend) {
        return GFX_RESULT_ERROR_NOT_FOUND;
    }
    return backend->surfaceEnumerateSupportedPresentModes(surface, presentModeCount, presentModes);
}

// Swapchain Functions
GfxResult gfxSwapchainGetInfo(GfxSwapchain swapchain, GfxSwapchainInfo* outInfo)
{
    if (!swapchain || !outInfo) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }
    auto backend = gfx::backend::BackendManager::instance().getBackend(swapchain);
    if (!backend) {
        outInfo->width = 0;
        outInfo->height = 0;
        outInfo->format = GFX_TEXTURE_FORMAT_UNDEFINED;
        outInfo->imageCount = 0;
        return GFX_RESULT_ERROR_NOT_FOUND;
    }
    return backend->swapchainGetInfo(swapchain, outInfo);
}

GfxResult gfxSwapchainAcquireNextImage(GfxSwapchain swapchain, uint64_t timeoutNs,
    GfxSemaphore imageAvailableSemaphore, GfxFence fence, uint32_t* outImageIndex)
{
    if (!swapchain || !outImageIndex) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }
    auto backend = gfx::backend::BackendManager::instance().getBackend(swapchain);
    if (!backend) {
        return GFX_RESULT_ERROR_NOT_FOUND;
    }

    GfxSemaphore nativeSemaphore = imageAvailableSemaphore ? imageAvailableSemaphore : nullptr;
    GfxFence nativeFence = fence ? fence : nullptr;

    return backend->swapchainAcquireNextImage(swapchain, timeoutNs,
        nativeSemaphore, nativeFence, outImageIndex);
}

GfxResult gfxSwapchainGetTextureView(GfxSwapchain swapchain, uint32_t imageIndex, GfxTextureView* outView)
{
    if (!swapchain || !outView) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }
    auto backend = gfx::backend::BackendManager::instance().getBackend(swapchain);
    if (!backend) {
        return GFX_RESULT_ERROR_NOT_FOUND;
    }
    // Swapchain texture views are NOT wrapped - managed by swapchain
    return backend->swapchainGetTextureView(swapchain, imageIndex, outView);
}

GfxResult gfxSwapchainGetCurrentTextureView(GfxSwapchain swapchain, GfxTextureView* outView)
{
    if (!swapchain || !outView) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }
    auto backend = gfx::backend::BackendManager::instance().getBackend(swapchain);
    if (!backend) {
        return GFX_RESULT_ERROR_NOT_FOUND;
    }
    // Swapchain texture views are NOT wrapped - managed by swapchain
    return backend->swapchainGetCurrentTextureView(swapchain, outView);
}

GfxResult gfxSwapchainPresent(GfxSwapchain swapchain, const GfxPresentInfo* presentInfo)
{
    if (!swapchain) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }
    auto backend = gfx::backend::BackendManager::instance().getBackend(swapchain);
    if (!backend) {
        return GFX_RESULT_ERROR_NOT_FOUND;
    }
    return backend->swapchainPresent(swapchain, presentInfo);
}

// Buffer Functions
GfxResult gfxBufferGetInfo(GfxBuffer buffer, GfxBufferInfo* outInfo)
{
    if (!buffer || !outInfo) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }
    auto backend = gfx::backend::BackendManager::instance().getBackend(buffer);
    if (!backend) {
        return GFX_RESULT_ERROR_NOT_FOUND;
    }
    return backend->bufferGetInfo(buffer, outInfo);
}

GfxResult gfxBufferGetNativeHandle(GfxBuffer buffer, void** outHandle)
{
    if (!buffer || !outHandle) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }
    auto backend = gfx::backend::BackendManager::instance().getBackend(buffer);
    if (!backend) {
        return GFX_RESULT_ERROR_NOT_FOUND;
    }
    return backend->bufferGetNativeHandle(buffer, outHandle);
}

GfxResult gfxBufferMap(GfxBuffer buffer, uint64_t offset, uint64_t size, void** outMappedPointer)
{
    if (!buffer || !outMappedPointer) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }
    auto backend = gfx::backend::BackendManager::instance().getBackend(buffer);
    if (!backend) {
        return GFX_RESULT_ERROR_NOT_FOUND;
    }
    return backend->bufferMap(buffer, offset, size, outMappedPointer);
}

GfxResult gfxBufferUnmap(GfxBuffer buffer)
{
    if (!buffer) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }
    auto backend = gfx::backend::BackendManager::instance().getBackend(buffer);
    if (!backend) {
        return GFX_RESULT_ERROR_NOT_FOUND;
    }
    return backend->bufferUnmap(buffer);
}

// Texture Functions
GfxResult gfxTextureGetInfo(GfxTexture texture, GfxTextureInfo* outInfo)
{
    if (!texture || !outInfo) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }
    auto backend = gfx::backend::BackendManager::instance().getBackend(texture);
    if (!backend) {
        return GFX_RESULT_ERROR_NOT_FOUND;
    }
    return backend->textureGetInfo(texture, outInfo);
}

GfxResult gfxTextureGetNativeHandle(GfxTexture texture, void** outHandle)
{
    if (!texture || !outHandle) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }
    auto backend = gfx::backend::BackendManager::instance().getBackend(texture);
    if (!backend) {
        return GFX_RESULT_ERROR_NOT_FOUND;
    }
    return backend->textureGetNativeHandle(texture, outHandle);
}

GfxResult gfxTextureGetLayout(GfxTexture texture, GfxTextureLayout* outLayout)
{
    if (!texture || !outLayout) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }
    auto backend = gfx::backend::BackendManager::instance().getBackend(texture);
    if (!backend) {
        return GFX_RESULT_ERROR_NOT_FOUND;
    }
    return backend->textureGetLayout(texture, outLayout);
}

GfxResult gfxTextureCreateView(GfxTexture texture, const GfxTextureViewDescriptor* descriptor, GfxTextureView* outView)
{
    if (!texture || !outView) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }
    auto backend = gfx::backend::BackendManager::instance().getBackend(texture);
    if (!backend) {
        return GFX_RESULT_ERROR_NOT_FOUND;
    }

    GfxBackend backendType = gfx::backend::BackendManager::instance().getBackendType(texture);
    GfxTextureView nativeView = nullptr;
    GfxResult result = backend->textureCreateView(texture, descriptor, &nativeView);
    if (result != GFX_RESULT_SUCCESS) {
        return result;
    }

    *outView = gfx::backend::BackendManager::instance().wrap(backendType, nativeView);
    return GFX_RESULT_SUCCESS;
}

// Queue Functions
GfxResult gfxQueueSubmit(GfxQueue queue, const GfxSubmitDescriptor* submitInfo)
{
    if (!queue) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }
    auto backend = gfx::backend::BackendManager::instance().getBackend(queue);
    if (!backend) {
        return GFX_RESULT_ERROR_NOT_FOUND;
    }
    return backend->queueSubmit(queue, submitInfo);
}

GfxResult gfxQueueWriteBuffer(GfxQueue queue, GfxBuffer buffer, uint64_t offset, const void* data, uint64_t size)
{
    if (!queue) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }
    auto backend = gfx::backend::BackendManager::instance().getBackend(queue);
    if (!backend) {
        return GFX_RESULT_ERROR_NOT_FOUND;
    }
    return backend->queueWriteBuffer(queue, buffer, offset, data, size);
}

GfxResult gfxQueueWriteTexture(GfxQueue queue, GfxTexture texture, const GfxOrigin3D* origin, uint32_t mipLevel, const void* data, uint64_t dataSize, const GfxExtent3D* extent, GfxTextureLayout finalLayout)
{
    if (!queue) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }
    auto backend = gfx::backend::BackendManager::instance().getBackend(queue);
    if (!backend) {
        return GFX_RESULT_ERROR_NOT_FOUND;
    }

    return backend->queueWriteTexture(queue, texture, origin, mipLevel, data, dataSize, extent, finalLayout);
}

GfxResult gfxQueueWaitIdle(GfxQueue queue)
{
    if (!queue) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }
    auto backend = gfx::backend::BackendManager::instance().getBackend(queue);
    if (!backend) {
        return GFX_RESULT_ERROR_NOT_FOUND;
    }
    return backend->queueWaitIdle(queue);
}

GfxResult gfxCommandEncoderPipelineBarrier(GfxCommandEncoder commandEncoder, const GfxPipelineBarrierDescriptor* descriptor)
{
    if (!commandEncoder) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }
    auto backend = gfx::backend::BackendManager::instance().getBackend(commandEncoder);
    if (!backend) {
        return GFX_RESULT_ERROR_NOT_FOUND;
    }
    return backend->commandEncoderPipelineBarrier(commandEncoder, descriptor);
}

GfxResult gfxCommandEncoderGenerateMipmaps(GfxCommandEncoder commandEncoder, GfxTexture texture)
{
    if (!commandEncoder) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }
    auto backend = gfx::backend::BackendManager::instance().getBackend(commandEncoder);
    if (!backend) {
        return GFX_RESULT_ERROR_NOT_FOUND;
    }
    return backend->commandEncoderGenerateMipmaps(commandEncoder, texture);
}

GfxResult gfxCommandEncoderGenerateMipmapsRange(GfxCommandEncoder commandEncoder, GfxTexture texture,
    uint32_t baseMipLevel, uint32_t levelCount)
{
    if (!commandEncoder) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }
    auto backend = gfx::backend::BackendManager::instance().getBackend(commandEncoder);
    if (!backend) {
        return GFX_RESULT_ERROR_NOT_FOUND;
    }
    return backend->commandEncoderGenerateMipmapsRange(commandEncoder, texture, baseMipLevel, levelCount);
}

GfxResult gfxCommandEncoderEnd(GfxCommandEncoder commandEncoder)
{
    if (!commandEncoder) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }
    auto backend = gfx::backend::BackendManager::instance().getBackend(commandEncoder);
    if (!backend) {
        return GFX_RESULT_ERROR_NOT_FOUND;
    }
    return backend->commandEncoderEnd(commandEncoder);
}

GfxResult gfxCommandEncoderBegin(GfxCommandEncoder commandEncoder)
{
    if (!commandEncoder) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }
    auto backend = gfx::backend::BackendManager::instance().getBackend(commandEncoder);
    if (!backend) {
        return GFX_RESULT_ERROR_NOT_FOUND;
    }
    return backend->commandEncoderBegin(commandEncoder);
}

GfxResult gfxCommandEncoderWriteTimestamp(GfxCommandEncoder commandEncoder, GfxQuerySet querySet, uint32_t queryIndex)
{
    if (!commandEncoder || !querySet) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }
    auto backend = gfx::backend::BackendManager::instance().getBackend(commandEncoder);
    if (!backend) {
        return GFX_RESULT_ERROR_NOT_FOUND;
    }
    return backend->commandEncoderWriteTimestamp(commandEncoder, querySet, queryIndex);
}

GfxResult gfxCommandEncoderResolveQuerySet(GfxCommandEncoder commandEncoder, GfxQuerySet querySet, uint32_t firstQuery, uint32_t queryCount, GfxBuffer destinationBuffer, uint64_t destinationOffset)
{
    if (!commandEncoder || !querySet || !destinationBuffer) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }
    auto backend = gfx::backend::BackendManager::instance().getBackend(commandEncoder);
    if (!backend) {
        return GFX_RESULT_ERROR_NOT_FOUND;
    }
    return backend->commandEncoderResolveQuerySet(commandEncoder, querySet, firstQuery, queryCount, destinationBuffer, destinationOffset);
}

GfxResult gfxCommandEncoderBeginRenderPass(GfxCommandEncoder encoder,
    const GfxRenderPassBeginDescriptor* beginDescriptor,
    GfxRenderPassEncoder* outEncoder)
{
    if (!encoder || !outEncoder) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }
    auto backend = gfx::backend::BackendManager::instance().getBackend(encoder);
    if (!backend) {
        return GFX_RESULT_ERROR_FEATURE_NOT_SUPPORTED;
    }

    GfxBackend backendType = gfx::backend::BackendManager::instance().getBackendType(encoder);
    GfxRenderPassEncoder nativePass = nullptr;
    GfxResult result = backend->commandEncoderBeginRenderPass(
        encoder,
        beginDescriptor,
        &nativePass);
    if (result != GFX_RESULT_SUCCESS) {
        return result;
    }

    *outEncoder = gfx::backend::BackendManager::instance().wrap(backendType, nativePass);
    return GFX_RESULT_SUCCESS;
}

GfxResult gfxCommandEncoderBeginComputePass(GfxCommandEncoder encoder, const GfxComputePassBeginDescriptor* beginDescriptor, GfxComputePassEncoder* outEncoder)
{
    if (!encoder || !outEncoder) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }
    auto backend = gfx::backend::BackendManager::instance().getBackend(encoder);
    if (!backend) {
        return GFX_RESULT_ERROR_FEATURE_NOT_SUPPORTED;
    }

    GfxBackend backendType = gfx::backend::BackendManager::instance().getBackendType(encoder);
    GfxComputePassEncoder nativePass = nullptr;
    GfxResult result = backend->commandEncoderBeginComputePass(encoder, beginDescriptor, &nativePass);
    if (result != GFX_RESULT_SUCCESS) {
        return result;
    }

    *outEncoder = gfx::backend::BackendManager::instance().wrap(backendType, nativePass);
    return GFX_RESULT_SUCCESS;
}

GfxResult gfxCommandEncoderCopyBufferToBuffer(GfxCommandEncoder commandEncoder, const GfxCopyBufferToBufferDescriptor* descriptor)
{
    if (!commandEncoder) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }
    auto backend = gfx::backend::BackendManager::instance().getBackend(commandEncoder);
    if (!backend) {
        return GFX_RESULT_ERROR_NOT_FOUND;
    }
    return backend->commandEncoderCopyBufferToBuffer(commandEncoder, descriptor);
}

GfxResult gfxCommandEncoderCopyBufferToTexture(GfxCommandEncoder commandEncoder, const GfxCopyBufferToTextureDescriptor* descriptor)
{
    if (!commandEncoder) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }
    auto backend = gfx::backend::BackendManager::instance().getBackend(commandEncoder);
    if (!backend) {
        return GFX_RESULT_ERROR_NOT_FOUND;
    }
    return backend->commandEncoderCopyBufferToTexture(commandEncoder, descriptor);
}

GfxResult gfxCommandEncoderCopyTextureToBuffer(GfxCommandEncoder commandEncoder, const GfxCopyTextureToBufferDescriptor* descriptor)
{
    if (!commandEncoder) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }
    auto backend = gfx::backend::BackendManager::instance().getBackend(commandEncoder);
    if (!backend) {
        return GFX_RESULT_ERROR_NOT_FOUND;
    }
    return backend->commandEncoderCopyTextureToBuffer(commandEncoder, descriptor);
}

GfxResult gfxCommandEncoderCopyTextureToTexture(GfxCommandEncoder commandEncoder, const GfxCopyTextureToTextureDescriptor* descriptor)
{
    if (!commandEncoder) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }
    auto backend = gfx::backend::BackendManager::instance().getBackend(commandEncoder);
    if (!backend) {
        return GFX_RESULT_ERROR_NOT_FOUND;
    }
    return backend->commandEncoderCopyTextureToTexture(commandEncoder, descriptor);
}

GfxResult gfxCommandEncoderBlitTextureToTexture(GfxCommandEncoder commandEncoder, const GfxBlitTextureToTextureDescriptor* descriptor)
{
    if (!commandEncoder) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }
    auto backend = gfx::backend::BackendManager::instance().getBackend(commandEncoder);
    if (!backend) {
        return GFX_RESULT_ERROR_NOT_FOUND;
    }
    return backend->commandEncoderBlitTextureToTexture(commandEncoder, descriptor);
}

// Render Pass Encoder Functions
GfxResult gfxRenderPassEncoderSetPipeline(GfxRenderPassEncoder encoder, GfxRenderPipeline pipeline)
{
    if (!encoder) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }
    auto backend = gfx::backend::BackendManager::instance().getBackend(encoder);
    if (!backend) {
        return GFX_RESULT_ERROR_NOT_FOUND;
    }
    return backend->renderPassEncoderSetPipeline(
        encoder,
        pipeline);
}

GfxResult gfxRenderPassEncoderSetBindGroup(GfxRenderPassEncoder encoder, uint32_t groupIndex, GfxBindGroup bindGroup, const uint32_t* dynamicOffsets, uint32_t dynamicOffsetCount)
{
    if (!encoder) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }
    auto backend = gfx::backend::BackendManager::instance().getBackend(encoder);
    if (!backend) {
        return GFX_RESULT_ERROR_NOT_FOUND;
    }
    return backend->renderPassEncoderSetBindGroup(
        encoder,
        groupIndex,
        bindGroup,
        dynamicOffsets,
        dynamicOffsetCount);
}

GfxResult gfxRenderPassEncoderSetVertexBuffer(GfxRenderPassEncoder encoder, uint32_t slot, GfxBuffer buffer, uint64_t offset, uint64_t size)
{
    if (!encoder) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }
    auto backend = gfx::backend::BackendManager::instance().getBackend(encoder);
    if (!backend) {
        return GFX_RESULT_ERROR_NOT_FOUND;
    }
    return backend->renderPassEncoderSetVertexBuffer(
        encoder,
        slot,
        buffer,
        offset,
        size);
}

GfxResult gfxRenderPassEncoderSetIndexBuffer(GfxRenderPassEncoder encoder, GfxBuffer buffer, GfxIndexFormat format, uint64_t offset, uint64_t size)
{
    if (!encoder) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }
    auto backend = gfx::backend::BackendManager::instance().getBackend(encoder);
    if (!backend) {
        return GFX_RESULT_ERROR_NOT_FOUND;
    }
    return backend->renderPassEncoderSetIndexBuffer(
        encoder,
        buffer,
        format,
        offset,
        size);
}

GfxResult gfxRenderPassEncoderSetViewport(GfxRenderPassEncoder encoder, const GfxViewport* viewport)
{
    if (!encoder) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }
    auto backend = gfx::backend::BackendManager::instance().getBackend(encoder);
    if (!backend) {
        return GFX_RESULT_ERROR_NOT_FOUND;
    }
    return backend->renderPassEncoderSetViewport(
        encoder,
        viewport);
}

GfxResult gfxRenderPassEncoderSetScissorRect(GfxRenderPassEncoder encoder, const GfxScissorRect* scissor)
{
    if (!encoder) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }
    auto backend = gfx::backend::BackendManager::instance().getBackend(encoder);
    if (!backend) {
        return GFX_RESULT_ERROR_NOT_FOUND;
    }
    return backend->renderPassEncoderSetScissorRect(
        encoder,
        scissor);
}

GfxResult gfxRenderPassEncoderDraw(GfxRenderPassEncoder encoder, uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance)
{
    if (!encoder) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }
    auto backend = gfx::backend::BackendManager::instance().getBackend(encoder);
    if (!backend) {
        return GFX_RESULT_ERROR_NOT_FOUND;
    }
    return backend->renderPassEncoderDraw(
        encoder,
        vertexCount, instanceCount, firstVertex, firstInstance);
}

GfxResult gfxRenderPassEncoderDrawIndexed(GfxRenderPassEncoder encoder, uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t baseVertex, uint32_t firstInstance)
{
    if (!encoder) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }
    auto backend = gfx::backend::BackendManager::instance().getBackend(encoder);
    if (!backend) {
        return GFX_RESULT_ERROR_NOT_FOUND;
    }
    return backend->renderPassEncoderDrawIndexed(
        encoder,
        indexCount, instanceCount, firstIndex, baseVertex, firstInstance);
}

GfxResult gfxRenderPassEncoderDrawIndirect(GfxRenderPassEncoder encoder, GfxBuffer indirectBuffer, uint64_t indirectOffset)
{
    if (!encoder) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }
    auto backend = gfx::backend::BackendManager::instance().getBackend(encoder);
    if (!backend) {
        return GFX_RESULT_ERROR_NOT_FOUND;
    }
    return backend->renderPassEncoderDrawIndirect(encoder, indirectBuffer, indirectOffset);
}

GfxResult gfxRenderPassEncoderDrawIndexedIndirect(GfxRenderPassEncoder encoder, GfxBuffer indirectBuffer, uint64_t indirectOffset)
{
    if (!encoder) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }
    auto backend = gfx::backend::BackendManager::instance().getBackend(encoder);
    if (!backend) {
        return GFX_RESULT_ERROR_NOT_FOUND;
    }
    return backend->renderPassEncoderDrawIndexedIndirect(encoder, indirectBuffer, indirectOffset);
}

GfxResult gfxRenderPassEncoderEnd(GfxRenderPassEncoder encoder)
{
    if (!encoder) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }
    auto backend = gfx::backend::BackendManager::instance().getBackend(encoder);
    if (!backend) {
        return GFX_RESULT_ERROR_NOT_FOUND;
    }
    return backend->renderPassEncoderEnd(encoder);
}

GfxResult gfxRenderPassEncoderBeginOcclusionQuery(GfxRenderPassEncoder renderPassEncoder, GfxQuerySet querySet, uint32_t queryIndex)
{
    if (!renderPassEncoder || !querySet) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }
    auto backend = gfx::backend::BackendManager::instance().getBackend(renderPassEncoder);
    if (!backend) {
        return GFX_RESULT_ERROR_NOT_FOUND;
    }
    return backend->renderPassEncoderBeginOcclusionQuery(renderPassEncoder, querySet, queryIndex);
}

GfxResult gfxRenderPassEncoderEndOcclusionQuery(GfxRenderPassEncoder renderPassEncoder)
{
    if (!renderPassEncoder) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }
    auto backend = gfx::backend::BackendManager::instance().getBackend(renderPassEncoder);
    if (!backend) {
        return GFX_RESULT_ERROR_NOT_FOUND;
    }
    return backend->renderPassEncoderEndOcclusionQuery(renderPassEncoder);
}

// Compute Pass Encoder Functions
GfxResult gfxComputePassEncoderSetPipeline(GfxComputePassEncoder encoder, GfxComputePipeline pipeline)
{
    if (!encoder) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }
    auto backend = gfx::backend::BackendManager::instance().getBackend(encoder);
    if (!backend) {
        return GFX_RESULT_ERROR_NOT_FOUND;
    }
    return backend->computePassEncoderSetPipeline(
        encoder,
        pipeline);
}

GfxResult gfxComputePassEncoderSetBindGroup(GfxComputePassEncoder encoder, uint32_t groupIndex, GfxBindGroup bindGroup, const uint32_t* dynamicOffsets, uint32_t dynamicOffsetCount)
{
    if (!encoder) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }
    auto backend = gfx::backend::BackendManager::instance().getBackend(encoder);
    if (!backend) {
        return GFX_RESULT_ERROR_NOT_FOUND;
    }
    return backend->computePassEncoderSetBindGroup(
        encoder,
        groupIndex,
        bindGroup,
        dynamicOffsets,
        dynamicOffsetCount);
}

GfxResult gfxComputePassEncoderDispatch(GfxComputePassEncoder encoder, uint32_t workgroupCountX, uint32_t workgroupCountY, uint32_t workgroupCountZ)
{
    if (!encoder) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }
    auto backend = gfx::backend::BackendManager::instance().getBackend(encoder);
    if (!backend) {
        return GFX_RESULT_ERROR_NOT_FOUND;
    }
    return backend->computePassEncoderDispatch(
        encoder,
        workgroupCountX, workgroupCountY, workgroupCountZ);
}

GfxResult gfxComputePassEncoderDispatchIndirect(GfxComputePassEncoder encoder, GfxBuffer indirectBuffer, uint64_t indirectOffset)
{
    if (!encoder) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }
    auto backend = gfx::backend::BackendManager::instance().getBackend(encoder);
    if (!backend) {
        return GFX_RESULT_ERROR_NOT_FOUND;
    }
    return backend->computePassEncoderDispatchIndirect(encoder, indirectBuffer, indirectOffset);
}

GfxResult gfxComputePassEncoderEnd(GfxComputePassEncoder encoder)
{
    if (!encoder) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }
    auto backend = gfx::backend::BackendManager::instance().getBackend(encoder);
    if (!backend) {
        return GFX_RESULT_ERROR_NOT_FOUND;
    }
    return backend->computePassEncoderEnd(encoder);
}

// Fence Functions
GfxResult gfxFenceGetStatus(GfxFence fence, bool* isSignaled)
{
    if (!fence || !isSignaled) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }
    auto backend = gfx::backend::BackendManager::instance().getBackend(fence);
    if (!backend) {
        return GFX_RESULT_ERROR_FEATURE_NOT_SUPPORTED;
    }
    return backend->fenceGetStatus(fence, isSignaled);
}

GfxResult gfxFenceWait(GfxFence fence, uint64_t timeoutNs)
{
    if (!fence) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }
    auto backend = gfx::backend::BackendManager::instance().getBackend(fence);
    if (!backend) {
        return GFX_RESULT_ERROR_FEATURE_NOT_SUPPORTED;
    }
    return backend->fenceWait(fence, timeoutNs);
}

GfxResult gfxFenceReset(GfxFence fence)
{
    if (!fence) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }
    auto backend = gfx::backend::BackendManager::instance().getBackend(fence);
    if (!backend) {
        return GFX_RESULT_ERROR_NOT_FOUND;
    }
    return backend->fenceReset(fence);
}

// Semaphore Functions
GfxResult gfxSemaphoreGetType(GfxSemaphore semaphore, GfxSemaphoreType* outType)
{
    if (!semaphore || !outType) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }
    auto backend = gfx::backend::BackendManager::instance().getBackend(semaphore);
    if (!backend) {
        return GFX_RESULT_ERROR_NOT_FOUND;
    }
    return backend->semaphoreGetType(semaphore, outType);
}

GfxResult gfxSemaphoreGetValue(GfxSemaphore semaphore, uint64_t* outValue)
{
    if (!semaphore || !outValue) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }
    auto backend = gfx::backend::BackendManager::instance().getBackend(semaphore);
    if (!backend) {
        return GFX_RESULT_ERROR_NOT_FOUND;
    }
    return backend->semaphoreGetValue(semaphore, outValue);
}

GfxResult gfxSemaphoreSignal(GfxSemaphore semaphore, uint64_t value)
{
    if (!semaphore) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }
    auto backend = gfx::backend::BackendManager::instance().getBackend(semaphore);
    if (!backend) {
        return GFX_RESULT_ERROR_FEATURE_NOT_SUPPORTED;
    }
    return backend->semaphoreSignal(semaphore, value);
}

GfxResult gfxSemaphoreWait(GfxSemaphore semaphore, uint64_t value, uint64_t timeoutNs)
{
    if (!semaphore) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }
    auto backend = gfx::backend::BackendManager::instance().getBackend(semaphore);
    if (!backend) {
        return GFX_RESULT_ERROR_FEATURE_NOT_SUPPORTED;
    }
    return backend->semaphoreWait(semaphore, value, timeoutNs);
}

// Helper function to deduce access flags from texture layout
GfxAccessFlags gfxGetAccessFlagsForLayout(GfxTextureLayout layout)
{
    // Use Vulkan-style explicit access flags (deterministic mapping)
    // WebGPU backends will ignore these as they use implicit synchronization
    auto backend = gfx::backend::BackendManager::instance().getBackend(GFX_BACKEND_VULKAN);
    if (!backend) {
        return GFX_ACCESS_NONE;
    }

    return backend->getAccessFlagsForLayout(layout);
}

void gfxSetLogCallback(GfxLogCallback callback, void* userData)
{
    gfx::common::Logger::instance().setCallback(callback, userData);
}

uint64_t gfxAlignUp(uint64_t value, uint64_t alignment)
{
    if (alignment == 0) {
        return value;
    }
    return (value + alignment - 1) & ~(alignment - 1);
}

uint64_t gfxAlignDown(uint64_t value, uint64_t alignment)
{
    if (alignment == 0) {
        return value;
    }
    return value & ~(alignment - 1);
}

uint32_t gfxGetFormatBytesPerPixel(GfxTextureFormat format)
{
    switch (format) {
    case GFX_TEXTURE_FORMAT_R8_UNORM:
        return 1;
    case GFX_TEXTURE_FORMAT_R8G8_UNORM:
        return 2;
    case GFX_TEXTURE_FORMAT_R8G8B8A8_UNORM:
    case GFX_TEXTURE_FORMAT_R8G8B8A8_UNORM_SRGB:
    case GFX_TEXTURE_FORMAT_B8G8R8A8_UNORM:
    case GFX_TEXTURE_FORMAT_B8G8R8A8_UNORM_SRGB:
        return 4;
    case GFX_TEXTURE_FORMAT_R16_FLOAT:
        return 2;
    case GFX_TEXTURE_FORMAT_R16G16_FLOAT:
        return 4;
    case GFX_TEXTURE_FORMAT_R16G16B16A16_FLOAT:
        return 8;
    case GFX_TEXTURE_FORMAT_R32_FLOAT:
        return 4;
    case GFX_TEXTURE_FORMAT_R32G32_FLOAT:
        return 8;
    case GFX_TEXTURE_FORMAT_R32G32B32_FLOAT:
        return 12;
    case GFX_TEXTURE_FORMAT_R32G32B32A32_FLOAT:
        return 16;
    case GFX_TEXTURE_FORMAT_DEPTH16_UNORM:
        return 2;
    case GFX_TEXTURE_FORMAT_DEPTH24_PLUS:
        return 4;
    case GFX_TEXTURE_FORMAT_DEPTH32_FLOAT:
        return 4;
    case GFX_TEXTURE_FORMAT_DEPTH24_PLUS_STENCIL8:
        return 4;
    case GFX_TEXTURE_FORMAT_DEPTH32_FLOAT_STENCIL8:
        return 8;
    case GFX_TEXTURE_FORMAT_UNDEFINED:
    default:
        return 0;
    }
}

GfxPlatformWindowHandle gfxPlatformWindowHandleFromXlib(void* display, unsigned long window)
{
    GfxPlatformWindowHandle handle = {};
    handle.windowingSystem = GFX_WINDOWING_SYSTEM_XLIB;
    handle.xlib.display = display;
    handle.xlib.window = window;
    return handle;
}

GfxPlatformWindowHandle gfxPlatformWindowHandleFromWayland(void* surface, void* display)
{
    GfxPlatformWindowHandle handle = {};
    handle.windowingSystem = GFX_WINDOWING_SYSTEM_WAYLAND;
    handle.wayland.surface = surface;
    handle.wayland.display = display;
    return handle;
}

GfxPlatformWindowHandle gfxPlatformWindowHandleFromXCB(void* connection, uint32_t window)
{
    GfxPlatformWindowHandle handle = {};
    handle.windowingSystem = GFX_WINDOWING_SYSTEM_XCB;
    handle.xcb.connection = connection;
    handle.xcb.window = window;
    return handle;
}

GfxPlatformWindowHandle gfxPlatformWindowHandleFromWin32(void* hwnd, void* hinstance)
{
    GfxPlatformWindowHandle handle = {};
    handle.windowingSystem = GFX_WINDOWING_SYSTEM_WIN32;
    handle.win32.hwnd = hwnd;
    handle.win32.hinstance = hinstance;
    return handle;
}

GfxPlatformWindowHandle gfxPlatformWindowHandleFromEmscripten(const char* canvasSelector)
{
    GfxPlatformWindowHandle handle = {};
    handle.windowingSystem = GFX_WINDOWING_SYSTEM_EMSCRIPTEN;
    handle.emscripten.canvasSelector = canvasSelector;
    return handle;
}

GfxPlatformWindowHandle gfxPlatformWindowHandleFromAndroid(void* window)
{
    GfxPlatformWindowHandle handle = {};
    handle.windowingSystem = GFX_WINDOWING_SYSTEM_ANDROID;
    handle.android.window = window;
    return handle;
}

GfxPlatformWindowHandle gfxPlatformWindowHandleFromMetal(void* layer)
{
    GfxPlatformWindowHandle handle = {};
    handle.windowingSystem = GFX_WINDOWING_SYSTEM_METAL;
    handle.metal.layer = layer;
    return handle;
}

} // extern "C"
