#pragma once

#include <gfx_cpp/gfx.hpp>

#include <gfx/gfx.h>

#include <memory>

namespace gfx {

// Forward declarations
class CSemaphoreImpl;
class CFenceImpl;

// Helper template to extract native C handles from C++ wrapper objects
template <typename CHandle>
inline CHandle extractNativeHandle(std::shared_ptr<void> /*ptr*/)
{
    return nullptr;
}

// Template specializations (defined in .cpp after class definitions)
template <>
GfxSemaphore extractNativeHandle<GfxSemaphore>(std::shared_ptr<void> ptr);

template <>
GfxFence extractNativeHandle<GfxFence>(std::shared_ptr<void> ptr);

} // namespace gfx
