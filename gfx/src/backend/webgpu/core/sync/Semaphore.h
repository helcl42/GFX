#ifndef GFX_WEBGPU_SEMAPHORE_H
#define GFX_WEBGPU_SEMAPHORE_H

#include "../CoreTypes.h"

namespace gfx::backend::webgpu::core {

class Semaphore {
public:
    // Prevent copying
    Semaphore(const Semaphore&) = delete;
    Semaphore& operator=(const Semaphore&) = delete;

    Semaphore(SemaphoreType type, uint64_t value);
    ~Semaphore() = default;

    SemaphoreType getType() const;
    uint64_t getValue() const;
    void signal(uint64_t value = 0);
    bool wait(uint64_t value = 0, uint64_t timeoutNs = UINT64_MAX);

private:
    void setValue(uint64_t value);

private:
    SemaphoreType m_type = SemaphoreType::Binary;
    uint64_t m_value = 0;
};

} // namespace gfx::backend::webgpu::core

#endif // GFX_WEBGPU_SEMAPHORE_H