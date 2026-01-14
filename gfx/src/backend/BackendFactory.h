#ifndef GFX_BACKEND_FACTORY_H
#define GFX_BACKEND_FACTORY_H

#include <gfx/gfx.h>

namespace gfx::backend {

class IBackend;

// Factory class for creating backend implementations
class BackendFactory {
public:
    static const IBackend* create(GfxBackend backend);
};

} // namespace gfx::backend

#endif // GFX_BACKEND_FACTORY_H