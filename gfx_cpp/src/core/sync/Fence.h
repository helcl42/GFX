#pragma once

#include <gfx_cpp/gfx.hpp>

#include <gfx/gfx.h>

#include <memory>

namespace gfx {

class FenceImpl : public Fence {
public:
    explicit FenceImpl(GfxFence h);
    ~FenceImpl() override;

    GfxFence getHandle() const;

    FenceStatus getStatus() const override;
    bool wait(uint64_t timeoutNanoseconds = UINT64_MAX) override;
    void reset() override;

private:
    GfxFence m_handle;
};

} // namespace gfx
