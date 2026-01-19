#pragma once

#include <gfx_cpp/gfx.hpp>

#include <gfx/gfx.h>

#include <memory>

namespace gfx {

class CAdapterImpl : public Adapter {
public:
    explicit CAdapterImpl(GfxAdapter h);
    ~CAdapterImpl() override;

    std::shared_ptr<Device> createDevice(const DeviceDescriptor& descriptor = {}) override;

    AdapterInfo getInfo() const override;

    DeviceLimits getLimits() const override;

private:
    GfxAdapter m_handle;
};

} // namespace gfx
