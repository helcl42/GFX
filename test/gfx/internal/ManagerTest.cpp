#include <gfx/gfx.h>

// Include internal headers for testing
#include <backend/Factory.h>
#include <backend/IBackend.h>
#include <backend/Manager.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using ::testing::Return;

namespace gfx::backend::test {

// Minimal mock backend - only needs to exist, doesn't need functional methods for Manager tests
class MinimalMockBackend : public IBackend {
public:
    // Just provide stub implementations - Manager tests don't actually call these
    GfxResult createInstance(const GfxInstanceDescriptor*, GfxInstance*) const override { return GFX_RESULT_SUCCESS; }
    GfxResult instanceDestroy(GfxInstance) const override { return GFX_RESULT_SUCCESS; }
    GfxResult instanceRequestAdapter(GfxInstance, const GfxAdapterDescriptor*, GfxAdapter*) const override { return GFX_RESULT_SUCCESS; }
    GfxResult instanceEnumerateAdapters(GfxInstance, uint32_t*, GfxAdapter*) const override { return GFX_RESULT_SUCCESS; }
    GfxResult enumerateInstanceExtensions(uint32_t*, const char**) const override { return GFX_RESULT_SUCCESS; }
    GfxResult adapterCreateDevice(GfxAdapter, const GfxDeviceDescriptor*, GfxDevice*) const override { return GFX_RESULT_SUCCESS; }
    GfxResult adapterGetInfo(GfxAdapter, GfxAdapterInfo*) const override { return GFX_RESULT_SUCCESS; }
    GfxResult adapterGetLimits(GfxAdapter, GfxDeviceLimits*) const override { return GFX_RESULT_SUCCESS; }
    GfxResult adapterEnumerateQueueFamilies(GfxAdapter, uint32_t*, GfxQueueFamilyProperties*) const override { return GFX_RESULT_SUCCESS; }
    GfxResult adapterGetQueueFamilySurfaceSupport(GfxAdapter, uint32_t, GfxSurface, bool*) const override { return GFX_RESULT_SUCCESS; }
    GfxResult adapterEnumerateExtensions(GfxAdapter, uint32_t*, const char**) const override { return GFX_RESULT_SUCCESS; }
    GfxResult deviceDestroy(GfxDevice) const override { return GFX_RESULT_SUCCESS; }
    GfxResult deviceGetQueue(GfxDevice, GfxQueue*) const override { return GFX_RESULT_SUCCESS; }
    GfxResult deviceGetQueueByIndex(GfxDevice, uint32_t, uint32_t, GfxQueue*) const override { return GFX_RESULT_SUCCESS; }
    GfxResult deviceCreateSurface(GfxDevice, const GfxSurfaceDescriptor*, GfxSurface*) const override { return GFX_RESULT_SUCCESS; }
    GfxResult deviceCreateSwapchain(GfxDevice, GfxSurface, const GfxSwapchainDescriptor*, GfxSwapchain*) const override { return GFX_RESULT_SUCCESS; }
    GfxResult deviceCreateBuffer(GfxDevice, const GfxBufferDescriptor*, GfxBuffer*) const override { return GFX_RESULT_SUCCESS; }
    GfxResult deviceImportBuffer(GfxDevice, const GfxBufferImportDescriptor*, GfxBuffer*) const override { return GFX_RESULT_SUCCESS; }
    GfxResult deviceCreateTexture(GfxDevice, const GfxTextureDescriptor*, GfxTexture*) const override { return GFX_RESULT_SUCCESS; }
    GfxResult deviceImportTexture(GfxDevice, const GfxTextureImportDescriptor*, GfxTexture*) const override { return GFX_RESULT_SUCCESS; }
    GfxResult deviceCreateSampler(GfxDevice, const GfxSamplerDescriptor*, GfxSampler*) const override { return GFX_RESULT_SUCCESS; }
    GfxResult deviceCreateShader(GfxDevice, const GfxShaderDescriptor*, GfxShader*) const override { return GFX_RESULT_SUCCESS; }
    GfxResult deviceCreateBindGroupLayout(GfxDevice, const GfxBindGroupLayoutDescriptor*, GfxBindGroupLayout*) const override { return GFX_RESULT_SUCCESS; }
    GfxResult deviceCreateBindGroup(GfxDevice, const GfxBindGroupDescriptor*, GfxBindGroup*) const override { return GFX_RESULT_SUCCESS; }
    GfxResult deviceCreateRenderPipeline(GfxDevice, const GfxRenderPipelineDescriptor*, GfxRenderPipeline*) const override { return GFX_RESULT_SUCCESS; }
    GfxResult deviceCreateComputePipeline(GfxDevice, const GfxComputePipelineDescriptor*, GfxComputePipeline*) const override { return GFX_RESULT_SUCCESS; }
    GfxResult deviceCreateCommandEncoder(GfxDevice, const GfxCommandEncoderDescriptor*, GfxCommandEncoder*) const override { return GFX_RESULT_SUCCESS; }
    GfxResult deviceCreateRenderPass(GfxDevice, const GfxRenderPassDescriptor*, GfxRenderPass*) const override { return GFX_RESULT_SUCCESS; }
    GfxResult deviceCreateFramebuffer(GfxDevice, const GfxFramebufferDescriptor*, GfxFramebuffer*) const override { return GFX_RESULT_SUCCESS; }
    GfxResult deviceCreateFence(GfxDevice, const GfxFenceDescriptor*, GfxFence*) const override { return GFX_RESULT_SUCCESS; }
    GfxResult deviceCreateSemaphore(GfxDevice, const GfxSemaphoreDescriptor*, GfxSemaphore*) const override { return GFX_RESULT_SUCCESS; }
    GfxResult deviceCreateQuerySet(GfxDevice, const GfxQuerySetDescriptor*, GfxQuerySet*) const override { return GFX_RESULT_SUCCESS; }
    GfxResult deviceWaitIdle(GfxDevice) const override { return GFX_RESULT_SUCCESS; }
    GfxResult deviceGetLimits(GfxDevice, GfxDeviceLimits*) const override { return GFX_RESULT_SUCCESS; }
    GfxResult deviceSupportsShaderFormat(GfxDevice, GfxShaderSourceType, bool*) const override { return GFX_RESULT_SUCCESS; }
    GfxResult surfaceDestroy(GfxSurface) const override { return GFX_RESULT_SUCCESS; }
    GfxResult surfaceEnumerateSupportedFormats(GfxSurface, uint32_t*, GfxTextureFormat*) const override { return GFX_RESULT_SUCCESS; }
    GfxResult surfaceEnumerateSupportedPresentModes(GfxSurface, uint32_t*, GfxPresentMode*) const override { return GFX_RESULT_SUCCESS; }
    GfxResult swapchainDestroy(GfxSwapchain) const override { return GFX_RESULT_SUCCESS; }
    GfxResult swapchainGetInfo(GfxSwapchain, GfxSwapchainInfo*) const override { return GFX_RESULT_SUCCESS; }
    GfxResult swapchainAcquireNextImage(GfxSwapchain, uint64_t, GfxSemaphore, GfxFence, uint32_t*) const override { return GFX_RESULT_SUCCESS; }
    GfxResult swapchainGetTextureView(GfxSwapchain, uint32_t, GfxTextureView*) const override { return GFX_RESULT_SUCCESS; }
    GfxResult swapchainGetCurrentTextureView(GfxSwapchain, GfxTextureView*) const override { return GFX_RESULT_SUCCESS; }
    GfxResult swapchainPresent(GfxSwapchain, const GfxPresentInfo*) const override { return GFX_RESULT_SUCCESS; }
    GfxResult bufferDestroy(GfxBuffer) const override { return GFX_RESULT_SUCCESS; }
    GfxResult bufferGetInfo(GfxBuffer, GfxBufferInfo*) const override { return GFX_RESULT_SUCCESS; }
    GfxResult bufferGetNativeHandle(GfxBuffer, void**) const override { return GFX_RESULT_SUCCESS; }
    GfxResult bufferMap(GfxBuffer, uint64_t, uint64_t, void**) const override { return GFX_RESULT_SUCCESS; }
    GfxResult bufferUnmap(GfxBuffer) const override { return GFX_RESULT_SUCCESS; }
    GfxResult bufferFlushMappedRange(GfxBuffer, uint64_t, uint64_t) const override { return GFX_RESULT_SUCCESS; }
    GfxResult bufferInvalidateMappedRange(GfxBuffer, uint64_t, uint64_t) const override { return GFX_RESULT_SUCCESS; }
    GfxResult textureDestroy(GfxTexture) const override { return GFX_RESULT_SUCCESS; }
    GfxResult textureGetInfo(GfxTexture, GfxTextureInfo*) const override { return GFX_RESULT_SUCCESS; }
    GfxResult textureGetNativeHandle(GfxTexture, void**) const override { return GFX_RESULT_SUCCESS; }
    GfxResult textureGetLayout(GfxTexture, GfxTextureLayout*) const override { return GFX_RESULT_SUCCESS; }
    GfxResult textureCreateView(GfxTexture, const GfxTextureViewDescriptor*, GfxTextureView*) const override { return GFX_RESULT_SUCCESS; }
    GfxResult textureViewDestroy(GfxTextureView) const override { return GFX_RESULT_SUCCESS; }
    GfxResult samplerDestroy(GfxSampler) const override { return GFX_RESULT_SUCCESS; }
    GfxResult shaderDestroy(GfxShader) const override { return GFX_RESULT_SUCCESS; }
    GfxResult bindGroupLayoutDestroy(GfxBindGroupLayout) const override { return GFX_RESULT_SUCCESS; }
    GfxResult bindGroupDestroy(GfxBindGroup) const override { return GFX_RESULT_SUCCESS; }
    GfxResult renderPipelineDestroy(GfxRenderPipeline) const override { return GFX_RESULT_SUCCESS; }
    GfxResult computePipelineDestroy(GfxComputePipeline) const override { return GFX_RESULT_SUCCESS; }
    GfxResult renderPassDestroy(GfxRenderPass) const override { return GFX_RESULT_SUCCESS; }
    GfxResult framebufferDestroy(GfxFramebuffer) const override { return GFX_RESULT_SUCCESS; }
    GfxResult querySetDestroy(GfxQuerySet) const override { return GFX_RESULT_SUCCESS; }
    GfxResult queueSubmit(GfxQueue, const GfxSubmitDescriptor*) const override { return GFX_RESULT_SUCCESS; }
    GfxResult queueWriteBuffer(GfxQueue, GfxBuffer, uint64_t, const void*, uint64_t) const override { return GFX_RESULT_SUCCESS; }
    GfxResult queueWriteTexture(GfxQueue, GfxTexture, const GfxOrigin3D*, uint32_t, const void*, uint64_t, const GfxExtent3D*, GfxTextureLayout) const override { return GFX_RESULT_SUCCESS; }
    GfxResult queueWaitIdle(GfxQueue) const override { return GFX_RESULT_SUCCESS; }
    GfxResult commandEncoderDestroy(GfxCommandEncoder) const override { return GFX_RESULT_SUCCESS; }
    GfxResult commandEncoderBeginRenderPass(GfxCommandEncoder, const GfxRenderPassBeginDescriptor*, GfxRenderPassEncoder*) const override { return GFX_RESULT_SUCCESS; }
    GfxResult commandEncoderBeginComputePass(GfxCommandEncoder, const GfxComputePassBeginDescriptor*, GfxComputePassEncoder*) const override { return GFX_RESULT_SUCCESS; }
    GfxResult commandEncoderCopyBufferToBuffer(GfxCommandEncoder, const GfxCopyBufferToBufferDescriptor*) const override { return GFX_RESULT_SUCCESS; }
    GfxResult commandEncoderCopyBufferToTexture(GfxCommandEncoder, const GfxCopyBufferToTextureDescriptor*) const override { return GFX_RESULT_SUCCESS; }
    GfxResult commandEncoderCopyTextureToBuffer(GfxCommandEncoder, const GfxCopyTextureToBufferDescriptor*) const override { return GFX_RESULT_SUCCESS; }
    GfxResult commandEncoderCopyTextureToTexture(GfxCommandEncoder, const GfxCopyTextureToTextureDescriptor*) const override { return GFX_RESULT_SUCCESS; }
    GfxResult commandEncoderBlitTextureToTexture(GfxCommandEncoder, const GfxBlitTextureToTextureDescriptor*) const override { return GFX_RESULT_SUCCESS; }
    GfxResult commandEncoderPipelineBarrier(GfxCommandEncoder, const GfxPipelineBarrierDescriptor*) const override { return GFX_RESULT_SUCCESS; }
    GfxResult commandEncoderGenerateMipmaps(GfxCommandEncoder, GfxTexture) const override { return GFX_RESULT_SUCCESS; }
    GfxResult commandEncoderGenerateMipmapsRange(GfxCommandEncoder, GfxTexture, uint32_t, uint32_t) const override { return GFX_RESULT_SUCCESS; }
    GfxResult commandEncoderWriteTimestamp(GfxCommandEncoder, GfxQuerySet, uint32_t) const override { return GFX_RESULT_SUCCESS; }
    GfxResult commandEncoderResolveQuerySet(GfxCommandEncoder, GfxQuerySet, uint32_t, uint32_t, GfxBuffer, uint64_t) const override { return GFX_RESULT_SUCCESS; }
    GfxResult commandEncoderEnd(GfxCommandEncoder) const override { return GFX_RESULT_SUCCESS; }
    GfxResult commandEncoderBegin(GfxCommandEncoder) const override { return GFX_RESULT_SUCCESS; }
    GfxResult renderPassEncoderSetPipeline(GfxRenderPassEncoder, GfxRenderPipeline) const override { return GFX_RESULT_SUCCESS; }
    GfxResult renderPassEncoderSetBindGroup(GfxRenderPassEncoder, uint32_t, GfxBindGroup, const uint32_t*, uint32_t) const override { return GFX_RESULT_SUCCESS; }
    GfxResult renderPassEncoderSetVertexBuffer(GfxRenderPassEncoder, uint32_t, GfxBuffer, uint64_t, uint64_t) const override { return GFX_RESULT_SUCCESS; }
    GfxResult renderPassEncoderSetIndexBuffer(GfxRenderPassEncoder, GfxBuffer, GfxIndexFormat, uint64_t, uint64_t) const override { return GFX_RESULT_SUCCESS; }
    GfxResult renderPassEncoderSetViewport(GfxRenderPassEncoder, const GfxViewport*) const override { return GFX_RESULT_SUCCESS; }
    GfxResult renderPassEncoderSetScissorRect(GfxRenderPassEncoder, const GfxScissorRect*) const override { return GFX_RESULT_SUCCESS; }
    GfxResult renderPassEncoderDraw(GfxRenderPassEncoder, uint32_t, uint32_t, uint32_t, uint32_t) const override { return GFX_RESULT_SUCCESS; }
    GfxResult renderPassEncoderDrawIndexed(GfxRenderPassEncoder, uint32_t, uint32_t, uint32_t, int32_t, uint32_t) const override { return GFX_RESULT_SUCCESS; }
    GfxResult renderPassEncoderDrawIndirect(GfxRenderPassEncoder, GfxBuffer, uint64_t) const override { return GFX_RESULT_SUCCESS; }
    GfxResult renderPassEncoderDrawIndexedIndirect(GfxRenderPassEncoder, GfxBuffer, uint64_t) const override { return GFX_RESULT_SUCCESS; }
    GfxResult renderPassEncoderEnd(GfxRenderPassEncoder) const override { return GFX_RESULT_SUCCESS; }
    GfxResult renderPassEncoderBeginOcclusionQuery(GfxRenderPassEncoder, GfxQuerySet, uint32_t) const override { return GFX_RESULT_SUCCESS; }
    GfxResult renderPassEncoderEndOcclusionQuery(GfxRenderPassEncoder) const override { return GFX_RESULT_SUCCESS; }
    GfxResult computePassEncoderSetPipeline(GfxComputePassEncoder, GfxComputePipeline) const override { return GFX_RESULT_SUCCESS; }
    GfxResult computePassEncoderSetBindGroup(GfxComputePassEncoder, uint32_t, GfxBindGroup, const uint32_t*, uint32_t) const override { return GFX_RESULT_SUCCESS; }
    GfxResult computePassEncoderDispatch(GfxComputePassEncoder, uint32_t, uint32_t, uint32_t) const override { return GFX_RESULT_SUCCESS; }
    GfxResult computePassEncoderDispatchIndirect(GfxComputePassEncoder, GfxBuffer, uint64_t) const override { return GFX_RESULT_SUCCESS; }
    GfxResult computePassEncoderEnd(GfxComputePassEncoder) const override { return GFX_RESULT_SUCCESS; }
    GfxResult fenceDestroy(GfxFence) const override { return GFX_RESULT_SUCCESS; }
    GfxResult fenceGetStatus(GfxFence, bool*) const override { return GFX_RESULT_SUCCESS; }
    GfxResult fenceWait(GfxFence, uint64_t) const override { return GFX_RESULT_SUCCESS; }
    GfxResult fenceReset(GfxFence) const override { return GFX_RESULT_SUCCESS; }
    GfxResult semaphoreDestroy(GfxSemaphore) const override { return GFX_RESULT_SUCCESS; }
    GfxResult semaphoreGetType(GfxSemaphore, GfxSemaphoreType*) const override { return GFX_RESULT_SUCCESS; }
    GfxResult semaphoreSignal(GfxSemaphore, uint64_t) const override { return GFX_RESULT_SUCCESS; }
    GfxResult semaphoreWait(GfxSemaphore, uint64_t, uint64_t) const override { return GFX_RESULT_SUCCESS; }
    GfxResult semaphoreGetValue(GfxSemaphore, uint64_t*) const override { return GFX_RESULT_SUCCESS; }
    GfxAccessFlags getAccessFlagsForLayout(GfxTextureLayout) const override { return 0; }
};

// Test fixture for BackendManager
class ManagerTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        // Get the singleton instance
        manager = &BackendManager::instance();
    }

    void TearDown() override
    {
        // Clean up any backends we loaded
        manager->unloadBackend(GFX_BACKEND_VULKAN);
        manager->unloadBackend(GFX_BACKEND_WEBGPU);
    }

    BackendManager* manager;
};

TEST_F(ManagerTest, SingletonInstance)
{
    auto& instance1 = BackendManager::instance();
    auto& instance2 = BackendManager::instance();
    ASSERT_EQ(&instance1, &instance2) << "BackendManager should be a singleton";
}

TEST_F(ManagerTest, LoadBackend)
{
    auto mockBackend = std::make_unique<MinimalMockBackend>();
    bool loaded = manager->loadBackend(GFX_BACKEND_VULKAN, std::move(mockBackend));

    ASSERT_TRUE(loaded) << "Loading backend should succeed";

    auto backend = manager->getBackend(GFX_BACKEND_VULKAN);
    ASSERT_NE(backend, nullptr) << "Loaded backend should be retrievable";
}

TEST_F(ManagerTest, LoadInvalidBackend)
{
    auto mockBackend = std::make_unique<MinimalMockBackend>();
    bool loaded = manager->loadBackend(GFX_BACKEND_AUTO, std::move(mockBackend));

    ASSERT_FALSE(loaded) << "Loading invalid backend type should fail";
}

TEST_F(ManagerTest, LoadOutOfRangeBackend)
{
    auto mockBackend = std::make_unique<MinimalMockBackend>();
    bool loaded = manager->loadBackend(static_cast<GfxBackend>(999), std::move(mockBackend));

    ASSERT_FALSE(loaded) << "Loading out of range backend should fail";
}

TEST_F(ManagerTest, UnloadBackend)
{
    auto mockBackend = std::make_unique<MinimalMockBackend>();
    manager->loadBackend(GFX_BACKEND_VULKAN, std::move(mockBackend));

    manager->unloadBackend(GFX_BACKEND_VULKAN);

    auto backend = manager->getBackend(GFX_BACKEND_VULKAN);
    ASSERT_EQ(backend, nullptr) << "Unloaded backend should not be retrievable";
}

TEST_F(ManagerTest, GetBackendNotLoaded)
{
    auto backend = manager->getBackend(GFX_BACKEND_WEBGPU);
    ASSERT_EQ(backend, nullptr) << "Getting unloaded backend should return nullptr";
}

TEST_F(ManagerTest, WrapHandle)
{
    auto mockBackend = std::make_unique<MinimalMockBackend>();
    manager->loadBackend(GFX_BACKEND_VULKAN, std::move(mockBackend));

    void* testHandle = reinterpret_cast<void*>(0x12345678);
    void* wrapped = manager->wrap(GFX_BACKEND_VULKAN, testHandle);

    ASSERT_EQ(wrapped, testHandle) << "Wrap should return the same handle";

    // Verify we can retrieve the backend by handle
    auto backend = manager->getBackend(testHandle);
    ASSERT_NE(backend, nullptr) << "Should be able to get backend by wrapped handle";
}

TEST_F(ManagerTest, WrapNullHandle)
{
    void* wrapped = manager->wrap<void*>(GFX_BACKEND_VULKAN, nullptr);
    ASSERT_EQ(wrapped, nullptr) << "Wrapping null handle should return nullptr";
}

TEST_F(ManagerTest, UnwrapHandle)
{
    auto mockBackend = std::make_unique<MinimalMockBackend>();
    manager->loadBackend(GFX_BACKEND_VULKAN, std::move(mockBackend));

    void* testHandle = reinterpret_cast<void*>(0x12345678);
    manager->wrap(GFX_BACKEND_VULKAN, testHandle);

    manager->unwrap(testHandle);

    auto backend = manager->getBackend(testHandle);
    ASSERT_EQ(backend, nullptr) << "Unwrapped handle should not be retrievable";
}

TEST_F(ManagerTest, GetBackendType)
{
    auto mockBackend = std::make_unique<MinimalMockBackend>();
    manager->loadBackend(GFX_BACKEND_VULKAN, std::move(mockBackend));

    void* testHandle = reinterpret_cast<void*>(0x12345678);
    manager->wrap(GFX_BACKEND_VULKAN, testHandle);

    GfxBackend backendType = manager->getBackendType(testHandle);
    ASSERT_EQ(backendType, GFX_BACKEND_VULKAN) << "Backend type should match wrapped type";
}

TEST_F(ManagerTest, GetBackendTypeNullHandle)
{
    GfxBackend backendType = manager->getBackendType(nullptr);
    ASSERT_EQ(backendType, GFX_BACKEND_AUTO) << "Null handle should return GFX_BACKEND_AUTO";
}

TEST_F(ManagerTest, GetBackendTypeUnwrappedHandle)
{
    void* testHandle = reinterpret_cast<void*>(0x99999999); // Use a different address that won't collide with other tests
    GfxBackend backendType = manager->getBackendType(testHandle);
    ASSERT_EQ(backendType, GFX_BACKEND_AUTO) << "Unwrapped handle should return GFX_BACKEND_AUTO";
}

TEST_F(ManagerTest, MultipleDifferentHandles)
{
    auto mockBackend1 = std::make_unique<MinimalMockBackend>();
    auto mockBackend2 = std::make_unique<MinimalMockBackend>();
    manager->loadBackend(GFX_BACKEND_VULKAN, std::move(mockBackend1));
    manager->loadBackend(GFX_BACKEND_WEBGPU, std::move(mockBackend2));

    void* handle1 = reinterpret_cast<void*>(0x1000);
    void* handle2 = reinterpret_cast<void*>(0x2000);

    manager->wrap(GFX_BACKEND_VULKAN, handle1);
    manager->wrap(GFX_BACKEND_WEBGPU, handle2);

    ASSERT_EQ(manager->getBackendType(handle1), GFX_BACKEND_VULKAN);
    ASSERT_EQ(manager->getBackendType(handle2), GFX_BACKEND_WEBGPU);
}

TEST_F(ManagerTest, LoadSameBackendTwice)
{
    auto mockBackend1 = std::make_unique<MinimalMockBackend>();
    auto mockBackend2 = std::make_unique<MinimalMockBackend>();

    bool loaded1 = manager->loadBackend(GFX_BACKEND_VULKAN, std::move(mockBackend1));
    ASSERT_TRUE(loaded1);

    // Second load should succeed but not replace the first backend
    bool loaded2 = manager->loadBackend(GFX_BACKEND_VULKAN, std::move(mockBackend2));
    ASSERT_TRUE(loaded2) << "Loading same backend twice should succeed";

    // Backend should still be available
    auto backend = manager->getBackend(GFX_BACKEND_VULKAN);
    ASSERT_NE(backend, nullptr);
}

} // namespace gfx::backend::test
