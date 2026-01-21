#ifndef GFX_CPP_SWAPCHAIN_H
#define GFX_CPP_SWAPCHAIN_H

#include <gfx_cpp/gfx.hpp>

#include <gfx/gfx.h>

#include <memory>

namespace gfx {

class SwapchainImpl : public Swapchain {
public:
    explicit SwapchainImpl(GfxSwapchain h);
    ~SwapchainImpl() override;

    SwapchainInfo getInfo() const override;
    std::shared_ptr<TextureView> getCurrentTextureView() override;
    Result acquireNextImage(uint64_t timeout, std::shared_ptr<Semaphore> signalSemaphore, std::shared_ptr<Fence> signalFence, uint32_t* imageIndex) override;
    std::shared_ptr<TextureView> getTextureView(uint32_t index) override;
    Result present(const PresentInfo& info) override;

private:
    GfxSwapchain m_handle;
};

} // namespace gfx

#endif // GFX_CPP_SWAPCHAIN_H
