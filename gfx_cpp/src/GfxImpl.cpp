#include <gfx/gfx.h> // Include C API
#include <gfx_cpp/Gfx.hpp>

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

static GfxSampleCount cppSampleCountToCCount(SampleCount sampleCount)
{
    switch (sampleCount) {
    case SampleCount::Count1:
        return GFX_SAMPLE_COUNT_1;
    case SampleCount::Count2:
        return GFX_SAMPLE_COUNT_2;
    case SampleCount::Count4:
        return GFX_SAMPLE_COUNT_4;
    case SampleCount::Count8:
        return GFX_SAMPLE_COUNT_8;
    case SampleCount::Count16:
        return GFX_SAMPLE_COUNT_16;
    case SampleCount::Count32:
        return GFX_SAMPLE_COUNT_32;
    case SampleCount::Count64:
        return GFX_SAMPLE_COUNT_64;
    default:
        return GFX_SAMPLE_COUNT_1;
    }
}

static GfxBufferUsage cppBufferUsageToCUsage(BufferUsage usage)
{
    return static_cast<GfxBufferUsage>(static_cast<uint32_t>(usage));
}

static GfxTextureUsage cppTextureUsageToCUsage(TextureUsage usage)
{
    return static_cast<GfxTextureUsage>(static_cast<uint32_t>(usage));
}

static GfxTextureType cppTextureTypeToCType(TextureType type)
{
    switch (type) {
    case TextureType::Texture1D:
        return GFX_TEXTURE_TYPE_1D;
    case TextureType::Texture2D:
        return GFX_TEXTURE_TYPE_2D;
    case TextureType::Texture3D:
        return GFX_TEXTURE_TYPE_3D;
    case TextureType::TextureCube:
        return GFX_TEXTURE_TYPE_CUBE;
    default:
        return GFX_TEXTURE_TYPE_2D;
    }
}

static GfxTextureViewType cppTextureViewTypeToCType(TextureViewType type)
{
    switch (type) {
    case TextureViewType::View1D:
        return GFX_TEXTURE_VIEW_TYPE_1D;
    case TextureViewType::View2D:
        return GFX_TEXTURE_VIEW_TYPE_2D;
    case TextureViewType::View3D:
        return GFX_TEXTURE_VIEW_TYPE_3D;
    case TextureViewType::ViewCube:
        return GFX_TEXTURE_VIEW_TYPE_CUBE;
    case TextureViewType::View1DArray:
        return GFX_TEXTURE_VIEW_TYPE_1D_ARRAY;
    case TextureViewType::View2DArray:
        return GFX_TEXTURE_VIEW_TYPE_2D_ARRAY;
    case TextureViewType::ViewCubeArray:
        return GFX_TEXTURE_VIEW_TYPE_CUBE_ARRAY;
    default:
        return GFX_TEXTURE_VIEW_TYPE_2D;
    }
}

static GfxWindowingSystem cppWindowingSystemToC(WindowingSystem sys)
{
    switch (sys) {
    case WindowingSystem::Win32:
        return GFX_WINDOWING_SYSTEM_WIN32;
    case WindowingSystem::Xlib:
        return GFX_WINDOWING_SYSTEM_XLIB;
    case WindowingSystem::Wayland:
        return GFX_WINDOWING_SYSTEM_WAYLAND;
    case WindowingSystem::XCB:
        return GFX_WINDOWING_SYSTEM_XCB;
    case WindowingSystem::Metal:
        return GFX_WINDOWING_SYSTEM_METAL;
    case WindowingSystem::Emscripten:
        return GFX_WINDOWING_SYSTEM_EMSCRIPTEN;
    case WindowingSystem::Android:
        return GFX_WINDOWING_SYSTEM_ANDROID;
    default:
        return GFX_WINDOWING_SYSTEM_XLIB;
    }
}

static GfxPlatformWindowHandle cppHandleToCHandle(const PlatformWindowHandle& windowHandle)
{
    GfxPlatformWindowHandle cHandle = {};
    cHandle.windowingSystem = cppWindowingSystemToC(windowHandle.windowingSystem);

    switch (windowHandle.windowingSystem) {
    case WindowingSystem::Win32:
        cHandle.win32.hwnd = windowHandle.handle.win32.hwnd;
        cHandle.win32.hinstance = windowHandle.handle.win32.hinstance;
        break;
    case WindowingSystem::Xlib:
        cHandle.xlib.window = windowHandle.handle.xlib.window;
        cHandle.xlib.display = windowHandle.handle.xlib.display;
        break;
    case WindowingSystem::Wayland:
        cHandle.wayland.surface = windowHandle.handle.wayland.surface;
        cHandle.wayland.display = windowHandle.handle.wayland.display;
        break;
    case WindowingSystem::XCB:
        cHandle.xcb.connection = windowHandle.handle.xcb.connection;
        cHandle.xcb.window = windowHandle.handle.xcb.window;
        break;
    case WindowingSystem::Metal:
        cHandle.metal.layer = windowHandle.handle.metal.layer;
        break;
    case WindowingSystem::Emscripten:
        cHandle.emscripten.canvasSelector = windowHandle.handle.emscripten.canvasSelector;
        break;
    case WindowingSystem::Android:
        cHandle.android.window = windowHandle.handle.android.window;
        break;
    case WindowingSystem::Unknown:
    default:
        // Unknown platform - leave handles null
        break;
    }

    return cHandle;
}

static Result cResultToCppResult(GfxResult result)
{
    switch (result) {
    case GFX_RESULT_SUCCESS:
        return Result::Success;
    case GFX_RESULT_TIMEOUT:
        return Result::Timeout;
    case GFX_RESULT_NOT_READY:
        return Result::NotReady;
    case GFX_RESULT_ERROR_OUT_OF_DATE:
        return Result::OutOfDateKHR;
    default:
        return Result::Error;
    }
}

// Forward declare implementation classes and helper template
class CSemaphoreImpl;
class CFenceImpl;

// Helper template to extract native C handles from C++ wrapper objects
template <typename CHandle>
CHandle extractNativeHandle(std::shared_ptr<void> /* unused */)
{
    return nullptr; // Default implementation
}

// ============================================================================
// C++ Wrapper Classes
// ============================================================================

class CBufferImpl : public Buffer {
public:
    explicit CBufferImpl(GfxBuffer h)
        : m_handle(h)
    {
    }
    ~CBufferImpl() override
    {
        if (m_handle) {
            gfxBufferDestroy(m_handle);
        }
    }

    GfxBuffer getHandle() const { return m_handle; }

    uint64_t getSize() const override { return gfxBufferGetSize(m_handle); }
    BufferUsage getUsage() const override { return static_cast<BufferUsage>(gfxBufferGetUsage(m_handle)); }

    void* map(uint64_t offset = 0, uint64_t size = 0) override
    {
        void* mappedPointer = nullptr;
        GfxResult result = gfxBufferMap(m_handle, offset, size, &mappedPointer);
        if (result != GFX_RESULT_SUCCESS) {
            return nullptr;
        }
        return mappedPointer;
    }

    void unmap() override
    {
        gfxBufferUnmap(m_handle);
    }

private:
    GfxBuffer m_handle;
};

class CTextureViewImpl : public TextureView {
public:
    explicit CTextureViewImpl(GfxTextureView h, bool owns = true)
        : m_handle(h)
        , m_ownsHandle(owns)
    {
    }
    ~CTextureViewImpl() override
    {
        if (m_handle && m_ownsHandle) {
            gfxTextureViewDestroy(m_handle);
        }
    }

    GfxTextureView getHandle() const { return m_handle; }

private:
    GfxTextureView m_handle;
    std::shared_ptr<Texture> m_texture;
    bool m_ownsHandle; // False for swapchain texture views
};

class CTextureImpl : public Texture {
public:
    explicit CTextureImpl(GfxTexture h)
        : m_handle(h)
    {
    }
    ~CTextureImpl() override
    {
        if (m_handle) {
            gfxTextureDestroy(m_handle);
        }
    }

    GfxTexture getHandle() const { return m_handle; }

    Extent3D getSize() const override
    {
        GfxExtent3D size = gfxTextureGetSize(m_handle);
        return Extent3D(size.width, size.height, size.depth);
    }

    TextureFormat getFormat() const override
    {
        return cFormatToCppFormat(gfxTextureGetFormat(m_handle));
    }

    uint32_t getMipLevelCount() const override { return gfxTextureGetMipLevelCount(m_handle); }
    uint32_t getSampleCount() const override { return gfxTextureGetSampleCount(m_handle); }
    TextureUsage getUsage() const override { return static_cast<TextureUsage>(gfxTextureGetUsage(m_handle)); }
    TextureLayout getLayout() const override { return static_cast<TextureLayout>(gfxTextureGetLayout(m_handle)); }

    std::shared_ptr<TextureView> createView(const TextureViewDescriptor& descriptor = {}) override
    {
        GfxTextureViewDescriptor cDesc = {};
        cDesc.label = descriptor.label.c_str();
        cDesc.viewType = cppTextureViewTypeToCType(descriptor.viewType);
        cDesc.format = cppFormatToCFormat(descriptor.format);
        cDesc.baseMipLevel = descriptor.baseMipLevel;
        cDesc.mipLevelCount = descriptor.mipLevelCount;
        cDesc.baseArrayLayer = descriptor.baseArrayLayer;
        cDesc.arrayLayerCount = descriptor.arrayLayerCount;

        GfxTextureView view = nullptr;
        GfxResult result = gfxTextureCreateView(m_handle, &cDesc, &view);
        if (result != GFX_RESULT_SUCCESS || !view) {
            throw std::runtime_error("Failed to create texture view");
        }

        return std::make_shared<CTextureViewImpl>(view, true);
    }

private:
    GfxTexture m_handle;
};

class CSamplerImpl : public Sampler {
public:
    explicit CSamplerImpl(GfxSampler h)
        : m_handle(h)
    {
    }
    ~CSamplerImpl() override
    {
        if (m_handle) {
            gfxSamplerDestroy(m_handle);
        }
    }

    GfxSampler getHandle() const { return m_handle; }

private:
    GfxSampler m_handle;
};

class CShaderImpl : public Shader {
public:
    explicit CShaderImpl(GfxShader h)
        : m_handle(h)
    {
    }
    ~CShaderImpl() override
    {
        if (m_handle) {
            gfxShaderDestroy(m_handle);
        }
    }

    GfxShader getHandle() const { return m_handle; }

private:
    GfxShader m_handle;
};

class CBindGroupLayoutImpl : public BindGroupLayout {
public:
    explicit CBindGroupLayoutImpl(GfxBindGroupLayout h)
        : m_handle(h)
    {
    }
    ~CBindGroupLayoutImpl() override
    {
        if (m_handle) {
            gfxBindGroupLayoutDestroy(m_handle);
        }
    }

    GfxBindGroupLayout getHandle() const { return m_handle; }

private:
    GfxBindGroupLayout m_handle;
};

class CBindGroupImpl : public BindGroup {
public:
    explicit CBindGroupImpl(GfxBindGroup h)
        : m_handle(h)
    {
    }
    ~CBindGroupImpl() override
    {
        if (m_handle) {
            gfxBindGroupDestroy(m_handle);
        }
    }

    GfxBindGroup getHandle() const { return m_handle; }

private:
    GfxBindGroup m_handle;
};

class CRenderPipelineImpl : public RenderPipeline {
public:
    explicit CRenderPipelineImpl(GfxRenderPipeline h)
        : m_handle(h)
    {
    }
    ~CRenderPipelineImpl() override
    {
        if (m_handle) {
            gfxRenderPipelineDestroy(m_handle);
        }
    }

    GfxRenderPipeline getHandle() const { return m_handle; }

private:
    GfxRenderPipeline m_handle;
};

class CComputePipelineImpl : public ComputePipeline {
public:
    explicit CComputePipelineImpl(GfxComputePipeline h)
        : m_handle(h)
    {
    }
    ~CComputePipelineImpl() override
    {
        if (m_handle) {
            gfxComputePipelineDestroy(m_handle);
        }
    }

    GfxComputePipeline getHandle() const { return m_handle; }

private:
    GfxComputePipeline m_handle;
};

class CRenderPassEncoderImpl : public RenderPassEncoder {
public:
    explicit CRenderPassEncoderImpl(GfxRenderPassEncoder h)
        : m_handle(h)
    {
    }
    ~CRenderPassEncoderImpl() override
    {
        if (m_handle) {
            gfxRenderPassEncoderEnd(m_handle);
        }
    }

    void setPipeline(std::shared_ptr<RenderPipeline> pipeline) override
    {
        auto impl = std::dynamic_pointer_cast<CRenderPipelineImpl>(pipeline);
        if (impl) {
            gfxRenderPassEncoderSetPipeline(m_handle, impl->getHandle());
        }
    }

    void setBindGroup(uint32_t index, std::shared_ptr<BindGroup> bindGroup, const uint32_t* dynamicOffsets = nullptr, uint32_t dynamicOffsetCount = 0) override
    {
        auto impl = std::dynamic_pointer_cast<CBindGroupImpl>(bindGroup);
        if (impl) {
            gfxRenderPassEncoderSetBindGroup(m_handle, index, impl->getHandle(), dynamicOffsets, dynamicOffsetCount);
        }
    }

    void setVertexBuffer(uint32_t slot, std::shared_ptr<Buffer> buffer, uint64_t offset = 0, uint64_t size = 0) override
    {
        auto impl = std::dynamic_pointer_cast<CBufferImpl>(buffer);
        if (impl) {
            gfxRenderPassEncoderSetVertexBuffer(m_handle, slot, impl->getHandle(), offset, size);
        }
    }

    void setIndexBuffer(std::shared_ptr<Buffer> buffer, IndexFormat format, uint64_t offset = 0, uint64_t size = UINT64_MAX) override
    {
        auto impl = std::dynamic_pointer_cast<CBufferImpl>(buffer);
        if (impl) {
            GfxIndexFormat cFormat = (format == IndexFormat::Uint16) ? GFX_INDEX_FORMAT_UINT16 : GFX_INDEX_FORMAT_UINT32;
            gfxRenderPassEncoderSetIndexBuffer(m_handle, impl->getHandle(), cFormat, offset, size);
        }
    }

    void setViewport(float x, float y, float width, float height, float minDepth = 0.0f, float maxDepth = 1.0f) override
    {
        GfxViewport viewport = { x, y, width, height, minDepth, maxDepth };
        gfxRenderPassEncoderSetViewport(m_handle, &viewport);
    }

    void setScissorRect(int32_t x, int32_t y, uint32_t width, uint32_t height) override
    {
        GfxScissorRect scissor = { x, y, width, height };
        gfxRenderPassEncoderSetScissorRect(m_handle, &scissor);
    }

    void draw(uint32_t vertexCount, uint32_t instanceCount = 1, uint32_t firstVertex = 0, uint32_t firstInstance = 0) override
    {
        gfxRenderPassEncoderDraw(m_handle, vertexCount, instanceCount, firstVertex, firstInstance);
    }

    void drawIndexed(uint32_t indexCount, uint32_t instanceCount = 1, uint32_t firstIndex = 0, int32_t baseVertex = 0, uint32_t firstInstance = 0) override
    {
        gfxRenderPassEncoderDrawIndexed(m_handle, indexCount, instanceCount, firstIndex, baseVertex, firstInstance);
    }

    void end() override
    {
        if (m_handle) {
            gfxRenderPassEncoderEnd(m_handle);
            m_handle = nullptr;
        }
    }

private:
    GfxRenderPassEncoder m_handle;
};

class CComputePassEncoderImpl : public ComputePassEncoder {
public:
    explicit CComputePassEncoderImpl(GfxComputePassEncoder h)
        : m_handle(h)
    {
    }
    ~CComputePassEncoderImpl() override
    {
        if (m_handle) {
            gfxComputePassEncoderEnd(m_handle);
        }
    }

    void setPipeline(std::shared_ptr<ComputePipeline> pipeline) override
    {
        auto impl = std::dynamic_pointer_cast<CComputePipelineImpl>(pipeline);
        if (impl) {
            gfxComputePassEncoderSetPipeline(m_handle, impl->getHandle());
        }
    }

    void setBindGroup(uint32_t index, std::shared_ptr<BindGroup> bindGroup, const uint32_t* dynamicOffsets = nullptr, uint32_t dynamicOffsetCount = 0) override
    {
        auto impl = std::dynamic_pointer_cast<CBindGroupImpl>(bindGroup);
        if (impl) {
            gfxComputePassEncoderSetBindGroup(m_handle, index, impl->getHandle(), dynamicOffsets, dynamicOffsetCount);
        }
    }

    void dispatchWorkgroups(uint32_t workgroupCountX, uint32_t workgroupCountY = 1, uint32_t workgroupCountZ = 1) override
    {
        gfxComputePassEncoderDispatchWorkgroups(m_handle, workgroupCountX, workgroupCountY, workgroupCountZ);
    }

    void end() override
    {
        if (m_handle) {
            gfxComputePassEncoderEnd(m_handle);
            m_handle = nullptr;
        }
    }

private:
    GfxComputePassEncoder m_handle;
};

class CCommandEncoderImpl : public CommandEncoder {
public:
    explicit CCommandEncoderImpl(GfxCommandEncoder h)
        : m_handle(h)
    {
    }
    ~CCommandEncoderImpl() override
    {
        if (m_handle) {
            gfxCommandEncoderDestroy(m_handle);
        }
    }

    GfxCommandEncoder getHandle() const { return m_handle; }

    std::shared_ptr<RenderPassEncoder> beginRenderPass(const RenderPassDescriptor& descriptor) override
    {
        // Convert C++ descriptor to C descriptor
        std::vector<GfxColorAttachmentTarget> cColorTargets;
        std::vector<GfxColorAttachmentTarget> cResolveTargets;
        std::vector<GfxColorAttachment> cColorAttachments;

        cColorTargets.reserve(descriptor.colorAttachments.size());
        cColorAttachments.reserve(descriptor.colorAttachments.size());

        for (const auto& colorAttachment : descriptor.colorAttachments) {
            // Main target
            GfxColorAttachmentTarget cTarget{};
            auto viewImpl = std::dynamic_pointer_cast<CTextureViewImpl>(colorAttachment.target.view);
            if (viewImpl) {
                cTarget.view = viewImpl->getHandle();
            }
            cTarget.ops.loadOp = static_cast<GfxLoadOp>(colorAttachment.target.ops.loadOp);
            cTarget.ops.storeOp = static_cast<GfxStoreOp>(colorAttachment.target.ops.storeOp);
            cTarget.ops.clearColor = { colorAttachment.target.ops.clearColor.r, colorAttachment.target.ops.clearColor.g,
                colorAttachment.target.ops.clearColor.b, colorAttachment.target.ops.clearColor.a };
            cTarget.finalLayout = static_cast<GfxTextureLayout>(colorAttachment.target.finalLayout);
            cColorTargets.push_back(cTarget);

            GfxColorAttachment cAttachment{};
            cAttachment.target = cColorTargets.back();

            // Handle resolve target if present
            if (colorAttachment.resolveTarget) {
                GfxColorAttachmentTarget cResolveTarget{};
                auto resolveImpl = std::dynamic_pointer_cast<CTextureViewImpl>(colorAttachment.resolveTarget->view);
                if (resolveImpl) {
                    cResolveTarget.view = resolveImpl->getHandle();
                }
                cResolveTarget.ops.loadOp = static_cast<GfxLoadOp>(colorAttachment.resolveTarget->ops.loadOp);
                cResolveTarget.ops.storeOp = static_cast<GfxStoreOp>(colorAttachment.resolveTarget->ops.storeOp);
                cResolveTarget.ops.clearColor = { colorAttachment.resolveTarget->ops.clearColor.r, colorAttachment.resolveTarget->ops.clearColor.g,
                    colorAttachment.resolveTarget->ops.clearColor.b, colorAttachment.resolveTarget->ops.clearColor.a };
                cResolveTarget.finalLayout = static_cast<GfxTextureLayout>(colorAttachment.resolveTarget->finalLayout);
                cResolveTargets.push_back(cResolveTarget);
                cAttachment.resolveTarget = &cResolveTargets.back();
            } else {
                cAttachment.resolveTarget = nullptr;
            }

            cColorAttachments.push_back(cAttachment);
        }

        GfxDepthAttachmentOps cDepthOps{};
        GfxStencilAttachmentOps cStencilOps{};
        GfxDepthStencilAttachmentTarget cDepthTarget{};
        GfxDepthStencilAttachment cDepthStencil{};
        GfxDepthStencilAttachment* cDepthStencilPtr = nullptr;

        if (descriptor.depthStencilAttachment) {
            auto viewImpl = std::dynamic_pointer_cast<CTextureViewImpl>(descriptor.depthStencilAttachment->target.view);
            if (viewImpl) {
                cDepthTarget.view = viewImpl->getHandle();
            }

            // Handle depth ops if present
            if (descriptor.depthStencilAttachment->target.depthOps) {
                cDepthOps.loadOp = static_cast<GfxLoadOp>(descriptor.depthStencilAttachment->target.depthOps->loadOp);
                cDepthOps.storeOp = static_cast<GfxStoreOp>(descriptor.depthStencilAttachment->target.depthOps->storeOp);
                cDepthOps.clearValue = descriptor.depthStencilAttachment->target.depthOps->clearValue;
                cDepthTarget.depthOps = &cDepthOps;
            } else {
                cDepthTarget.depthOps = nullptr;
            }

            // Handle stencil ops if present
            if (descriptor.depthStencilAttachment->target.stencilOps) {
                cStencilOps.loadOp = static_cast<GfxLoadOp>(descriptor.depthStencilAttachment->target.stencilOps->loadOp);
                cStencilOps.storeOp = static_cast<GfxStoreOp>(descriptor.depthStencilAttachment->target.stencilOps->storeOp);
                cStencilOps.clearValue = descriptor.depthStencilAttachment->target.stencilOps->clearValue;
                cDepthTarget.stencilOps = &cStencilOps;
            } else {
                cDepthTarget.stencilOps = nullptr;
            }

            cDepthTarget.finalLayout = static_cast<GfxTextureLayout>(descriptor.depthStencilAttachment->target.finalLayout);
            cDepthStencil.target = cDepthTarget;
            cDepthStencil.resolveTarget = nullptr; // TODO: handle resolve if needed
            cDepthStencilPtr = &cDepthStencil;
        }

        GfxRenderPassDescriptor cDescriptor{};
        cDescriptor.label = descriptor.label.c_str();
        cDescriptor.colorAttachments = cColorAttachments.data();
        cDescriptor.colorAttachmentCount = static_cast<uint32_t>(cColorAttachments.size());
        cDescriptor.depthStencilAttachment = cDepthStencilPtr;

        GfxRenderPassEncoder encoder = nullptr;
        GfxResult result = gfxCommandEncoderBeginRenderPass(m_handle, &cDescriptor, &encoder);

        if (result != GFX_RESULT_SUCCESS || !encoder) {
            throw std::runtime_error("Failed to begin render pass");
        }
        return std::make_shared<CRenderPassEncoderImpl>(encoder);
    }

    std::shared_ptr<ComputePassEncoder> beginComputePass(const ComputePassDescriptor& descriptor) override
    {
        GfxComputePassDescriptor cDesc = {};
        cDesc.label = descriptor.label.c_str();

        GfxComputePassEncoder encoder = nullptr;
        GfxResult result = gfxCommandEncoderBeginComputePass(m_handle, &cDesc, &encoder);
        if (result != GFX_RESULT_SUCCESS || !encoder) {
            throw std::runtime_error("Failed to begin compute pass");
        }
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
            gfxCommandEncoderCopyBufferToBuffer(m_handle, src->getHandle(), sourceOffset,
                dst->getHandle(), destinationOffset, size);
        }
    }

    void copyBufferToTexture(
        std::shared_ptr<Buffer> source, uint64_t sourceOffset, uint32_t bytesPerRow,
        std::shared_ptr<Texture> destination, const Origin3D& origin,
        const Extent3D& extent, uint32_t mipLevel, TextureLayout finalLayout) override
    {
        auto src = std::dynamic_pointer_cast<CBufferImpl>(source);
        auto dst = std::dynamic_pointer_cast<CTextureImpl>(destination);
        if (src && dst) {
            GfxOrigin3D cOrigin = { origin.x, origin.y, origin.z };
            GfxExtent3D cExtent = { extent.width, extent.height, extent.depth };
            gfxCommandEncoderCopyBufferToTexture(m_handle, src->getHandle(), sourceOffset, bytesPerRow,
                dst->getHandle(), &cOrigin, &cExtent, mipLevel, static_cast<GfxTextureLayout>(finalLayout));
        }
    }

    void copyTextureToBuffer(
        std::shared_ptr<Texture> source, const Origin3D& origin, uint32_t mipLevel,
        std::shared_ptr<Buffer> destination, uint64_t destinationOffset, uint32_t bytesPerRow,
        const Extent3D& extent, TextureLayout finalLayout) override
    {
        auto src = std::dynamic_pointer_cast<CTextureImpl>(source);
        auto dst = std::dynamic_pointer_cast<CBufferImpl>(destination);
        if (src && dst) {
            GfxOrigin3D cOrigin = { origin.x, origin.y, origin.z };
            GfxExtent3D cExtent = { extent.width, extent.height, extent.depth };
            gfxCommandEncoderCopyTextureToBuffer(m_handle, src->getHandle(), &cOrigin, mipLevel,
                dst->getHandle(), destinationOffset, bytesPerRow, &cExtent, static_cast<GfxTextureLayout>(finalLayout));
        }
    }

    void copyTextureToTexture(
        std::shared_ptr<Texture> source, const Origin3D& sourceOrigin, uint32_t sourceMipLevel,
        std::shared_ptr<Texture> destination, const Origin3D& destinationOrigin, uint32_t destinationMipLevel,
        const Extent3D& extent, TextureLayout sourceFinalLayout, TextureLayout destinationFinalLayout) override
    {
        auto src = std::dynamic_pointer_cast<CTextureImpl>(source);
        auto dst = std::dynamic_pointer_cast<CTextureImpl>(destination);
        if (src && dst) {
            GfxOrigin3D cSourceOrigin = { sourceOrigin.x, sourceOrigin.y, sourceOrigin.z };
            GfxOrigin3D cDestOrigin = { destinationOrigin.x, destinationOrigin.y, destinationOrigin.z };
            GfxExtent3D cExtent = { extent.width, extent.height, extent.depth };
            gfxCommandEncoderCopyTextureToTexture(m_handle,
                src->getHandle(), &cSourceOrigin, sourceMipLevel,
                dst->getHandle(), &cDestOrigin, destinationMipLevel,
                &cExtent, static_cast<GfxTextureLayout>(sourceFinalLayout), static_cast<GfxTextureLayout>(destinationFinalLayout));
        }
    }

    void pipelineBarrier(
        const std::vector<MemoryBarrier>& memoryBarriers = {},
        const std::vector<BufferBarrier>& bufferBarriers = {},
        const std::vector<TextureBarrier>& textureBarriers = {}) override
    {
        if (memoryBarriers.empty() && bufferBarriers.empty() && textureBarriers.empty()) {
            return;
        }

        std::vector<GfxMemoryBarrier> cMemoryBarriers;
        cMemoryBarriers.reserve(memoryBarriers.size());

        for (const auto& barrier : memoryBarriers) {
            GfxMemoryBarrier cBarrier{};
            cBarrier.srcStageMask = static_cast<GfxPipelineStage>(barrier.srcStageMask);
            cBarrier.dstStageMask = static_cast<GfxPipelineStage>(barrier.dstStageMask);
            cBarrier.srcAccessMask = static_cast<GfxAccessFlags>(barrier.srcAccessMask);
            cBarrier.dstAccessMask = static_cast<GfxAccessFlags>(barrier.dstAccessMask);
            cMemoryBarriers.push_back(cBarrier);
        }

        std::vector<GfxBufferBarrier> cBufferBarriers;
        cBufferBarriers.reserve(bufferBarriers.size());

        for (const auto& barrier : bufferBarriers) {
            auto buf = std::dynamic_pointer_cast<CBufferImpl>(barrier.buffer);
            if (buf) {
                GfxBufferBarrier cBarrier{};
                cBarrier.buffer = buf->getHandle();
                cBarrier.srcStageMask = static_cast<GfxPipelineStage>(barrier.srcStageMask);
                cBarrier.dstStageMask = static_cast<GfxPipelineStage>(barrier.dstStageMask);
                cBarrier.srcAccessMask = static_cast<GfxAccessFlags>(barrier.srcAccessMask);
                cBarrier.dstAccessMask = static_cast<GfxAccessFlags>(barrier.dstAccessMask);
                cBarrier.offset = barrier.offset;
                cBarrier.size = barrier.size;
                cBufferBarriers.push_back(cBarrier);
            }
        }

        std::vector<GfxTextureBarrier> cTextureBarriers;
        cTextureBarriers.reserve(textureBarriers.size());

        for (const auto& barrier : textureBarriers) {
            auto tex = std::dynamic_pointer_cast<CTextureImpl>(barrier.texture);
            if (tex) {
                GfxTextureBarrier cBarrier{};
                cBarrier.texture = tex->getHandle();
                cBarrier.oldLayout = static_cast<GfxTextureLayout>(barrier.oldLayout);
                cBarrier.newLayout = static_cast<GfxTextureLayout>(barrier.newLayout);
                cBarrier.srcStageMask = static_cast<GfxPipelineStage>(barrier.srcStageMask);
                cBarrier.dstStageMask = static_cast<GfxPipelineStage>(barrier.dstStageMask);

                // Auto-deduce access masks if not explicitly set
                cBarrier.srcAccessMask = (barrier.srcAccessMask == AccessFlags::None)
                    ? gfxGetAccessFlagsForLayout(cBarrier.oldLayout)
                    : static_cast<GfxAccessFlags>(barrier.srcAccessMask);
                cBarrier.dstAccessMask = (barrier.dstAccessMask == AccessFlags::None)
                    ? gfxGetAccessFlagsForLayout(cBarrier.newLayout)
                    : static_cast<GfxAccessFlags>(barrier.dstAccessMask);

                cBarrier.baseMipLevel = barrier.baseMipLevel;
                cBarrier.mipLevelCount = barrier.mipLevelCount;
                cBarrier.baseArrayLayer = barrier.baseArrayLayer;
                cBarrier.arrayLayerCount = barrier.arrayLayerCount;
                cTextureBarriers.push_back(cBarrier);
            }
        }

        if (!cMemoryBarriers.empty() || !cBufferBarriers.empty() || !cTextureBarriers.empty()) {
            gfxCommandEncoderPipelineBarrier(m_handle,
                cMemoryBarriers.empty() ? nullptr : cMemoryBarriers.data(),
                static_cast<uint32_t>(cMemoryBarriers.size()),
                cBufferBarriers.empty() ? nullptr : cBufferBarriers.data(),
                static_cast<uint32_t>(cBufferBarriers.size()),
                cTextureBarriers.empty() ? nullptr : cTextureBarriers.data(),
                static_cast<uint32_t>(cTextureBarriers.size()));
        }
    }

    void end() override
    {
        gfxCommandEncoderEnd(m_handle);
    }

    void begin() override
    {
        gfxCommandEncoderBegin(m_handle);
    }

private:
    GfxCommandEncoder m_handle;
};

class CFenceImpl : public Fence {
public:
    explicit CFenceImpl(GfxFence h)
        : m_handle(h)
    {
    }
    ~CFenceImpl() override
    {
        if (m_handle) {
            gfxFenceDestroy(m_handle);
        }
    }

    GfxFence getHandle() const { return m_handle; }

    FenceStatus getStatus() const override
    {
        bool signaled;
        gfxFenceGetStatus(m_handle, &signaled);
        return signaled ? FenceStatus::Signaled : FenceStatus::Unsignaled;
    }

    bool wait(uint64_t timeoutNanoseconds = UINT64_MAX) override
    {
        return gfxFenceWait(m_handle, timeoutNanoseconds) == GFX_RESULT_SUCCESS;
    }

    void reset() override
    {
        gfxFenceReset(m_handle);
    }

private:
    GfxFence m_handle;
};

class CSemaphoreImpl : public Semaphore {
public:
    explicit CSemaphoreImpl(GfxSemaphore h)
        : m_handle(h)
    {
    }
    ~CSemaphoreImpl() override
    {
        if (m_handle) {
            gfxSemaphoreDestroy(m_handle);
        }
    }

    GfxSemaphore getHandle() const { return m_handle; }

    SemaphoreType getType() const override
    {
        return static_cast<SemaphoreType>(gfxSemaphoreGetType(m_handle));
    }

    uint64_t getValue() const override
    {
        return gfxSemaphoreGetValue(m_handle);
    }

    void signal(uint64_t value) override
    {
        gfxSemaphoreSignal(m_handle, value);
    }

    bool wait(uint64_t value, uint64_t timeoutNanoseconds = UINT64_MAX) override
    {
        return gfxSemaphoreWait(m_handle, value, timeoutNanoseconds) == GFX_RESULT_SUCCESS;
    }

private:
    GfxSemaphore m_handle;
};

// Template specializations for extractNativeHandle (must come after class definitions)
template <>
inline GfxSemaphore extractNativeHandle<GfxSemaphore>(std::shared_ptr<void> ptr)
{
    if (!ptr) {
        return nullptr;
    }
    auto impl = std::static_pointer_cast<CSemaphoreImpl>(ptr);
    return impl->getHandle();
}

template <>
inline GfxFence extractNativeHandle<GfxFence>(std::shared_ptr<void> ptr)
{
    if (!ptr) {
        return nullptr;
    }
    auto impl = std::static_pointer_cast<CFenceImpl>(ptr);
    return impl->getHandle();
}

class CQueueImpl : public Queue {
public:
    explicit CQueueImpl(GfxQueue h)
        : m_handle(h)
    {
    }
    // Queue is owned by device, do not destroy
    ~CQueueImpl() override = default;

    void submit(const SubmitInfo& submitInfo) override
    {
        // Convert C++ structures to C
        std::vector<GfxCommandEncoder> cEncoders;
        for (auto& encoder : submitInfo.commandEncoders) {
            auto impl = std::dynamic_pointer_cast<CCommandEncoderImpl>(encoder);
            if (impl) {
                cEncoders.push_back(impl->getHandle());
            }
        }

        std::vector<GfxSemaphore> cWaitSems;
        for (auto& sem : submitInfo.waitSemaphores) {
            auto impl = std::dynamic_pointer_cast<CSemaphoreImpl>(sem);
            if (impl) {
                cWaitSems.push_back(impl->getHandle());
            }
        }

        std::vector<GfxSemaphore> cSignalSems;
        for (auto& sem : submitInfo.signalSemaphores) {
            auto impl = std::dynamic_pointer_cast<CSemaphoreImpl>(sem);
            if (impl) {
                cSignalSems.push_back(impl->getHandle());
            }
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
                cInfo.signalFence = fenceImpl->getHandle();
            }
        } else {
            cInfo.signalFence = nullptr;
        }

        gfxQueueSubmit(m_handle, &cInfo);
    }

    void writeBuffer(std::shared_ptr<Buffer> buffer, uint64_t offset, const void* data, uint64_t size) override
    {
        auto impl = std::dynamic_pointer_cast<CBufferImpl>(buffer);
        if (impl) {
            gfxQueueWriteBuffer(m_handle, impl->getHandle(), offset, data, size);
        }
    }

    void writeTexture(
        std::shared_ptr<Texture> texture, const Origin3D& origin, uint32_t mipLevel,
        const void* data, uint64_t dataSize, uint32_t bytesPerRow,
        const Extent3D& extent, TextureLayout finalLayout) override
    {
        auto impl = std::dynamic_pointer_cast<CTextureImpl>(texture);
        if (impl) {
            GfxOrigin3D cOrigin = { origin.x, origin.y, origin.z };
            GfxExtent3D cExtent = { extent.width, extent.height, extent.depth };
            gfxQueueWriteTexture(m_handle, impl->getHandle(), &cOrigin, mipLevel,
                data, dataSize, bytesPerRow, &cExtent, static_cast<GfxTextureLayout>(finalLayout));
        }
    }

    void waitIdle() override
    {
        gfxQueueWaitIdle(m_handle);
    }

private:
    GfxQueue m_handle;
};

class CSurfaceImpl : public Surface {
public:
    explicit CSurfaceImpl(GfxSurface h)
        : m_handle(h)
    {
    }
    ~CSurfaceImpl() override
    {
        if (m_handle) {
            gfxSurfaceDestroy(m_handle);
        }
    }

    GfxSurface getHandle() const { return m_handle; }

    std::vector<TextureFormat> getSupportedFormats() const override
    {
        GfxTextureFormat formats[16];
        uint32_t count = gfxSurfaceGetSupportedFormats(m_handle, formats, 16);
        std::vector<TextureFormat> result;
        for (uint32_t i = 0; i < count; ++i) {
            result.push_back(cFormatToCppFormat(formats[i]));
        }
        return result;
    }

    std::vector<PresentMode> getSupportedPresentModes() const override
    {
        GfxPresentMode modes[8];
        uint32_t count = gfxSurfaceGetSupportedPresentModes(m_handle, modes, 8);
        std::vector<PresentMode> result;
        for (uint32_t i = 0; i < count; ++i) {
            result.push_back(static_cast<PresentMode>(modes[i]));
        }
        return result;
    }

private:
    GfxSurface m_handle;
};

class CSwapchainImpl : public Swapchain {
public:
    explicit CSwapchainImpl(GfxSwapchain h)
        : m_handle(h)
    {
    }
    ~CSwapchainImpl() override
    {
        if (m_handle) {
            gfxSwapchainDestroy(m_handle);
        }
    }

    uint32_t getWidth() const override { return gfxSwapchainGetWidth(m_handle); }
    uint32_t getHeight() const override { return gfxSwapchainGetHeight(m_handle); }
    TextureFormat getFormat() const override { return cFormatToCppFormat(gfxSwapchainGetFormat(m_handle)); }
    uint32_t getBufferCount() const override { return gfxSwapchainGetBufferCount(m_handle); }

    std::shared_ptr<TextureView> getCurrentTextureView() override
    {
        GfxTextureView view = gfxSwapchainGetCurrentTextureView(m_handle);
        if (!view) {
            return nullptr;
        }
        // Swapchain texture views are owned by the swapchain, not by the wrapper
        return std::make_shared<CTextureViewImpl>(view, false);
    }

    Result acquireNextImage(uint64_t timeout,
        std::shared_ptr<Semaphore> signalSemaphore,
        std::shared_ptr<Fence> signalFence,
        uint32_t* imageIndex) override
    {
        GfxSemaphore cSemaphore = signalSemaphore ? extractNativeHandle<GfxSemaphore>(signalSemaphore) : nullptr;
        GfxFence cFence = signalFence ? extractNativeHandle<GfxFence>(signalFence) : nullptr;

        GfxResult result = gfxSwapchainAcquireNextImage(m_handle, timeout, cSemaphore, cFence, imageIndex);
        return cResultToCppResult(result);
    }

    std::shared_ptr<TextureView> getImageView(uint32_t index) override
    {
        GfxTextureView view = gfxSwapchainGetImageView(m_handle, index);
        if (!view) {
            return nullptr;
        }
        // Swapchain texture views are now cached by the backend (both Vulkan and WebGPU)
        // The view is owned by the swapchain, not by this wrapper
        return std::make_shared<CTextureViewImpl>(view, false);
    }

    Result present(const PresentInfo& info) override
    {
        GfxPresentInfo cInfo = {};

        // Convert semaphores
        std::vector<GfxSemaphore> cWaitSemaphores;

        for (const auto& sem : info.waitSemaphores) {
            cWaitSemaphores.push_back(extractNativeHandle<GfxSemaphore>(sem));
        }

        cInfo.waitSemaphores = cWaitSemaphores.empty() ? nullptr : cWaitSemaphores.data();
        cInfo.waitSemaphoreCount = cWaitSemaphores.size();

        GfxResult result = gfxSwapchainPresent(m_handle, &cInfo);
        return cResultToCppResult(result);
    }

private:
    GfxSwapchain m_handle;
};

class CDeviceImpl : public Device {
public:
    explicit CDeviceImpl(GfxDevice h)
        : m_handle(h)
    {
        GfxQueue queueHandle = gfxDeviceGetQueue(m_handle);
        m_queue = std::make_shared<CQueueImpl>(queueHandle);
    }
    ~CDeviceImpl() override
    {
        if (m_handle) {
            gfxDeviceWaitIdle(m_handle);
            gfxDeviceDestroy(m_handle);
        }
    }

    std::shared_ptr<Queue> getQueue() override { return m_queue; }

    std::shared_ptr<Surface> createSurface(const SurfaceDescriptor& descriptor) override
    {
        GfxSurfaceDescriptor cDesc = {};
        cDesc.label = descriptor.label.c_str();
        cDesc.windowHandle = cppHandleToCHandle(descriptor.windowHandle);

        GfxSurface surface = nullptr;
        GfxResult result = gfxDeviceCreateSurface(m_handle, &cDesc, &surface);
        if (result != GFX_RESULT_SUCCESS || !surface) {
            throw std::runtime_error("Failed to create surface");
        }
        return std::make_shared<CSurfaceImpl>(surface);
    }

    std::shared_ptr<Swapchain> createSwapchain(
        std::shared_ptr<Surface> surface,
        const SwapchainDescriptor& descriptor) override
    {
        auto surfaceImpl = std::dynamic_pointer_cast<CSurfaceImpl>(surface);
        if (!surfaceImpl) {
            throw std::runtime_error("Invalid surface type");
        }

        GfxSwapchainDescriptor cDesc = {};
        cDesc.label = descriptor.label.c_str();
        cDesc.width = descriptor.width;
        cDesc.height = descriptor.height;
        cDesc.format = cppFormatToCFormat(descriptor.format);
        cDesc.usage = cppTextureUsageToCUsage(descriptor.usage);
        cDesc.presentMode = static_cast<GfxPresentMode>(descriptor.presentMode);
        cDesc.bufferCount = descriptor.bufferCount;

        GfxSwapchain swapchain = nullptr;
        GfxResult result = gfxDeviceCreateSwapchain(m_handle, surfaceImpl->getHandle(), &cDesc, &swapchain);
        if (result != GFX_RESULT_SUCCESS || !swapchain) {
            throw std::runtime_error("Failed to create swapchain");
        }
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
        GfxResult result = gfxDeviceCreateBuffer(m_handle, &cDesc, &buffer);
        if (result != GFX_RESULT_SUCCESS || !buffer) {
            throw std::runtime_error("Failed to create buffer");
        }
        return std::make_shared<CBufferImpl>(buffer);
    }

    std::shared_ptr<Texture> createTexture(const TextureDescriptor& descriptor) override
    {
        GfxTextureDescriptor cDesc = {};
        cDesc.label = descriptor.label.c_str();
        cDesc.type = cppTextureTypeToCType(descriptor.type);
        cDesc.size = { descriptor.size.width, descriptor.size.height, descriptor.size.depth };
        cDesc.arrayLayerCount = descriptor.arrayLayerCount;
        cDesc.mipLevelCount = descriptor.mipLevelCount;
        cDesc.sampleCount = cppSampleCountToCCount(descriptor.sampleCount);
        cDesc.format = cppFormatToCFormat(descriptor.format);
        cDesc.usage = cppTextureUsageToCUsage(descriptor.usage);

        GfxTexture texture = nullptr;
        GfxResult result = gfxDeviceCreateTexture(m_handle, &cDesc, &texture);
        if (result != GFX_RESULT_SUCCESS || !texture) {
            throw std::runtime_error("Failed to create texture");
        }
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
        cDesc.compare = static_cast<GfxCompareFunction>(descriptor.compare);

        GfxSampler sampler = nullptr;
        GfxResult result = gfxDeviceCreateSampler(m_handle, &cDesc, &sampler);
        if (result != GFX_RESULT_SUCCESS || !sampler) {
            throw std::runtime_error("Failed to create sampler");
        }
        return std::make_shared<CSamplerImpl>(sampler);
    }

    std::shared_ptr<Shader> createShader(const ShaderDescriptor& descriptor) override
    {
        GfxShaderDescriptor cDesc = {};
        cDesc.label = descriptor.label.c_str();
        cDesc.sourceType = static_cast<GfxShaderSourceType>(descriptor.sourceType);
        cDesc.code = descriptor.code.c_str();
        cDesc.codeSize = descriptor.code.size(); // Set the actual binary size
        cDesc.entryPoint = descriptor.entryPoint.c_str();

        GfxShader shader = nullptr;
        GfxResult result = gfxDeviceCreateShader(m_handle, &cDesc, &shader);
        if (result != GFX_RESULT_SUCCESS || !shader) {
            throw std::runtime_error("Failed to create shader");
        }
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
                cEntries[i].texture.viewDimension = cppTextureViewTypeToCType(texture.viewDimension);
            } else if (entry.resource.index() == 3) {
                // Storage texture binding
                cEntries[i].type = GFX_BINDING_TYPE_STORAGE_TEXTURE;
                const auto& storageTexture = std::get<BindGroupLayoutEntry::StorageTextureBinding>(entry.resource);
                cEntries[i].storageTexture.format = cppFormatToCFormat(storageTexture.format);
                cEntries[i].storageTexture.writeOnly = storageTexture.writeOnly;
                cEntries[i].storageTexture.viewDimension = cppTextureViewTypeToCType(storageTexture.viewDimension);
            }
        }

        GfxBindGroupLayoutDescriptor cDesc = {};
        cDesc.label = descriptor.label.c_str();
        cDesc.entries = cEntries.data();
        cDesc.entryCount = static_cast<uint32_t>(cEntries.size());

        GfxBindGroupLayout layout = nullptr;
        GfxResult result = gfxDeviceCreateBindGroupLayout(m_handle, &cDesc, &layout);
        if (result != GFX_RESULT_SUCCESS || !layout) {
            throw std::runtime_error("Failed to create bind group layout");
        }
        return std::make_shared<CBindGroupLayoutImpl>(layout);
    }

    std::shared_ptr<BindGroup> createBindGroup(const BindGroupDescriptor& descriptor) override
    {
        auto layoutImpl = std::dynamic_pointer_cast<CBindGroupLayoutImpl>(descriptor.layout);
        if (!layoutImpl) {
            throw std::runtime_error("Invalid bind group layout type");
        }

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
        GfxResult result = gfxDeviceCreateBindGroup(m_handle, &cDesc, &bindGroup);
        if (result != GFX_RESULT_SUCCESS || !bindGroup) {
            throw std::runtime_error("Failed to create bind group");
        }
        return std::make_shared<CBindGroupImpl>(bindGroup);
    }

    std::shared_ptr<RenderPipeline> createRenderPipeline(const RenderPipelineDescriptor& descriptor) override
    {
        // Extract shader handles
        auto vertexShaderImpl = std::dynamic_pointer_cast<CShaderImpl>(descriptor.vertex.module);
        if (!vertexShaderImpl) {
            throw std::runtime_error("Invalid vertex shader type");
        }

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
            if (!fragmentShaderImpl) {
                throw std::runtime_error("Invalid fragment shader type");
            }

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
        cPrimitiveState.frontFace = static_cast<GfxFrontFace>(descriptor.primitive.frontFace);
        cPrimitiveState.cullMode = static_cast<GfxCullMode>(descriptor.primitive.cullMode);
        cPrimitiveState.polygonMode = static_cast<GfxPolygonMode>(descriptor.primitive.polygonMode);

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
        cDesc.sampleCount = cppSampleCountToCCount(descriptor.sampleCount);

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
        GfxResult result = gfxDeviceCreateRenderPipeline(m_handle, &cDesc, &pipeline);
        if (result != GFX_RESULT_SUCCESS || !pipeline) {
            throw std::runtime_error("Failed to create render pipeline");
        }
        return std::make_shared<CRenderPipelineImpl>(pipeline);
    }

    std::shared_ptr<ComputePipeline> createComputePipeline(const ComputePipelineDescriptor& descriptor) override
    {
        auto shaderImpl = std::dynamic_pointer_cast<CShaderImpl>(descriptor.compute);
        if (!shaderImpl) {
            throw std::runtime_error("Invalid shader type");
        }

        // Convert bind group layouts to C handles
        std::vector<GfxBindGroupLayout> bindGroupLayoutHandles;
        for (const auto& layout : descriptor.bindGroupLayouts) {
            auto layoutImpl = std::dynamic_pointer_cast<CBindGroupLayoutImpl>(layout);
            if (layoutImpl) {
                bindGroupLayoutHandles.push_back(layoutImpl->getHandle());
            }
        }

        GfxComputePipelineDescriptor cDesc = {};
        cDesc.label = descriptor.label.c_str();
        cDesc.compute = shaderImpl->getHandle();
        cDesc.entryPoint = descriptor.entryPoint.c_str();
        cDesc.bindGroupLayouts = bindGroupLayoutHandles.empty() ? nullptr : bindGroupLayoutHandles.data();
        cDesc.bindGroupLayoutCount = static_cast<uint32_t>(bindGroupLayoutHandles.size());

        GfxComputePipeline pipeline = nullptr;
        GfxResult result = gfxDeviceCreateComputePipeline(m_handle, &cDesc, &pipeline);
        if (result != GFX_RESULT_SUCCESS || !pipeline) {
            throw std::runtime_error("Failed to create compute pipeline");
        }
        return std::make_shared<CComputePipelineImpl>(pipeline);
    }

    std::shared_ptr<CommandEncoder> createCommandEncoder(const CommandEncoderDescriptor& descriptor = {}) override
    {
        GfxCommandEncoderDescriptor cDesc = {};
        cDesc.label = descriptor.label.c_str();

        GfxCommandEncoder encoder = nullptr;
        GfxResult result = gfxDeviceCreateCommandEncoder(m_handle, &cDesc, &encoder);
        if (result != GFX_RESULT_SUCCESS || !encoder) {
            throw std::runtime_error("Failed to create command encoder");
        }
        return std::make_shared<CCommandEncoderImpl>(encoder);
    }

    std::shared_ptr<Fence> createFence(const FenceDescriptor& descriptor = {}) override
    {
        GfxFenceDescriptor cDesc = {};
        cDesc.label = descriptor.label.c_str();
        cDesc.signaled = descriptor.signaled;

        GfxFence fence = nullptr;
        GfxResult result = gfxDeviceCreateFence(m_handle, &cDesc, &fence);
        if (result != GFX_RESULT_SUCCESS || !fence) {
            throw std::runtime_error("Failed to create fence");
        }
        return std::make_shared<CFenceImpl>(fence);
    }

    std::shared_ptr<Semaphore> createSemaphore(const SemaphoreDescriptor& descriptor = {}) override
    {
        GfxSemaphoreDescriptor cDesc = {};
        cDesc.label = descriptor.label.c_str();
        cDesc.type = static_cast<GfxSemaphoreType>(descriptor.type);
        cDesc.initialValue = descriptor.initialValue;

        GfxSemaphore semaphore = nullptr;
        GfxResult result = gfxDeviceCreateSemaphore(m_handle, &cDesc, &semaphore);
        if (result != GFX_RESULT_SUCCESS || !semaphore) {
            throw std::runtime_error("Failed to create semaphore");
        }
        return std::make_shared<CSemaphoreImpl>(semaphore);
    }

    void waitIdle() override
    {
        gfxDeviceWaitIdle(m_handle);
    }

    DeviceLimits getLimits() const override
    {
        GfxDeviceLimits cLimits;
        gfxDeviceGetLimits(m_handle, &cLimits);

        DeviceLimits limits;
        limits.minUniformBufferOffsetAlignment = cLimits.minUniformBufferOffsetAlignment;
        limits.minStorageBufferOffsetAlignment = cLimits.minStorageBufferOffsetAlignment;
        limits.maxUniformBufferBindingSize = cLimits.maxUniformBufferBindingSize;
        limits.maxStorageBufferBindingSize = cLimits.maxStorageBufferBindingSize;
        limits.maxBufferSize = cLimits.maxBufferSize;
        limits.maxTextureDimension1D = cLimits.maxTextureDimension1D;
        limits.maxTextureDimension2D = cLimits.maxTextureDimension2D;
        limits.maxTextureDimension3D = cLimits.maxTextureDimension3D;
        limits.maxTextureArrayLayers = cLimits.maxTextureArrayLayers;
        return limits;
    }

private:
    GfxDevice m_handle;
    std::shared_ptr<CQueueImpl> m_queue;
};

class CAdapterImpl : public Adapter {
public:
    explicit CAdapterImpl(GfxAdapter h)
        : m_handle(h)
    {
    }
    ~CAdapterImpl() override
    {
        if (m_handle) {
            gfxAdapterDestroy(m_handle);
        }
    }

    std::shared_ptr<Device> createDevice(const DeviceDescriptor& descriptor = {}) override
    {
        GfxDeviceDescriptor cDesc = {};
        cDesc.label = descriptor.label.c_str();
        // Convert required features if needed

        GfxDevice device = nullptr;
        GfxResult result = gfxAdapterCreateDevice(m_handle, &cDesc, &device);
        if (result != GFX_RESULT_SUCCESS || !device) {
            throw std::runtime_error("Failed to create device");
        }
        return std::make_shared<CDeviceImpl>(device);
    }

    std::string getName() const override
    {
        const char* name = gfxAdapterGetName(m_handle);
        return name ? name : "Unknown";
    }

    Backend getBackend() const override
    {
        return cBackendToCppBackend(gfxAdapterGetBackend(m_handle));
    }

    DeviceLimits getLimits() const override
    {
        GfxDeviceLimits cLimits = {};
        gfxAdapterGetLimits(m_handle, &cLimits);

        DeviceLimits limits;
        limits.minUniformBufferOffsetAlignment = cLimits.minUniformBufferOffsetAlignment;
        limits.minStorageBufferOffsetAlignment = cLimits.minStorageBufferOffsetAlignment;
        limits.maxUniformBufferBindingSize = cLimits.maxUniformBufferBindingSize;
        limits.maxStorageBufferBindingSize = cLimits.maxStorageBufferBindingSize;
        limits.maxBufferSize = cLimits.maxBufferSize;
        limits.maxTextureDimension1D = cLimits.maxTextureDimension1D;
        limits.maxTextureDimension2D = cLimits.maxTextureDimension2D;
        limits.maxTextureDimension3D = cLimits.maxTextureDimension3D;
        limits.maxTextureArrayLayers = cLimits.maxTextureArrayLayers;
        return limits;
    }

private:
    GfxAdapter m_handle;
};

class CInstanceImpl : public Instance {
public:
    explicit CInstanceImpl(GfxInstance h)
        : m_handle(h)
    {
    }
    ~CInstanceImpl() override
    {
        if (m_handle) {
            gfxInstanceDestroy(m_handle);
        }
    }

    std::shared_ptr<Adapter> requestAdapter(const AdapterDescriptor& descriptor = {}) override
    {
        GfxAdapterDescriptor cDesc = {};
        cDesc.preference = static_cast<GfxAdapterPreference>(descriptor.preference);

        GfxAdapter adapter = nullptr;
        GfxResult result = gfxInstanceRequestAdapter(m_handle, &cDesc, &adapter);
        if (result != GFX_RESULT_SUCCESS || !adapter) {
            throw std::runtime_error("Failed to request adapter");
        }
        return std::make_shared<CAdapterImpl>(adapter);
    }

    std::vector<std::shared_ptr<Adapter>> enumerateAdapters() override
    {
        GfxAdapter adapters[16];
        uint32_t count = gfxInstanceEnumerateAdapters(m_handle, adapters, 16);

        std::vector<std::shared_ptr<Adapter>> result;
        for (uint32_t i = 0; i < count; ++i) {
            result.push_back(std::make_shared<CAdapterImpl>(adapters[i]));
        }
        return result;
    }

    void setDebugCallback(DebugCallback callback) override
    {
        m_debugCallback = callback;

        if (callback) {
            // Set C callback that forwards to C++ callback
            gfxInstanceSetDebugCallback(m_handle, [](GfxDebugMessageSeverity severity, GfxDebugMessageType type, const char* message, void* userData) {
                    auto* self = static_cast<CInstanceImpl*>(userData);
                    if (self && self->m_debugCallback) {
                        self->m_debugCallback(
                            static_cast<DebugMessageSeverity>(severity),
                            static_cast<DebugMessageType>(type),
                            message
                        );
                    } }, this);
        } else {
            gfxInstanceSetDebugCallback(m_handle, nullptr, nullptr);
        }
    }

private:
    GfxInstance m_handle;
    DebugCallback m_debugCallback;
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
    cDesc.enabledHeadless = descriptor.enabledHeadless;
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

namespace utils {

    uint64_t alignUp(uint64_t value, uint64_t alignment)
    {
        return gfxAlignUp(value, alignment);
    }

    uint64_t alignDown(uint64_t value, uint64_t alignment)
    {
        return gfxAlignDown(value, alignment);
    }

} // namespace utils

} // namespace gfx