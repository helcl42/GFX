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

void Semaphore::setValue(uint64_t value)
{
    m_value = value;
}

} // namespace gfx::backend::webgpu::core