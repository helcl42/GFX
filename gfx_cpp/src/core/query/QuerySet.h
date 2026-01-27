#ifndef GFX_CPP_QUERYSET_H
#define GFX_CPP_QUERYSET_H

#include <gfx_cpp/gfx.hpp>

#include <gfx/gfx.h>

#include <memory>

namespace gfx {

class QuerySetImpl : public QuerySet {
public:
    QuerySetImpl(GfxQuerySet h, QueryType type, uint32_t count);
    ~QuerySetImpl() override;

    GfxQuerySet getHandle() const;

    QueryType getType() const override;
    uint32_t getCount() const override;

private:
    GfxQuerySet m_handle;
    QueryType m_type;
    uint32_t m_count;
};

} // namespace gfx

#endif // GFX_CPP_QUERYSET_H
