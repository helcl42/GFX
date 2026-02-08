#ifndef GFX_BACKEND_WEBGPU_SYSTEM_COMPONENT_H
#define GFX_BACKEND_WEBGPU_SYSTEM_COMPONENT_H

#include <gfx/gfx.h>

namespace gfx::backend::webgpu::component {

class SystemComponent {
public:
    // Instance functions
    GfxResult createInstance(const GfxInstanceDescriptor* descriptor, GfxInstance* outInstance) const;
    GfxResult instanceDestroy(GfxInstance instance) const;
    GfxResult instanceRequestAdapter(GfxInstance instance, const GfxAdapterDescriptor* descriptor, GfxAdapter* outAdapter) const;
    GfxResult instanceEnumerateAdapters(GfxInstance instance, uint32_t* adapterCount, GfxAdapter* adapters) const;
    GfxResult enumerateInstanceExtensions(uint32_t* extensionCount, const char** extensionNames) const;

    // Adapter functions
    GfxResult adapterCreateDevice(GfxAdapter adapter, const GfxDeviceDescriptor* descriptor, GfxDevice* outDevice) const;
    GfxResult adapterGetInfo(GfxAdapter adapter, GfxAdapterInfo* outInfo) const;
    GfxResult adapterGetLimits(GfxAdapter adapter, GfxDeviceLimits* outLimits) const;
    GfxResult adapterEnumerateQueueFamilies(GfxAdapter adapter, uint32_t* queueFamilyCount, GfxQueueFamilyProperties* queueFamilies) const;
    GfxResult adapterGetQueueFamilySurfaceSupport(GfxAdapter adapter, uint32_t queueFamilyIndex, GfxSurface surface, bool* outSupported) const;
    GfxResult adapterEnumerateExtensions(GfxAdapter adapter, uint32_t* extensionCount, const char** extensionNames) const;

    // Device functions
    GfxResult deviceDestroy(GfxDevice device) const;
    GfxResult deviceGetQueue(GfxDevice device, GfxQueue* outQueue) const;
    GfxResult deviceGetQueueByIndex(GfxDevice device, uint32_t queueFamilyIndex, uint32_t queueIndex, GfxQueue* outQueue) const;
    GfxResult deviceWaitIdle(GfxDevice device) const;
    GfxResult deviceGetLimits(GfxDevice device, GfxDeviceLimits* outLimits) const;
    GfxResult deviceSupportsShaderFormat(GfxDevice device, GfxShaderSourceType format, bool* outSupported) const;

    // Queue functions
    GfxResult queueSubmit(GfxQueue queue, const GfxSubmitDescriptor* submitDescriptor) const;
    GfxResult queueWriteBuffer(GfxQueue queue, GfxBuffer buffer, uint64_t offset, const void* data, uint64_t size) const;
    GfxResult queueWriteTexture(GfxQueue queue, GfxTexture texture, const GfxOrigin3D* origin, const GfxExtent3D* extent, uint32_t mipLevel, const void* data, uint64_t dataSize, GfxTextureLayout finalLayout) const;
    GfxResult queueWaitIdle(GfxQueue queue) const;
};

} // namespace gfx::backend::webgpu::component

#endif // GFX_BACKEND_WEBGPU_SYSTEM_COMPONENT_H
