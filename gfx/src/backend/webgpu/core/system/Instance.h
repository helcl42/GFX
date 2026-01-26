#ifndef GFX_WEBGPU_INSTANCE_H
#define GFX_WEBGPU_INSTANCE_H

#include "../CoreTypes.h"

#include <memory>

namespace gfx::backend::webgpu::core {

class Adapter;

class Instance {
public:
    // Prevent copying
    Instance(const Instance&) = delete;
    Instance& operator=(const Instance&) = delete;

    Instance(const InstanceCreateInfo& createInfo);
    ~Instance();

    WGPUInstance handle() const;

    static std::vector<const char*> enumerateSupportedExtensions();

    // Adapter management
    Adapter* requestAdapter(const AdapterCreateInfo& createInfo) const;
    const std::vector<std::unique_ptr<Adapter>>& getAdapters() const;

private:
    WGPUInstance m_instance = nullptr;
    mutable std::vector<std::unique_ptr<Adapter>> m_adapters;
};

} // namespace gfx::backend::webgpu::core

#endif // GFX_WEBGPU_INSTANCE_H