#pragma once

#include <gfx_cpp/gfx.hpp>

#include <gfx/gfx.h>

#include <memory>

namespace gfx {

class CSemaphoreImpl : public Semaphore {
public:
    explicit CSemaphoreImpl(GfxSemaphore h);
    ~CSemaphoreImpl() override;

    GfxSemaphore getHandle() const;

    SemaphoreType getType() const override;
    uint64_t getValue() const override;
    void signal(uint64_t value) override;
    bool wait(uint64_t value, uint64_t timeoutNanoseconds = UINT64_MAX) override;

private:
    GfxSemaphore m_handle;
};

} // namespace gfx
