#ifndef GFX_CPP_ADAPTER_H
#define GFX_CPP_ADAPTER_H

#include <gfx_cpp/gfx.hpp>

#include <gfx/gfx.h>

#include <memory>

namespace gfx {

class AdapterImpl : public Adapter {
public:
    explicit AdapterImpl(GfxAdapter h);
    ~AdapterImpl() override;

    std::shared_ptr<Device> createDevice(const DeviceDescriptor& descriptor = {}) override;

    AdapterInfo getInfo() const override;

    DeviceLimits getLimits() const override;

    std::vector<QueueFamilyProperties> enumerateQueueFamilies() const override;
    bool getQueueFamilySurfaceSupport(uint32_t queueFamilyIndex, Surface* surface) const override;

    std::vector<std::string> enumerateExtensions() const override;

private:
    GfxAdapter m_handle;
};

} // namespace gfx

#endif // GFX_CPP_ADAPTER_H
