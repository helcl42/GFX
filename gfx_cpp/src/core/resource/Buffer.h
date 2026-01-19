#pragma once

#include <gfx_cpp/gfx.hpp>

#include <gfx/gfx.h>

namespace gfx {

class CBufferImpl : public Buffer {
public:
    explicit CBufferImpl(GfxBuffer h);
    ~CBufferImpl() override;

    GfxBuffer getHandle() const;

    BufferInfo getInfo() const override;
    void* map(uint64_t offset = 0, uint64_t size = 0) override;
    void unmap() override;

private:
    GfxBuffer m_handle;
    GfxBufferInfo m_info;
};

} // namespace gfx
