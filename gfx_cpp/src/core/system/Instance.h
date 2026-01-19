#pragma once

#include <gfx_cpp/gfx.hpp>

#include <gfx/gfx.h>

#include <memory>
#include <vector>

namespace gfx {

class CInstanceImpl : public Instance {
public:
    explicit CInstanceImpl(GfxInstance h);
    ~CInstanceImpl() override;

    std::shared_ptr<Adapter> requestAdapter(const AdapterDescriptor& descriptor = {}) override;

    std::vector<std::shared_ptr<Adapter>> enumerateAdapters() override;

private:
    GfxInstance m_handle;
};

} // namespace gfx
