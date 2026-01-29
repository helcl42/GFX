#include "Adapter.h"

#include "Instance.h"

#include <stdexcept>

namespace gfx::backend::webgpu::core {

Adapter::Adapter(WGPUAdapter adapter, Instance* instance)
    : m_adapter(adapter)
    , m_instance(instance)
    , m_info(createAdapterInfo())
{
}

Adapter::~Adapter()
{
    if (m_adapter) {
        wgpuAdapterRelease(m_adapter);
    }
}

WGPUAdapter Adapter::handle() const
{
    return m_adapter;
}

Instance* Adapter::getInstance() const
{
    return m_instance;
}

const AdapterInfo& Adapter::getInfo() const
{
    return m_info;
}

WGPULimits Adapter::getLimits() const
{
    WGPULimits limits = WGPU_LIMITS_INIT;
    WGPUStatus status = wgpuAdapterGetLimits(m_adapter, &limits);
    if (status != WGPUStatus_Success) {
        throw std::runtime_error("Failed to get adapter limits");
    }
    return limits;
}

std::vector<QueueFamilyProperties> Adapter::getQueueFamilyProperties() const
{
    // WebGPU has a single unified queue family with all capabilities
    QueueFamilyProperties props{};
    props.queueCount = 1;
    props.supportsGraphics = true;
    props.supportsCompute = true;
    props.supportsTransfer = true;
    return { props };
}

bool Adapter::supportsPresentation(uint32_t queueFamilyIndex) const
{
    // WebGPU's single queue family (index 0) always supports presentation
    return queueFamilyIndex == 0;
}

AdapterInfo Adapter::createAdapterInfo() const
{
    AdapterInfo adapterInfo{};

#ifdef GFX_HAS_EMSCRIPTEN
    // Emscripten's wgpuAdapterGetInfo has issues - provide reasonable defaults
    adapterInfo.name = "WebGPU Adapter";
    adapterInfo.driverDescription = "WebGPU";
    adapterInfo.vendorID = 0;
    adapterInfo.deviceID = 0;
    adapterInfo.adapterType = WGPUAdapterType_Unknown;
#else
    WGPUAdapterInfo info = WGPU_ADAPTER_INFO_INIT;
    wgpuAdapterGetInfo(m_adapter, &info);

    adapterInfo.name = info.device.data ? std::string(info.device.data, info.device.length) : "Unknown";
    adapterInfo.driverDescription = info.description.data ? std::string(info.description.data, info.description.length) : "";
    adapterInfo.vendorID = info.vendorID;
    adapterInfo.deviceID = info.deviceID;
    adapterInfo.adapterType = info.adapterType;

    wgpuAdapterInfoFreeMembers(info);
#endif
    return adapterInfo;
}

std::vector<const char*> Adapter::enumerateSupportedExtensions() const
{
    static const std::vector<const char*> supportedExtensions = {
        extensions::SWAPCHAIN,
        extensions::TIMELINE_SEMAPHORE,
        extensions::ANISOTROPIC_FILTERING
    };
    return supportedExtensions;
}

} // namespace gfx::backend::webgpu::core