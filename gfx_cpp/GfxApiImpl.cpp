#include "../gfx/GfxApi.h" // Include C API
#include "GfxApi.hpp"

#include <cstring>
#include <memory>
#include <stdexcept>

namespace gfx {

// ============================================================================
// Helper Functions - Convert between C++ and C types
// ============================================================================

static GfxBackend cppBackendToCBackend(Backend backend)
{
    switch (backend) {
    case Backend::Vulkan:
        return GFX_BACKEND_VULKAN;
    case Backend::WebGPU:
        return GFX_BACKEND_WEBGPU;
    case Backend::Auto:
        return GFX_BACKEND_AUTO;
    default:
        return GFX_BACKEND_AUTO;
    }
}

static Backend cBackendToCppBackend(GfxBackend backend)
{
    switch (backend) {
    case GFX_BACKEND_VULKAN:
        return Backend::Vulkan;
    case GFX_BACKEND_WEBGPU:
        return Backend::WebGPU;
    case GFX_BACKEND_AUTO:
        return Backend::Auto;
    default:
        return Backend::Auto;
    }
}

static GfxTextureFormat cppFormatToCFormat(TextureFormat format)
{
    return static_cast<GfxTextureFormat>(format);
}

static TextureFormat cFormatToCppFormat(GfxTextureFormat format)
{
    return static_cast<TextureFormat>(format);
}

static GfxBufferUsage cppBufferUsageToCUsage(BufferUsage usage)
{
    return static_cast<GfxBufferUsage>(static_cast<uint32_t>(usage));
}

static GfxTextureUsage cppTextureUsageToCUsage(TextureUsage usage)
{
    return static_cast<GfxTextureUsage>(static_cast<uint32_t>(usage));
}

static GfxPlatformWindowHandle cppHandleToCHandle(const PlatformWindowHandle& handle)
{
    GfxPlatformWindowHandle cHandle;
#ifdef _WIN32
    cHandle.hwnd = handle.hwnd;
    cHandle.hinstance = handle.hinstance;
#elif defined(__linux__)
    cHandle.window = handle.window;
    cHandle.display = handle.display;
    cHandle.isWayland = handle.isWayland;
#elif defined(__APPLE__)
    cHandle.nsWindow = handle.nsWindow;
    cHandle.metalLayer = handle.metalLayer;
#else
    cHandle.handle = handle.handle;
    cHandle.display = handle.display;
    cHandle.extra = handle.extra;
#endif
    return cHandle;
}

// ============================================================================
// C++ Wrapper Classes
// ============================================================================

class CBufferImpl : public Buffer {
private:
    GfxBuffer handle;

public:
    explicit CBufferImpl(GfxBuffer h)
        : handle(h)
    {
    }
    ~CBufferImpl() override
    {
        if (handle)
            gfxBufferDestroy(handle);
    }

    GfxBuffer getHandle() const { return handle; }

    uint64_t getSize() const override { return gfxBufferGetSize(handle); }
    BufferUsage getUsage() const override { return static_cast<BufferUsage>(gfxBufferGetUsage(handle)); }

    void* mapAsync(uint64_t offset = 0, uint64_t size = 0) override
    {
        void* mappedPointer = nullptr;
        GfxResult result = gfxBufferMapAsync(handle, offset, size, &mappedPointer);
        if (result != GFX_RESULT_SUCCESS) {
            return nullptr;
        }
        return mappedPointer;
    }

    void unmap() override
    {
        gfxBufferUnmap(handle);
    }
};

class CTextureViewImpl : public TextureView {
private:
    GfxTextureView handle;
    std::shared_ptr<Texture> texture;

public:
    explicit CTextureViewImpl(GfxTextureView h, std::shared_ptr<Texture> tex = nullptr)
        : handle(h)
        , texture(tex)
    {
    }
    ~CTextureViewImpl() override
    {
        if (handle)
            gfxTextureViewDestroy(handle);
    }

    GfxTextureView getHandle() const { return handle; }

    std::shared_ptr<Texture> getTexture() override { return texture; }
};

class CTextureImpl : public Texture, public std::enable_shared_from_this<CTextureImpl> {
private:
    GfxTexture handle;

public:
    explicit CTextureImpl(GfxTexture h)
        : handle(h)
    {
    }
    ~CTextureImpl() override
    {
        if (handle)
            gfxTextureDestroy(handle);
    }

    GfxTexture getHandle() const { return handle; }

    Extent3D getSize() const override
    {
        GfxExtent3D size = gfxTextureGetSize(handle);
        return Extent3D(size.width, size.height, size.depth);
    }

    TextureFormat getFormat() const override
    {
        return cFormatToCppFormat(gfxTextureGetFormat(handle));
    }

    uint32_t getMipLevelCount() const override { return gfxTextureGetMipLevelCount(handle); }
    uint32_t getSampleCount() const override { return gfxTextureGetSampleCount(handle); }
    TextureUsage getUsage() const override { return static_cast<TextureUsage>(gfxTextureGetUsage(handle)); }

    std::shared_ptr<TextureView> createView(const TextureViewDescriptor& descriptor = {}) override
    {
        GfxTextureViewDescriptor cDesc = {};
        cDesc.label = descriptor.label.c_str();
        cDesc.format = cppFormatToCFormat(descriptor.format);
        cDesc.baseMipLevel = descriptor.baseMipLevel;
        cDesc.mipLevelCount = descriptor.mipLevelCount;
        cDesc.baseArrayLayer = descriptor.baseArrayLayer;
        cDesc.arrayLayerCount = descriptor.arrayLayerCount;

        GfxTextureView view = nullptr;
        GfxResult result = gfxTextureCreateView(handle, &cDesc, &view);
        if (result != GFX_RESULT_SUCCESS || !view)
            throw std::runtime_error("Failed to create texture view");

        return std::make_shared<CTextureViewImpl>(view, shared_from_this());
    }
};

class CSamplerImpl : public Sampler {
private:
    GfxSampler handle;

public:
    explicit CSamplerImpl(GfxSampler h)
        : handle(h)
    {
    }
    ~CSamplerImpl() override
    {
        if (handle)
            gfxSamplerDestroy(handle);
    }

    GfxSampler getHandle() const { return handle; }
};

class CShaderImpl : public Shader {
private:
    GfxShader handle;

public:
    explicit CShaderImpl(GfxShader h)
        : handle(h)
    {
    }
    ~CShaderImpl() override
    {
        if (handle)
            gfxShaderDestroy(handle);
    }

    GfxShader getHandle() const { return handle; }
};

class CBindGroupLayoutImpl : public BindGroupLayout {
private:
    GfxBindGroupLayout handle;

public:
    explicit CBindGroupLayoutImpl(GfxBindGroupLayout h)
        : handle(h)
    {
    }
    ~CBindGroupLayoutImpl() override
    {
        if (handle)
            gfxBindGroupLayoutDestroy(handle);
    }

    GfxBindGroupLayout getHandle() const { return handle; }
};

class CBindGroupImpl : public BindGroup {
private:
    GfxBindGroup handle;

public:
    explicit CBindGroupImpl(GfxBindGroup h)
        : handle(h)
    {
    }
    ~CBindGroupImpl() override
    {
        if (handle)
            gfxBindGroupDestroy(handle);
    }

    GfxBindGroup getHandle() const { return handle; }
};

class CRenderPipelineImpl : public RenderPipeline {
private:
    GfxRenderPipeline handle;

public:
    explicit CRenderPipelineImpl(GfxRenderPipeline h)
        : handle(h)
    {
    }
    ~CRenderPipelineImpl() override
    {
        if (handle)
            gfxRenderPipelineDestroy(handle);
    }

    GfxRenderPipeline getHandle() const { return handle; }
};

class CComputePipelineImpl : public ComputePipeline {
private:
    GfxComputePipeline handle;

public:
    explicit CComputePipelineImpl(GfxComputePipeline h)
        : handle(h)
    {
    }
    ~CComputePipelineImpl() override
    {
        if (handle)
            gfxComputePipelineDestroy(handle);
    }

    GfxComputePipeline getHandle() const { return handle; }
};

class CRenderPassEncoderImpl : public RenderPassEncoder {
private:
    GfxRenderPassEncoder handle;

public:
    explicit CRenderPassEncoderImpl(GfxRenderPassEncoder h)
        : handle(h)
    {
    }
    ~CRenderPassEncoderImpl() override
    {
        if (handle)
            gfxRenderPassEncoderDestroy(handle);
    }

    void setPipeline(std::shared_ptr<RenderPipeline> pipeline) override
    {
        auto impl = std::dynamic_pointer_cast<CRenderPipelineImpl>(pipeline);
        if (impl)
            gfxRenderPassEncoderSetPipeline(handle, impl->getHandle());
    }

    void setBindGroup(uint32_t index, std::shared_ptr<BindGroup> bindGroup) override
    {
        auto impl = std::dynamic_pointer_cast<CBindGroupImpl>(bindGroup);
        if (impl)
            gfxRenderPassEncoderSetBindGroup(handle, index, impl->getHandle());
    }

    void setVertexBuffer(uint32_t slot, std::shared_ptr<Buffer> buffer, uint64_t offset = 0, uint64_t size = 0) override
    {
        auto impl = std::dynamic_pointer_cast<CBufferImpl>(buffer);
        if (impl)
            gfxRenderPassEncoderSetVertexBuffer(handle, slot, impl->getHandle(), offset, size);
    }

    void setIndexBuffer(std::shared_ptr<Buffer> buffer, IndexFormat format, uint64_t offset = 0, uint64_t size = 0) override
    {
        auto impl = std::dynamic_pointer_cast<CBufferImpl>(buffer);
        if (impl) {
            GfxIndexFormat cFormat = (format == IndexFormat::Uint16) ? GFX_INDEX_FORMAT_UINT16 : GFX_INDEX_FORMAT_UINT32;
            gfxRenderPassEncoderSetIndexBuffer(handle, impl->getHandle(), cFormat, offset, size);
        }
    }

    void draw(uint32_t vertexCount, uint32_t instanceCount = 1, uint32_t firstVertex = 0, uint32_t firstInstance = 0) override
    {
        gfxRenderPassEncoderDraw(handle, vertexCount, instanceCount, firstVertex, firstInstance);
    }

    void drawIndexed(uint32_t indexCount, uint32_t instanceCount = 1, uint32_t firstIndex = 0, int32_t baseVertex = 0, uint32_t firstInstance = 0) override
    {
        gfxRenderPassEncoderDrawIndexed(handle, indexCount, instanceCount, firstIndex, baseVertex, firstInstance);
    }

    void end() override
    {
        gfxRenderPassEncoderEnd(handle);
    }
};

class CComputePassEncoderImpl : public ComputePassEncoder {
private:
    GfxComputePassEncoder handle;

public:
    explicit CComputePassEncoderImpl(GfxComputePassEncoder h)
        : handle(h)
    {
    }
    ~CComputePassEncoderImpl() override
    {
        if (handle)
            gfxComputePassEncoderDestroy(handle);
    }

    void setPipeline(std::shared_ptr<ComputePipeline> pipeline) override
    {
        auto impl = std::dynamic_pointer_cast<CComputePipelineImpl>(pipeline);
        if (impl)
            gfxComputePassEncoderSetPipeline(handle, impl->getHandle());
    }

    void setBindGroup(uint32_t index, std::shared_ptr<BindGroup> bindGroup) override
    {
        auto impl = std::dynamic_pointer_cast<CBindGroupImpl>(bindGroup);
        if (impl)
            gfxComputePassEncoderSetBindGroup(handle, index, impl->getHandle());
    }

    void dispatchWorkgroups(uint32_t workgroupCountX, uint32_t workgroupCountY = 1, uint32_t workgroupCountZ = 1) override
    {
        gfxComputePassEncoderDispatchWorkgroups(handle, workgroupCountX, workgroupCountY, workgroupCountZ);
    }

    void end() override
    {
        gfxComputePassEncoderEnd(handle);
    }
};

class CCommandEncoderImpl : public CommandEncoder {
private:
    GfxCommandEncoder handle;

public:
    explicit CCommandEncoderImpl(GfxCommandEncoder h)
        : handle(h)
    {
    }
    ~CCommandEncoderImpl() override
    {
        if (handle)
            gfxCommandEncoderDestroy(handle);
    }

    GfxCommandEncoder getHandle() const { return handle; }

    std::shared_ptr<RenderPassEncoder> beginRenderPass(
        const std::vector<std::shared_ptr<TextureView>>& colorAttachments,
        const std::vector<Color>& clearColors = {},
        std::shared_ptr<TextureView> depthStencilAttachment = nullptr,
        float depthClearValue = 1.0f,
        uint32_t stencilClearValue = 0) override
    {
        std::vector<GfxTextureView> cColorAttachments;
        for (auto& view : colorAttachments) {
            auto impl = std::dynamic_pointer_cast<CTextureViewImpl>(view);
            if (impl)
                cColorAttachments.push_back(impl->getHandle());
        }

        std::vector<GfxColor> cClearColors;
        for (auto& color : clearColors) {
            cClearColors.push_back({ color.r, color.g, color.b, color.a });
        }

        GfxTextureView cDepthStencil = nullptr;
        if (depthStencilAttachment) {
            auto impl = std::dynamic_pointer_cast<CTextureViewImpl>(depthStencilAttachment);
            if (impl)
                cDepthStencil = impl->getHandle();
        }

        GfxRenderPassEncoder encoder = nullptr;
        GfxResult result = gfxCommandEncoderBeginRenderPass(
            handle,
            cColorAttachments.data(),
            static_cast<uint32_t>(cColorAttachments.size()),
            cClearColors.empty() ? nullptr : cClearColors.data(),
            cDepthStencil,
            depthClearValue,
            stencilClearValue,
            &encoder);

        if (result != GFX_RESULT_SUCCESS || !encoder)
            throw std::runtime_error("Failed to begin render pass");
        return std::make_shared<CRenderPassEncoderImpl>(encoder);
    }

    std::shared_ptr<ComputePassEncoder> beginComputePass(const std::string& label = "") override
    {
        GfxComputePassEncoder encoder = nullptr;
        GfxResult result = gfxCommandEncoderBeginComputePass(handle, label.c_str(), &encoder);
        if (result != GFX_RESULT_SUCCESS || !encoder)
            throw std::runtime_error("Failed to begin compute pass");
        return std::make_shared<CComputePassEncoderImpl>(encoder);
    }

    void copyBufferToBuffer(
        std::shared_ptr<Buffer> source, uint64_t sourceOffset,
        std::shared_ptr<Buffer> destination, uint64_t destinationOffset,
        uint64_t size) override
    {
        auto src = std::dynamic_pointer_cast<CBufferImpl>(source);
        auto dst = std::dynamic_pointer_cast<CBufferImpl>(destination);
        if (src && dst) {
            gfxCommandEncoderCopyBufferToBuffer(handle, src->getHandle(), sourceOffset,
                dst->getHandle(), destinationOffset, size);
        }
    }

    void copyBufferToTexture(
        std::shared_ptr<Buffer> source, uint64_t sourceOffset, uint32_t bytesPerRow,
        std::shared_ptr<Texture> destination, const Origin3D& origin,
        const Extent3D& extent, uint32_t mipLevel = 0) override
    {
        auto src = std::dynamic_pointer_cast<CBufferImpl>(source);
        auto dst = std::dynamic_pointer_cast<CTextureImpl>(destination);
        if (src && dst) {
            GfxOrigin3D cOrigin = { origin.x, origin.y, origin.z };
            GfxExtent3D cExtent = { extent.width, extent.height, extent.depth };
            gfxCommandEncoderCopyBufferToTexture(handle, src->getHandle(), sourceOffset, bytesPerRow,
                dst->getHandle(), &cOrigin, &cExtent, mipLevel);
        }
    }

    void copyTextureToBuffer(
        std::shared_ptr<Texture> source, const Origin3D& origin, uint32_t mipLevel,
        std::shared_ptr<Buffer> destination, uint64_t destinationOffset, uint32_t bytesPerRow,
        const Extent3D& extent) override
    {
        auto src = std::dynamic_pointer_cast<CTextureImpl>(source);
        auto dst = std::dynamic_pointer_cast<CBufferImpl>(destination);
        if (src && dst) {
            GfxOrigin3D cOrigin = { origin.x, origin.y, origin.z };
            GfxExtent3D cExtent = { extent.width, extent.height, extent.depth };
            gfxCommandEncoderCopyTextureToBuffer(handle, src->getHandle(), &cOrigin, mipLevel,
                dst->getHandle(), destinationOffset, bytesPerRow, &cExtent);
        }
    }

    void finish() override
    {
        gfxCommandEncoderFinish(handle);
    }
};

class CFenceImpl : public Fence {
private:
    GfxFence handle;

public:
    explicit CFenceImpl(GfxFence h)
        : handle(h)
    {
    }
    ~CFenceImpl() override
    {
        if (handle)
            gfxFenceDestroy(handle);
    }

    FenceStatus getStatus() const override
    {
        bool signaled;
        gfxFenceGetStatus(handle, &signaled);
        return signaled ? FenceStatus::Signaled : FenceStatus::Unsignaled;
    }

    bool wait(uint64_t timeoutNanoseconds = UINT64_MAX) override
    {
        return gfxFenceWait(handle, timeoutNanoseconds) == GFX_RESULT_SUCCESS;
    }

    void reset() override
    {
        gfxFenceReset(handle);
    }
};

class CSemaphoreImpl : public Semaphore {
private:
    GfxSemaphore handle;

public:
    explicit CSemaphoreImpl(GfxSemaphore h)
        : handle(h)
    {
    }
    ~CSemaphoreImpl() override
    {
        if (handle)
            gfxSemaphoreDestroy(handle);
    }

    GfxSemaphore getHandle() const { return handle; }

    SemaphoreType getType() const override
    {
        return static_cast<SemaphoreType>(gfxSemaphoreGetType(handle));
    }

    uint64_t getValue() const override
    {
        return gfxSemaphoreGetValue(handle);
    }

    void signal(uint64_t value) override
    {
        gfxSemaphoreSignal(handle, value);
    }

    bool wait(uint64_t value, uint64_t timeoutNanoseconds = UINT64_MAX) override
    {
        return gfxSemaphoreWait(handle, value, timeoutNanoseconds) == GFX_RESULT_SUCCESS;
    }
};

class CQueueImpl : public Queue {
private:
    GfxQueue handle;

public:
    explicit CQueueImpl(GfxQueue h)
        : handle(h)
    {
    }

    void submit(std::shared_ptr<CommandEncoder> commandEncoder) override
    {
        auto impl = std::dynamic_pointer_cast<CCommandEncoderImpl>(commandEncoder);
        if (impl)
            gfxQueueSubmit(handle, impl->getHandle());
    }

    void submitWithSync(const SubmitInfo& submitInfo) override
    {
        // Convert C++ structures to C
        std::vector<GfxCommandEncoder> cEncoders;
        for (auto& encoder : submitInfo.commandEncoders) {
            auto impl = std::dynamic_pointer_cast<CCommandEncoderImpl>(encoder);
            if (impl)
                cEncoders.push_back(impl->getHandle());
        }

        std::vector<GfxSemaphore> cWaitSems;
        for (auto& sem : submitInfo.waitSemaphores) {
            auto impl = std::dynamic_pointer_cast<CSemaphoreImpl>(sem);
            if (impl)
                cWaitSems.push_back(impl->getHandle());
        }

        std::vector<GfxSemaphore> cSignalSems;
        for (auto& sem : submitInfo.signalSemaphores) {
            auto impl = std::dynamic_pointer_cast<CSemaphoreImpl>(sem);
            if (impl)
                cSignalSems.push_back(impl->getHandle());
        }

        GfxSubmitInfo cInfo = {};
        cInfo.commandEncoders = cEncoders.data();
        cInfo.commandEncoderCount = static_cast<uint32_t>(cEncoders.size());
        cInfo.waitSemaphores = cWaitSems.data();
        cInfo.waitSemaphoreCount = static_cast<uint32_t>(cWaitSems.size());
        cInfo.signalSemaphores = cSignalSems.data();
        cInfo.signalSemaphoreCount = static_cast<uint32_t>(cSignalSems.size());

        if (submitInfo.signalFence) {
            auto fenceImpl = std::dynamic_pointer_cast<CFenceImpl>(submitInfo.signalFence);
            if (fenceImpl) {
                // Would need to add fence handle access
            }
        }

        gfxQueueSubmitWithSync(handle, &cInfo);
    }

    void writeBuffer(std::shared_ptr<Buffer> buffer, uint64_t offset, const void* data, uint64_t size) override
    {
        auto impl = std::dynamic_pointer_cast<CBufferImpl>(buffer);
        if (impl)
            gfxQueueWriteBuffer(handle, impl->getHandle(), offset, data, size);
    }

    void writeTexture(
        std::shared_ptr<Texture> texture, const Origin3D& origin, uint32_t mipLevel,
        const void* data, uint64_t dataSize, uint32_t bytesPerRow,
        const Extent3D& extent) override
    {
        auto impl = std::dynamic_pointer_cast<CTextureImpl>(texture);
        if (impl) {
            GfxOrigin3D cOrigin = { origin.x, origin.y, origin.z };
            GfxExtent3D cExtent = { extent.width, extent.height, extent.depth };
            gfxQueueWriteTexture(handle, impl->getHandle(), &cOrigin, mipLevel,
                data, dataSize, bytesPerRow, &cExtent);
        }
    }

    void waitIdle() override
    {
        gfxQueueWaitIdle(handle);
    }
};

class CSurfaceImpl : public Surface {
private:
    GfxSurface handle;

public:
    explicit CSurfaceImpl(GfxSurface h)
        : handle(h)
    {
    }
    ~CSurfaceImpl() override
    {
        if (handle)
            gfxSurfaceDestroy(handle);
    }

    GfxSurface getHandle() const { return handle; }

    uint32_t getWidth() const override { return gfxSurfaceGetWidth(handle); }
    uint32_t getHeight() const override { return gfxSurfaceGetHeight(handle); }
    void resize(uint32_t width, uint32_t height) override { gfxSurfaceResize(handle, width, height); }

    std::vector<TextureFormat> getSupportedFormats() const override
    {
        GfxTextureFormat formats[16];
        uint32_t count = gfxSurfaceGetSupportedFormats(handle, formats, 16);
        std::vector<TextureFormat> result;
        for (uint32_t i = 0; i < count; ++i) {
            result.push_back(cFormatToCppFormat(formats[i]));
        }
        return result;
    }

    std::vector<PresentMode> getSupportedPresentModes() const override
    {
        GfxPresentMode modes[8];
        uint32_t count = gfxSurfaceGetSupportedPresentModes(handle, modes, 8);
        std::vector<PresentMode> result;
        for (uint32_t i = 0; i < count; ++i) {
            result.push_back(static_cast<PresentMode>(modes[i]));
        }
        return result;
    }

    PlatformWindowHandle getPlatformHandle() const override
    {
        GfxPlatformWindowHandle cHandle = gfxSurfaceGetPlatformHandle(handle);
        PlatformWindowHandle result;
#ifdef _WIN32
        result.hwnd = cHandle.hwnd;
        result.hinstance = cHandle.hinstance;
#elif defined(__linux__)
        result.window = cHandle.window;
        result.display = cHandle.display;
        result.isWayland = cHandle.isWayland;
#elif defined(__APPLE__)
        result.nsWindow = cHandle.nsWindow;
        result.metalLayer = cHandle.metalLayer;
#else
        result.handle = cHandle.handle;
        result.display = cHandle.display;
        result.extra = cHandle.extra;
#endif
        return result;
    }
};

class CSwapchainImpl : public Swapchain {
private:
    GfxSwapchain handle;

public:
    explicit CSwapchainImpl(GfxSwapchain h)
        : handle(h)
    {
    }
    ~CSwapchainImpl() override
    {
        if (handle)
            gfxSwapchainDestroy(handle);
    }

    uint32_t getWidth() const override { return gfxSwapchainGetWidth(handle); }
    uint32_t getHeight() const override { return gfxSwapchainGetHeight(handle); }
    TextureFormat getFormat() const override { return cFormatToCppFormat(gfxSwapchainGetFormat(handle)); }
    uint32_t getBufferCount() const override { return gfxSwapchainGetBufferCount(handle); }

    std::shared_ptr<TextureView> getCurrentTextureView() override
    {
        GfxTextureView view = gfxSwapchainGetCurrentTextureView(handle);
        if (!view)
            return nullptr;
        return std::make_shared<CTextureViewImpl>(view);
    }

    void present() override
    {
        gfxSwapchainPresent(handle);
    }

    void resize(uint32_t width, uint32_t height) override
    {
        gfxSwapchainResize(handle, width, height);
    }

    bool needsRecreation() const override
    {
        return gfxSwapchainNeedsRecreation(handle);
    }
};

class CDeviceImpl : public Device {
private:
    GfxDevice handle;
    std::shared_ptr<CQueueImpl> queue;

public:
    explicit CDeviceImpl(GfxDevice h)
        : handle(h)
    {
        GfxQueue queueHandle = gfxDeviceGetQueue(handle);
        queue = std::make_shared<CQueueImpl>(queueHandle);
    }
    ~CDeviceImpl() override
    {
        if (handle) {
            gfxDeviceWaitIdle(handle);
            gfxDeviceDestroy(handle);
        }
    }

    std::shared_ptr<Queue> getQueue() override { return queue; }

    std::shared_ptr<Surface> createSurface(const SurfaceDescriptor& descriptor) override
    {
        GfxSurfaceDescriptor cDesc = {};
        cDesc.label = descriptor.label.c_str();
        cDesc.windowHandle = cppHandleToCHandle(descriptor.windowHandle);
        cDesc.width = descriptor.width;
        cDesc.height = descriptor.height;

        GfxSurface surface = nullptr;
        GfxResult result = gfxDeviceCreateSurface(handle, &cDesc, &surface);
        if (result != GFX_RESULT_SUCCESS || !surface)
            throw std::runtime_error("Failed to create surface");
        return std::make_shared<CSurfaceImpl>(surface);
    }

    std::shared_ptr<Swapchain> createSwapchain(
        std::shared_ptr<Surface> surface,
        const SwapchainDescriptor& descriptor) override
    {
        auto surfaceImpl = std::dynamic_pointer_cast<CSurfaceImpl>(surface);
        if (!surfaceImpl)
            throw std::runtime_error("Invalid surface type");

        GfxSwapchainDescriptor cDesc = {};
        cDesc.label = descriptor.label.c_str();
        cDesc.width = descriptor.width;
        cDesc.height = descriptor.height;
        cDesc.format = cppFormatToCFormat(descriptor.format);
        cDesc.usage = cppTextureUsageToCUsage(descriptor.usage);
        cDesc.presentMode = static_cast<GfxPresentMode>(descriptor.presentMode);
        cDesc.bufferCount = descriptor.bufferCount;

        GfxSwapchain swapchain = nullptr;
        GfxResult result = gfxDeviceCreateSwapchain(handle, surfaceImpl->getHandle(), &cDesc, &swapchain);
        if (result != GFX_RESULT_SUCCESS || !swapchain)
            throw std::runtime_error("Failed to create swapchain");
        return std::make_shared<CSwapchainImpl>(swapchain);
    }

    std::shared_ptr<Buffer> createBuffer(const BufferDescriptor& descriptor) override
    {
        GfxBufferDescriptor cDesc = {};
        cDesc.label = descriptor.label.c_str();
        cDesc.size = descriptor.size;
        cDesc.usage = cppBufferUsageToCUsage(descriptor.usage);
        cDesc.mappedAtCreation = descriptor.mappedAtCreation;

        GfxBuffer buffer = nullptr;
        GfxResult result = gfxDeviceCreateBuffer(handle, &cDesc, &buffer);
        if (result != GFX_RESULT_SUCCESS || !buffer)
            throw std::runtime_error("Failed to create buffer");
        return std::make_shared<CBufferImpl>(buffer);
    }

    std::shared_ptr<Texture> createTexture(const TextureDescriptor& descriptor) override
    {
        GfxTextureDescriptor cDesc = {};
        cDesc.label = descriptor.label.c_str();
        cDesc.size = { descriptor.size.width, descriptor.size.height, descriptor.size.depth };
        cDesc.mipLevelCount = descriptor.mipLevelCount;
        cDesc.sampleCount = descriptor.sampleCount;
        cDesc.format = cppFormatToCFormat(descriptor.format);
        cDesc.usage = cppTextureUsageToCUsage(descriptor.usage);

        GfxTexture texture = nullptr;
        GfxResult result = gfxDeviceCreateTexture(handle, &cDesc, &texture);
        if (result != GFX_RESULT_SUCCESS || !texture)
            throw std::runtime_error("Failed to create texture");
        return std::make_shared<CTextureImpl>(texture);
    }

    std::shared_ptr<Sampler> createSampler(const SamplerDescriptor& descriptor = {}) override
    {
        GfxSamplerDescriptor cDesc = {};
        cDesc.label = descriptor.label.c_str();
        cDesc.addressModeU = static_cast<GfxAddressMode>(descriptor.addressModeU);
        cDesc.addressModeV = static_cast<GfxAddressMode>(descriptor.addressModeV);
        cDesc.addressModeW = static_cast<GfxAddressMode>(descriptor.addressModeW);
        cDesc.magFilter = static_cast<GfxFilterMode>(descriptor.magFilter);
        cDesc.minFilter = static_cast<GfxFilterMode>(descriptor.minFilter);
        cDesc.mipmapFilter = static_cast<GfxFilterMode>(descriptor.mipmapFilter);
        cDesc.lodMinClamp = descriptor.lodMinClamp;
        cDesc.lodMaxClamp = descriptor.lodMaxClamp;
        cDesc.maxAnisotropy = descriptor.maxAnisotropy;

        GfxCompareFunction cCompare;
        if (descriptor.compare.has_value()) {
            cCompare = static_cast<GfxCompareFunction>(*descriptor.compare);
            cDesc.compare = &cCompare;
        } else {
            cDesc.compare = nullptr;
        }

        GfxSampler sampler = nullptr;
        GfxResult result = gfxDeviceCreateSampler(handle, &cDesc, &sampler);
        if (result != GFX_RESULT_SUCCESS || !sampler)
            throw std::runtime_error("Failed to create sampler");
        return std::make_shared<CSamplerImpl>(sampler);
    }

    std::shared_ptr<Shader> createShader(const ShaderDescriptor& descriptor) override
    {
        GfxShaderDescriptor cDesc = {};
        cDesc.label = descriptor.label.c_str();
        cDesc.code = descriptor.code.c_str();
        cDesc.codeSize = descriptor.code.size(); // Set the actual binary size
        cDesc.entryPoint = descriptor.entryPoint.c_str();

        GfxShader shader = nullptr;
        GfxResult result = gfxDeviceCreateShader(handle, &cDesc, &shader);
        if (result != GFX_RESULT_SUCCESS || !shader)
            throw std::runtime_error("Failed to create shader");
        return std::make_shared<CShaderImpl>(shader);
    }

    std::shared_ptr<BindGroupLayout> createBindGroupLayout(const BindGroupLayoutDescriptor& descriptor) override
    {
        // Convert entries properly
        std::vector<GfxBindGroupLayoutEntry> cEntries(descriptor.entries.size());
        for (size_t i = 0; i < descriptor.entries.size(); ++i) {
            const auto& entry = descriptor.entries[i];
            cEntries[i].binding = entry.binding;
            cEntries[i].visibility = static_cast<GfxShaderStage>(static_cast<uint32_t>(entry.visibility));

            // Determine binding type based on variant index
            if (entry.resource.index() == 0) {
                // Buffer binding
                cEntries[i].type = GFX_BINDING_TYPE_BUFFER;
                const auto& buffer = std::get<BindGroupLayoutEntry::BufferBinding>(entry.resource);
                cEntries[i].buffer.hasDynamicOffset = buffer.hasDynamicOffset;
                cEntries[i].buffer.minBindingSize = buffer.minBindingSize;
            } else if (entry.resource.index() == 1) {
                // Sampler binding
                cEntries[i].type = GFX_BINDING_TYPE_SAMPLER;
                const auto& sampler = std::get<BindGroupLayoutEntry::SamplerBinding>(entry.resource);
                cEntries[i].sampler.comparison = sampler.comparison;
            } else if (entry.resource.index() == 2) {
                // Texture binding
                cEntries[i].type = GFX_BINDING_TYPE_TEXTURE;
                const auto& texture = std::get<BindGroupLayoutEntry::TextureBinding>(entry.resource);
                cEntries[i].texture.multisampled = texture.multisampled;
            } else if (entry.resource.index() == 3) {
                // Storage texture binding
                cEntries[i].type = GFX_BINDING_TYPE_STORAGE_TEXTURE;
                const auto& storageTexture = std::get<BindGroupLayoutEntry::StorageTextureBinding>(entry.resource);
                cEntries[i].storageTexture.format = cppFormatToCFormat(storageTexture.format);
                cEntries[i].storageTexture.writeOnly = storageTexture.writeOnly;
            }
        }

        GfxBindGroupLayoutDescriptor cDesc = {};
        cDesc.label = descriptor.label.c_str();
        cDesc.entries = cEntries.data();
        cDesc.entryCount = static_cast<uint32_t>(cEntries.size());

        GfxBindGroupLayout layout = nullptr;
        GfxResult result = gfxDeviceCreateBindGroupLayout(handle, &cDesc, &layout);
        if (result != GFX_RESULT_SUCCESS || !layout)
            throw std::runtime_error("Failed to create bind group layout");
        return std::make_shared<CBindGroupLayoutImpl>(layout);
    }

    std::shared_ptr<BindGroup> createBindGroup(const BindGroupDescriptor& descriptor) override
    {
        auto layoutImpl = std::dynamic_pointer_cast<CBindGroupLayoutImpl>(descriptor.layout);
        if (!layoutImpl)
            throw std::runtime_error("Invalid bind group layout type");

        // Convert entries properly
        std::vector<GfxBindGroupEntry> cEntries(descriptor.entries.size());
        for (size_t i = 0; i < descriptor.entries.size(); ++i) {
            const auto& entry = descriptor.entries[i];
            cEntries[i].binding = entry.binding;

            // Determine type based on variant index
            if (entry.resource.index() == 0) {
                // Buffer resource
                cEntries[i].type = GFX_BIND_GROUP_ENTRY_TYPE_BUFFER;
                auto buffer = std::get<std::shared_ptr<Buffer>>(entry.resource);
                auto bufferImpl = std::dynamic_pointer_cast<CBufferImpl>(buffer);
                if (bufferImpl) {
                    cEntries[i].resource.buffer.buffer = bufferImpl->getHandle();
                    cEntries[i].resource.buffer.offset = entry.offset;
                    cEntries[i].resource.buffer.size = entry.size;
                }
            } else if (entry.resource.index() == 1) {
                // Sampler resource
                cEntries[i].type = GFX_BIND_GROUP_ENTRY_TYPE_SAMPLER;
                auto sampler = std::get<std::shared_ptr<Sampler>>(entry.resource);
                auto samplerImpl = std::dynamic_pointer_cast<CSamplerImpl>(sampler);
                if (samplerImpl) {
                    cEntries[i].resource.sampler = samplerImpl->getHandle();
                }
            } else if (entry.resource.index() == 2) {
                // TextureView resource
                cEntries[i].type = GFX_BIND_GROUP_ENTRY_TYPE_TEXTURE_VIEW;
                auto textureView = std::get<std::shared_ptr<TextureView>>(entry.resource);
                auto textureViewImpl = std::dynamic_pointer_cast<CTextureViewImpl>(textureView);
                if (textureViewImpl) {
                    cEntries[i].resource.textureView = textureViewImpl->getHandle();
                }
            }
        }

        GfxBindGroupDescriptor cDesc = {};
        cDesc.label = descriptor.label.c_str();
        cDesc.layout = layoutImpl->getHandle();
        cDesc.entries = cEntries.data();
        cDesc.entryCount = static_cast<uint32_t>(cEntries.size());

        GfxBindGroup bindGroup = nullptr;
        GfxResult result = gfxDeviceCreateBindGroup(handle, &cDesc, &bindGroup);
        if (result != GFX_RESULT_SUCCESS || !bindGroup)
            throw std::runtime_error("Failed to create bind group");
        return std::make_shared<CBindGroupImpl>(bindGroup);
    }

    std::shared_ptr<RenderPipeline> createRenderPipeline(const RenderPipelineDescriptor& descriptor) override
    {
        // Extract shader handles
        auto vertexShaderImpl = std::dynamic_pointer_cast<CShaderImpl>(descriptor.vertex.module);
        if (!vertexShaderImpl)
            throw std::runtime_error("Invalid vertex shader type");

        // Convert vertex attributes
        std::vector<std::vector<GfxVertexAttribute>> cAttributesPerBuffer;
        std::vector<GfxVertexBufferLayout> cVertexBuffers;

        for (const auto& buffer : descriptor.vertex.buffers) {
            std::vector<GfxVertexAttribute> cAttributes;
            for (const auto& attr : buffer.attributes) {
                GfxVertexAttribute cAttr = {};
                cAttr.format = cppFormatToCFormat(attr.format);
                cAttr.offset = attr.offset;
                cAttr.shaderLocation = attr.shaderLocation;
                cAttributes.push_back(cAttr);
            }
            cAttributesPerBuffer.push_back(std::move(cAttributes));

            GfxVertexBufferLayout cBuffer = {};
            cBuffer.arrayStride = buffer.arrayStride;
            cBuffer.attributes = cAttributesPerBuffer.back().data();
            cBuffer.attributeCount = static_cast<uint32_t>(cAttributesPerBuffer.back().size());
            cBuffer.stepModeInstance = buffer.stepModeInstance;
            cVertexBuffers.push_back(cBuffer);
        }

        // Vertex state
        GfxVertexState cVertexState = {};
        cVertexState.module = vertexShaderImpl->getHandle();
        cVertexState.entryPoint = descriptor.vertex.entryPoint.c_str();
        cVertexState.buffers = cVertexBuffers.empty() ? nullptr : cVertexBuffers.data();
        cVertexState.bufferCount = static_cast<uint32_t>(cVertexBuffers.size());

        // Fragment state (optional)
        GfxFragmentState cFragmentState = {};
        std::vector<GfxColorTargetState> cColorTargets;
        std::vector<GfxBlendState> cBlendStates;
        GfxFragmentState* pFragmentState = nullptr;

        if (descriptor.fragment.has_value()) {
            const auto& fragment = *descriptor.fragment;
            auto fragmentShaderImpl = std::dynamic_pointer_cast<CShaderImpl>(fragment.module);
            if (!fragmentShaderImpl)
                throw std::runtime_error("Invalid fragment shader type");

            // Convert color targets
            for (const auto& target : fragment.targets) {
                GfxColorTargetState cTarget = {};
                cTarget.format = cppFormatToCFormat(target.format);
                cTarget.writeMask = target.writeMask;

                // Convert blend state if present
                if (target.blend.has_value()) {
                    GfxBlendState cBlend = {};
                    cBlend.color.operation = static_cast<GfxBlendOperation>(target.blend->color.operation);
                    cBlend.color.srcFactor = static_cast<GfxBlendFactor>(target.blend->color.srcFactor);
                    cBlend.color.dstFactor = static_cast<GfxBlendFactor>(target.blend->color.dstFactor);
                    cBlend.alpha.operation = static_cast<GfxBlendOperation>(target.blend->alpha.operation);
                    cBlend.alpha.srcFactor = static_cast<GfxBlendFactor>(target.blend->alpha.srcFactor);
                    cBlend.alpha.dstFactor = static_cast<GfxBlendFactor>(target.blend->alpha.dstFactor);
                    cBlendStates.push_back(cBlend);
                    cTarget.blend = &cBlendStates.back();
                } else {
                    cTarget.blend = nullptr;
                }

                cColorTargets.push_back(cTarget);
            }

            cFragmentState.module = fragmentShaderImpl->getHandle();
            cFragmentState.entryPoint = fragment.entryPoint.c_str();
            cFragmentState.targets = cColorTargets.data();
            cFragmentState.targetCount = static_cast<uint32_t>(cColorTargets.size());
            pFragmentState = &cFragmentState;
        }

        // Primitive state
        GfxPrimitiveState cPrimitiveState = {};
        cPrimitiveState.topology = static_cast<GfxPrimitiveTopology>(descriptor.primitive.topology);
        cPrimitiveState.frontFaceCounterClockwise = descriptor.primitive.frontFaceCounterClockwise;
        cPrimitiveState.cullBackFace = descriptor.primitive.cullBackFace;
        cPrimitiveState.unclippedDepth = descriptor.primitive.unclippedDepth;

        GfxIndexFormat cStripIndexFormat;
        if (descriptor.primitive.stripIndexFormat.has_value()) {
            cStripIndexFormat = (*descriptor.primitive.stripIndexFormat == IndexFormat::Uint16)
                ? GFX_INDEX_FORMAT_UINT16
                : GFX_INDEX_FORMAT_UINT32;
            cPrimitiveState.stripIndexFormat = &cStripIndexFormat;
        } else {
            cPrimitiveState.stripIndexFormat = nullptr;
        }

        // Depth/stencil state (optional)
        GfxDepthStencilState cDepthStencilState = {};
        GfxDepthStencilState* pDepthStencilState = nullptr;

        if (descriptor.depthStencil.has_value()) {
            const auto& depthStencil = *descriptor.depthStencil;
            cDepthStencilState.format = cppFormatToCFormat(depthStencil.format);
            cDepthStencilState.depthWriteEnabled = depthStencil.depthWriteEnabled;
            cDepthStencilState.depthCompare = static_cast<GfxCompareFunction>(depthStencil.depthCompare);

            cDepthStencilState.stencilFront.compare = static_cast<GfxCompareFunction>(depthStencil.stencilFront.compare);
            cDepthStencilState.stencilFront.failOp = static_cast<GfxStencilOperation>(depthStencil.stencilFront.failOp);
            cDepthStencilState.stencilFront.depthFailOp = static_cast<GfxStencilOperation>(depthStencil.stencilFront.depthFailOp);
            cDepthStencilState.stencilFront.passOp = static_cast<GfxStencilOperation>(depthStencil.stencilFront.passOp);

            cDepthStencilState.stencilBack.compare = static_cast<GfxCompareFunction>(depthStencil.stencilBack.compare);
            cDepthStencilState.stencilBack.failOp = static_cast<GfxStencilOperation>(depthStencil.stencilBack.failOp);
            cDepthStencilState.stencilBack.depthFailOp = static_cast<GfxStencilOperation>(depthStencil.stencilBack.depthFailOp);
            cDepthStencilState.stencilBack.passOp = static_cast<GfxStencilOperation>(depthStencil.stencilBack.passOp);

            cDepthStencilState.stencilReadMask = depthStencil.stencilReadMask;
            cDepthStencilState.stencilWriteMask = depthStencil.stencilWriteMask;
            cDepthStencilState.depthBias = depthStencil.depthBias;
            cDepthStencilState.depthBiasSlopeScale = depthStencil.depthBiasSlopeScale;
            cDepthStencilState.depthBiasClamp = depthStencil.depthBiasClamp;

            pDepthStencilState = &cDepthStencilState;
        }

        // Create pipeline descriptor
        GfxRenderPipelineDescriptor cDesc = {};
        cDesc.label = descriptor.label.c_str();
        cDesc.vertex = &cVertexState;
        cDesc.fragment = pFragmentState;
        cDesc.primitive = &cPrimitiveState;
        cDesc.depthStencil = pDepthStencilState;
        cDesc.sampleCount = descriptor.sampleCount;

        // Convert bind group layouts
        std::vector<GfxBindGroupLayout> cBindGroupLayouts;
        for (const auto& layout : descriptor.bindGroupLayouts) {
            auto layoutImpl = std::dynamic_pointer_cast<CBindGroupLayoutImpl>(layout);
            if (layoutImpl) {
                cBindGroupLayouts.push_back(layoutImpl->getHandle());
            }
        }
        cDesc.bindGroupLayouts = cBindGroupLayouts.empty() ? nullptr : cBindGroupLayouts.data();
        cDesc.bindGroupLayoutCount = static_cast<uint32_t>(cBindGroupLayouts.size());

        GfxRenderPipeline pipeline = nullptr;
        GfxResult result = gfxDeviceCreateRenderPipeline(handle, &cDesc, &pipeline);
        if (result != GFX_RESULT_SUCCESS || !pipeline)
            throw std::runtime_error("Failed to create render pipeline");
        return std::make_shared<CRenderPipelineImpl>(pipeline);
    }

    std::shared_ptr<ComputePipeline> createComputePipeline(const ComputePipelineDescriptor& descriptor) override
    {
        auto shaderImpl = std::dynamic_pointer_cast<CShaderImpl>(descriptor.compute);
        if (!shaderImpl)
            throw std::runtime_error("Invalid shader type");

        GfxComputePipelineDescriptor cDesc = {};
        cDesc.label = descriptor.label.c_str();
        cDesc.compute = shaderImpl->getHandle();
        cDesc.entryPoint = descriptor.entryPoint.c_str();

        GfxComputePipeline pipeline = nullptr;
        GfxResult result = gfxDeviceCreateComputePipeline(handle, &cDesc, &pipeline);
        if (result != GFX_RESULT_SUCCESS || !pipeline)
            throw std::runtime_error("Failed to create compute pipeline");
        return std::make_shared<CComputePipelineImpl>(pipeline);
    }

    std::shared_ptr<CommandEncoder> createCommandEncoder(const std::string& label = "") override
    {
        GfxCommandEncoder encoder = nullptr;
        GfxResult result = gfxDeviceCreateCommandEncoder(handle, label.c_str(), &encoder);
        if (result != GFX_RESULT_SUCCESS || !encoder)
            throw std::runtime_error("Failed to create command encoder");
        return std::make_shared<CCommandEncoderImpl>(encoder);
    }

    std::shared_ptr<Fence> createFence(const FenceDescriptor& descriptor = {}) override
    {
        GfxFenceDescriptor cDesc = {};
        cDesc.label = descriptor.label.c_str();
        cDesc.signaled = descriptor.signaled;

        GfxFence fence = nullptr;
        GfxResult result = gfxDeviceCreateFence(handle, &cDesc, &fence);
        if (result != GFX_RESULT_SUCCESS || !fence)
            throw std::runtime_error("Failed to create fence");
        return std::make_shared<CFenceImpl>(fence);
    }

    std::shared_ptr<Semaphore> createSemaphore(const SemaphoreDescriptor& descriptor = {}) override
    {
        GfxSemaphoreDescriptor cDesc = {};
        cDesc.label = descriptor.label.c_str();
        cDesc.type = static_cast<GfxSemaphoreType>(descriptor.type);
        cDesc.initialValue = descriptor.initialValue;

        GfxSemaphore semaphore = nullptr;
        GfxResult result = gfxDeviceCreateSemaphore(handle, &cDesc, &semaphore);
        if (result != GFX_RESULT_SUCCESS || !semaphore)
            throw std::runtime_error("Failed to create semaphore");
        return std::make_shared<CSemaphoreImpl>(semaphore);
    }

    void waitIdle() override
    {
        gfxDeviceWaitIdle(handle);
    }
};

class CAdapterImpl : public Adapter {
private:
    GfxAdapter handle;

public:
    explicit CAdapterImpl(GfxAdapter h)
        : handle(h)
    {
    }
    ~CAdapterImpl() override
    {
        if (handle)
            gfxAdapterDestroy(handle);
    }

    std::shared_ptr<Device> createDevice(const DeviceDescriptor& descriptor = {}) override
    {
        GfxDeviceDescriptor cDesc = {};
        cDesc.label = descriptor.label.c_str();
        // Convert required features if needed

        GfxDevice device = nullptr;
        GfxResult result = gfxAdapterCreateDevice(handle, &cDesc, &device);
        if (result != GFX_RESULT_SUCCESS || !device)
            throw std::runtime_error("Failed to create device");
        return std::make_shared<CDeviceImpl>(device);
    }

    std::string getName() const override
    {
        const char* name = gfxAdapterGetName(handle);
        return name ? name : "Unknown";
    }

    Backend getBackend() const override
    {
        return cBackendToCppBackend(gfxAdapterGetBackend(handle));
    }
};

class CInstanceImpl : public Instance {
private:
    GfxInstance handle;

public:
    explicit CInstanceImpl(GfxInstance h)
        : handle(h)
    {
    }
    ~CInstanceImpl() override
    {
        if (handle)
            gfxInstanceDestroy(handle);
    }

    std::shared_ptr<Adapter> requestAdapter(const AdapterDescriptor& descriptor = {}) override
    {
        GfxAdapterDescriptor cDesc = {};
        cDesc.powerPreference = static_cast<GfxPowerPreference>(descriptor.powerPreference);
        cDesc.forceFallbackAdapter = descriptor.forceFallbackAdapter;

        GfxAdapter adapter = nullptr;
        GfxResult result = gfxInstanceRequestAdapter(handle, &cDesc, &adapter);
        if (result != GFX_RESULT_SUCCESS || !adapter)
            throw std::runtime_error("Failed to request adapter");
        return std::make_shared<CAdapterImpl>(adapter);
    }

    std::vector<std::shared_ptr<Adapter>> enumerateAdapters() override
    {
        GfxAdapter adapters[16];
        uint32_t count = gfxInstanceEnumerateAdapters(handle, adapters, 16);

        std::vector<std::shared_ptr<Adapter>> result;
        for (uint32_t i = 0; i < count; ++i) {
            result.push_back(std::make_shared<CAdapterImpl>(adapters[i]));
        }
        return result;
    }
};

// ============================================================================
// Factory Function
// ============================================================================

std::shared_ptr<Instance> createInstance(const InstanceDescriptor& descriptor)
{
    // Load the backend first (required by the C API)
    GfxBackend cBackend = cppBackendToCBackend(descriptor.backend);
    if (!gfxLoadBackend(cBackend)) {
        throw std::runtime_error("Failed to load graphics backend");
    }

    GfxInstanceDescriptor cDesc = {};
    cDesc.backend = cBackend;
    cDesc.enableValidation = descriptor.enableValidation;
    cDesc.applicationName = descriptor.applicationName.c_str();
    cDesc.applicationVersion = descriptor.applicationVersion;

    // Convert extensions
    std::vector<const char*> extensions;
    for (const auto& ext : descriptor.requiredExtensions) {
        extensions.push_back(ext.c_str());
    }
    cDesc.requiredExtensions = extensions.empty() ? nullptr : extensions.data();
    cDesc.requiredExtensionCount = static_cast<uint32_t>(extensions.size());

    GfxInstance instance = nullptr;
    GfxResult result = gfxCreateInstance(&cDesc, &instance);
    if (result != GFX_RESULT_SUCCESS || !instance) {
        throw std::runtime_error("Failed to create instance");
    }

    return std::make_shared<CInstanceImpl>(instance);
}

} // namespace gfx