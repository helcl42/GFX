#include "QuerySet.h"

namespace gfx {

QuerySetImpl::QuerySetImpl(GfxQuerySet h, QueryType type, uint32_t count)
    : m_handle(h)
    , m_type(type)
    , m_count(count)
{
}

QuerySetImpl::~QuerySetImpl()
{
    if (m_handle) {
        gfxQuerySetDestroy(m_handle);
    }
}

GfxQuerySet QuerySetImpl::getHandle() const
{
    return m_handle;
}

QueryType QuerySetImpl::getType() const
{
    return m_type;
}

uint32_t QuerySetImpl::getCount() const
{
    return m_count;
}

} // namespace gfx
