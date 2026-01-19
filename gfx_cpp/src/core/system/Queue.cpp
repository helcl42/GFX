#include "Queue.h"

#include "../command/CommandEncoder.h"
#include "../resource/Buffer.h"
#include "../resource/Texture.h"
#include "../sync/Fence.h"
#include "../sync/Semaphore.h"

#include "../../converter/Conversions.h"

#include <vector>

namespace gfx {

QueueImpl::QueueImpl(GfxQueue h)
    : m_handle(h)
{
}

void QueueImpl::submit(const SubmitDescriptor& submitInfo)
{
    // Convert C++ structures to C
    std::vector<GfxCommandEncoder> cEncoders;
    for (auto& encoder : submitInfo.commandEncoders) {
        auto impl = std::dynamic_pointer_cast<CommandEncoderImpl>(encoder);
        if (impl) {
            cEncoders.push_back(impl->getHandle());
        }
    }

    std::vector<GfxSemaphore> cWaitSems;
    for (auto& sem : submitInfo.waitSemaphores) {
        auto impl = std::dynamic_pointer_cast<SemaphoreImpl>(sem);
        if (impl) {
            cWaitSems.push_back(impl->getHandle());
        }
    }

    std::vector<GfxSemaphore> cSignalSems;
    for (auto& sem : submitInfo.signalSemaphores) {
        auto impl = std::dynamic_pointer_cast<SemaphoreImpl>(sem);
        if (impl) {
            cSignalSems.push_back(impl->getHandle());
        }
    }

    GfxSubmitDescriptor cInfo = {};
    cInfo.commandEncoders = cEncoders.data();
    cInfo.commandEncoderCount = static_cast<uint32_t>(cEncoders.size());
    cInfo.waitSemaphores = cWaitSems.data();
    cInfo.waitSemaphoreCount = static_cast<uint32_t>(cWaitSems.size());
    cInfo.signalSemaphores = cSignalSems.data();
    cInfo.signalSemaphoreCount = static_cast<uint32_t>(cSignalSems.size());

    if (submitInfo.signalFence) {
        auto fenceImpl = std::dynamic_pointer_cast<FenceImpl>(submitInfo.signalFence);
        if (fenceImpl) {
            cInfo.signalFence = fenceImpl->getHandle();
        }
    } else {
        cInfo.signalFence = nullptr;
    }

    gfxQueueSubmit(m_handle, &cInfo);
}

void QueueImpl::writeBuffer(std::shared_ptr<Buffer> buffer, uint64_t offset, const void* data, uint64_t size)
{
    auto impl = std::dynamic_pointer_cast<BufferImpl>(buffer);
    if (impl) {
        gfxQueueWriteBuffer(m_handle, impl->getHandle(), offset, data, size);
    }
}

void QueueImpl::writeTexture(
    std::shared_ptr<Texture> texture, const Origin3D& origin, uint32_t mipLevel,
    const void* data, uint64_t dataSize, uint32_t bytesPerRow,
    const Extent3D& extent, TextureLayout finalLayout)
{
    auto impl = std::dynamic_pointer_cast<TextureImpl>(texture);
    if (impl) {
        GfxOrigin3D cOrigin = { origin.x, origin.y, origin.z };
        GfxExtent3D cExtent = { extent.width, extent.height, extent.depth };
        gfxQueueWriteTexture(m_handle, impl->getHandle(), &cOrigin, mipLevel,
            data, dataSize, bytesPerRow, &cExtent, cppLayoutToCLayout(finalLayout));
    }
}

void QueueImpl::waitIdle()
{
    gfxQueueWaitIdle(m_handle);
}

} // namespace gfx
