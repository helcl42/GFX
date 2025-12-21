#include "GfxApi.h"
#include "GfxBackend.h"
#include <mutex>
#include <unordered_map>
#include <vector>

// Forward declarations for backend getters
extern "C" {
extern const GfxBackendAPI* gfxGetVulkanBackendNew(void);
extern const GfxBackendAPI* gfxGetWebGPUBackend(void);
}

// ============================================================================
// Type-Safe Handle System - No void* casting, cleaner design
// ============================================================================

namespace gfx {

// Handle metadata stores backend info
struct HandleMeta {
    GfxBackend m_backend;
    void* m_nativeHandle;
};

// Singleton class to manage backend state
class BackendRegistry {
public:
    static BackendRegistry& getInstance()
    {
        static BackendRegistry instance;
        return instance;
    }

    // Delete copy and move constructors/assignments
    BackendRegistry(const BackendRegistry&) = delete;
    BackendRegistry& operator=(const BackendRegistry&) = delete;
    BackendRegistry(BackendRegistry&&) = delete;
    BackendRegistry& operator=(BackendRegistry&&) = delete;

    const GfxBackendAPI* getBackendAPI(GfxBackend backend)
    {
        if (backend >= 0 && backend < GFX_BACKEND_AUTO) {
            return m_backends[backend];
        }
        return nullptr;
    }

    template <typename T>
    T wrap(GfxBackend backend, T nativeHandle)
    {
        if (!nativeHandle) {
            return nullptr;
        }
        std::scoped_lock lock(m_mutex);
        m_handles[nativeHandle] = { backend, nativeHandle };
        return nativeHandle;
    }

    const GfxBackendAPI* getAPI(void* handle)
    {
        if (!handle) {
            return nullptr;
        }
        std::scoped_lock lock(m_mutex);
        auto it = m_handles.find(handle);
        if (it == m_handles.end()) {
            return nullptr;
        }
        return getBackendAPI(it->second.m_backend);
    }

    GfxBackend getBackend(void* handle)
    {
        if (!handle) {
            return GFX_BACKEND_AUTO;
        }
        std::scoped_lock lock(m_mutex);
        auto it = m_handles.find(handle);
        if (it == m_handles.end()) {
            return GFX_BACKEND_AUTO;
        }
        return it->second.m_backend;
    }

    void unwrap(void* handle)
    {
        if (!handle) {
            return;
        }
        std::scoped_lock lock(m_mutex);
        m_handles.erase(handle);
    }

    std::mutex& getMutex() { return m_mutex; }
    const GfxBackendAPI** getBackends() { return m_backends; }
    int* getRefCounts() { return m_refCounts; }

private:
    BackendRegistry()
    {
        for (int i = 0; i < 3; ++i) {
            m_backends[i] = nullptr;
            m_refCounts[i] = 0;
        }
    }

    ~BackendRegistry() = default;

    const GfxBackendAPI* m_backends[3];
    int m_refCounts[3];
    std::mutex m_mutex;
    std::unordered_map<void*, HandleMeta> m_handles;
};

// Convenience inline functions for backward compatibility
inline const GfxBackendAPI* getBackendAPI(GfxBackend backend)
{
    return BackendRegistry::getInstance().getBackendAPI(backend);
}

template <typename T>
inline T wrap(GfxBackend backend, T nativeHandle)
{
    return BackendRegistry::getInstance().wrap(backend, nativeHandle);
}

inline const GfxBackendAPI* getAPI(void* handle)
{
    return BackendRegistry::getInstance().getAPI(handle);
}

inline GfxBackend getBackend(void* handle)
{
    return BackendRegistry::getInstance().getBackend(handle);
}

// Native handle passthrough - template preserves type automatically
template <typename T>
inline T native(T handle)
{
    return handle;
}

inline void unwrap(void* handle)
{
    BackendRegistry::getInstance().unwrap(handle);
}

} // namespace gfx

// ============================================================================
// C API Implementation
// ============================================================================

extern "C" {

// Backend Loading - helper without lock
static bool gfxLoadBackendInternal(GfxBackend backend)
{
    if (backend < 0 || backend >= GFX_BACKEND_AUTO) {
        return false;
    }

    auto& registry = gfx::BackendRegistry::getInstance();
    auto backends = registry.getBackends();
    auto refCounts = registry.getRefCounts();

    if (!backends[backend]) {
        switch (backend) {
#ifdef GFX_ENABLE_VULKAN
        case GFX_BACKEND_VULKAN:
            backends[backend] = gfxGetVulkanBackendNew();
            break;
#endif
#ifdef GFX_ENABLE_WEBGPU
        case GFX_BACKEND_WEBGPU:
            backends[backend] = gfxGetWebGPUBackend();
            break;
#endif
        default:
            return false;
        }

        if (!backends[backend]) {
            return false;
        }
        refCounts[backend] = 0;
    }

    refCounts[backend]++;
    return true;
}

bool gfxLoadBackend(GfxBackend backend)
{
    if (backend == GFX_BACKEND_AUTO) {
        // Try backends without holding lock during recursion
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

    std::scoped_lock lock(gfx::BackendRegistry::getInstance().getMutex());
    return gfxLoadBackendInternal(backend);
}

static void gfxUnloadBackendInternal(GfxBackend backend)
{
    if (backend >= 0 && backend < GFX_BACKEND_AUTO) {
        auto& registry = gfx::BackendRegistry::getInstance();
        auto backends = registry.getBackends();
        auto refCounts = registry.getRefCounts();

        if (backends[backend] && refCounts[backend] > 0) {
            refCounts[backend]--;
            if (refCounts[backend] == 0) {
                backends[backend] = nullptr;
            }
        }
    }
}

void gfxUnloadBackend(GfxBackend backend)
{
    if (backend == GFX_BACKEND_AUTO) {
        // Unload the first loaded backend without holding lock
        auto& registry = gfx::BackendRegistry::getInstance();
        auto backends = registry.getBackends();
#ifdef GFX_ENABLE_VULKAN
        if (backends[GFX_BACKEND_VULKAN]) {
            gfxUnloadBackend(GFX_BACKEND_VULKAN);
            return;
        }
#endif
#ifdef GFX_ENABLE_WEBGPU
        if (backends[GFX_BACKEND_WEBGPU]) {
            gfxUnloadBackend(GFX_BACKEND_WEBGPU);
            return;
        }
#endif
        return;
    }

    std::scoped_lock lock(gfx::BackendRegistry::getInstance().getMutex());
    gfxUnloadBackendInternal(backend);
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
    auto& registry = gfx::BackendRegistry::getInstance();
    auto refCounts = registry.getRefCounts();
#ifdef GFX_ENABLE_VULKAN
    while (refCounts[GFX_BACKEND_VULKAN] > 0) {
        gfxUnloadBackend(GFX_BACKEND_VULKAN);
    }
#endif
#ifdef GFX_ENABLE_WEBGPU
    while (refCounts[GFX_BACKEND_WEBGPU] > 0) {
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
    auto& registry = gfx::BackendRegistry::getInstance();
    auto backends = registry.getBackends();

    if (backend == GFX_BACKEND_AUTO) {
#ifdef GFX_ENABLE_VULKAN
        if (backends[GFX_BACKEND_VULKAN]) {
            backend = GFX_BACKEND_VULKAN;
        }
#endif
#ifdef GFX_ENABLE_WEBGPU
        if (backend == GFX_BACKEND_AUTO && backends[GFX_BACKEND_WEBGPU]) {
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
    if (api && api->instanceSetDebugCallback) {
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

const char* gfxAdapterGetName(GfxAdapter adapter)
{
    if (!adapter) {
        return nullptr;
    }
    auto api = gfx::getAPI(adapter);
    if (!api) {
        return nullptr;
    }
    return api->adapterGetName(gfx::native(adapter));
}

GfxBackend gfxAdapterGetBackend(GfxAdapter adapter)
{
    return gfx::getBackend(adapter);
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

GfxResult gfxDeviceCreateCommandEncoder(GfxDevice device, const char* label, GfxCommandEncoder* outEncoder)
{
    if (!device || !outEncoder) {
        return GFX_RESULT_ERROR_INVALID_PARAMETER;
    }
    *outEncoder = nullptr;
    auto api = gfx::getAPI(device);
    if (!api) {
        return GFX_RESULT_ERROR_FEATURE_NOT_SUPPORTED;
    }

    GfxBackend backend = gfx::getBackend(device);
    GfxCommandEncoder nativeEncoder = nullptr;
    GfxResult result = api->deviceCreateCommandEncoder(gfx::native(device), label, &nativeEncoder);
    if (result != GFX_RESULT_SUCCESS) {
        return result;
    }

    *outEncoder = gfx::wrap(backend, nativeEncoder);
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

// Alignment helper functions
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
DESTROY_FUNC(CommandEncoder, commandEncoder)
DESTROY_FUNC(Fence, fence)
DESTROY_FUNC(Semaphore, semaphore)

// Queue is owned by device - don't destroy it separately
void gfxQueueDestroy(GfxQueue queue)
{
    // Queue is owned by device, do nothing
    (void)queue;
}

// Render/Compute pass encoders are just aliases to command encoder - don't unwrap them
void gfxRenderPassEncoderDestroy(GfxRenderPassEncoder renderPassEncoder)
{
    if (!renderPassEncoder) {
        return;
    }
    auto api = gfx::getAPI(renderPassEncoder);
    if (api) {
        api->renderPassEncoderDestroy(gfx::native(renderPassEncoder));
    }
    // Do NOT unwrap - it's an alias to the command encoder
}

void gfxComputePassEncoderDestroy(GfxComputePassEncoder computePassEncoder)
{
    if (!computePassEncoder) {
        return;
    }
    auto api = gfx::getAPI(computePassEncoder);
    if (api) {
        api->computePassEncoderDestroy(gfx::native(computePassEncoder));
    }
    // Do NOT unwrap - it's an alias to the command encoder
}

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

GfxPlatformWindowHandle gfxSurfaceGetPlatformHandle(GfxSurface surface)
{
    if (!surface) {
        return {};
    }
    auto api = gfx::getAPI(surface);
    if (!api) {
        return {};
    }
    return api->surfaceGetPlatformHandle(gfx::native(surface));
}

// Swapchain Functions
uint32_t gfxSwapchainGetWidth(GfxSwapchain swapchain)
{
    if (!swapchain) {
        return 0;
    }
    auto api = gfx::getAPI(swapchain);
    if (!api) {
        return 0;
    }
    return api->swapchainGetWidth(gfx::native(swapchain));
}

uint32_t gfxSwapchainGetHeight(GfxSwapchain swapchain)
{
    if (!swapchain) {
        return 0;
    }
    auto api = gfx::getAPI(swapchain);
    if (!api) {
        return 0;
    }
    return api->swapchainGetHeight(gfx::native(swapchain));
}

GfxTextureFormat gfxSwapchainGetFormat(GfxSwapchain swapchain)
{
    if (!swapchain) {
        return GFX_TEXTURE_FORMAT_UNDEFINED;
    }
    auto api = gfx::getAPI(swapchain);
    if (!api) {
        return GFX_TEXTURE_FORMAT_UNDEFINED;
    }
    return api->swapchainGetFormat(gfx::native(swapchain));
}

uint32_t gfxSwapchainGetBufferCount(GfxSwapchain swapchain)
{
    if (!swapchain) {
        return 0;
    }
    auto api = gfx::getAPI(swapchain);
    if (!api) {
        return 0;
    }
    return api->swapchainGetBufferCount(gfx::native(swapchain));
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

GfxResult gfxSwapchainPresentWithSync(GfxSwapchain swapchain, const GfxPresentInfo* presentInfo)
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

    return api->swapchainPresentWithSync(gfx::native(swapchain), presentInfo ? &nativePresentInfo : nullptr);
}

GfxResult gfxSwapchainPresent(GfxSwapchain swapchain)
{
    if (!swapchain) {
        return GFX_RESULT_ERROR_INVALID_PARAMETER;
    }
    auto api = gfx::getAPI(swapchain);
    if (!api) {
        return GFX_RESULT_ERROR_FEATURE_NOT_SUPPORTED;
    }
    return api->swapchainPresent(gfx::native(swapchain));
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

GfxResult gfxBufferMapAsync(GfxBuffer buffer, uint64_t offset, uint64_t size, void** outMappedPointer)
{
    if (!buffer || !outMappedPointer) {
        return GFX_RESULT_ERROR_INVALID_PARAMETER;
    }
    auto api = gfx::getAPI(buffer);
    if (!api) {
        return GFX_RESULT_ERROR_FEATURE_NOT_SUPPORTED;
    }
    return api->bufferMapAsync(gfx::native(buffer), offset, size, outMappedPointer);
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
GfxExtent3D gfxTextureGetSize(GfxTexture texture)
{
    if (!texture) {
        return { 0, 0, 0 };
    }
    auto api = gfx::getAPI(texture);
    if (!api) {
        return { 0, 0, 0 };
    }
    return api->textureGetSize(gfx::native(texture));
}

GfxTextureFormat gfxTextureGetFormat(GfxTexture texture)
{
    if (!texture) {
        return GFX_TEXTURE_FORMAT_UNDEFINED;
    }
    auto api = gfx::getAPI(texture);
    if (!api) {
        return GFX_TEXTURE_FORMAT_UNDEFINED;
    }
    return api->textureGetFormat(gfx::native(texture));
}

uint32_t gfxTextureGetMipLevelCount(GfxTexture texture)
{
    if (!texture) {
        return 0;
    }
    auto api = gfx::getAPI(texture);
    if (!api) {
        return 0;
    }
    return api->textureGetMipLevelCount(gfx::native(texture));
}

GfxSampleCount gfxTextureGetSampleCount(GfxTexture texture)
{
    if (!texture) {
        return GFX_SAMPLE_COUNT_1;
    }
    auto api = gfx::getAPI(texture);
    if (!api) {
        return GFX_SAMPLE_COUNT_1;
    }
    return api->textureGetSampleCount(gfx::native(texture));
}

GfxTextureUsage gfxTextureGetUsage(GfxTexture texture)
{
    if (!texture) {
        return GFX_TEXTURE_USAGE_NONE;
    }
    auto api = gfx::getAPI(texture);
    if (!api) {
        return GFX_TEXTURE_USAGE_NONE;
    }
    return api->textureGetUsage(gfx::native(texture));
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
GfxResult gfxQueueSubmit(GfxQueue queue, GfxCommandEncoder commandEncoder)
{
    if (!queue || !commandEncoder) {
        return GFX_RESULT_ERROR_INVALID_PARAMETER;
    }
    auto api = gfx::getAPI(queue);
    if (!api) {
        return GFX_RESULT_ERROR_FEATURE_NOT_SUPPORTED;
    }
    return api->queueSubmit(
        gfx::native(queue),
        gfx::native(commandEncoder));
}

// Command Encoder Functions
void gfxCommandEncoderPipelineBarrier(GfxCommandEncoder commandEncoder,
    const GfxTextureBarrier* textureBarriers, uint32_t textureBarrierCount)
{
    if (!commandEncoder || !textureBarriers) {
        return;
    }
    auto api = gfx::getAPI(commandEncoder);
    if (api && api->commandEncoderPipelineBarrier) {
        api->commandEncoderPipelineBarrier(gfx::native(commandEncoder), textureBarriers, textureBarrierCount);
    }
}

void gfxCommandEncoderFinish(GfxCommandEncoder commandEncoder)
{
    if (!commandEncoder) {
        return;
    }
    auto api = gfx::getAPI(commandEncoder);
    if (api) {
        api->commandEncoderFinish(gfx::native(commandEncoder));
    }
}

// Helper function to deduce access flags from texture layout
GfxAccessFlags gfxGetAccessFlagsForLayout(GfxTextureLayout layout)
{
    switch (layout) {
    case GFX_TEXTURE_LAYOUT_UNDEFINED:
        return GFX_ACCESS_NONE;
    case GFX_TEXTURE_LAYOUT_GENERAL:
        return static_cast<GfxAccessFlags>(GFX_ACCESS_MEMORY_READ | GFX_ACCESS_MEMORY_WRITE);
    case GFX_TEXTURE_LAYOUT_COLOR_ATTACHMENT:
        return static_cast<GfxAccessFlags>(GFX_ACCESS_COLOR_ATTACHMENT_READ | GFX_ACCESS_COLOR_ATTACHMENT_WRITE);
    case GFX_TEXTURE_LAYOUT_DEPTH_STENCIL_ATTACHMENT:
        return static_cast<GfxAccessFlags>(GFX_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ | GFX_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE);
    case GFX_TEXTURE_LAYOUT_DEPTH_STENCIL_READ_ONLY:
        return GFX_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ;
    case GFX_TEXTURE_LAYOUT_SHADER_READ_ONLY:
        return GFX_ACCESS_SHADER_READ;
    case GFX_TEXTURE_LAYOUT_TRANSFER_SRC:
        return GFX_ACCESS_TRANSFER_READ;
    case GFX_TEXTURE_LAYOUT_TRANSFER_DST:
        return GFX_ACCESS_TRANSFER_WRITE;
    case GFX_TEXTURE_LAYOUT_PRESENT_SRC:
        return GFX_ACCESS_MEMORY_READ;
    default:
        return GFX_ACCESS_NONE;
    }
}

GfxResult gfxCommandEncoderBeginRenderPass(GfxCommandEncoder encoder,
    const GfxTextureView* colorAttachments, uint32_t colorAttachmentCount,
    const GfxColor* clearColors,
    const GfxTextureLayout* colorFinalLayouts,
    GfxTextureView depthStencilAttachment,
    float depthClearValue, uint32_t stencilClearValue,
    GfxTextureLayout depthFinalLayout,
    GfxRenderPassEncoder* outEncoder)
{
    if (!encoder || !outEncoder) {
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
        colorAttachments, colorAttachmentCount,
        clearColors,
        colorFinalLayouts,
        depthStencilAttachment,
        depthClearValue, stencilClearValue,
        depthFinalLayout,
        &nativePass);
    if (result != GFX_RESULT_SUCCESS) {
        return result;
    }

    *outEncoder = gfx::wrap(backend, nativePass);
    return GFX_RESULT_SUCCESS;
}

GfxResult gfxCommandEncoderBeginComputePass(GfxCommandEncoder encoder, const char* label, GfxComputePassEncoder* outEncoder)
{
    if (!encoder || !outEncoder) {
        return GFX_RESULT_ERROR_INVALID_PARAMETER;
    }
    *outEncoder = nullptr;
    auto api = gfx::getAPI(encoder);
    if (!api) {
        return GFX_RESULT_ERROR_FEATURE_NOT_SUPPORTED;
    }

    GfxBackend backend = gfx::getBackend(encoder);
    GfxComputePassEncoder nativePass = nullptr;
    GfxResult result = api->commandEncoderBeginComputePass(gfx::native(encoder), label, &nativePass);
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

// Additional Queue Functions
GfxResult gfxQueueSubmitWithSync(GfxQueue queue, const GfxSubmitInfo* submitInfo)
{
    if (!queue || !submitInfo) {
        return GFX_RESULT_ERROR_INVALID_PARAMETER;
    }
    auto api = gfx::getAPI(queue);
    if (!api) {
        return GFX_RESULT_ERROR_FEATURE_NOT_SUPPORTED;
    }
    return api->queueSubmitWithSync(gfx::native(queue), submitInfo);
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

} // extern "C"
