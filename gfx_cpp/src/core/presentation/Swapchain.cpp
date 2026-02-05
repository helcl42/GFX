#include "Swapchain.h"

#include "../resource/TextureView.h"
#include "../util/HandleExtractor.h"

#include "../../converter/Conversions.h"

namespace gfx {

SwapchainImpl::SwapchainImpl(GfxSwapchain h)
    : m_handle(h)
{
}

SwapchainImpl::~SwapchainImpl()
{
    if (m_handle) {
        gfxSwapchainDestroy(m_handle);
    }
}

SwapchainInfo SwapchainImpl::getInfo() const
{
    GfxSwapchainInfo cInfo;
    gfxSwapchainGetInfo(m_handle, &cInfo);
    return cSwapchainInfoToCppSwapchainInfo(cInfo);
}

std::shared_ptr<TextureView> SwapchainImpl::getCurrentTextureView() const
{
    GfxTextureView view = nullptr;
    GfxResult result = gfxSwapchainGetCurrentTextureView(m_handle, &view);
    if (result != GFX_RESULT_SUCCESS || !view) {
        return nullptr;
    }
    return std::make_shared<TextureViewImpl>(view);
}

Result SwapchainImpl::acquireNextImage(uint64_t timeout, std::shared_ptr<Semaphore> signalSemaphore, std::shared_ptr<Fence> signalFence, uint32_t* imageIndex)
{
    GfxSemaphore cSemaphore = signalSemaphore ? extractNativeHandle<GfxSemaphore>(signalSemaphore) : nullptr;
    GfxFence cFence = signalFence ? extractNativeHandle<GfxFence>(signalFence) : nullptr;

    GfxResult result = gfxSwapchainAcquireNextImage(m_handle, timeout, cSemaphore, cFence, imageIndex);
    return cResultToCppResult(result);
}

std::shared_ptr<TextureView> SwapchainImpl::getTextureView(uint32_t index) const
{
    GfxTextureView view = nullptr;
    GfxResult result = gfxSwapchainGetTextureView(m_handle, index, &view);
    if (result != GFX_RESULT_SUCCESS || !view) {
        return nullptr;
    }
    return std::make_shared<TextureViewImpl>(view);
}

Result SwapchainImpl::present(const PresentDescriptor& descriptor)
{
    std::vector<GfxSemaphore> cWaitSemaphores;
    GfxPresentDescriptor cDescriptor;
    convertPresentDescriptor(descriptor, cWaitSemaphores, cDescriptor);

    GfxResult result = gfxSwapchainPresent(m_handle, &cDescriptor);
    return cResultToCppResult(result);
}

} // namespace gfx
