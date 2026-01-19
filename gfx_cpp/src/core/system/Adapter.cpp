#include "Adapter.h"

#include "Device.h"

#include "../../converter/Conversions.h"

#include <stdexcept>

namespace gfx {

AdapterImpl::AdapterImpl(GfxAdapter h)
    : m_handle(h)
{
}

AdapterImpl::~AdapterImpl()
{
    if (m_handle) {
        gfxAdapterDestroy(m_handle);
    }
}

std::shared_ptr<Device> AdapterImpl::createDevice(const DeviceDescriptor& descriptor)
{
    // Convert enabled features
    std::vector<GfxDeviceFeatureType> cFeatures;
    cFeatures.reserve(descriptor.enabledFeatures.size());
    for (const auto& feature : descriptor.enabledFeatures) {
        cFeatures.push_back(cppDeviceFeatureTypeToCDeviceFeatureType(feature));
    }

    GfxDeviceDescriptor cDesc = {};
    cDesc.label = descriptor.label.c_str();
    cDesc.queuePriority = descriptor.queuePriority;
    cDesc.enabledFeatures = cFeatures.empty() ? nullptr : cFeatures.data();
    cDesc.enabledFeatureCount = static_cast<uint32_t>(cFeatures.size());

    GfxDevice device = nullptr;
    GfxResult result = gfxAdapterCreateDevice(m_handle, &cDesc, &device);
    if (result != GFX_RESULT_SUCCESS || !device) {
        throw std::runtime_error("Failed to create device");
    }
    return std::make_shared<DeviceImpl>(device);
}

AdapterInfo AdapterImpl::getInfo() const
{
    GfxAdapterInfo cInfo = {};
    gfxAdapterGetInfo(m_handle, &cInfo);

    AdapterInfo info;
    info.name = cInfo.name ? cInfo.name : "Unknown";
    info.driverDescription = cInfo.driverDescription ? cInfo.driverDescription : "";
    info.vendorID = cInfo.vendorID;
    info.deviceID = cInfo.deviceID;

    // Convert adapter type
    switch (cInfo.adapterType) {
    case GFX_ADAPTER_TYPE_DISCRETE_GPU:
        info.adapterType = AdapterType::DiscreteGPU;
        break;
    case GFX_ADAPTER_TYPE_INTEGRATED_GPU:
        info.adapterType = AdapterType::IntegratedGPU;
        break;
    case GFX_ADAPTER_TYPE_CPU:
        info.adapterType = AdapterType::CPU;
        break;
    default:
        info.adapterType = AdapterType::Unknown;
        break;
    }

    // Convert backend
    info.backend = cBackendToCppBackend(cInfo.backend);

    return info;
}

DeviceLimits AdapterImpl::getLimits() const
{
    GfxDeviceLimits cLimits = {};
    gfxAdapterGetLimits(m_handle, &cLimits);

    DeviceLimits limits;
    limits.minUniformBufferOffsetAlignment = cLimits.minUniformBufferOffsetAlignment;
    limits.minStorageBufferOffsetAlignment = cLimits.minStorageBufferOffsetAlignment;
    limits.maxUniformBufferBindingSize = cLimits.maxUniformBufferBindingSize;
    limits.maxStorageBufferBindingSize = cLimits.maxStorageBufferBindingSize;
    limits.maxBufferSize = cLimits.maxBufferSize;
    limits.maxTextureDimension1D = cLimits.maxTextureDimension1D;
    limits.maxTextureDimension2D = cLimits.maxTextureDimension2D;
    limits.maxTextureDimension3D = cLimits.maxTextureDimension3D;
    limits.maxTextureArrayLayers = cLimits.maxTextureArrayLayers;
    return limits;
}

} // namespace gfx
