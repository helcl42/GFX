#pragma once

#include <gfx/gfx.h>

namespace gfx {

class IBackend;

// Factory class for creating backend implementations
class BackendFactory {
public:
    static const IBackend* create(GfxBackend backend);
};

} // namespace gfx
