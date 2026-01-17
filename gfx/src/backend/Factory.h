#ifndef GFX_FACTORY_H
#define GFX_FACTORY_H

#include <gfx/gfx.h>
#include <memory>

namespace gfx::backend {

class IBackend;

// Factory class for creating backend implementations
class BackendFactory {
public:
    static std::unique_ptr<const IBackend> create(GfxBackend backend);
};

} // namespace gfx::backend

#endif // GFX_BACKEND_FACTORY_H