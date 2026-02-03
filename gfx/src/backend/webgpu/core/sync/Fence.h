#ifndef GFX_WEBGPU_FENCE_H
#define GFX_WEBGPU_FENCE_H

#include "../CoreTypes.h"

namespace gfx::backend::webgpu::core {

class Fence {
public:
    // Prevent copying
    Fence(const Fence&) = delete;
    Fence& operator=(const Fence&) = delete;

    Fence(bool signaled);
    ~Fence() = default;

    bool isSignaled() const;
    void signal();
    void reset();
    bool wait(uint64_t timeoutNs);

private:
    void setSignaled(bool signaled);
    bool m_signaled = false;
};

} // namespace gfx::backend::webgpu::core

#endif // GFX_WEBGPU_FENCE_H