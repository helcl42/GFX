#include "Manager.h"

namespace gfx::backend {

BackendManager& BackendManager::getInstance()
{
    static BackendManager instance;
    return instance;
}

const IBackend* BackendManager::getBackendAPI(GfxBackend backend)
{
    if (backend >= 0 && backend < GFX_BACKEND_AUTO) {
        return m_backends[backend];
    }
    return nullptr;
}

const IBackend* BackendManager::getAPI(void* handle)
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

GfxBackend BackendManager::getBackend(void* handle)
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

void BackendManager::unwrap(void* handle)
{
    if (!handle) {
        return;
    }
    SCOPED_LOCK(m_mutex);
    m_handles.erase(handle);
}

bool BackendManager::loadBackend(GfxBackend backend, const IBackend* backendImpl)
{
    if (backend < 0 || backend >= GFX_BACKEND_AUTO) {
        return false;
    }

    SCOPED_LOCK(m_mutex);
    if (!m_backends[backend]) {
        m_backends[backend] = backendImpl;
        m_refCounts[backend] = 0;
    }
    ++m_refCounts[backend];
    return true;
}

void BackendManager::unloadBackend(GfxBackend backend)
{
    if (backend < 0 || backend >= GFX_BACKEND_AUTO) {
        return;
    }

    SCOPED_LOCK(m_mutex);
    if (m_backends[backend] && m_refCounts[backend] > 0) {
        --m_refCounts[backend];
        if (m_refCounts[backend] == 0) {
            m_backends[backend] = nullptr;
        }
    }
}

BackendManager::BackendManager()
{
    for (int i = 0; i < GFX_BACKEND_AUTO; ++i) {
        m_backends[i] = nullptr;
        m_refCounts[i] = 0;
    }
}

} // namespace gfx::backend
