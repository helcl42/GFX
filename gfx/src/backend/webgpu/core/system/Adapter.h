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

    // Wrap existing WGPUAdapter
    Adapter(WGPUAdapter adapter, Instance* instance);
    ~Adapter();

    WGPUAdapter handle() const;
    Instance* getInstance() const;

    const AdapterInfo& getInfo() const;
    WGPULimits getLimits() const;

    // Queue family properties (WebGPU has single unified queue)
    std::vector<QueueFamilyProperties> getQueueFamilyProperties() const;
    bool supportsPresentation(uint32_t queueFamilyIndex) const;

    // Get supported device extensions for this adapter
    std::vector<const char*> enumerateSupportedExtensions() const;

private:
    AdapterInfo createAdapterInfo() const;

private:
    WGPUAdapter m_adapter = nullptr;
    Instance* m_instance = nullptr; // Non-owning
    AdapterInfo m_info;
};

} // namespace gfx::backend::webgpu::core

#endif // GFX_WEBGPU_ADAPTER_H