#include "Semaphore.h"

namespace gfx::backend::webgpu::core {

Semaphore::Semaphore(SemaphoreType type, uint64_t value)
    : m_type(type)
    , m_value(value)
{
}

SemaphoreType Semaphore::getType() const
{
    return m_type;
}

uint64_t Semaphore::getValue() const
{
    return m_value;
}

void Semaphore::signal(uint64_t value)
{
    if (m_type == SemaphoreType::Binary) {
        // Binary semaphores always signal to 1
        m_value = 1;
    } else {
        // Timeline semaphores: if value provided, set to that value; otherwise increment
        if (value > 0) {
            m_value = value;
        } else {
            m_value++;
        }
    }
}

bool Semaphore::wait(uint64_t value, uint64_t timeoutNs)
{
    // WebGPU doesn't support explicit semaphore waiting
    // For API consistency, check if the wait condition is satisfied
    (void)timeoutNs;

    if (m_type == SemaphoreType::Timeline) {
        return m_value >= value;
    } else {
        return m_value > 0;
    }
}

void Semaphore::setValue(uint64_t value)
{
    m_value = value;
}

} // namespace gfx::backend::webgpu::core