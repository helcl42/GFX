#pragma once

#include <gfx_cpp/gfx.hpp>

#include <gfx/gfx.h>

#include <memory>

namespace gfx {

class QueueImpl : public Queue {
public:
    explicit QueueImpl(GfxQueue h);
    // Queue is owned by device, do not destroy
    ~QueueImpl() override = default;

    void submit(const SubmitDescriptor& submitInfo) override;
    void writeBuffer(std::shared_ptr<Buffer> buffer, uint64_t offset, const void* data, uint64_t size) override;
    void writeTexture(
        std::shared_ptr<Texture> texture, const Origin3D& origin, uint32_t mipLevel,
        const void* data, uint64_t dataSize, uint32_t bytesPerRow,
        const Extent3D& extent, TextureLayout finalLayout) override;
    void waitIdle() override;

    GfxQueue getHandle() const { return m_handle; }

private:
    GfxQueue m_handle;
};

} // namespace gfx
