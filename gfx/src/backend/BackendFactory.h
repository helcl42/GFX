#pragma once

#include <gfx/gfx.h>

namespace gfx {

class IBackend;

// Factory class for creating backend implementations
class BackendFactory {
public:
    static const IBackend* createBackend(GfxBackend backend);

private:
    BackendFactory() = delete;
    ~BackendFactory() = delete;
    BackendFactory(const BackendFactory&) = delete;
    BackendFactory& operator=(const BackendFactory&) = delete;
};

} // namespace gfx
