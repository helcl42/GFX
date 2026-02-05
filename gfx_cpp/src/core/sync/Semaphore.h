#ifndef GFX_CPP_SEMAPHORE_H
#define GFX_CPP_SEMAPHORE_H

#include <gfx_cpp/gfx.hpp>

#include <gfx/gfx.h>

#include <memory>

namespace gfx {

class SemaphoreImpl : public Semaphore {
public:
    explicit SemaphoreImpl(GfxSemaphore h);
    ~SemaphoreImpl() override;

    GfxSemaphore getHandle() const;

    SemaphoreType getType() const override;
    uint64_t getValue() const override;
    void signal(uint64_t value) override;
    Result wait(uint64_t value, uint64_t timeoutNanoseconds = UINT64_MAX) override;

private:
    GfxSemaphore m_handle;
};

} // namespace gfx

#endif // GFX_CPP_SEMAPHORE_H
