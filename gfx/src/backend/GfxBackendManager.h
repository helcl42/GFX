#pragma once

#include <gfx/gfx.h>

#include "GfxBackend.h"

#include <mutex>
#include <unordered_map>

namespace gfx {

// Handle metadata stores backend info
struct HandleMeta {
    GfxBackend m_backend;
    void* m_nativeHandle;
};

// Singleton class to manage backend state
class GfxBackendManager {
public:
    static GfxBackendManager& getInstance()
    {
        static GfxBackendManager instance;
        return instance;
    }

    // Delete copy and move constructors/assignments
    GfxBackendManager(const GfxBackendManager&) = delete;
    GfxBackendManager& operator=(const GfxBackendManager&) = delete;
    GfxBackendManager(GfxBackendManager&&) = delete;
    GfxBackendManager& operator=(GfxBackendManager&&) = delete;

    const GfxBackendAPI* getBackendAPI(GfxBackend backend)
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

    const GfxBackendAPI* getAPI(void* handle)
    {
        if (!handle) {
            return nullptr;
        }
        std::scoped_lock lock(m_mutex);
        auto it = m_handles.find(handle);
        if (it == m_handles.end()) {
            return nullptr;
        }
        return getBackendAPI(it->second.m_backend);
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
        return it->second.m_backend;
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
    const GfxBackendAPI** getBackends() { return m_backends; }
    int* getRefCounts() { return m_refCounts; }

private:
    GfxBackendManager()
    {
        for (int i = 0; i < 3; ++i) {
            m_backends[i] = nullptr;
            m_refCounts[i] = 0;
        }
    }

    ~GfxBackendManager() = default;

    const GfxBackendAPI* m_backends[3];
    int m_refCounts[3];
    std::mutex m_mutex;
    std::unordered_map<void*, HandleMeta> m_handles;
};

// Convenience inline functions for backward compatibility
inline const GfxBackendAPI* getBackendAPI(GfxBackend backend)
{
    return GfxBackendManager::getInstance().getBackendAPI(backend);
}

template <typename T>
inline T wrap(GfxBackend backend, T nativeHandle)
{
    return GfxBackendManager::getInstance().wrap(backend, nativeHandle);
}

inline const GfxBackendAPI* getAPI(void* handle)
{
    return GfxBackendManager::getInstance().getAPI(handle);
}

inline GfxBackend getBackend(void* handle)
{
    return GfxBackendManager::getInstance().getBackend(handle);
}

// Native handle passthrough - template preserves type automatically
template <typename T>
inline T native(T handle)
{
    return handle;
}

inline void unwrap(void* handle)
{
    GfxBackendManager::getInstance().unwrap(handle);
}

} // namespace gfx
