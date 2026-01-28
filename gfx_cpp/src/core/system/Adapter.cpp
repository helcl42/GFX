#include "Adapter.h"

#include "Device.h"

#include "../presentation/Surface.h"

#include "../../converter/Conversions.h"

#include <stdexcept>

namespace gfx {

AdapterImpl::AdapterImpl(GfxAdapter h)
    : m_handle(h)
{
}

AdapterImpl::~AdapterImpl()
{
}

std::shared_ptr<Device> AdapterImpl::createDevice(const DeviceDescriptor& descriptor)
{
    std::vector<const char*> cExtensions;
    std::vector<GfxQueueRequest> cQueueRequests;
    GfxDeviceDescriptor cDesc = {};
    convertDeviceDescriptor(descriptor, cExtensions, cQueueRequests, cDesc);

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
    return cAdapterInfoToCppAdapterInfo(cInfo);
}

DeviceLimits AdapterImpl::getLimits() const
{
    GfxDeviceLimits cLimits = {};
    gfxAdapterGetLimits(m_handle, &cLimits);
    return cDeviceLimitsToCppDeviceLimits(cLimits);
}

std::vector<QueueFamilyProperties> AdapterImpl::enumerateQueueFamilies() const
{
    uint32_t count = 0;
    gfxAdapterEnumerateQueueFamilies(m_handle, &count, nullptr);

    std::vector<GfxQueueFamilyProperties> cProps(count);
    gfxAdapterEnumerateQueueFamilies(m_handle, &count, cProps.data());

    std::vector<QueueFamilyProperties> props;
    props.reserve(count);
    for (const auto& cProp : cProps) {
        props.push_back(cQueueFamilyPropertiesToCppQueueFamilyProperties(cProp));
    }

    return props;
}

bool AdapterImpl::getQueueFamilySurfaceSupport(uint32_t queueFamilyIndex, Surface* surface) const
{
    if (!surface) {
        return false;
    }

    auto* surfaceImpl = dynamic_cast<SurfaceImpl*>(surface);
    if (!surfaceImpl) {
        return false;
    }

    bool supported = false;
    gfxAdapterGetQueueFamilySurfaceSupport(m_handle, queueFamilyIndex, surfaceImpl->getHandle(), &supported);
    return supported;
}

std::vector<std::string> AdapterImpl::enumerateExtensions() const
{
    uint32_t count = 0;
    gfxAdapterEnumerateExtensions(m_handle, &count, nullptr);

    std::vector<const char*> extensionNames(count);
    gfxAdapterEnumerateExtensions(m_handle, &count, extensionNames.data());

    return cStringArrayToCppStringVector(extensionNames.data(), count);
}

} // namespace gfx
