#pragma once

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

private:
    GfxAdapter m_handle;
};

} // namespace gfx
