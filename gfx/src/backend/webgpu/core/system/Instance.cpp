#include "Instance.h"

namespace gfx::backend::webgpu::core {

Instance::Instance(const InstanceCreateInfo& createInfo)
{
    (void)createInfo; // Descriptor not used in current implementation
    // Request TimedWaitAny feature for proper async callback handling
    static const WGPUInstanceFeatureName requiredFeatures[] = {
        WGPUInstanceFeatureName_TimedWaitAny
    };

    WGPUInstanceDescriptor wgpuDesc = WGPU_INSTANCE_DESCRIPTOR_INIT;
    wgpuDesc.requiredFeatureCount = 1;
    wgpuDesc.requiredFeatures = requiredFeatures;
    m_instance = wgpuCreateInstance(&wgpuDesc);
}

Instance::~Instance()
{
    if (m_instance) {
        wgpuInstanceRelease(m_instance);
    }
}

WGPUInstance Instance::handle() const
{
    return m_instance;
}

std::vector<const char*> Instance::enumerateSupportedExtensions()
{
    static const std::vector<const char*> supportedExtensions = {
        extensions::SURFACE,
        extensions::DEBUG
    };
    return supportedExtensions;
}

} // namespace gfx::backend::webgpu::core