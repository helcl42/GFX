#include "../sync/Fence.h"

namespace gfx::backend::webgpu::core {

Fence::Fence(bool signaled)
    : m_signaled(signaled)
{
}

bool Fence::isSignaled() const
{
    return m_signaled;
}

void Fence::signal()
{
    m_signaled = true;
}

void Fence::reset()
{
    m_signaled = false;
}

bool Fence::wait(uint64_t timeoutNs)
{
    // WebGPU fence is mock implementation - always immediately satisfied
    (void)timeoutNs;
    return m_signaled;
}

void Fence::setSignaled(bool signaled)
{
    m_signaled = signaled;
}

} // namespace gfx::backend::webgpu::core