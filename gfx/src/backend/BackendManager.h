#pragma once

#include <gfx/gfx.h>

#include "IBackend.h"

#ifndef __EMSCRIPTEN__
#include <mutex>
#endif
#include <unordered_map>

namespace gfx {

// No-op lock for single-threaded environments (Emscripten/WebAssembly)
#ifdef __EMSCRIPTEN__
struct NoOpLock {
    template<typename T>
    NoOpLock(T&) {} // Accept any parameter but do nothing
};
#define SCOPED_LOCK(mutex) NoOpLock _lock(mutex)
#else
#define SCOPED_LOCK(mutex) std::scoped_lock _lock(mutex)
#endif

// Handle metadata stores backend info
struct HandleMeta {
    GfxBackend backend;
    void* nativeHandle;
};

// Singleton class to manage backend state
class BackendManager {
public:
    static BackendManager& getInstance()
    {
        static BackendManager instance;
        return instance;
    }

    // Delete copy and move constructors/assignments
    BackendManager(const BackendManager&) = delete;
    BackendManager& operator=(const BackendManager&) = delete;
    BackendManager(BackendManager&&) = delete;
    BackendManager& operator=(BackendManager&&) = delete;

    const IBackend* getBackendAPI(GfxBackend backend)
    {
        if (backend >= 0 && backend < GFX_BACKEND_AUTO) {
            return m_backends[backend];
        }
        return nullptr;
    }

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

    const IBackend* getAPI(void* handle)
    {
        if (!handle) {
            return nullptr;
        }
        SCOPED_LOCK(m_mutex);
        auto it = m_handles.find(handle);
        if (it == m_handles.end()) {
            return nullptr;
        }
        return getBackendAPI(it->second.backend);
    }

    GfxBackend getBackend(void* handle)
    {
        if (!handle) {
            return GFX_BACKEND_AUTO;
        }
        SCOPED_LOCK(m_mutex);
        auto it = m_handles.find(handle);
        if (it == m_handles.end()) {
            return GFX_BACKEND_AUTO;
        }
        return it->second.backend;
    }

    void unwrap(void* handle)
    {
        if (!handle) {
            return;
        }
        SCOPED_LOCK(m_mutex);
        m_handles.erase(handle);
    }

    // Backend loading/unloading with internal reference counting
    bool loadBackend(GfxBackend backend, const IBackend* backendImpl)
    {
        SCOPED_LOCK(m_mutex);
        return loadBackendInternal(backend, backendImpl);
    }

    void unloadBackend(GfxBackend backend)
    {
        SCOPED_LOCK(m_mutex);
        unloadBackendInternal(backend);
    }

private:
    // Internal methods for use when mutex is already locked
    bool loadBackendInternal(GfxBackend backend, const IBackend* backendImpl)
    {
        if (backend < 0 || backend >= GFX_BACKEND_AUTO) {
            return false;
        }
        
        if (!m_backends[backend]) {
            m_backends[backend] = backendImpl;
            m_refCounts[backend] = 0;
        }
        m_refCounts[backend]++;
        return true;
    }

    void unloadBackendInternal(GfxBackend backend)
    {
        if (backend < 0 || backend >= GFX_BACKEND_AUTO) {
            return;
        }
        
        if (m_backends[backend] && m_refCounts[backend] > 0) {
            --m_refCounts[backend];
            if (m_refCounts[backend] == 0) {
                m_backends[backend] = nullptr;
            }
        }
    }

    BackendManager()
    {
        for (int i = 0; i < GFX_BACKEND_AUTO; ++i) {
            m_backends[i] = nullptr;
            m_refCounts[i] = 0;
        }
    }

    ~BackendManager() = default;

    const IBackend* m_backends[GFX_BACKEND_AUTO];
    int m_refCounts[GFX_BACKEND_AUTO];
#ifndef __EMSCRIPTEN__
    std::mutex m_mutex;
#else
    int m_mutex;  // Dummy variable for NoOpLock template parameter
#endif
    std::unordered_map<void*, HandleMeta> m_handles;
};

// Convenience inline functions for backward compatibility
inline const IBackend* getBackendAPI(GfxBackend backend)
{
    return BackendManager::getInstance().getBackendAPI(backend);
}

template <typename T>
inline T wrap(GfxBackend backend, T nativeHandle)
{
    return BackendManager::getInstance().wrap(backend, nativeHandle);
}

inline const IBackend* getAPI(void* handle)
{
    return BackendManager::getInstance().getAPI(handle);
}

inline GfxBackend getBackend(void* handle)
{
    return BackendManager::getInstance().getBackend(handle);
}

// Native handle passthrough - template preserves type automatically
template <typename T>
inline T native(T handle)
{
    return handle;
}

inline void unwrap(void* handle)
{
    BackendManager::getInstance().unwrap(handle);
}

} // namespace gfx
