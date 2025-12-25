#pragma once

#include <gfx/gfx.h>

#include "IBackend.h"

#include <mutex>
#include <unordered_map>

namespace gfx {

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
        std::scoped_lock lock(m_mutex);
        m_handles[nativeHandle] = { backend, nativeHandle };
        return nativeHandle;
    }

    const IBackend* getAPI(void* handle)
    {
        if (!handle) {
            return nullptr;
        }
        std::scoped_lock lock(m_mutex);
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
        std::scoped_lock lock(m_mutex);
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
        std::scoped_lock lock(m_mutex);
        m_handles.erase(handle);
    }

    std::mutex& getMutex() { return m_mutex; }
    const IBackend** getBackends() { return m_backends; }
    int* getRefCounts() { return m_refCounts; }

private:
    BackendManager()
    {
        for (int i = 0; i < 3; ++i) {
            m_backends[i] = nullptr;
            m_refCounts[i] = 0;
        }
    }

    ~BackendManager() = default;

    const IBackend* m_backends[3];
    int m_refCounts[3];
    std::mutex m_mutex;
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
