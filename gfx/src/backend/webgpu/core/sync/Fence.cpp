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

void Fence::setSignaled(bool signaled)
{
    m_signaled = signaled;
}

} // namespace gfx::backend::webgpu::core