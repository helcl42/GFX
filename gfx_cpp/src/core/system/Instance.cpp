#include "Instance.h"

#include "Adapter.h"

#include "../../converter/Conversions.h"

#include <stdexcept>

namespace gfx {

InstanceImpl::InstanceImpl(GfxInstance h)
    : m_handle(h)
{
}

InstanceImpl::~InstanceImpl()
{
    if (m_handle) {
        gfxInstanceDestroy(m_handle);
    }
}

std::shared_ptr<Adapter> InstanceImpl::requestAdapter(const AdapterDescriptor& descriptor)
{
    GfxAdapterDescriptor cDesc = {};
    convertAdapterDescriptor(descriptor, cDesc);

    GfxAdapter adapter = nullptr;
    GfxResult result = gfxInstanceRequestAdapter(m_handle, &cDesc, &adapter);
    if (result != GFX_RESULT_SUCCESS || !adapter) {
        throw std::runtime_error("Failed to request adapter");
    }
    return std::make_shared<AdapterImpl>(adapter);
}

std::vector<std::shared_ptr<Adapter>> InstanceImpl::enumerateAdapters()
{
    // First call: get count
    uint32_t count = 0;
    GfxResult result = gfxInstanceEnumerateAdapters(m_handle, &count, nullptr);
    if (result != GFX_RESULT_SUCCESS || count == 0) {
        return {};
    }

    // Second call: get adapters
    std::vector<GfxAdapter> adapters(count);
    result = gfxInstanceEnumerateAdapters(m_handle, &count, adapters.data());
    if (result != GFX_RESULT_SUCCESS) {
        return {};
    }

    std::vector<std::shared_ptr<Adapter>> resultVec;
    for (uint32_t i = 0; i < count; ++i) {
        resultVec.push_back(std::make_shared<AdapterImpl>(adapters[i]));
    }
    return resultVec;
}

} // namespace gfx
