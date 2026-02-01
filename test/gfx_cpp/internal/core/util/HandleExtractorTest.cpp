#include <core/sync/Fence.h>
#include <core/sync/Semaphore.h>
#include <core/util/HandleExtractor.h>

#include <gfx/gfx.h>

#include <gfx_cpp/gfx.hpp>

#include <gtest/gtest.h>

namespace gfx {

// =============================================================================
// HandleExtractor Tests
// =============================================================================

TEST(HandleExtractorTest, ExtractNativeHandle_GenericReturnsNull)
{
    std::shared_ptr<void> ptr = std::make_shared<int>(42);
    auto handle = extractNativeHandle<GfxBuffer>(ptr);
    EXPECT_EQ(handle, nullptr);
}

TEST(HandleExtractorTest, ExtractNativeHandle_NullPtrReturnsNull)
{
    std::shared_ptr<void> ptr = nullptr;
    auto semHandle = extractNativeHandle<GfxSemaphore>(ptr);
    EXPECT_EQ(semHandle, nullptr);

    auto fenceHandle = extractNativeHandle<GfxFence>(ptr);
    EXPECT_EQ(fenceHandle, nullptr);
}

TEST(HandleExtractorTest, ExtractNativeHandle_SemaphoreReturnsCorrectHandle)
{
    // Create a fake semaphore handle
    GfxSemaphore fakeHandle = reinterpret_cast<GfxSemaphore>(0x12345678);
    auto semImpl = std::make_shared<SemaphoreImpl>(fakeHandle);
    std::shared_ptr<void> ptr = semImpl;

    auto extractedHandle = extractNativeHandle<GfxSemaphore>(ptr);
    EXPECT_EQ(extractedHandle, fakeHandle);
}

TEST(HandleExtractorTest, ExtractNativeHandle_FenceReturnsCorrectHandle)
{
    // Create a fake fence handle
    GfxFence fakeHandle = reinterpret_cast<GfxFence>(0x87654321);
    auto fenceImpl = std::make_shared<FenceImpl>(fakeHandle);
    std::shared_ptr<void> ptr = fenceImpl;

    auto extractedHandle = extractNativeHandle<GfxFence>(ptr);
    EXPECT_EQ(extractedHandle, fakeHandle);
}

TEST(HandleExtractorTest, ExtractNativeHandle_WrongTypeCast)
{
    // Create a semaphore but try to extract as fence - should handle gracefully
    GfxSemaphore fakeHandle = reinterpret_cast<GfxSemaphore>(0x12345678);
    auto semImpl = std::make_shared<SemaphoreImpl>(fakeHandle);
    std::shared_ptr<void> ptr = semImpl;

    // This should not crash, but will return nullptr or undefined behavior
    // depending on implementation. We just ensure it doesn't crash.
    auto extractedHandle = extractNativeHandle<GfxFence>(ptr);
    // Can't assert specific value as it's undefined behavior
    (void)extractedHandle;
}

} // namespace gfx
