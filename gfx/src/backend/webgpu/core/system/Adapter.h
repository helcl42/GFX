#ifndef GFX_WEBGPU_ADAPTER_H
#define GFX_WEBGPU_ADAPTER_H

#include "../CoreTypes.h"

namespace gfx::backend::webgpu::core {

class Instance;

class Adapter {
public:
    // Prevent copying
    Adapter(const Adapter&) = delete;
    Adapter& operator=(const Adapter&) = delete;

    // Constructor 1: Request adapter based on preferences
    Adapter(Instance* instance, const AdapterCreateInfo& createInfo);
    // Constructor 2: Wrap existing WGPUAdapter (used by enumerate)
    Adapter(WGPUAdapter adapter, Instance* instance);
    ~Adapter();

    // Static method to enumerate all available adapters
    // NOTE: WebGPU doesn't have a native enumerate API - returns the default adapter if available
    // Each adapter returned must be freed by the caller using the backend's adapterDestroy method
    // (e.g., gfxAdapterDestroy() in the public API)
    static uint32_t enumerate(Instance* instance, Adapter** outAdapters, uint32_t maxAdapters);

    WGPUAdapter handle() const;
    Instance* getInstance() const;

    const AdapterInfo& getInfo() const;
    WGPULimits getLimits() const;

private:
    AdapterInfo createAdapterInfo() const;

private:
    WGPUAdapter m_adapter = nullptr;
    Instance* m_instance = nullptr; // Non-owning
    AdapterInfo m_info;
};

} // namespace gfx::backend::webgpu::core

#endif // GFX_WEBGPU_ADAPTER_H