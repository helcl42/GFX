#ifndef GFX_CPP_BUFFER_H
#define GFX_CPP_BUFFER_H

#include <gfx_cpp/gfx.hpp>

#include <gfx/gfx.h>

namespace gfx {

class BufferImpl : public Buffer {
public:
    explicit BufferImpl(GfxBuffer h);
    ~BufferImpl() override;

    GfxBuffer getHandle() const;

    BufferInfo getInfo() const override;
    void* getNativeHandle() const override;
    void* map(uint64_t offset = 0, uint64_t size = 0) override;
    void unmap() override;
    void flushMappedRange(uint64_t offset, uint64_t size) override;
    void invalidateMappedRange(uint64_t offset, uint64_t size) override;

private:
    GfxBuffer m_handle;
    GfxBufferInfo m_info;
};

} // namespace gfx

#endif // GFX_CPP_BUFFER_H
