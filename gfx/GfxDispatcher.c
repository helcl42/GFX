#include "GfxApi.h"
#include "GfxBackend.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

// ============================================================================
// Runtime Backend Dispatcher - Handle Metadata System
// ============================================================================

#define GFX_HANDLE_MAGIC 0x47465800 // "GFX\0"

typedef struct {
    uint32_t magic;
    GfxBackend backend;
    void* backendHandle;
} GfxHandleMetadata;

static const GfxBackendAPI* g_backendAPIs[3] = { NULL, NULL, NULL };
static int g_backendRefCounts[3] = { 0, 0, 0 };

static inline const GfxBackendAPI* getBackendAPI(GfxBackend backend)
{
    if (backend < 0 || backend >= GFX_BACKEND_AUTO)
        return NULL;
    return g_backendAPIs[backend];
}

// Helper function to safely unwrap or pass through backend handles
static inline void* unwrapHandleOrPassthrough(void* handle)
{
    if (!handle)
        return NULL;

    GfxHandleMetadata* meta = (GfxHandleMetadata*)handle;

    // Check if this is a wrapped handle
    if (meta->magic == GFX_HANDLE_MAGIC) {
        return meta->backendHandle;
    }

    // Not wrapped - return as-is (e.g., swapchain texture views)
    return handle;
}

static inline void* wrapHandle(GfxBackend backend, void* backendHandle)
{
    if (!backendHandle)
        return NULL;

    GfxHandleMetadata* meta = (GfxHandleMetadata*)malloc(sizeof(GfxHandleMetadata));
    if (!meta)
        return NULL;

    meta->magic = GFX_HANDLE_MAGIC;
    meta->backend = backend;
    meta->backendHandle = backendHandle;

    return (void*)meta;
}

static inline const GfxBackendAPI* unwrapHandle(void* handle, void** backendHandle)
{
    if (!handle)
        return NULL;

    GfxHandleMetadata* meta = (GfxHandleMetadata*)handle;

    if (meta->magic != GFX_HANDLE_MAGIC) {
        return NULL;
    }

    if (backendHandle) {
        *backendHandle = meta->backendHandle;
    }

    return getBackendAPI(meta->backend);
}

static inline void destroyHandle(void* handle)
{
    if (!handle)
        return;

    GfxHandleMetadata* meta = (GfxHandleMetadata*)handle;
    if (meta->magic == GFX_HANDLE_MAGIC) {
        meta->magic = 0;
        free(meta);
    }
}

// ============================================================================
// Backend Loading/Unloading Functions
// ============================================================================

bool gfxLoadBackend(GfxBackend backend)
{
    switch (backend) {
    case GFX_BACKEND_VULKAN:
#ifdef GFX_ENABLE_VULKAN
        if (!g_backendAPIs[GFX_BACKEND_VULKAN]) {
            g_backendAPIs[GFX_BACKEND_VULKAN] = gfxGetVulkanBackend();
            if (!g_backendAPIs[GFX_BACKEND_VULKAN]) {
                return false;
            }
            g_backendRefCounts[GFX_BACKEND_VULKAN] = 0;
        }
        g_backendRefCounts[GFX_BACKEND_VULKAN]++;
        return true;
#else
        return false;
#endif

    case GFX_BACKEND_WEBGPU:
#ifdef GFX_ENABLE_WEBGPU
        if (!g_backendAPIs[GFX_BACKEND_WEBGPU]) {
            g_backendAPIs[GFX_BACKEND_WEBGPU] = gfxGetWebGPUBackend();
            if (!g_backendAPIs[GFX_BACKEND_WEBGPU]) {
                return false;
            }
            g_backendRefCounts[GFX_BACKEND_WEBGPU] = 0;
        }
        g_backendRefCounts[GFX_BACKEND_WEBGPU]++;
        return true;
#else
        return false;
#endif

    case GFX_BACKEND_AUTO:
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

    default:
        return false;
    }
}

void gfxUnloadBackend(GfxBackend backend)
{
    switch (backend) {
    case GFX_BACKEND_VULKAN:
#ifdef GFX_ENABLE_VULKAN
        if (g_backendAPIs[GFX_BACKEND_VULKAN] && g_backendRefCounts[GFX_BACKEND_VULKAN] > 0) {
            g_backendRefCounts[GFX_BACKEND_VULKAN]--;
            if (g_backendRefCounts[GFX_BACKEND_VULKAN] == 0) {
                g_backendAPIs[GFX_BACKEND_VULKAN] = NULL;
            }
        }
#endif
        break;

    case GFX_BACKEND_WEBGPU:
#ifdef GFX_ENABLE_WEBGPU
        if (g_backendAPIs[GFX_BACKEND_WEBGPU] && g_backendRefCounts[GFX_BACKEND_WEBGPU] > 0) {
            g_backendRefCounts[GFX_BACKEND_WEBGPU]--;
            if (g_backendRefCounts[GFX_BACKEND_WEBGPU] == 0) {
                g_backendAPIs[GFX_BACKEND_WEBGPU] = NULL;
            }
        }
#endif
        break;

    case GFX_BACKEND_AUTO:
#ifdef GFX_ENABLE_VULKAN
        if (g_backendAPIs[GFX_BACKEND_VULKAN]) {
            gfxUnloadBackend(GFX_BACKEND_VULKAN);
            return;
        }
#endif
#ifdef GFX_ENABLE_WEBGPU
        if (g_backendAPIs[GFX_BACKEND_WEBGPU]) {
            gfxUnloadBackend(GFX_BACKEND_WEBGPU);
            return;
        }
#endif
        break;

    default:
        break;
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
#ifdef GFX_ENABLE_VULKAN
    while (g_backendRefCounts[GFX_BACKEND_VULKAN] > 0) {
        gfxUnloadBackend(GFX_BACKEND_VULKAN);
    }
#endif

#ifdef GFX_ENABLE_WEBGPU
    while (g_backendRefCounts[GFX_BACKEND_WEBGPU] > 0) {
        gfxUnloadBackend(GFX_BACKEND_WEBGPU);
    }
#endif
}

// ============================================================================
// Instance Functions
// ============================================================================

GfxInstance gfxCreateInstance(const GfxInstanceDescriptor* descriptor)
{
    if (!descriptor)
        return NULL;

    GfxBackend backend = descriptor->backend;
    if (backend == GFX_BACKEND_AUTO) {
#ifdef GFX_ENABLE_VULKAN
        if (g_backendAPIs[GFX_BACKEND_VULKAN]) {
            backend = GFX_BACKEND_VULKAN;
        } else
#endif
#ifdef GFX_ENABLE_WEBGPU
            if (g_backendAPIs[GFX_BACKEND_WEBGPU]) {
            backend = GFX_BACKEND_WEBGPU;
        } else
#endif
        {
            return NULL;
        }
    }

    const GfxBackendAPI* api = getBackendAPI(backend);
    if (!api)
        return NULL;

    void* backendInstance = api->createInstance(descriptor);
    if (!backendInstance)
        return NULL;

    return wrapHandle(backend, backendInstance);
}

void gfxInstanceDestroy(GfxInstance instance)
{
    if (!instance)
        return;

    void* backendHandle;
    const GfxBackendAPI* api = unwrapHandle(instance, &backendHandle);

    if (api && backendHandle) {
        api->instanceDestroy(backendHandle);
    }

    destroyHandle(instance);
}

GfxAdapter gfxInstanceRequestAdapter(GfxInstance instance, const GfxAdapterDescriptor* descriptor)
{
    void* backendHandle;
    const GfxBackendAPI* api = unwrapHandle(instance, &backendHandle);
    if (!api || !backendHandle)
        return NULL;

    void* result = api->instanceRequestAdapter(backendHandle, descriptor);
    if (!result)
        return NULL;

    GfxHandleMetadata* meta = (GfxHandleMetadata*)instance;
    return wrapHandle(meta->backend, result);
}

uint32_t gfxInstanceEnumerateAdapters(GfxInstance instance, GfxAdapter* adapters, uint32_t maxAdapters)
{
    void* backendHandle;
    const GfxBackendAPI* api = unwrapHandle(instance, &backendHandle);
    if (!api || !backendHandle)
        return 0;

    GfxAdapter* backendAdapters = (GfxAdapter*)malloc(maxAdapters * sizeof(GfxAdapter));
    if (!backendAdapters)
        return 0;

    uint32_t count = api->instanceEnumerateAdapters(backendHandle, backendAdapters, maxAdapters);

    GfxHandleMetadata* meta = (GfxHandleMetadata*)instance;
    for (uint32_t i = 0; i < count && i < maxAdapters; i++) {
        adapters[i] = wrapHandle(meta->backend, backendAdapters[i]);
    }

    free(backendAdapters);
    return count;
}

// ============================================================================
// Adapter Functions
// ============================================================================

void gfxAdapterDestroy(GfxAdapter adapter)
{
    if (!adapter)
        return;

    void* backendHandle;
    const GfxBackendAPI* api = unwrapHandle(adapter, &backendHandle);

    if (api && backendHandle) {
        api->adapterDestroy(backendHandle);
    }

    destroyHandle(adapter);
}

GfxDevice gfxAdapterCreateDevice(GfxAdapter adapter, const GfxDeviceDescriptor* descriptor)
{
    void* backendHandle;
    const GfxBackendAPI* api = unwrapHandle(adapter, &backendHandle);
    if (!api || !backendHandle)
        return NULL;

    void* result = api->adapterCreateDevice(backendHandle, descriptor);
    if (!result)
        return NULL;

    GfxHandleMetadata* meta = (GfxHandleMetadata*)adapter;
    return wrapHandle(meta->backend, result);
}

const char* gfxAdapterGetName(GfxAdapter adapter)
{
    void* backendHandle;
    const GfxBackendAPI* api = unwrapHandle(adapter, &backendHandle);
    if (!api || !backendHandle)
        return NULL;

    return api->adapterGetName(backendHandle);
}

GfxBackend gfxAdapterGetBackend(GfxAdapter adapter)
{
    if (!adapter)
        return GFX_BACKEND_AUTO;

    GfxHandleMetadata* meta = (GfxHandleMetadata*)adapter;
    if (meta->magic != GFX_HANDLE_MAGIC)
        return GFX_BACKEND_AUTO;

    return meta->backend;
}

// ============================================================================
// Device Functions
// ============================================================================

void gfxDeviceDestroy(GfxDevice device)
{
    if (!device)
        return;

    void* backendHandle;
    const GfxBackendAPI* api = unwrapHandle(device, &backendHandle);

    if (api && backendHandle) {
        api->deviceDestroy(backendHandle);
    }

    destroyHandle(device);
}

GfxQueue gfxDeviceGetQueue(GfxDevice device)
{
    void* backendHandle;
    const GfxBackendAPI* api = unwrapHandle(device, &backendHandle);
    if (!api || !backendHandle)
        return NULL;

    void* result = api->deviceGetQueue(backendHandle);
    if (!result)
        return NULL;

    // IMPORTANT: Do NOT wrap queue handles!
    // Queues are owned by the device and should not be destroyed by the user.
    // Wrapping them would create a memory leak since the wrapper is never freed.
    // Return the backend handle directly.
    return result;
}

GfxSurface gfxDeviceCreateSurface(GfxDevice device, const GfxSurfaceDescriptor* descriptor)
{
    void* backendHandle;
    const GfxBackendAPI* api = unwrapHandle(device, &backendHandle);
    if (!api || !backendHandle)
        return NULL;

    void* result = api->deviceCreateSurface(backendHandle, descriptor);
    if (!result)
        return NULL;

    GfxHandleMetadata* meta = (GfxHandleMetadata*)device;
    return wrapHandle(meta->backend, result);
}

GfxSwapchain gfxDeviceCreateSwapchain(GfxDevice device, GfxSurface surface, const GfxSwapchainDescriptor* descriptor)
{
    void* backendDevice;
    const GfxBackendAPI* api = unwrapHandle(device, &backendDevice);
    if (!api || !backendDevice)
        return NULL;

    void* backendSurface;
    unwrapHandle(surface, &backendSurface);
    if (!backendSurface)
        return NULL;

    GfxHandleMetadata* meta = (GfxHandleMetadata*)device;
    void* result = api->deviceCreateSwapchain(backendDevice, backendSurface, descriptor);
    return result ? wrapHandle(meta->backend, result) : NULL;
}

GfxBuffer gfxDeviceCreateBuffer(GfxDevice device, const GfxBufferDescriptor* descriptor)
{
    void* backendHandle;
    const GfxBackendAPI* api = unwrapHandle(device, &backendHandle);
    if (!api || !backendHandle)
        return NULL;

    void* result = api->deviceCreateBuffer(backendHandle, descriptor);
    if (!result)
        return NULL;

    GfxHandleMetadata* meta = (GfxHandleMetadata*)device;
    return wrapHandle(meta->backend, result);
}

GfxTexture gfxDeviceCreateTexture(GfxDevice device, const GfxTextureDescriptor* descriptor)
{
    void* backendHandle;
    const GfxBackendAPI* api = unwrapHandle(device, &backendHandle);
    if (!api || !backendHandle)
        return NULL;

    void* result = api->deviceCreateTexture(backendHandle, descriptor);
    if (!result)
        return NULL;

    GfxHandleMetadata* meta = (GfxHandleMetadata*)device;
    return wrapHandle(meta->backend, result);
}

GfxSampler gfxDeviceCreateSampler(GfxDevice device, const GfxSamplerDescriptor* descriptor)
{
    void* backendHandle;
    const GfxBackendAPI* api = unwrapHandle(device, &backendHandle);
    if (!api || !backendHandle)
        return NULL;

    void* result = api->deviceCreateSampler(backendHandle, descriptor);
    if (!result)
        return NULL;

    GfxHandleMetadata* meta = (GfxHandleMetadata*)device;
    return wrapHandle(meta->backend, result);
}

GfxShader gfxDeviceCreateShader(GfxDevice device, const GfxShaderDescriptor* descriptor)
{
    void* backendHandle;
    const GfxBackendAPI* api = unwrapHandle(device, &backendHandle);
    if (!api || !backendHandle)
        return NULL;

    void* result = api->deviceCreateShader(backendHandle, descriptor);
    if (!result)
        return NULL;

    GfxHandleMetadata* meta = (GfxHandleMetadata*)device;
    return wrapHandle(meta->backend, result);
}

GfxBindGroupLayout gfxDeviceCreateBindGroupLayout(GfxDevice device, const GfxBindGroupLayoutDescriptor* descriptor)
{
    void* backendHandle;
    const GfxBackendAPI* api = unwrapHandle(device, &backendHandle);
    if (!api || !backendHandle)
        return NULL;

    void* result = api->deviceCreateBindGroupLayout(backendHandle, descriptor);
    if (!result)
        return NULL;

    GfxHandleMetadata* meta = (GfxHandleMetadata*)device;
    return wrapHandle(meta->backend, result);
}

GfxBindGroup gfxDeviceCreateBindGroup(GfxDevice device, const GfxBindGroupDescriptor* descriptor)
{
    void* backendHandle;
    const GfxBackendAPI* api = unwrapHandle(device, &backendHandle);
    if (!api || !backendHandle)
        return NULL;

    // Create a copy of the descriptor with unwrapped layout and entries
    GfxBindGroupDescriptor backendDescriptor = *descriptor;
    void* backendLayout;
    unwrapHandle((void*)descriptor->layout, &backendLayout);
    backendDescriptor.layout = (GfxBindGroupLayout)backendLayout;

    // Unwrap resource handles in entries
    GfxBindGroupEntry* backendEntries = NULL;
    if (descriptor->entryCount > 0) {
        backendEntries = (GfxBindGroupEntry*)malloc(descriptor->entryCount * sizeof(GfxBindGroupEntry));
        for (uint32_t i = 0; i < descriptor->entryCount; i++) {
            backendEntries[i] = descriptor->entries[i];

            // Unwrap the resource handle based on entry type
            switch (descriptor->entries[i].type) {
            case GFX_BIND_GROUP_ENTRY_TYPE_BUFFER: {
                void* backendBuffer;
                unwrapHandle((void*)descriptor->entries[i].resource.buffer.buffer, &backendBuffer);
                backendEntries[i].resource.buffer.buffer = (GfxBuffer)backendBuffer;
                break;
            }
            case GFX_BIND_GROUP_ENTRY_TYPE_SAMPLER: {
                void* backendSampler;
                unwrapHandle((void*)descriptor->entries[i].resource.sampler, &backendSampler);
                backendEntries[i].resource.sampler = (GfxSampler)backendSampler;
                break;
            }
            case GFX_BIND_GROUP_ENTRY_TYPE_TEXTURE_VIEW: {
                void* backendTextureView;
                unwrapHandle((void*)descriptor->entries[i].resource.textureView, &backendTextureView);
                backendEntries[i].resource.textureView = (GfxTextureView)backendTextureView;
                break;
            }
            default:
                break;
            }
        }
        backendDescriptor.entries = backendEntries;
    }

    void* result = api->deviceCreateBindGroup(backendHandle, &backendDescriptor);

    // Free the temporary entries array
    if (backendEntries) {
        free(backendEntries);
    }

    if (!result)
        return NULL;

    GfxHandleMetadata* meta = (GfxHandleMetadata*)device;
    return wrapHandle(meta->backend, result);
}

GfxRenderPipeline gfxDeviceCreateRenderPipeline(GfxDevice device, const GfxRenderPipelineDescriptor* descriptor)
{
    void* backendHandle;
    const GfxBackendAPI* api = unwrapHandle(device, &backendHandle);
    if (!api || !backendHandle)
        return NULL;

    // Create a copy of the descriptor with unwrapped handles
    GfxRenderPipelineDescriptor backendDescriptor = *descriptor;

    // Unwrap shader modules
    GfxVertexState vertexState = *descriptor->vertex;
    if (descriptor->vertex->module) {
        void* backendShader;
        unwrapHandle((void*)descriptor->vertex->module, &backendShader);
        vertexState.module = (GfxShader)backendShader;
    }
    backendDescriptor.vertex = &vertexState;

    GfxFragmentState fragmentState;
    if (descriptor->fragment) {
        fragmentState = *descriptor->fragment;
        if (descriptor->fragment->module) {
            void* backendShader;
            unwrapHandle((void*)descriptor->fragment->module, &backendShader);
            fragmentState.module = (GfxShader)backendShader;
        }
        backendDescriptor.fragment = &fragmentState;
    }

    // Unwrap bind group layouts
    GfxBindGroupLayout* backendLayouts = NULL;
    if (descriptor->bindGroupLayoutCount > 0 && descriptor->bindGroupLayouts) {
        backendLayouts = (GfxBindGroupLayout*)malloc(descriptor->bindGroupLayoutCount * sizeof(GfxBindGroupLayout));
        for (uint32_t i = 0; i < descriptor->bindGroupLayoutCount; i++) {
            void* backendLayout;
            unwrapHandle((void*)descriptor->bindGroupLayouts[i], &backendLayout);
            backendLayouts[i] = (GfxBindGroupLayout)backendLayout;
        }
        backendDescriptor.bindGroupLayouts = backendLayouts;
    }

    void* result = api->deviceCreateRenderPipeline(backendHandle, &backendDescriptor);

    // Free temporary allocations
    if (backendLayouts) {
        free(backendLayouts);
    }

    if (!result)
        return NULL;

    GfxHandleMetadata* meta = (GfxHandleMetadata*)device;
    return wrapHandle(meta->backend, result);
}

GfxComputePipeline gfxDeviceCreateComputePipeline(GfxDevice device, const GfxComputePipelineDescriptor* descriptor)
{
    void* backendHandle;
    const GfxBackendAPI* api = unwrapHandle(device, &backendHandle);
    if (!api || !backendHandle)
        return NULL;

    void* result = api->deviceCreateComputePipeline(backendHandle, descriptor);
    if (!result)
        return NULL;

    GfxHandleMetadata* meta = (GfxHandleMetadata*)device;
    return wrapHandle(meta->backend, result);
}

GfxCommandEncoder gfxDeviceCreateCommandEncoder(GfxDevice device, const char* label)
{
    void* backendHandle;
    const GfxBackendAPI* api = unwrapHandle(device, &backendHandle);
    if (!api || !backendHandle)
        return NULL;

    void* result = api->deviceCreateCommandEncoder(backendHandle, label);
    if (!result)
        return NULL;

    GfxHandleMetadata* meta = (GfxHandleMetadata*)device;
    return wrapHandle(meta->backend, result);
}

GfxFence gfxDeviceCreateFence(GfxDevice device, const GfxFenceDescriptor* descriptor)
{
    void* backendHandle;
    const GfxBackendAPI* api = unwrapHandle(device, &backendHandle);
    if (!api || !backendHandle)
        return NULL;

    void* result = api->deviceCreateFence(backendHandle, descriptor);
    if (!result)
        return NULL;

    GfxHandleMetadata* meta = (GfxHandleMetadata*)device;
    return wrapHandle(meta->backend, result);
}

GfxSemaphore gfxDeviceCreateSemaphore(GfxDevice device, const GfxSemaphoreDescriptor* descriptor)
{
    void* backendHandle;
    const GfxBackendAPI* api = unwrapHandle(device, &backendHandle);
    if (!api || !backendHandle)
        return NULL;

    void* result = api->deviceCreateSemaphore(backendHandle, descriptor);
    if (!result)
        return NULL;

    GfxHandleMetadata* meta = (GfxHandleMetadata*)device;
    return wrapHandle(meta->backend, result);
}

void gfxDeviceWaitIdle(GfxDevice device)
{
    void* backendHandle;
    const GfxBackendAPI* api = unwrapHandle(device, &backendHandle);
    if (api && backendHandle) {
        api->deviceWaitIdle(backendHandle);
    }
}

// ============================================================================
// Surface Functions
// ============================================================================

void gfxSurfaceDestroy(GfxSurface surface)
{
    if (!surface)
        return;

    void* backendHandle;
    const GfxBackendAPI* api = unwrapHandle(surface, &backendHandle);

    if (api && backendHandle) {
        api->surfaceDestroy(backendHandle);
    }

    destroyHandle(surface);
}

uint32_t gfxSurfaceGetWidth(GfxSurface surface)
{
    void* backendHandle;
    const GfxBackendAPI* api = unwrapHandle(surface, &backendHandle);
    if (!api || !backendHandle)
        return 0;

    return api->surfaceGetWidth(backendHandle);
}

uint32_t gfxSurfaceGetHeight(GfxSurface surface)
{
    void* backendHandle;
    const GfxBackendAPI* api = unwrapHandle(surface, &backendHandle);
    if (!api || !backendHandle)
        return 0;

    return api->surfaceGetHeight(backendHandle);
}

void gfxSurfaceResize(GfxSurface surface, uint32_t width, uint32_t height)
{
    void* backendHandle;
    const GfxBackendAPI* api = unwrapHandle(surface, &backendHandle);
    if (api && backendHandle) {
        api->surfaceResize(backendHandle, width, height);
    }
}

uint32_t gfxSurfaceGetSupportedFormats(GfxSurface surface, GfxTextureFormat* formats, uint32_t maxFormats)
{
    void* backendHandle;
    const GfxBackendAPI* api = unwrapHandle(surface, &backendHandle);
    if (!api || !backendHandle)
        return 0;

    return api->surfaceGetSupportedFormats(backendHandle, formats, maxFormats);
}

uint32_t gfxSurfaceGetSupportedPresentModes(GfxSurface surface, GfxPresentMode* presentModes, uint32_t maxModes)
{
    void* backendHandle;
    const GfxBackendAPI* api = unwrapHandle(surface, &backendHandle);
    if (!api || !backendHandle)
        return 0;

    return api->surfaceGetSupportedPresentModes(backendHandle, presentModes, maxModes);
}

GfxPlatformWindowHandle gfxSurfaceGetPlatformHandle(GfxSurface surface)
{
    void* backendHandle;
    const GfxBackendAPI* api = unwrapHandle(surface, &backendHandle);

    GfxPlatformWindowHandle handle = { 0 };
    if (!api || !backendHandle)
        return handle;

    return api->surfaceGetPlatformHandle(backendHandle);
}

// ============================================================================
// Swapchain Functions
// ============================================================================

void gfxSwapchainDestroy(GfxSwapchain swapchain)
{
    if (!swapchain)
        return;

    void* backendHandle;
    const GfxBackendAPI* api = unwrapHandle(swapchain, &backendHandle);

    if (api && backendHandle) {
        api->swapchainDestroy(backendHandle);
    }

    destroyHandle(swapchain);
}

uint32_t gfxSwapchainGetWidth(GfxSwapchain swapchain)
{
    void* backendHandle;
    const GfxBackendAPI* api = unwrapHandle(swapchain, &backendHandle);
    if (!api || !backendHandle)
        return 0;

    return api->swapchainGetWidth(backendHandle);
}

uint32_t gfxSwapchainGetHeight(GfxSwapchain swapchain)
{
    void* backendHandle;
    const GfxBackendAPI* api = unwrapHandle(swapchain, &backendHandle);
    if (!api || !backendHandle)
        return 0;

    return api->swapchainGetHeight(backendHandle);
}

GfxTextureFormat gfxSwapchainGetFormat(GfxSwapchain swapchain)
{
    void* backendHandle;
    const GfxBackendAPI* api = unwrapHandle(swapchain, &backendHandle);
    if (!api || !backendHandle)
        return GFX_TEXTURE_FORMAT_UNDEFINED;

    return api->swapchainGetFormat(backendHandle);
}

uint32_t gfxSwapchainGetBufferCount(GfxSwapchain swapchain)
{
    void* backendHandle;
    const GfxBackendAPI* api = unwrapHandle(swapchain, &backendHandle);
    if (!api || !backendHandle)
        return 0;

    return api->swapchainGetBufferCount(backendHandle);
}

GfxTextureView gfxSwapchainGetCurrentTextureView(GfxSwapchain swapchain)
{
    void* backendHandle;
    const GfxBackendAPI* api = unwrapHandle(swapchain, &backendHandle);
    if (!api || !backendHandle)
        return NULL;

    void* result = api->swapchainGetCurrentTextureView(backendHandle);
    if (!result)
        return NULL;

    // IMPORTANT: Do NOT wrap swapchain texture views!
    // These are owned by the swapchain and should not be destroyed by the user.
    // Wrapping them would create a memory leak since the wrapper is never freed.
    // Return the backend handle directly.
    return result;
}

void gfxSwapchainPresent(GfxSwapchain swapchain)
{
    void* backendHandle;
    const GfxBackendAPI* api = unwrapHandle(swapchain, &backendHandle);
    if (api && backendHandle) {
        api->swapchainPresent(backendHandle);
    }
}

void gfxSwapchainResize(GfxSwapchain swapchain, uint32_t width, uint32_t height)
{
    void* backendHandle;
    const GfxBackendAPI* api = unwrapHandle(swapchain, &backendHandle);
    if (api && backendHandle) {
        api->swapchainResize(backendHandle, width, height);
    }
}

bool gfxSwapchainNeedsRecreation(GfxSwapchain swapchain)
{
    void* backendHandle;
    const GfxBackendAPI* api = unwrapHandle(swapchain, &backendHandle);
    if (!api || !backendHandle)
        return false;

    return api->swapchainNeedsRecreation(backendHandle);
}

// ============================================================================
// Buffer Functions
// ============================================================================

void gfxBufferDestroy(GfxBuffer buffer)
{
    if (!buffer)
        return;

    void* backendHandle;
    const GfxBackendAPI* api = unwrapHandle(buffer, &backendHandle);

    if (api && backendHandle) {
        api->bufferDestroy(backendHandle);
    }

    destroyHandle(buffer);
}

uint64_t gfxBufferGetSize(GfxBuffer buffer)
{
    void* backendHandle;
    const GfxBackendAPI* api = unwrapHandle(buffer, &backendHandle);
    if (!api || !backendHandle)
        return 0;

    return api->bufferGetSize(backendHandle);
}

GfxBufferUsage gfxBufferGetUsage(GfxBuffer buffer)
{
    void* backendHandle;
    const GfxBackendAPI* api = unwrapHandle(buffer, &backendHandle);
    if (!api || !backendHandle)
        return GFX_BUFFER_USAGE_NONE;

    return api->bufferGetUsage(backendHandle);
}

void* gfxBufferMapAsync(GfxBuffer buffer, uint64_t offset, uint64_t size)
{
    void* backendHandle;
    const GfxBackendAPI* api = unwrapHandle(buffer, &backendHandle);
    if (!api || !backendHandle)
        return NULL;

    return api->bufferMapAsync(backendHandle, offset, size);
}

void gfxBufferUnmap(GfxBuffer buffer)
{
    void* backendHandle;
    const GfxBackendAPI* api = unwrapHandle(buffer, &backendHandle);
    if (api && backendHandle) {
        api->bufferUnmap(backendHandle);
    }
}

// ============================================================================
// Texture Functions
// ============================================================================

void gfxTextureDestroy(GfxTexture texture)
{
    if (!texture)
        return;

    void* backendHandle;
    const GfxBackendAPI* api = unwrapHandle(texture, &backendHandle);

    if (api && backendHandle) {
        api->textureDestroy(backendHandle);
    }

    destroyHandle(texture);
}

GfxExtent3D gfxTextureGetSize(GfxTexture texture)
{
    void* backendHandle;
    const GfxBackendAPI* api = unwrapHandle(texture, &backendHandle);

    GfxExtent3D size = { 0, 0, 0 };
    if (!api || !backendHandle)
        return size;

    return api->textureGetSize(backendHandle);
}

GfxTextureFormat gfxTextureGetFormat(GfxTexture texture)
{
    void* backendHandle;
    const GfxBackendAPI* api = unwrapHandle(texture, &backendHandle);
    if (!api || !backendHandle)
        return GFX_TEXTURE_FORMAT_UNDEFINED;

    return api->textureGetFormat(backendHandle);
}

uint32_t gfxTextureGetMipLevelCount(GfxTexture texture)
{
    void* backendHandle;
    const GfxBackendAPI* api = unwrapHandle(texture, &backendHandle);
    if (!api || !backendHandle)
        return 0;

    return api->textureGetMipLevelCount(backendHandle);
}

uint32_t gfxTextureGetSampleCount(GfxTexture texture)
{
    void* backendHandle;
    const GfxBackendAPI* api = unwrapHandle(texture, &backendHandle);
    if (!api || !backendHandle)
        return 0;

    return api->textureGetSampleCount(backendHandle);
}

GfxTextureUsage gfxTextureGetUsage(GfxTexture texture)
{
    void* backendHandle;
    const GfxBackendAPI* api = unwrapHandle(texture, &backendHandle);
    if (!api || !backendHandle)
        return GFX_TEXTURE_USAGE_NONE;

    return api->textureGetUsage(backendHandle);
}

GfxTextureView gfxTextureCreateView(GfxTexture texture, const GfxTextureViewDescriptor* descriptor)
{
    void* backendHandle;
    const GfxBackendAPI* api = unwrapHandle(texture, &backendHandle);
    if (!api || !backendHandle)
        return NULL;

    void* result = api->textureCreateView(backendHandle, descriptor);
    if (!result)
        return NULL;

    GfxHandleMetadata* meta = (GfxHandleMetadata*)texture;
    return wrapHandle(meta->backend, result);
}

// ============================================================================
// TextureView Functions
// ============================================================================

void gfxTextureViewDestroy(GfxTextureView textureView)
{
    if (!textureView)
        return;

    void* backendHandle;
    const GfxBackendAPI* api = unwrapHandle(textureView, &backendHandle);

    if (api && backendHandle) {
        api->textureViewDestroy(backendHandle);
    }

    destroyHandle(textureView);
}

GfxTexture gfxTextureViewGetTexture(GfxTextureView textureView)
{
    void* backendHandle = unwrapHandleOrPassthrough(textureView);
    const GfxBackendAPI* api = getBackendAPI(((GfxHandleMetadata*)textureView)->backend);
    if (!api || !backendHandle)
        return NULL;

    void* result = api->textureViewGetTexture(backendHandle);
    if (!result)
        return NULL;

    GfxHandleMetadata* meta = (GfxHandleMetadata*)textureView;
    return wrapHandle(meta->backend, result);
}

// ============================================================================
// Sampler Functions
// ============================================================================

void gfxSamplerDestroy(GfxSampler sampler)
{
    if (!sampler)
        return;

    void* backendHandle;
    const GfxBackendAPI* api = unwrapHandle(sampler, &backendHandle);

    if (api && backendHandle) {
        api->samplerDestroy(backendHandle);
    }

    destroyHandle(sampler);
}

// ============================================================================
// Shader Functions
// ============================================================================

void gfxShaderDestroy(GfxShader shader)
{
    if (!shader)
        return;

    void* backendHandle;
    const GfxBackendAPI* api = unwrapHandle(shader, &backendHandle);

    if (api && backendHandle) {
        api->shaderDestroy(backendHandle);
    }

    destroyHandle(shader);
}

// ============================================================================
// BindGroupLayout Functions
// ============================================================================

void gfxBindGroupLayoutDestroy(GfxBindGroupLayout bindGroupLayout)
{
    if (!bindGroupLayout)
        return;

    void* backendHandle;
    const GfxBackendAPI* api = unwrapHandle(bindGroupLayout, &backendHandle);

    if (api && backendHandle) {
        api->bindGroupLayoutDestroy(backendHandle);
    }

    destroyHandle(bindGroupLayout);
}

// ============================================================================
// BindGroup Functions
// ============================================================================

void gfxBindGroupDestroy(GfxBindGroup bindGroup)
{
    if (!bindGroup)
        return;

    void* backendHandle;
    const GfxBackendAPI* api = unwrapHandle(bindGroup, &backendHandle);

    if (api && backendHandle) {
        api->bindGroupDestroy(backendHandle);
    }

    destroyHandle(bindGroup);
}

// ============================================================================
// RenderPipeline Functions
// ============================================================================

void gfxRenderPipelineDestroy(GfxRenderPipeline renderPipeline)
{
    if (!renderPipeline)
        return;

    void* backendHandle;
    const GfxBackendAPI* api = unwrapHandle(renderPipeline, &backendHandle);

    if (api && backendHandle) {
        api->renderPipelineDestroy(backendHandle);
    }

    destroyHandle(renderPipeline);
}

// ============================================================================
// ComputePipeline Functions
// ============================================================================

void gfxComputePipelineDestroy(GfxComputePipeline computePipeline)
{
    if (!computePipeline)
        return;

    void* backendHandle;
    const GfxBackendAPI* api = unwrapHandle(computePipeline, &backendHandle);

    if (api && backendHandle) {
        api->computePipelineDestroy(backendHandle);
    }

    destroyHandle(computePipeline);
}

// ============================================================================
// Queue Functions
// ============================================================================

void gfxQueueSubmit(GfxQueue queue, GfxCommandEncoder commandEncoder)
{
    void* backendQueue = unwrapHandleOrPassthrough(queue);
    const GfxBackendAPI* api = unwrapHandle(commandEncoder, NULL);
    if (!api || !backendQueue)
        return;

    void* backendEncoder;
    unwrapHandle(commandEncoder, &backendEncoder);
    if (!backendEncoder)
        return;

    api->queueSubmit(backendQueue, backendEncoder);
}

void gfxQueueSubmitWithSync(GfxQueue queue, const GfxSubmitInfo* submitInfo)
{
    void* backendHandle = unwrapHandleOrPassthrough(queue);
    if (!backendHandle)
        return;

    // Get API from the first backend available (since queue isn't wrapped)
    const GfxBackendAPI* api = NULL;
#ifdef GFX_ENABLE_VULKAN
    if (g_backendAPIs[GFX_BACKEND_VULKAN]) {
        api = g_backendAPIs[GFX_BACKEND_VULKAN];
    }
#endif

    if (api && backendHandle) {
        api->queueSubmitWithSync(backendHandle, submitInfo);
    }
}

void gfxQueueWriteBuffer(GfxQueue queue, GfxBuffer buffer, uint64_t offset, const void* data, uint64_t size)
{
    void* backendQueue = unwrapHandleOrPassthrough(queue);
    const GfxBackendAPI* api = unwrapHandle(buffer, NULL);
    if (!api || !backendQueue)
        return;

    void* backendBuffer;
    unwrapHandle(buffer, &backendBuffer);
    if (!backendBuffer)
        return;

    api->queueWriteBuffer(backendQueue, backendBuffer, offset, data, size);
}

void gfxQueueWriteTexture(GfxQueue queue, GfxTexture texture, const GfxOrigin3D* origin, uint32_t mipLevel,
    const void* data, uint64_t dataSize, uint32_t bytesPerRow, const GfxExtent3D* extent)
{
    void* backendQueue = unwrapHandleOrPassthrough(queue);
    const GfxBackendAPI* api = unwrapHandle(texture, NULL);
    if (!api || !backendQueue)
        return;

    void* backendTexture;
    unwrapHandle(texture, &backendTexture);
    if (!backendTexture)
        return;

    api->queueWriteTexture(backendQueue, backendTexture, origin, mipLevel, data, dataSize, bytesPerRow, extent);
}

void gfxQueueWaitIdle(GfxQueue queue)
{
    void* backendHandle = unwrapHandleOrPassthrough(queue);
    if (!backendHandle)
        return;

    // Get API from the first backend available (since queue isn't wrapped)
    const GfxBackendAPI* api = NULL;
#ifdef GFX_ENABLE_VULKAN
    if (g_backendAPIs[GFX_BACKEND_VULKAN]) {
        api = g_backendAPIs[GFX_BACKEND_VULKAN];
    }
#endif

    if (api && backendHandle) {
        api->queueWaitIdle(backendHandle);
    }
}

// ============================================================================
// CommandEncoder Functions
// ============================================================================

void gfxCommandEncoderDestroy(GfxCommandEncoder commandEncoder)
{
    if (!commandEncoder)
        return;

    void* backendHandle;
    const GfxBackendAPI* api = unwrapHandle(commandEncoder, &backendHandle);

    if (api && backendHandle) {
        api->commandEncoderDestroy(backendHandle);
    }

    destroyHandle(commandEncoder);
}

GfxRenderPassEncoder gfxCommandEncoderBeginRenderPass(GfxCommandEncoder commandEncoder,
    const GfxTextureView* colorAttachments, uint32_t colorAttachmentCount,
    const GfxColor* clearColors,
    GfxTextureView depthStencilAttachment,
    float depthClearValue, uint32_t stencilClearValue)
{
    void* backendEncoder;
    const GfxBackendAPI* api = unwrapHandle(commandEncoder, &backendEncoder);
    if (!api || !backendEncoder)
        return NULL;

    GfxTextureView* backendColorAttachments = NULL;
    if (colorAttachmentCount > 0) {
        backendColorAttachments = (GfxTextureView*)malloc(colorAttachmentCount * sizeof(GfxTextureView));
        for (uint32_t i = 0; i < colorAttachmentCount; i++) {
            backendColorAttachments[i] = (GfxTextureView)unwrapHandleOrPassthrough((void*)colorAttachments[i]);
        }
    }

    void* backendDepthAttachment = NULL;
    if (depthStencilAttachment) {
        backendDepthAttachment = unwrapHandleOrPassthrough(depthStencilAttachment);
    }

    GfxHandleMetadata* meta = (GfxHandleMetadata*)commandEncoder;
    void* result = api->commandEncoderBeginRenderPass(backendEncoder, backendColorAttachments, colorAttachmentCount,
        clearColors, backendDepthAttachment, depthClearValue, stencilClearValue);

    free(backendColorAttachments);

    return result ? wrapHandle(meta->backend, result) : NULL;
}

GfxComputePassEncoder gfxCommandEncoderBeginComputePass(GfxCommandEncoder commandEncoder, const char* label)
{
    void* backendHandle;
    const GfxBackendAPI* api = unwrapHandle(commandEncoder, &backendHandle);
    if (!api || !backendHandle)
        return NULL;

    void* result = api->commandEncoderBeginComputePass(backendHandle, label);
    if (!result)
        return NULL;

    GfxHandleMetadata* meta = (GfxHandleMetadata*)commandEncoder;
    return wrapHandle(meta->backend, result);
}

void gfxCommandEncoderCopyBufferToBuffer(GfxCommandEncoder commandEncoder,
    GfxBuffer source, uint64_t sourceOffset,
    GfxBuffer destination, uint64_t destinationOffset,
    uint64_t size)
{
    void* backendEncoder;
    const GfxBackendAPI* api = unwrapHandle(commandEncoder, &backendEncoder);
    if (!api || !backendEncoder)
        return;

    void* backendSource;
    void* backendDest;
    unwrapHandle(source, &backendSource);
    unwrapHandle(destination, &backendDest);

    api->commandEncoderCopyBufferToBuffer(backendEncoder, backendSource, sourceOffset, backendDest, destinationOffset, size);
}

void gfxCommandEncoderCopyBufferToTexture(GfxCommandEncoder commandEncoder,
    GfxBuffer source, uint64_t sourceOffset, uint32_t bytesPerRow,
    GfxTexture destination, const GfxOrigin3D* origin,
    const GfxExtent3D* extent, uint32_t mipLevel)
{
    void* backendEncoder;
    const GfxBackendAPI* api = unwrapHandle(commandEncoder, &backendEncoder);
    if (!api || !backendEncoder)
        return;

    void* backendSource;
    void* backendDest;
    unwrapHandle(source, &backendSource);
    unwrapHandle(destination, &backendDest);

    api->commandEncoderCopyBufferToTexture(backendEncoder, backendSource, sourceOffset, bytesPerRow,
        backendDest, origin, extent, mipLevel);
}

void gfxCommandEncoderCopyTextureToBuffer(GfxCommandEncoder commandEncoder,
    GfxTexture source, const GfxOrigin3D* origin, uint32_t mipLevel,
    GfxBuffer destination, uint64_t destinationOffset, uint32_t bytesPerRow,
    const GfxExtent3D* extent)
{
    void* backendEncoder;
    const GfxBackendAPI* api = unwrapHandle(commandEncoder, &backendEncoder);
    if (!api || !backendEncoder)
        return;

    void* backendSource;
    void* backendDest;
    unwrapHandle(source, &backendSource);
    unwrapHandle(destination, &backendDest);

    api->commandEncoderCopyTextureToBuffer(backendEncoder, backendSource, origin, mipLevel,
        backendDest, destinationOffset, bytesPerRow, extent);
}

void gfxCommandEncoderFinish(GfxCommandEncoder commandEncoder)
{
    void* backendHandle;
    const GfxBackendAPI* api = unwrapHandle(commandEncoder, &backendHandle);
    if (api && backendHandle) {
        api->commandEncoderFinish(backendHandle);
    }
}

// ============================================================================
// RenderPassEncoder Functions
// ============================================================================

void gfxRenderPassEncoderDestroy(GfxRenderPassEncoder renderPassEncoder)
{
    if (!renderPassEncoder)
        return;

    void* backendHandle;
    const GfxBackendAPI* api = unwrapHandle(renderPassEncoder, &backendHandle);

    if (api && backendHandle) {
        api->renderPassEncoderDestroy(backendHandle);
    }

    destroyHandle(renderPassEncoder);
}

void gfxRenderPassEncoderSetPipeline(GfxRenderPassEncoder renderPassEncoder, GfxRenderPipeline pipeline)
{
    void* backendEncoder;
    const GfxBackendAPI* api = unwrapHandle(renderPassEncoder, &backendEncoder);
    if (!api || !backendEncoder)
        return;

    void* backendPipeline;
    unwrapHandle(pipeline, &backendPipeline);

    api->renderPassEncoderSetPipeline(backendEncoder, backendPipeline);
}

void gfxRenderPassEncoderSetBindGroup(GfxRenderPassEncoder renderPassEncoder, uint32_t index, GfxBindGroup bindGroup)
{
    void* backendEncoder;
    const GfxBackendAPI* api = unwrapHandle(renderPassEncoder, &backendEncoder);
    if (!api || !backendEncoder)
        return;

    void* backendBindGroup;
    unwrapHandle(bindGroup, &backendBindGroup);

    api->renderPassEncoderSetBindGroup(backendEncoder, index, backendBindGroup);
}

void gfxRenderPassEncoderSetVertexBuffer(GfxRenderPassEncoder renderPassEncoder, uint32_t slot, GfxBuffer buffer, uint64_t offset, uint64_t size)
{
    void* backendEncoder;
    const GfxBackendAPI* api = unwrapHandle(renderPassEncoder, &backendEncoder);
    if (!api || !backendEncoder)
        return;

    void* backendBuffer;
    unwrapHandle(buffer, &backendBuffer);

    api->renderPassEncoderSetVertexBuffer(backendEncoder, slot, backendBuffer, offset, size);
}

void gfxRenderPassEncoderSetIndexBuffer(GfxRenderPassEncoder renderPassEncoder, GfxBuffer buffer, GfxIndexFormat format, uint64_t offset, uint64_t size)
{
    void* backendEncoder;
    const GfxBackendAPI* api = unwrapHandle(renderPassEncoder, &backendEncoder);
    if (!api || !backendEncoder)
        return;

    void* backendBuffer;
    unwrapHandle(buffer, &backendBuffer);

    api->renderPassEncoderSetIndexBuffer(backendEncoder, backendBuffer, format, offset, size);
}

void gfxRenderPassEncoderDraw(GfxRenderPassEncoder renderPassEncoder, uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance)
{
    void* backendHandle;
    const GfxBackendAPI* api = unwrapHandle(renderPassEncoder, &backendHandle);
    if (api && backendHandle) {
        api->renderPassEncoderDraw(backendHandle, vertexCount, instanceCount, firstVertex, firstInstance);
    }
}

void gfxRenderPassEncoderDrawIndexed(GfxRenderPassEncoder renderPassEncoder, uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t baseVertex, uint32_t firstInstance)
{
    void* backendHandle;
    const GfxBackendAPI* api = unwrapHandle(renderPassEncoder, &backendHandle);
    if (api && backendHandle) {
        api->renderPassEncoderDrawIndexed(backendHandle, indexCount, instanceCount, firstIndex, baseVertex, firstInstance);
    }
}

void gfxRenderPassEncoderEnd(GfxRenderPassEncoder renderPassEncoder)
{
    void* backendHandle;
    const GfxBackendAPI* api = unwrapHandle(renderPassEncoder, &backendHandle);
    if (api && backendHandle) {
        api->renderPassEncoderEnd(backendHandle);
    }
}

// ============================================================================
// ComputePassEncoder Functions
// ============================================================================

void gfxComputePassEncoderDestroy(GfxComputePassEncoder computePassEncoder)
{
    if (!computePassEncoder)
        return;

    void* backendHandle;
    const GfxBackendAPI* api = unwrapHandle(computePassEncoder, &backendHandle);

    if (api && backendHandle) {
        api->computePassEncoderDestroy(backendHandle);
    }

    destroyHandle(computePassEncoder);
}

void gfxComputePassEncoderSetPipeline(GfxComputePassEncoder computePassEncoder, GfxComputePipeline pipeline)
{
    void* backendEncoder;
    const GfxBackendAPI* api = unwrapHandle(computePassEncoder, &backendEncoder);
    if (!api || !backendEncoder)
        return;

    void* backendPipeline;
    unwrapHandle(pipeline, &backendPipeline);

    api->computePassEncoderSetPipeline(backendEncoder, backendPipeline);
}

void gfxComputePassEncoderSetBindGroup(GfxComputePassEncoder computePassEncoder, uint32_t index, GfxBindGroup bindGroup)
{
    void* backendEncoder;
    const GfxBackendAPI* api = unwrapHandle(computePassEncoder, &backendEncoder);
    if (!api || !backendEncoder)
        return;

    void* backendBindGroup;
    unwrapHandle(bindGroup, &backendBindGroup);

    api->computePassEncoderSetBindGroup(backendEncoder, index, backendBindGroup);
}

void gfxComputePassEncoderDispatchWorkgroups(GfxComputePassEncoder computePassEncoder, uint32_t workgroupCountX, uint32_t workgroupCountY, uint32_t workgroupCountZ)
{
    void* backendHandle;
    const GfxBackendAPI* api = unwrapHandle(computePassEncoder, &backendHandle);
    if (api && backendHandle) {
        api->computePassEncoderDispatchWorkgroups(backendHandle, workgroupCountX, workgroupCountY, workgroupCountZ);
    }
}

void gfxComputePassEncoderEnd(GfxComputePassEncoder computePassEncoder)
{
    void* backendHandle;
    const GfxBackendAPI* api = unwrapHandle(computePassEncoder, &backendHandle);
    if (api && backendHandle) {
        api->computePassEncoderEnd(backendHandle);
    }
}

// ============================================================================
// Fence Functions
// ============================================================================

void gfxFenceDestroy(GfxFence fence)
{
    if (!fence)
        return;

    void* backendHandle;
    const GfxBackendAPI* api = unwrapHandle(fence, &backendHandle);

    if (api && backendHandle) {
        api->fenceDestroy(backendHandle);
    }

    destroyHandle(fence);
}

GfxResult gfxFenceGetStatus(GfxFence fence, bool* isSignaled)
{
    void* backendHandle;
    const GfxBackendAPI* api = unwrapHandle(fence, &backendHandle);
    if (!api || !backendHandle)
        return GFX_RESULT_ERROR_UNKNOWN;

    return api->fenceGetStatus(backendHandle, isSignaled);
}

GfxResult gfxFenceWait(GfxFence fence, uint64_t timeoutNs)
{
    void* backendHandle;
    const GfxBackendAPI* api = unwrapHandle(fence, &backendHandle);
    if (!api || !backendHandle)
        return GFX_RESULT_ERROR_UNKNOWN;

    return api->fenceWait(backendHandle, timeoutNs);
}

void gfxFenceReset(GfxFence fence)
{
    void* backendHandle;
    const GfxBackendAPI* api = unwrapHandle(fence, &backendHandle);
    if (api && backendHandle) {
        api->fenceReset(backendHandle);
    }
}

// ============================================================================
// Semaphore Functions
// ============================================================================

void gfxSemaphoreDestroy(GfxSemaphore semaphore)
{
    if (!semaphore)
        return;

    void* backendHandle;
    const GfxBackendAPI* api = unwrapHandle(semaphore, &backendHandle);

    if (api && backendHandle) {
        api->semaphoreDestroy(backendHandle);
    }

    destroyHandle(semaphore);
}

GfxSemaphoreType gfxSemaphoreGetType(GfxSemaphore semaphore)
{
    void* backendHandle;
    const GfxBackendAPI* api = unwrapHandle(semaphore, &backendHandle);
    if (!api || !backendHandle)
        return GFX_SEMAPHORE_TYPE_BINARY;

    return api->semaphoreGetType(backendHandle);
}

GfxResult gfxSemaphoreSignal(GfxSemaphore semaphore, uint64_t value)
{
    void* backendHandle;
    const GfxBackendAPI* api = unwrapHandle(semaphore, &backendHandle);
    if (!api || !backendHandle)
        return GFX_RESULT_ERROR_UNKNOWN;

    return api->semaphoreSignal(backendHandle, value);
}

GfxResult gfxSemaphoreWait(GfxSemaphore semaphore, uint64_t value, uint64_t timeoutNs)
{
    void* backendHandle;
    const GfxBackendAPI* api = unwrapHandle(semaphore, &backendHandle);
    if (!api || !backendHandle)
        return GFX_RESULT_ERROR_UNKNOWN;

    return api->semaphoreWait(backendHandle, value, timeoutNs);
}

uint64_t gfxSemaphoreGetValue(GfxSemaphore semaphore)
{
    void* backendHandle;
    const GfxBackendAPI* api = unwrapHandle(semaphore, &backendHandle);
    if (!api || !backendHandle)
        return 0;

    return api->semaphoreGetValue(backendHandle);
}
