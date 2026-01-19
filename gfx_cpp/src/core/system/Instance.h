#ifndef GFX_CPP_INSTANCE_H
#define GFX_CPP_INSTANCE_H

#include <gfx_cpp/gfx.hpp>

#include <gfx/gfx.h>

#include <memory>
#include <vector>

namespace gfx {

class InstanceImpl : public Instance {
public:
    explicit InstanceImpl(GfxInstance h);
    ~InstanceImpl() override;

    std::shared_ptr<Adapter> requestAdapter(const AdapterDescriptor& descriptor = {}) override;

    std::vector<std::shared_ptr<Adapter>> enumerateAdapters() override;

private:
    GfxInstance m_handle;
};

} // namespace gfx

#endif // GFX_CPP_INSTANCE_H
