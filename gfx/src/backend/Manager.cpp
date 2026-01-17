#include "Manager.h"

namespace gfx::backend {

BackendManager& BackendManager::instance()
{
    static BackendManager instance;
    return instance;
}

std::shared_ptr<const IBackend> BackendManager::getBackend(GfxBackend backend)
{
    if (backend >= 0 && backend < GFX_BACKEND_AUTO) {
        return m_backends[backend];
    }
    return nullptr;
}

std::shared_ptr<const IBackend> BackendManager::getBackend(void* handle)
{
    if (!handle) {
        return nullptr;
    }
    SCOPED_LOCK(m_mutex);
    auto it = m_handles.find(handle);
    if (it == m_handles.end()) {
        return nullptr;
    }
    return getBackend(it->second.backend);
}

GfxBackend BackendManager::getBackendType(void* handle)
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

bool BackendManager::loadBackend(GfxBackend backend, std::unique_ptr<const IBackend> backendImpl)
{
    if (backend < 0 || backend >= GFX_BACKEND_AUTO) {
        return false;
    }

    SCOPED_LOCK(m_mutex);
    if (!m_backends[backend]) {
        // Convert unique_ptr to shared_ptr for storage and reference counting
        m_backends[backend] = std::move(backendImpl);
    }
    return true;
}

void BackendManager::unloadBackend(GfxBackend backend)
{
    if (backend < 0 || backend >= GFX_BACKEND_AUTO) {
        return;
    }

    SCOPED_LOCK(m_mutex);
    // Simply reset the shared_ptr - it will automatically delete when ref count reaches 0
    m_backends[backend].reset();
}

BackendManager::BackendManager()
{
    // shared_ptr default constructs to nullptr, no explicit initialization needed
}

} // namespace gfx::backend
