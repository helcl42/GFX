#ifndef GFX_BACKEND_MANAGER_H
#define GFX_BACKEND_MANAGER_H

#include <gfx/gfx.h>

#include "IBackend.h"

#ifndef GFX_HAS_EMSCRIPTEN
#include <mutex>
#endif
#include <unordered_map>

namespace gfx::backend {

// No-op lock and mutex for single-threaded environments (Emscripten/WebAssembly)
#ifdef GFX_HAS_EMSCRIPTEN
struct NoOpLock {
    template <typename T>
    NoOpLock(T&) {} // Accept any parameter but do nothing
};
struct NoOpMutex {};
#define SCOPED_LOCK(mutex) NoOpLock _lock(mutex)
using Mutex = NoOpMutex;
#else
#define SCOPED_LOCK(mutex) std::scoped_lock _lock(mutex)
using Mutex = std::mutex;
#endif

// Handle metadata stores backend info
struct HandleMeta {
    GfxBackend backend;
    void* nativeHandle;
};

// Singleton class to manage backend state
class BackendManager {
public:
    static BackendManager& getInstance();

    // Delete copy and move constructors/assignments
    BackendManager(const BackendManager&) = delete;
    BackendManager& operator=(const BackendManager&) = delete;
    BackendManager(BackendManager&&) = delete;
    BackendManager& operator=(BackendManager&&) = delete;

    const IBackend* getBackendAPI(GfxBackend backend);

    template <typename T>
    T wrap(GfxBackend backend, T nativeHandle)
    {
        if (!nativeHandle) {
            return nullptr;
        }
        SCOPED_LOCK(m_mutex);
        m_handles[nativeHandle] = { backend, nativeHandle };
        return nativeHandle;
    }

    void unwrap(void* handle);
    const IBackend* getAPI(void* handle);
    GfxBackend getBackend(void* handle);

    // Backend loading/unloading with internal reference counting
    bool loadBackend(GfxBackend backend, const IBackend* backendImpl);
    void unloadBackend(GfxBackend backend);

private:
    BackendManager();
    ~BackendManager() = default;

    const IBackend* m_backends[GFX_BACKEND_AUTO];
    int m_refCounts[GFX_BACKEND_AUTO];
    Mutex m_mutex;
    std::unordered_map<void*, HandleMeta> m_handles;
};

} // namespace gfx::backend

#endif // GFX_BACKEND_MANAGER_H