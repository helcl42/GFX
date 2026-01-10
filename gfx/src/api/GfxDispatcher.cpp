#include "backend/BackendFactory.h"
#include "backend/BackendManager.h"

#include <gfx/gfx.h>

#include <vector>

// ============================================================================
// C API Implementation
// ============================================================================

extern "C" {

bool gfxLoadBackend(GfxBackend backend)
{
    if (backend == GFX_BACKEND_AUTO) {
#ifdef GFX_ENABLE_VULKAN
        if (gfxLoadBackend(GFX_BACKEND_VULKAN)) {
            return true;
        }
#endif
#ifdef GFX_ENABLE_WEBGPU
        if (gfxLoadBackend(GFX_BACKEND_WEBGPU)) {
            return true;
        }
#endif
        return false;
    }

    if (backend < 0 || backend >= GFX_BACKEND_AUTO) {
        return false;
    }

    auto& manager = gfx::BackendManager::getInstance();

    // Check if backend needs to be created
    if (!manager.getBackendAPI(backend)) {
        const gfx::IBackend* backendImpl = gfx::BackendFactory::createBackend(backend);
        if (!backendImpl) {
            return false;
        }

        return manager.loadBackend(backend, backendImpl);
    }

    // Backend already loaded, just increment refcount
    return manager.loadBackend(backend, manager.getBackendAPI(backend));
}

void gfxUnloadBackend(GfxBackend backend)
{
    if (backend == GFX_BACKEND_AUTO) {
        // Unload the first loaded backend
        auto& manager = gfx::BackendManager::getInstance();
#ifdef GFX_ENABLE_VULKAN
        if (manager.getBackendAPI(GFX_BACKEND_VULKAN)) {
            gfxUnloadBackend(GFX_BACKEND_VULKAN);
            return;
        }
#endif
#ifdef GFX_ENABLE_WEBGPU
        if (manager.getBackendAPI(GFX_BACKEND_WEBGPU)) {
            gfxUnloadBackend(GFX_BACKEND_WEBGPU);
            return;
        }
#endif
        return;
    }

    if (backend >= 0 && backend < GFX_BACKEND_AUTO) {
        gfx::BackendManager::getInstance().unloadBackend(backend);
    }
}

bool gfxLoadAllBackends(void)
{
    bool loadedAny = false;
#ifdef GFX_ENABLE_VULKAN
    if (gfxLoadBackend(GFX_BACKEND_VULKAN)) {
        loadedAny = true;
    }
#endif
#ifdef GFX_ENABLE_WEBGPU
    if (gfxLoadBackend(GFX_BACKEND_WEBGPU)) {
        loadedAny = true;
    }
#endif
    return loadedAny;
}

void gfxUnloadAllBackends(void)
{
    auto& manager = gfx::BackendManager::getInstance();
#ifdef GFX_ENABLE_VULKAN
    while (manager.getBackendAPI(GFX_BACKEND_VULKAN)) {
        gfxUnloadBackend(GFX_BACKEND_VULKAN);
    }
#endif
#ifdef GFX_ENABLE_WEBGPU
    while (manager.getBackendAPI(GFX_BACKEND_WEBGPU)) {
        gfxUnloadBackend(GFX_BACKEND_WEBGPU);
    }
#endif
}

// Instance Functions
GfxResult gfxCreateInstance(const GfxInstanceDescriptor* descriptor, GfxInstance* outInstance)
{
    if (!descriptor || !outInstance) {
        return GFX_RESULT_ERROR_INVALID_PARAMETER;
    }

    *outInstance = nullptr;

    GfxBackend backend = descriptor->backend;
    auto& manager = gfx::BackendManager::getInstance();

    if (backend == GFX_BACKEND_AUTO) {
#ifdef GFX_ENABLE_VULKAN
        if (manager.getBackendAPI(GFX_BACKEND_VULKAN)) {
            backend = GFX_BACKEND_VULKAN;
        }
#endif
#ifdef GFX_ENABLE_WEBGPU
        if (backend == GFX_BACKEND_AUTO && manager.getBackendAPI(GFX_BACKEND_WEBGPU)) {
            backend = GFX_BACKEND_WEBGPU;
        }
#endif
        if (backend == GFX_BACKEND_AUTO) {
            return GFX_RESULT_ERROR_BACKEND_NOT_LOADED;
        }
    }

    auto api = gfx::getBackendAPI(backend);
    if (!api) {
        return GFX_RESULT_ERROR_BACKEND_NOT_LOADED;
    }

    GfxInstance nativeInstance = nullptr;
    GfxResult result = api->createInstance(descriptor, &nativeInstance);
    if (result != GFX_RESULT_SUCCESS) {
        return result;
    }

    *outInstance = gfx::wrap(backend, nativeInstance);
    return GFX_RESULT_SUCCESS;
}

void gfxInstanceDestroy(GfxInstance instance)
{
    if (!instance) {
        return;
    }
    auto api = gfx::getAPI(instance);
    if (api) {
        api->instanceDestroy(gfx::native(instance));
    }
    gfx::unwrap(instance);
}

void gfxInstanceSetDebugCallback(GfxInstance instance, GfxDebugCallback callback, void* userData)
{
    if (!instance) {
        return;
    }
    auto api = gfx::getAPI(instance);
    if (api) {
        api->instanceSetDebugCallback(gfx::native(instance), callback, userData);
    }
}

GfxResult gfxInstanceRequestAdapter(GfxInstance instance, const GfxAdapterDescriptor* descriptor, GfxAdapter* outAdapter)
{
    if (!instance || !outAdapter) {
        return GFX_RESULT_ERROR_INVALID_PARAMETER;
    }

    *outAdapter = nullptr;
    auto api = gfx::getAPI(instance);
    if (!api) {
        return GFX_RESULT_ERROR_FEATURE_NOT_SUPPORTED;
    }

    GfxBackend backend = gfx::getBackend(instance);
    GfxAdapter nativeAdapter = nullptr;
    GfxResult result = api->instanceRequestAdapter(gfx::native(instance), descriptor, &nativeAdapter);
    if (result != GFX_RESULT_SUCCESS) {
        return result;
    }

    *outAdapter = gfx::wrap(backend, nativeAdapter);
    return GFX_RESULT_SUCCESS;
}

uint32_t gfxInstanceEnumerateAdapters(GfxInstance instance, GfxAdapter* adapters, uint32_t maxAdapters)
{
    if (!instance) {
        return 0;
    }
    auto api = gfx::getAPI(instance);
    if (!api) {
        return 0;
    }
    return api->instanceEnumerateAdapters(gfx::native(instance), adapters, maxAdapters);
}

// Adapter Functions
void gfxAdapterDestroy(GfxAdapter adapter)
{
    if (!adapter) {
        return;
    }
    auto api = gfx::getAPI(adapter);
    if (api) {
        api->adapterDestroy(gfx::native(adapter));
    }
    gfx::unwrap(adapter);
}

GfxResult gfxAdapterCreateDevice(GfxAdapter adapter, const GfxDeviceDescriptor* descriptor, GfxDevice* outDevice)
{
    if (!adapter || !outDevice) {
        return GFX_RESULT_ERROR_INVALID_PARAMETER;
    }

    *outDevice = nullptr;
    auto api = gfx::getAPI(adapter);
    if (!api) {
        return GFX_RESULT_ERROR_FEATURE_NOT_SUPPORTED;
    }

    GfxBackend backend = gfx::getBackend(adapter);
    GfxDevice nativeDevice = nullptr;
    GfxResult result = api->adapterCreateDevice(gfx::native(adapter), descriptor, &nativeDevice);
    if (result != GFX_RESULT_SUCCESS) {
        return result;
    }

    *outDevice = gfx::wrap(backend, nativeDevice);
    return GFX_RESULT_SUCCESS;
}

void gfxAdapterGetInfo(GfxAdapter adapter, GfxAdapterInfo* outInfo)
{
    if (!adapter || !outInfo) {
        return;
    }
    auto api = gfx::getAPI(adapter);
    if (!api) {
        return;
    }
    api->adapterGetInfo(gfx::native(adapter), outInfo);
}

void gfxAdapterGetLimits(GfxAdapter adapter, GfxDeviceLimits* outLimits)
{
    if (!adapter || !outLimits) {
        return;
    }
    auto api = gfx::getAPI(adapter);
    if (!api) {
        return;
    }
    api->adapterGetLimits(gfx::native(adapter), outLimits);
}

// Device Functions
void gfxDeviceDestroy(GfxDevice device)
{
    if (!device) {
        return;
    }
    auto api = gfx::getAPI(device);
    if (api) {
        api->deviceDestroy(gfx::native(device));
    }
    gfx::unwrap(device);
}

GfxQueue gfxDeviceGetQueue(GfxDevice device)
{
    if (!device) {
        return nullptr;
    }
    auto api = gfx::getAPI(device);
    if (!api) {
        return nullptr;
    }

    GfxBackend backend = gfx::getBackend(device);
    GfxQueue nativeQueue = api->deviceGetQueue(gfx::native(device));
    if (!nativeQueue) {
        return nullptr;
    }

    return gfx::wrap(backend, nativeQueue);
}

// Macro to generate device create functions with less code duplication
#define DEVICE_CREATE_FUNC(TypeName, funcName)                                                                                       \
    GfxResult gfxDeviceCreate##funcName(GfxDevice device, const Gfx##funcName##Descriptor* descriptor, Gfx##TypeName* out##TypeName) \
    {                                                                                                                                \
        if (!device || !descriptor || !out##TypeName)                                                                                \
            return GFX_RESULT_ERROR_INVALID_PARAMETER;                                                                               \
        *out##TypeName = nullptr;                                                                                                    \
        auto api = gfx::getAPI(device);                                                                                              \
        if (!api)                                                                                                                    \
            return GFX_RESULT_ERROR_FEATURE_NOT_SUPPORTED;                                                                           \
        GfxBackend backend = gfx::getBackend(device);                                                                                \
        Gfx##TypeName native##TypeName = nullptr;                                                                                    \
        GfxResult result = api->deviceCreate##funcName(gfx::native(device), descriptor, &native##TypeName);                          \
        if (result != GFX_RESULT_SUCCESS)                                                                                            \
            return result;                                                                                                           \
        *out##TypeName = gfx::wrap(backend, native##TypeName);                                                                       \
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

// Import functions for external resources
GfxResult gfxDeviceImportBuffer(GfxDevice device, const GfxExternalBufferDescriptor* descriptor, GfxBuffer* outBuffer)
{
    if (!device || !descriptor || !outBuffer) {
        return GFX_RESULT_ERROR_INVALID_PARAMETER;
    }
    *outBuffer = nullptr;
    auto api = gfx::getAPI(device);
    if (!api) {
        return GFX_RESULT_ERROR_FEATURE_NOT_SUPPORTED;
    }
    GfxBackend backend = gfx::getBackend(device);
    GfxBuffer nativeBuffer = nullptr;
    GfxResult result = api->deviceImportBuffer(gfx::native(device), descriptor, &nativeBuffer);
    if (result != GFX_RESULT_SUCCESS) {
        return result;
    }
    *outBuffer = gfx::wrap(backend, nativeBuffer);
    return GFX_RESULT_SUCCESS;
}

GfxResult gfxDeviceImportTexture(GfxDevice device, const GfxExternalTextureDescriptor* descriptor, GfxTexture* outTexture)
{
    if (!device || !descriptor || !outTexture) {
        return GFX_RESULT_ERROR_INVALID_PARAMETER;
    }
    *outTexture = nullptr;
    auto api = gfx::getAPI(device);
    if (!api) {
        return GFX_RESULT_ERROR_FEATURE_NOT_SUPPORTED;
    }
    GfxBackend backend = gfx::getBackend(device);
    GfxTexture nativeTexture = nullptr;
    GfxResult result = api->deviceImportTexture(gfx::native(device), descriptor, &nativeTexture);
    if (result != GFX_RESULT_SUCCESS) {
        return result;
    }
    *outTexture = gfx::wrap(backend, nativeTexture);
    return GFX_RESULT_SUCCESS;
}

GfxResult gfxDeviceCreateSwapchain(GfxDevice device, GfxSurface surface, const GfxSwapchainDescriptor* descriptor, GfxSwapchain* outSwapchain)
{
    if (!device || !surface || !descriptor || !outSwapchain) {
        return GFX_RESULT_ERROR_INVALID_PARAMETER;
    }
    *outSwapchain = nullptr;
    auto api = gfx::getAPI(device);
    if (!api) {
        return GFX_RESULT_ERROR_FEATURE_NOT_SUPPORTED;
    }

    GfxBackend backend = gfx::getBackend(device);
    GfxSwapchain nativeSwapchain = nullptr;
    GfxResult result = api->deviceCreateSwapchain(
        gfx::native(device),
        gfx::native(surface),
        descriptor,
        &nativeSwapchain);

    if (result != GFX_RESULT_SUCCESS) {
        return result;
    }
    *outSwapchain = gfx::wrap(backend, nativeSwapchain);
    return GFX_RESULT_SUCCESS;
}

GfxResult gfxDeviceCreateCommandEncoder(GfxDevice device, const GfxCommandEncoderDescriptor* descriptor, GfxCommandEncoder* outEncoder)
{
    if (!device || !descriptor || !outEncoder) {
        return GFX_RESULT_ERROR_INVALID_PARAMETER;
    }
    *outEncoder = nullptr;
    auto api = gfx::getAPI(device);
    if (!api) {
        return GFX_RESULT_ERROR_FEATURE_NOT_SUPPORTED;
    }

    GfxBackend backend = gfx::getBackend(device);
    GfxCommandEncoder nativeEncoder = nullptr;
    GfxResult result = api->deviceCreateCommandEncoder(gfx::native(device), descriptor, &nativeEncoder);
    if (result != GFX_RESULT_SUCCESS) {
        return result;
    }

    *outEncoder = gfx::wrap(backend, nativeEncoder);
    return GFX_RESULT_SUCCESS;
}

GfxResult gfxDeviceCreateRenderPass(GfxDevice device, const GfxRenderPassDescriptor* descriptor, GfxRenderPass* outRenderPass)
{
    if (!device || !descriptor || !outRenderPass) {
        return GFX_RESULT_ERROR_INVALID_PARAMETER;
    }
    *outRenderPass = nullptr;
    auto api = gfx::getAPI(device);
    if (!api) {
        return GFX_RESULT_ERROR_FEATURE_NOT_SUPPORTED;
    }

    GfxBackend backend = gfx::getBackend(device);
    GfxRenderPass nativeRenderPass = nullptr;
    GfxResult result = api->deviceCreateRenderPass(gfx::native(device), descriptor, &nativeRenderPass);
    if (result != GFX_RESULT_SUCCESS) {
        return result;
    }

    *outRenderPass = gfx::wrap(backend, nativeRenderPass);
    return GFX_RESULT_SUCCESS;
}

GfxResult gfxDeviceCreateFramebuffer(GfxDevice device, const GfxFramebufferDescriptor* descriptor, GfxFramebuffer* outFramebuffer)
{
    if (!device || !descriptor || !outFramebuffer) {
        return GFX_RESULT_ERROR_INVALID_PARAMETER;
    }
    *outFramebuffer = nullptr;
    auto api = gfx::getAPI(device);
    if (!api) {
        return GFX_RESULT_ERROR_FEATURE_NOT_SUPPORTED;
    }

    GfxBackend backend = gfx::getBackend(device);
    GfxFramebuffer nativeFramebuffer = nullptr;
    GfxResult result = api->deviceCreateFramebuffer(gfx::native(device), descriptor, &nativeFramebuffer);
    if (result != GFX_RESULT_SUCCESS) {
        return result;
    }

    *outFramebuffer = gfx::wrap(backend, nativeFramebuffer);
    return GFX_RESULT_SUCCESS;
}

void gfxDeviceWaitIdle(GfxDevice device)
{
    if (!device) {
        return;
    }
    auto api = gfx::getAPI(device);
    if (api) {
        api->deviceWaitIdle(gfx::native(device));
    }
}

void gfxDeviceGetLimits(GfxDevice device, GfxDeviceLimits* outLimits)
{
    if (!device || !outLimits) {
        return;
    }
    auto api = gfx::getAPI(device);
    if (api) {
        api->deviceGetLimits(gfx::native(device), outLimits);
    }
}

// Macro for simple destroy functions
#define DESTROY_FUNC(TypeName, typeName)                   \
    void gfx##TypeName##Destroy(Gfx##TypeName typeName)    \
    {                                                      \
        if (!typeName)                                     \
            return;                                        \
        auto api = gfx::getAPI(typeName);                  \
        if (api) {                                         \
            api->typeName##Destroy(gfx::native(typeName)); \
        }                                                  \
        gfx::unwrap(typeName);                             \
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

uint32_t gfxSurfaceGetSupportedFormats(GfxSurface surface, GfxTextureFormat* formats, uint32_t maxFormats)
{
    if (!surface) {
        return 0;
    }
    auto api = gfx::getAPI(surface);
    if (!api) {
        return 0;
    }
    return api->surfaceGetSupportedFormats(gfx::native(surface), formats, maxFormats);
}

uint32_t gfxSurfaceGetSupportedPresentModes(GfxSurface surface, GfxPresentMode* presentModes, uint32_t maxModes)
{
    if (!surface) {
        return 0;
    }
    auto api = gfx::getAPI(surface);
    if (!api) {
        return 0;
    }
    return api->surfaceGetSupportedPresentModes(gfx::native(surface), presentModes, maxModes);
}

// Swapchain Functions
void gfxSwapchainGetInfo(GfxSwapchain swapchain, GfxSwapchainInfo* outInfo)
{
    if (!swapchain || !outInfo) {
        if (outInfo) {
            outInfo->width = 0;
            outInfo->height = 0;
            outInfo->format = GFX_TEXTURE_FORMAT_UNDEFINED;
            outInfo->imageCount = 0;
        }
        return;
    }
    auto api = gfx::getAPI(swapchain);
    if (!api) {
        outInfo->width = 0;
        outInfo->height = 0;
        outInfo->format = GFX_TEXTURE_FORMAT_UNDEFINED;
        outInfo->imageCount = 0;
        return;
    }
    api->swapchainGetInfo(gfx::native(swapchain), outInfo);
}

GfxResult gfxSwapchainAcquireNextImage(GfxSwapchain swapchain, uint64_t timeoutNs,
    GfxSemaphore imageAvailableSemaphore, GfxFence fence, uint32_t* outImageIndex)
{
    if (!swapchain || !outImageIndex) {
        return GFX_RESULT_ERROR_INVALID_PARAMETER;
    }
    auto api = gfx::getAPI(swapchain);
    if (!api) {
        return GFX_RESULT_ERROR_FEATURE_NOT_SUPPORTED;
    }

    GfxSemaphore nativeSemaphore = imageAvailableSemaphore ? gfx::native(imageAvailableSemaphore) : nullptr;
    GfxFence nativeFence = fence ? gfx::native(fence) : nullptr;

    return api->swapchainAcquireNextImage(gfx::native(swapchain), timeoutNs,
        nativeSemaphore, nativeFence, outImageIndex);
}

GfxTextureView gfxSwapchainGetImageView(GfxSwapchain swapchain, uint32_t imageIndex)
{
    if (!swapchain) {
        return nullptr;
    }
    auto api = gfx::getAPI(swapchain);
    if (!api) {
        return nullptr;
    }
    // Swapchain texture views are NOT wrapped - managed by swapchain
    return api->swapchainGetImageView(gfx::native(swapchain), imageIndex);
}

GfxTextureView gfxSwapchainGetCurrentTextureView(GfxSwapchain swapchain)
{
    if (!swapchain) {
        return nullptr;
    }
    auto api = gfx::getAPI(swapchain);
    if (!api) {
        return nullptr;
    }
    // Swapchain texture views are NOT wrapped - managed by swapchain
    return api->swapchainGetCurrentTextureView(gfx::native(swapchain));
}

GfxResult gfxSwapchainPresent(GfxSwapchain swapchain, const GfxPresentInfo* presentInfo)
{
    if (!swapchain) {
        return GFX_RESULT_ERROR_INVALID_PARAMETER;
    }
    auto api = gfx::getAPI(swapchain);
    if (!api) {
        return GFX_RESULT_ERROR_FEATURE_NOT_SUPPORTED;
    }

    // Convert semaphores to native handles
    GfxPresentInfo nativePresentInfo = {};
    std::vector<GfxSemaphore> nativeSemaphores;

    if (presentInfo && presentInfo->waitSemaphoreCount > 0) {
        nativeSemaphores.reserve(presentInfo->waitSemaphoreCount);
        for (uint32_t i = 0; i < presentInfo->waitSemaphoreCount; ++i) {
            nativeSemaphores.push_back(gfx::native(presentInfo->waitSemaphores[i]));
        }
        nativePresentInfo.waitSemaphores = nativeSemaphores.data();
        nativePresentInfo.waitSemaphoreCount = presentInfo->waitSemaphoreCount;
    }

    return api->swapchainPresent(gfx::native(swapchain), presentInfo ? &nativePresentInfo : nullptr);
}

// Buffer Functions
uint64_t gfxBufferGetSize(GfxBuffer buffer)
{
    if (!buffer) {
        return 0;
    }
    auto api = gfx::getAPI(buffer);
    if (!api) {
        return 0;
    }
    return api->bufferGetSize(gfx::native(buffer));
}

GfxBufferUsage gfxBufferGetUsage(GfxBuffer buffer)
{
    if (!buffer) {
        return GFX_BUFFER_USAGE_NONE;
    }
    auto api = gfx::getAPI(buffer);
    if (!api) {
        return GFX_BUFFER_USAGE_NONE;
    }
    return api->bufferGetUsage(gfx::native(buffer));
}

GfxResult gfxBufferMap(GfxBuffer buffer, uint64_t offset, uint64_t size, void** outMappedPointer)
{
    if (!buffer || !outMappedPointer) {
        return GFX_RESULT_ERROR_INVALID_PARAMETER;
    }
    auto api = gfx::getAPI(buffer);
    if (!api) {
        return GFX_RESULT_ERROR_FEATURE_NOT_SUPPORTED;
    }
    return api->bufferMap(gfx::native(buffer), offset, size, outMappedPointer);
}

void gfxBufferUnmap(GfxBuffer buffer)
{
    if (!buffer) {
        return;
    }
    auto api = gfx::getAPI(buffer);
    if (api) {
        api->bufferUnmap(gfx::native(buffer));
    }
}

// Texture Functions
void gfxTextureGetInfo(GfxTexture texture, GfxTextureInfo* outInfo)
{
    if (!texture || !outInfo) {
        return;
    }
    auto api = gfx::getAPI(texture);
    if (!api) {
        return;
    }
    api->textureGetInfo(gfx::native(texture), outInfo);
}

GfxTextureLayout gfxTextureGetLayout(GfxTexture texture)
{
    if (!texture) {
        return GFX_TEXTURE_LAYOUT_UNDEFINED;
    }
    auto api = gfx::getAPI(texture);
    if (!api) {
        return GFX_TEXTURE_LAYOUT_UNDEFINED;
    }
    return api->textureGetLayout(gfx::native(texture));
}

GfxResult gfxTextureCreateView(GfxTexture texture, const GfxTextureViewDescriptor* descriptor, GfxTextureView* outView)
{
    if (!texture || !outView) {
        return GFX_RESULT_ERROR_INVALID_PARAMETER;
    }
    *outView = nullptr;
    auto api = gfx::getAPI(texture);
    if (!api) {
        return GFX_RESULT_ERROR_FEATURE_NOT_SUPPORTED;
    }

    GfxBackend backend = gfx::getBackend(texture);
    GfxTextureView nativeView = nullptr;
    GfxResult result = api->textureCreateView(gfx::native(texture), descriptor, &nativeView);
    if (result != GFX_RESULT_SUCCESS) {
        return result;
    }

    *outView = gfx::wrap(backend, nativeView);
    return GFX_RESULT_SUCCESS;
}

// Queue Functions
GfxResult gfxQueueSubmit(GfxQueue queue, const GfxSubmitInfo* submitInfo)
{
    if (!queue || !submitInfo) {
        return GFX_RESULT_ERROR_INVALID_PARAMETER;
    }
    auto api = gfx::getAPI(queue);
    if (!api) {
        return GFX_RESULT_ERROR_FEATURE_NOT_SUPPORTED;
    }
    return api->queueSubmit(gfx::native(queue), submitInfo);
}

void gfxQueueWriteBuffer(GfxQueue queue, GfxBuffer buffer, uint64_t offset, const void* data, uint64_t size)
{
    if (!queue || !buffer) {
        return;
    }
    auto api = gfx::getAPI(queue);
    if (api) {
        api->queueWriteBuffer(gfx::native(queue), gfx::native(buffer), offset, data, size);
    }
}

void gfxQueueWriteTexture(GfxQueue queue, GfxTexture texture, const GfxOrigin3D* origin, uint32_t mipLevel,
    const void* data, uint64_t dataSize, uint32_t bytesPerRow, const GfxExtent3D* extent, GfxTextureLayout finalLayout)
{
    if (!queue || !texture) {
        return;
    }
    auto api = gfx::getAPI(queue);
    if (api) {
        api->queueWriteTexture(gfx::native(queue), gfx::native(texture), origin, mipLevel, data, dataSize, bytesPerRow, extent, finalLayout);
    }
}

GfxResult gfxQueueWaitIdle(GfxQueue queue)
{
    if (!queue) {
        return GFX_RESULT_ERROR_INVALID_PARAMETER;
    }
    auto api = gfx::getAPI(queue);
    if (!api) {
        return GFX_RESULT_ERROR_FEATURE_NOT_SUPPORTED;
    }
    return api->queueWaitIdle(gfx::native(queue));
}

// Command Encoder Functions
void gfxCommandEncoderPipelineBarrier(GfxCommandEncoder commandEncoder,
    const GfxMemoryBarrier* memoryBarriers, uint32_t memoryBarrierCount,
    const GfxBufferBarrier* bufferBarriers, uint32_t bufferBarrierCount,
    const GfxTextureBarrier* textureBarriers, uint32_t textureBarrierCount)
{
    if (!commandEncoder) {
        return;
    }
    auto api = gfx::getAPI(commandEncoder);
    if (api) {
        api->commandEncoderPipelineBarrier(gfx::native(commandEncoder), memoryBarriers, memoryBarrierCount, bufferBarriers, bufferBarrierCount, textureBarriers, textureBarrierCount);
    }
}

void gfxCommandEncoderGenerateMipmaps(GfxCommandEncoder commandEncoder, GfxTexture texture)
{
    if (!commandEncoder || !texture) {
        return;
    }
    auto api = gfx::getAPI(commandEncoder);
    if (api) {
        api->commandEncoderGenerateMipmaps(gfx::native(commandEncoder), gfx::native(texture));
    }
}

void gfxCommandEncoderGenerateMipmapsRange(GfxCommandEncoder commandEncoder, GfxTexture texture,
    uint32_t baseMipLevel, uint32_t levelCount)
{
    if (!commandEncoder || !texture) {
        return;
    }
    auto api = gfx::getAPI(commandEncoder);
    if (api) {
        api->commandEncoderGenerateMipmapsRange(gfx::native(commandEncoder), gfx::native(texture), baseMipLevel, levelCount);
    }
}

void gfxCommandEncoderEnd(GfxCommandEncoder commandEncoder)
{
    if (!commandEncoder) {
        return;
    }
    auto api = gfx::getAPI(commandEncoder);
    if (api) {
        api->commandEncoderEnd(gfx::native(commandEncoder));
    }
}

void gfxCommandEncoderBegin(GfxCommandEncoder commandEncoder)
{
    if (!commandEncoder) {
        return;
    }
    auto api = gfx::getAPI(commandEncoder);
    if (api) {
        api->commandEncoderBegin(gfx::native(commandEncoder));
    }
}

GfxResult gfxCommandEncoderBeginRenderPass(GfxCommandEncoder encoder,
    const GfxRenderPassBeginDescriptor* beginDescriptor,
    GfxRenderPassEncoder* outEncoder)
{
    if (!encoder || !outEncoder || !beginDescriptor) {
        return GFX_RESULT_ERROR_INVALID_PARAMETER;
    }
    *outEncoder = nullptr;
    auto api = gfx::getAPI(encoder);
    if (!api) {
        return GFX_RESULT_ERROR_FEATURE_NOT_SUPPORTED;
    }

    GfxBackend backend = gfx::getBackend(encoder);
    GfxRenderPassEncoder nativePass = nullptr;
    GfxResult result = api->commandEncoderBeginRenderPass(
        gfx::native(encoder),
        beginDescriptor,
        &nativePass);
    if (result != GFX_RESULT_SUCCESS) {
        return result;
    }

    *outEncoder = gfx::wrap(backend, nativePass);
    return GFX_RESULT_SUCCESS;
}

GfxResult gfxCommandEncoderBeginComputePass(GfxCommandEncoder encoder, const GfxComputePassBeginDescriptor* beginDescriptor, GfxComputePassEncoder* outEncoder)
{
    if (!encoder || !beginDescriptor || !outEncoder) {
        return GFX_RESULT_ERROR_INVALID_PARAMETER;
    }
    *outEncoder = nullptr;
    auto api = gfx::getAPI(encoder);
    if (!api) {
        return GFX_RESULT_ERROR_FEATURE_NOT_SUPPORTED;
    }

    GfxBackend backend = gfx::getBackend(encoder);
    GfxComputePassEncoder nativePass = nullptr;
    GfxResult result = api->commandEncoderBeginComputePass(gfx::native(encoder), beginDescriptor, &nativePass);
    if (result != GFX_RESULT_SUCCESS) {
        return result;
    }

    *outEncoder = gfx::wrap(backend, nativePass);
    return GFX_RESULT_SUCCESS;
}

// Render Pass Encoder Functions
void gfxRenderPassEncoderSetPipeline(GfxRenderPassEncoder encoder, GfxRenderPipeline pipeline)
{
    if (!encoder || !pipeline) {
        return;
    }
    auto api = gfx::getAPI(encoder);
    if (api) {
        api->renderPassEncoderSetPipeline(
            gfx::native(encoder),
            gfx::native(pipeline));
    }
}

void gfxRenderPassEncoderSetBindGroup(GfxRenderPassEncoder encoder, uint32_t groupIndex, GfxBindGroup bindGroup, const uint32_t* dynamicOffsets, uint32_t dynamicOffsetCount)
{
    if (!encoder || !bindGroup) {
        return;
    }
    auto api = gfx::getAPI(encoder);
    if (api) {
        api->renderPassEncoderSetBindGroup(
            gfx::native(encoder),
            groupIndex,
            gfx::native(bindGroup),
            dynamicOffsets,
            dynamicOffsetCount);
    }
}

void gfxRenderPassEncoderSetVertexBuffer(GfxRenderPassEncoder encoder, uint32_t slot, GfxBuffer buffer, uint64_t offset, uint64_t size)
{
    if (!encoder || !buffer) {
        return;
    }
    auto api = gfx::getAPI(encoder);
    if (api) {
        api->renderPassEncoderSetVertexBuffer(
            gfx::native(encoder),
            slot,
            gfx::native(buffer),
            offset,
            size);
    }
}

void gfxRenderPassEncoderSetIndexBuffer(GfxRenderPassEncoder encoder, GfxBuffer buffer, GfxIndexFormat format, uint64_t offset, uint64_t size)
{
    if (!encoder || !buffer) {
        return;
    }
    auto api = gfx::getAPI(encoder);
    if (api) {
        api->renderPassEncoderSetIndexBuffer(
            gfx::native(encoder),
            gfx::native(buffer),
            format,
            offset,
            size);
    }
}

void gfxRenderPassEncoderSetViewport(GfxRenderPassEncoder encoder, const GfxViewport* viewport)
{
    if (!encoder || !viewport) {
        return;
    }
    auto api = gfx::getAPI(encoder);
    if (api) {
        api->renderPassEncoderSetViewport(
            gfx::native(encoder),
            viewport);
    }
}

void gfxRenderPassEncoderSetScissorRect(GfxRenderPassEncoder encoder, const GfxScissorRect* scissor)
{
    if (!encoder || !scissor) {
        return;
    }
    auto api = gfx::getAPI(encoder);
    if (api) {
        api->renderPassEncoderSetScissorRect(
            gfx::native(encoder),
            scissor);
    }
}

void gfxRenderPassEncoderDraw(GfxRenderPassEncoder encoder, uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance)
{
    if (!encoder) {
        return;
    }
    auto api = gfx::getAPI(encoder);
    if (api) {
        api->renderPassEncoderDraw(
            gfx::native(encoder),
            vertexCount, instanceCount, firstVertex, firstInstance);
    }
}

void gfxRenderPassEncoderDrawIndexed(GfxRenderPassEncoder encoder, uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t baseVertex, uint32_t firstInstance)
{
    if (!encoder) {
        return;
    }
    auto api = gfx::getAPI(encoder);
    if (api) {
        api->renderPassEncoderDrawIndexed(
            gfx::native(encoder),
            indexCount, instanceCount, firstIndex, baseVertex, firstInstance);
    }
}

void gfxRenderPassEncoderEnd(GfxRenderPassEncoder encoder)
{
    if (!encoder) {
        return;
    }
    auto api = gfx::getAPI(encoder);
    if (api) {
        api->renderPassEncoderEnd(gfx::native(encoder));
    }
}

// Compute Pass Encoder Functions
void gfxComputePassEncoderSetPipeline(GfxComputePassEncoder encoder, GfxComputePipeline pipeline)
{
    if (!encoder || !pipeline) {
        return;
    }
    auto api = gfx::getAPI(encoder);
    if (api) {
        api->computePassEncoderSetPipeline(
            gfx::native(encoder),
            gfx::native(pipeline));
    }
}

void gfxComputePassEncoderSetBindGroup(GfxComputePassEncoder encoder, uint32_t groupIndex, GfxBindGroup bindGroup, const uint32_t* dynamicOffsets, uint32_t dynamicOffsetCount)
{
    if (!encoder || !bindGroup) {
        return;
    }
    auto api = gfx::getAPI(encoder);
    if (api) {
        api->computePassEncoderSetBindGroup(
            gfx::native(encoder),
            groupIndex,
            gfx::native(bindGroup),
            dynamicOffsets,
            dynamicOffsetCount);
    }
}

void gfxComputePassEncoderDispatchWorkgroups(GfxComputePassEncoder encoder, uint32_t workgroupCountX, uint32_t workgroupCountY, uint32_t workgroupCountZ)
{
    if (!encoder) {
        return;
    }
    auto api = gfx::getAPI(encoder);
    if (api) {
        api->computePassEncoderDispatchWorkgroups(
            gfx::native(encoder),
            workgroupCountX, workgroupCountY, workgroupCountZ);
    }
}

void gfxComputePassEncoderEnd(GfxComputePassEncoder encoder)
{
    if (!encoder) {
        return;
    }
    auto api = gfx::getAPI(encoder);
    if (api) {
        api->computePassEncoderEnd(gfx::native(encoder));
    }
}

// Fence Functions
GfxResult gfxFenceGetStatus(GfxFence fence, bool* isSignaled)
{
    if (!fence) {
        return GFX_RESULT_ERROR_INVALID_PARAMETER;
    }
    auto api = gfx::getAPI(fence);
    if (!api) {
        return GFX_RESULT_ERROR_FEATURE_NOT_SUPPORTED;
    }
    return api->fenceGetStatus(gfx::native(fence), isSignaled);
}

GfxResult gfxFenceWait(GfxFence fence, uint64_t timeoutNs)
{
    if (!fence) {
        return GFX_RESULT_ERROR_INVALID_PARAMETER;
    }
    auto api = gfx::getAPI(fence);
    if (!api) {
        return GFX_RESULT_ERROR_FEATURE_NOT_SUPPORTED;
    }
    return api->fenceWait(gfx::native(fence), timeoutNs);
}

void gfxFenceReset(GfxFence fence)
{
    if (!fence) {
        return;
    }
    auto api = gfx::getAPI(fence);
    if (api) {
        api->fenceReset(gfx::native(fence));
    }
}

// CommandEncoder Copy Functions
void gfxCommandEncoderCopyBufferToBuffer(GfxCommandEncoder commandEncoder,
    GfxBuffer source, uint64_t sourceOffset,
    GfxBuffer destination, uint64_t destinationOffset,
    uint64_t size)
{
    if (!commandEncoder || !source || !destination) {
        return;
    }
    auto api = gfx::getAPI(commandEncoder);
    if (api) {
        api->commandEncoderCopyBufferToBuffer(gfx::native(commandEncoder),
            gfx::native(source), sourceOffset,
            gfx::native(destination), destinationOffset,
            size);
    }
}

void gfxCommandEncoderCopyBufferToTexture(GfxCommandEncoder commandEncoder,
    GfxBuffer source, uint64_t sourceOffset, uint32_t bytesPerRow,
    GfxTexture destination, const GfxOrigin3D* origin,
    const GfxExtent3D* extent, uint32_t mipLevel, GfxTextureLayout finalLayout)
{
    if (!commandEncoder || !source || !destination) {
        return;
    }
    auto api = gfx::getAPI(commandEncoder);
    if (api) {
        api->commandEncoderCopyBufferToTexture(gfx::native(commandEncoder),
            gfx::native(source), sourceOffset, bytesPerRow,
            gfx::native(destination), origin,
            extent, mipLevel, finalLayout);
    }
}

void gfxCommandEncoderCopyTextureToBuffer(GfxCommandEncoder commandEncoder,
    GfxTexture source, const GfxOrigin3D* origin, uint32_t mipLevel,
    GfxBuffer destination, uint64_t destinationOffset, uint32_t bytesPerRow,
    const GfxExtent3D* extent, GfxTextureLayout finalLayout)
{
    if (!commandEncoder || !source || !destination) {
        return;
    }
    auto api = gfx::getAPI(commandEncoder);
    if (api) {
        api->commandEncoderCopyTextureToBuffer(gfx::native(commandEncoder),
            gfx::native(source), origin, mipLevel,
            gfx::native(destination), destinationOffset, bytesPerRow,
            extent, finalLayout);
    }
}

void gfxCommandEncoderCopyTextureToTexture(GfxCommandEncoder commandEncoder,
    GfxTexture source, const GfxOrigin3D* sourceOrigin, uint32_t sourceMipLevel,
    GfxTexture destination, const GfxOrigin3D* destinationOrigin, uint32_t destinationMipLevel,
    const GfxExtent3D* extent, GfxTextureLayout sourceFinalLayout, GfxTextureLayout destinationFinalLayout)
{
    if (!commandEncoder || !source || !destination) {
        return;
    }
    auto api = gfx::getAPI(commandEncoder);
    if (api) {
        api->commandEncoderCopyTextureToTexture(gfx::native(commandEncoder),
            gfx::native(source), sourceOrigin, sourceMipLevel,
            gfx::native(destination), destinationOrigin, destinationMipLevel,
            extent, sourceFinalLayout, destinationFinalLayout);
    }
}

void gfxCommandEncoderBlitTextureToTexture(GfxCommandEncoder commandEncoder,
    GfxTexture source, const GfxOrigin3D* sourceOrigin, const GfxExtent3D* sourceExtent, uint32_t sourceMipLevel,
    GfxTexture destination, const GfxOrigin3D* destinationOrigin, const GfxExtent3D* destinationExtent, uint32_t destinationMipLevel,
    GfxFilterMode filter, GfxTextureLayout sourceFinalLayout, GfxTextureLayout destinationFinalLayout)
{
    if (!commandEncoder || !source || !destination) {
        return;
    }
    auto api = gfx::getAPI(commandEncoder);
    if (api) {
        api->commandEncoderBlitTextureToTexture(gfx::native(commandEncoder),
            gfx::native(source), sourceOrigin, sourceExtent, sourceMipLevel,
            gfx::native(destination), destinationOrigin, destinationExtent, destinationMipLevel,
            filter, sourceFinalLayout, destinationFinalLayout);
    }
}

// Semaphore Functions
GfxSemaphoreType gfxSemaphoreGetType(GfxSemaphore semaphore)
{
    if (!semaphore) {
        return GFX_SEMAPHORE_TYPE_BINARY;
    }
    auto api = gfx::getAPI(semaphore);
    if (!api) {
        return GFX_SEMAPHORE_TYPE_BINARY;
    }
    return api->semaphoreGetType(gfx::native(semaphore));
}

uint64_t gfxSemaphoreGetValue(GfxSemaphore semaphore)
{
    if (!semaphore) {
        return 0;
    }
    auto api = gfx::getAPI(semaphore);
    if (!api) {
        return 0;
    }
    return api->semaphoreGetValue(gfx::native(semaphore));
}

GfxResult gfxSemaphoreSignal(GfxSemaphore semaphore, uint64_t value)
{
    if (!semaphore) {
        return GFX_RESULT_ERROR_INVALID_PARAMETER;
    }
    auto api = gfx::getAPI(semaphore);
    if (!api) {
        return GFX_RESULT_ERROR_FEATURE_NOT_SUPPORTED;
    }
    return api->semaphoreSignal(gfx::native(semaphore), value);
}

GfxResult gfxSemaphoreWait(GfxSemaphore semaphore, uint64_t value, uint64_t timeoutNs)
{
    if (!semaphore) {
        return GFX_RESULT_ERROR_INVALID_PARAMETER;
    }
    auto api = gfx::getAPI(semaphore);
    if (!api) {
        return GFX_RESULT_ERROR_FEATURE_NOT_SUPPORTED;
    }
    return api->semaphoreWait(gfx::native(semaphore), value, timeoutNs);
}

// Helper function to deduce access flags from texture layout
GfxAccessFlags gfxGetAccessFlagsForLayout(GfxTextureLayout layout)
{
    // Use Vulkan-style explicit access flags (deterministic mapping)
    // WebGPU backends will ignore these as they use implicit synchronization
    auto api = gfx::getBackendAPI(GFX_BACKEND_VULKAN);
    if (!api) {
        return GFX_ACCESS_NONE;
    }

    return api->getAccessFlagsForLayout(layout);
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

GfxPlatformWindowHandle gfxPlatformWindowHandleMakeXlib(void* display, unsigned long window)
{
    GfxPlatformWindowHandle handle = {};
    handle.windowingSystem = GFX_WINDOWING_SYSTEM_XLIB;
    handle.xlib.display = display;
    handle.xlib.window = window;
    return handle;
}

GfxPlatformWindowHandle gfxPlatformWindowHandleMakeWayland(void* surface, void* display)
{
    GfxPlatformWindowHandle handle = {};
    handle.windowingSystem = GFX_WINDOWING_SYSTEM_WAYLAND;
    handle.wayland.surface = surface;
    handle.wayland.display = display;
    return handle;
}

GfxPlatformWindowHandle gfxPlatformWindowHandleMakeXCB(void* connection, uint32_t window)
{
    GfxPlatformWindowHandle handle = {};
    handle.windowingSystem = GFX_WINDOWING_SYSTEM_XCB;
    handle.xcb.connection = connection;
    handle.xcb.window = window;
    return handle;
}

GfxPlatformWindowHandle gfxPlatformWindowHandleMakeWin32(void* hwnd, void* hinstance)
{
    GfxPlatformWindowHandle handle = {};
    handle.windowingSystem = GFX_WINDOWING_SYSTEM_WIN32;
    handle.win32.hwnd = hwnd;
    handle.win32.hinstance = hinstance;
    return handle;
}

GfxPlatformWindowHandle gfxPlatformWindowHandleMakeEmscripten(const char* canvasSelector)
{
    GfxPlatformWindowHandle handle = {};
    handle.windowingSystem = GFX_WINDOWING_SYSTEM_EMSCRIPTEN;
    handle.emscripten.canvasSelector = canvasSelector;
    return handle;
}

GfxPlatformWindowHandle gfxPlatformWindowHandleMakeAndroid(void* window)
{
    GfxPlatformWindowHandle handle = {};
    handle.windowingSystem = GFX_WINDOWING_SYSTEM_ANDROID;
    handle.android.window = window;
    return handle;
}

GfxPlatformWindowHandle gfxPlatformWindowHandleMakeMetal(void* layer)
{
    GfxPlatformWindowHandle handle = {};
    handle.windowingSystem = GFX_WINDOWING_SYSTEM_METAL;
    handle.metal.layer = layer;
    return handle;
}

} // extern "C"
