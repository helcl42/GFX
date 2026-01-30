#include "Validations.h"

#include <cstdint>

namespace gfx::backend::webgpu::validator {

namespace {

    // ============================================================================
    // Internal descriptor validation functions
    // ============================================================================

    GfxResult validateInstanceDescriptor(const GfxInstanceDescriptor* descriptor)
    {
        // Descriptor is optional
        if (!descriptor) {
            return GFX_RESULT_SUCCESS;
        }

        // All fields are optional - no specific validation needed
        // applicationName, applicationVersion, enabledExtensions are all optional
        return GFX_RESULT_SUCCESS;
    }

    GfxResult validateAdapterDescriptor(const GfxAdapterDescriptor* descriptor)
    {
        // Descriptor is optional
        if (!descriptor) {
            return GFX_RESULT_SUCCESS;
        }

        // All fields are optional - no specific validation needed
        // adapterIndex and preference are both valid selection criteria
        return GFX_RESULT_SUCCESS;
    }

    GfxResult validateSwapchainDescriptor(const GfxSwapchainDescriptor* descriptor)
    {
        if (!descriptor) {
            return GFX_RESULT_ERROR_INVALID_ARGUMENT;
        }

        // Validate dimensions
        if (descriptor->width == 0 || descriptor->height == 0) {
            return GFX_RESULT_ERROR_INVALID_ARGUMENT;
        }

        // Validate image count
        if (descriptor->imageCount == 0) {
            return GFX_RESULT_ERROR_INVALID_ARGUMENT;
        }

        // Validate format
        if (descriptor->format == GFX_TEXTURE_FORMAT_UNDEFINED) {
            return GFX_RESULT_ERROR_INVALID_ARGUMENT;
        }

        // Validate usage flags
        if (descriptor->usage == 0) {
            return GFX_RESULT_ERROR_INVALID_ARGUMENT;
        }

        // Validate present mode
        if (descriptor->presentMode < GFX_PRESENT_MODE_IMMEDIATE || descriptor->presentMode > GFX_PRESENT_MODE_FIFO_RELAXED) {
            return GFX_RESULT_ERROR_INVALID_ARGUMENT;
        }

        return GFX_RESULT_SUCCESS;
    }

    GfxResult validateDeviceDescriptor(const GfxDeviceDescriptor* descriptor)
    {
        // Descriptor is optional - NULL means use defaults
        if (!descriptor) {
            return GFX_RESULT_SUCCESS;
        }

        // Validate queueRequests and queueRequestCount consistency
        if (descriptor->queueRequests != nullptr && descriptor->queueRequestCount == 0) {
            return GFX_RESULT_ERROR_INVALID_ARGUMENT;
        }
        if (descriptor->queueRequests == nullptr && descriptor->queueRequestCount != 0) {
            return GFX_RESULT_ERROR_INVALID_ARGUMENT;
        }

        // Validate individual queue requests
        if (descriptor->queueRequests != nullptr) {
            for (uint32_t i = 0; i < descriptor->queueRequestCount; ++i) {
                const GfxQueueRequest& request = descriptor->queueRequests[i];

                // Validate priority (0.0 to 1.0)
                if (request.priority < 0.0f || request.priority > 1.0f) {
                    return GFX_RESULT_ERROR_INVALID_ARGUMENT;
                }
            }
        }

        // Validate enabledExtensions and enabledExtensionCount consistency
        if (descriptor->enabledExtensions != nullptr && descriptor->enabledExtensionCount == 0) {
            return GFX_RESULT_ERROR_INVALID_ARGUMENT;
        }
        if (descriptor->enabledExtensions == nullptr && descriptor->enabledExtensionCount != 0) {
            return GFX_RESULT_ERROR_INVALID_ARGUMENT;
        }

        return GFX_RESULT_SUCCESS;
    }

    GfxResult validateBufferDescriptor(const GfxBufferDescriptor* descriptor)
    {
        if (!descriptor) {
            return GFX_RESULT_ERROR_INVALID_ARGUMENT;
        }

        // Validate size
        if (descriptor->size == 0) {
            return GFX_RESULT_ERROR_INVALID_ARGUMENT;
        }

        // Validate usage flags
        if (descriptor->usage == 0) {
            return GFX_RESULT_ERROR_INVALID_ARGUMENT;
        }

        return GFX_RESULT_SUCCESS;
    }

    GfxResult validateTextureDescriptor(const GfxTextureDescriptor* descriptor)
    {
        if (!descriptor) {
            return GFX_RESULT_ERROR_INVALID_ARGUMENT;
        }

        // Validate dimensions based on texture type
        switch (descriptor->type) {
        case GFX_TEXTURE_TYPE_1D:
            if (descriptor->size.width == 0) {
                return GFX_RESULT_ERROR_INVALID_ARGUMENT;
            }
            break;
        case GFX_TEXTURE_TYPE_2D:
        case GFX_TEXTURE_TYPE_CUBE:
            if (descriptor->size.width == 0 || descriptor->size.height == 0) {
                return GFX_RESULT_ERROR_INVALID_ARGUMENT;
            }
            break;
        case GFX_TEXTURE_TYPE_3D:
            if (descriptor->size.width == 0 || descriptor->size.height == 0 || descriptor->size.depth == 0) {
                return GFX_RESULT_ERROR_INVALID_ARGUMENT;
            }
            break;
        default:
            return GFX_RESULT_ERROR_INVALID_ARGUMENT;
        }

        // Validate format
        if (descriptor->format == GFX_TEXTURE_FORMAT_UNDEFINED) {
            return GFX_RESULT_ERROR_INVALID_ARGUMENT;
        }

        // Validate usage flags
        if (descriptor->usage == 0) {
            return GFX_RESULT_ERROR_INVALID_ARGUMENT;
        }

        // Validate mip levels
        if (descriptor->mipLevelCount == 0) {
            return GFX_RESULT_ERROR_INVALID_ARGUMENT;
        }

        // Validate array layers
        if (descriptor->arrayLayerCount == 0) {
            return GFX_RESULT_ERROR_INVALID_ARGUMENT;
        }

        return GFX_RESULT_SUCCESS;
    }

    GfxResult validateBufferImportDescriptor(const GfxBufferImportDescriptor* descriptor)
    {
        if (!descriptor) {
            return GFX_RESULT_ERROR_INVALID_ARGUMENT;
        }

        // Validate native handle
        if (!descriptor->nativeHandle) {
            return GFX_RESULT_ERROR_INVALID_ARGUMENT;
        }

        // Validate size
        if (descriptor->size == 0) {
            return GFX_RESULT_ERROR_INVALID_ARGUMENT;
        }

        // Validate usage flags
        if (descriptor->usage == 0) {
            return GFX_RESULT_ERROR_INVALID_ARGUMENT;
        }

        return GFX_RESULT_SUCCESS;
    }

    GfxResult validateTextureImportDescriptor(const GfxTextureImportDescriptor* descriptor)
    {
        if (!descriptor) {
            return GFX_RESULT_ERROR_INVALID_ARGUMENT;
        }

        // Validate native handle
        if (!descriptor->nativeHandle) {
            return GFX_RESULT_ERROR_INVALID_ARGUMENT;
        }

        // Validate dimensions based on texture type
        switch (descriptor->type) {
        case GFX_TEXTURE_TYPE_1D:
            if (descriptor->size.width == 0) {
                return GFX_RESULT_ERROR_INVALID_ARGUMENT;
            }
            break;
        case GFX_TEXTURE_TYPE_2D:
        case GFX_TEXTURE_TYPE_CUBE:
            if (descriptor->size.width == 0 || descriptor->size.height == 0) {
                return GFX_RESULT_ERROR_INVALID_ARGUMENT;
            }
            break;
        case GFX_TEXTURE_TYPE_3D:
            if (descriptor->size.width == 0 || descriptor->size.height == 0 || descriptor->size.depth == 0) {
                return GFX_RESULT_ERROR_INVALID_ARGUMENT;
            }
            break;
        default:
            return GFX_RESULT_ERROR_INVALID_ARGUMENT;
        }

        // Validate usage flags
        if (descriptor->usage == 0) {
            return GFX_RESULT_ERROR_INVALID_ARGUMENT;
        }

        // Validate mip levels
        if (descriptor->mipLevelCount == 0) {
            return GFX_RESULT_ERROR_INVALID_ARGUMENT;
        }

        // Validate array layers
        if (descriptor->arrayLayerCount == 0) {
            return GFX_RESULT_ERROR_INVALID_ARGUMENT;
        }

        return GFX_RESULT_SUCCESS;
    }

    GfxResult validateSamplerDescriptor(const GfxSamplerDescriptor* descriptor)
    {
        if (!descriptor) {
            return GFX_RESULT_ERROR_INVALID_ARGUMENT;
        }

        // Validate filter modes
        if (descriptor->magFilter < GFX_FILTER_MODE_NEAREST || descriptor->magFilter > GFX_FILTER_MODE_LINEAR) {
            return GFX_RESULT_ERROR_INVALID_ARGUMENT;
        }

        if (descriptor->minFilter < GFX_FILTER_MODE_NEAREST || descriptor->minFilter > GFX_FILTER_MODE_LINEAR) {
            return GFX_RESULT_ERROR_INVALID_ARGUMENT;
        }

        if (descriptor->mipmapFilter < GFX_FILTER_MODE_NEAREST || descriptor->mipmapFilter > GFX_FILTER_MODE_LINEAR) {
            return GFX_RESULT_ERROR_INVALID_ARGUMENT;
        }

        // Validate address modes
        if (descriptor->addressModeU < GFX_ADDRESS_MODE_REPEAT || descriptor->addressModeU > GFX_ADDRESS_MODE_CLAMP_TO_EDGE) {
            return GFX_RESULT_ERROR_INVALID_ARGUMENT;
        }

        if (descriptor->addressModeV < GFX_ADDRESS_MODE_REPEAT || descriptor->addressModeV > GFX_ADDRESS_MODE_CLAMP_TO_EDGE) {
            return GFX_RESULT_ERROR_INVALID_ARGUMENT;
        }

        if (descriptor->addressModeW < GFX_ADDRESS_MODE_REPEAT || descriptor->addressModeW > GFX_ADDRESS_MODE_CLAMP_TO_EDGE) {
            return GFX_RESULT_ERROR_INVALID_ARGUMENT;
        }

        return GFX_RESULT_SUCCESS;
    }

    GfxResult validateShaderDescriptor(const GfxShaderDescriptor* descriptor)
    {
        if (!descriptor) {
            return GFX_RESULT_ERROR_INVALID_ARGUMENT;
        }

        // WebGPU supports WGSL and SPIR-V (via Dawn extension)
        if (descriptor->sourceType != GFX_SHADER_SOURCE_WGSL && descriptor->sourceType != GFX_SHADER_SOURCE_SPIRV) {
            return GFX_RESULT_ERROR_INVALID_ARGUMENT;
        }

        // Validate code pointer and size
        if (!descriptor->code || descriptor->codeSize == 0) {
            return GFX_RESULT_ERROR_INVALID_ARGUMENT;
        }

        // For SPIR-V, code size should be multiple of 4 bytes
        if (descriptor->sourceType == GFX_SHADER_SOURCE_SPIRV && descriptor->codeSize % 4 != 0) {
            return GFX_RESULT_ERROR_INVALID_ARGUMENT;
        }

        return GFX_RESULT_SUCCESS;
    }

    GfxResult validateTextureViewDescriptor(const GfxTextureViewDescriptor* descriptor)
    {
        // Descriptor is required
        if (!descriptor) {
            return GFX_RESULT_ERROR_INVALID_ARGUMENT;
        }

        // Validate format
        if (descriptor->format == GFX_TEXTURE_FORMAT_UNDEFINED) {
            return GFX_RESULT_ERROR_INVALID_ARGUMENT;
        }

        // Validate mip level count
        if (descriptor->mipLevelCount == 0) {
            return GFX_RESULT_ERROR_INVALID_ARGUMENT;
        }

        // Validate array layer count
        if (descriptor->arrayLayerCount == 0) {
            return GFX_RESULT_ERROR_INVALID_ARGUMENT;
        }

        return GFX_RESULT_SUCCESS;
    }

    GfxResult validateBindGroupLayoutDescriptor(const GfxBindGroupLayoutDescriptor* descriptor)
    {
        if (!descriptor) {
            return GFX_RESULT_ERROR_INVALID_ARGUMENT;
        }

        // Validate entries if provided
        if (descriptor->entryCount > 0 && !descriptor->entries) {
            return GFX_RESULT_ERROR_INVALID_ARGUMENT;
        }

        return GFX_RESULT_SUCCESS;
    }

    GfxResult validateBindGroupDescriptor(const GfxBindGroupDescriptor* descriptor)
    {
        if (!descriptor) {
            return GFX_RESULT_ERROR_INVALID_ARGUMENT;
        }

        // Validate layout
        if (!descriptor->layout) {
            return GFX_RESULT_ERROR_INVALID_ARGUMENT;
        }

        // Validate entries if provided
        if (descriptor->entryCount > 0 && !descriptor->entries) {
            return GFX_RESULT_ERROR_INVALID_ARGUMENT;
        }

        return GFX_RESULT_SUCCESS;
    }

    GfxResult validateRenderPipelineDescriptor(const GfxRenderPipelineDescriptor* descriptor)
    {
        if (!descriptor) {
            return GFX_RESULT_ERROR_INVALID_ARGUMENT;
        }

        // Validate vertex state
        if (!descriptor->vertex) {
            return GFX_RESULT_ERROR_INVALID_ARGUMENT;
        }

        // Validate render pass
        if (!descriptor->renderPass) {
            return GFX_RESULT_ERROR_INVALID_ARGUMENT;
        }

        return GFX_RESULT_SUCCESS;
    }

    GfxResult validateComputePipelineDescriptor(const GfxComputePipelineDescriptor* descriptor)
    {
        if (!descriptor) {
            return GFX_RESULT_ERROR_INVALID_ARGUMENT;
        }

        // Validate compute shader
        if (!descriptor->compute) {
            return GFX_RESULT_ERROR_INVALID_ARGUMENT;
        }

        return GFX_RESULT_SUCCESS;
    }

    GfxResult validateRenderPassDescriptor(const GfxRenderPassDescriptor* descriptor)
    {
        if (!descriptor) {
            return GFX_RESULT_ERROR_INVALID_ARGUMENT;
        }

        // Validate color attachments if provided
        if (descriptor->colorAttachmentCount > 0 && !descriptor->colorAttachments) {
            return GFX_RESULT_ERROR_INVALID_ARGUMENT;
        }

        return GFX_RESULT_SUCCESS;
    }

    GfxResult validateFramebufferDescriptor(const GfxFramebufferDescriptor* descriptor)
    {
        if (!descriptor) {
            return GFX_RESULT_ERROR_INVALID_ARGUMENT;
        }

        // Validate render pass
        if (!descriptor->renderPass) {
            return GFX_RESULT_ERROR_INVALID_ARGUMENT;
        }

        // Validate dimensions
        if (descriptor->width == 0 || descriptor->height == 0) {
            return GFX_RESULT_ERROR_INVALID_ARGUMENT;
        }

        // Validate color attachments if provided
        if (descriptor->colorAttachmentCount > 0 && !descriptor->colorAttachments) {
            return GFX_RESULT_ERROR_INVALID_ARGUMENT;
        }

        return GFX_RESULT_SUCCESS;
    }

    GfxResult validateFenceDescriptor(const GfxFenceDescriptor* descriptor)
    {
        // Descriptor is optional
        if (!descriptor) {
            return GFX_RESULT_SUCCESS;
        }

        // No specific validation needed - signaled flag is any bool value
        return GFX_RESULT_SUCCESS;
    }

    GfxResult validateSemaphoreDescriptor(const GfxSemaphoreDescriptor* descriptor)
    {
        // Descriptor is optional
        if (!descriptor) {
            return GFX_RESULT_SUCCESS;
        }

        // No specific validation needed - type and initialValue are both valid
        return GFX_RESULT_SUCCESS;
    }

    GfxResult validateRenderPassBeginDescriptor(const GfxRenderPassBeginDescriptor* descriptor)
    {
        if (!descriptor) {
            return GFX_RESULT_ERROR_INVALID_ARGUMENT;
        }

        // Validate render pass and framebuffer
        if (!descriptor->renderPass || !descriptor->framebuffer) {
            return GFX_RESULT_ERROR_INVALID_ARGUMENT;
        }

        // Validate color clear values if provided
        if (descriptor->colorClearValueCount > 0 && !descriptor->colorClearValues) {
            return GFX_RESULT_ERROR_INVALID_ARGUMENT;
        }

        return GFX_RESULT_SUCCESS;
    }

    GfxResult validateComputePassBeginDescriptor(const GfxComputePassBeginDescriptor* descriptor)
    {
        if (!descriptor) {
            return GFX_RESULT_ERROR_INVALID_ARGUMENT;
        }

        // No specific validation needed - label is optional
        return GFX_RESULT_SUCCESS;
    }

    GfxResult validateCopyBufferToBufferDescriptor(const GfxCopyBufferToBufferDescriptor* descriptor)
    {
        if (!descriptor) {
            return GFX_RESULT_ERROR_INVALID_ARGUMENT;
        }

        // Validate source and destination buffers
        if (!descriptor->source || !descriptor->destination) {
            return GFX_RESULT_ERROR_INVALID_ARGUMENT;
        }

        // Validate size
        if (descriptor->size == 0) {
            return GFX_RESULT_ERROR_INVALID_ARGUMENT;
        }

        return GFX_RESULT_SUCCESS;
    }

    GfxResult validateCopyBufferToTextureDescriptor(const GfxCopyBufferToTextureDescriptor* descriptor)
    {
        if (!descriptor) {
            return GFX_RESULT_ERROR_INVALID_ARGUMENT;
        }

        // Validate source and destination
        if (!descriptor->source || !descriptor->destination) {
            return GFX_RESULT_ERROR_INVALID_ARGUMENT;
        }

        // Validate extent
        if (descriptor->extent.width == 0 || descriptor->extent.height == 0 || descriptor->extent.depth == 0) {
            return GFX_RESULT_ERROR_INVALID_ARGUMENT;
        }

        return GFX_RESULT_SUCCESS;
    }

    GfxResult validateCopyTextureToBufferDescriptor(const GfxCopyTextureToBufferDescriptor* descriptor)
    {
        if (!descriptor) {
            return GFX_RESULT_ERROR_INVALID_ARGUMENT;
        }

        // Validate source and destination
        if (!descriptor->source || !descriptor->destination) {
            return GFX_RESULT_ERROR_INVALID_ARGUMENT;
        }

        // Validate extent
        if (descriptor->extent.width == 0 || descriptor->extent.height == 0 || descriptor->extent.depth == 0) {
            return GFX_RESULT_ERROR_INVALID_ARGUMENT;
        }

        return GFX_RESULT_SUCCESS;
    }

    GfxResult validateCopyTextureToTextureDescriptor(const GfxCopyTextureToTextureDescriptor* descriptor)
    {
        if (!descriptor) {
            return GFX_RESULT_ERROR_INVALID_ARGUMENT;
        }

        // Validate source and destination
        if (!descriptor->source || !descriptor->destination) {
            return GFX_RESULT_ERROR_INVALID_ARGUMENT;
        }

        // Validate extent
        if (descriptor->extent.width == 0 || descriptor->extent.height == 0 || descriptor->extent.depth == 0) {
            return GFX_RESULT_ERROR_INVALID_ARGUMENT;
        }

        return GFX_RESULT_SUCCESS;
    }

    GfxResult validateBlitTextureToTextureDescriptor(const GfxBlitTextureToTextureDescriptor* descriptor)
    {
        if (!descriptor) {
            return GFX_RESULT_ERROR_INVALID_ARGUMENT;
        }

        // Validate source and destination
        if (!descriptor->source || !descriptor->destination) {
            return GFX_RESULT_ERROR_INVALID_ARGUMENT;
        }

        // Validate extents
        if (descriptor->sourceExtent.width == 0 || descriptor->sourceExtent.height == 0 || descriptor->sourceExtent.depth == 0) {
            return GFX_RESULT_ERROR_INVALID_ARGUMENT;
        }

        if (descriptor->destinationExtent.width == 0 || descriptor->destinationExtent.height == 0 || descriptor->destinationExtent.depth == 0) {
            return GFX_RESULT_ERROR_INVALID_ARGUMENT;
        }

        return GFX_RESULT_SUCCESS;
    }

    GfxResult validatePipelineBarrierDescriptor(const GfxPipelineBarrierDescriptor* descriptor)
    {
        if (!descriptor) {
            return GFX_RESULT_ERROR_INVALID_ARGUMENT;
        }

        // Validate memory barriers
        if (descriptor->memoryBarrierCount > 0 && !descriptor->memoryBarriers) {
            return GFX_RESULT_ERROR_INVALID_ARGUMENT;
        }

        // Validate buffer barriers
        if (descriptor->bufferBarrierCount > 0 && !descriptor->bufferBarriers) {
            return GFX_RESULT_ERROR_INVALID_ARGUMENT;
        }

        // Validate texture barriers
        if (descriptor->textureBarrierCount > 0 && !descriptor->textureBarriers) {
            return GFX_RESULT_ERROR_INVALID_ARGUMENT;
        }

        return GFX_RESULT_SUCCESS;
    }

} // anonymous namespace

// ============================================================================
// Combined validation functions (parameters + descriptors)
// ============================================================================

GfxResult validateCreateInstance(const GfxInstanceDescriptor* descriptor, GfxInstance* outInstance)
{
    if (!outInstance) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }
    return validateInstanceDescriptor(descriptor);
}

GfxResult validateInstanceRequestAdapter(GfxInstance instance, const GfxAdapterDescriptor* descriptor, GfxAdapter* outAdapter)
{
    if (!instance || !outAdapter) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }
    return validateAdapterDescriptor(descriptor);
}

GfxResult validateInstanceEnumerateAdapters(GfxInstance instance, uint32_t* adapterCount)
{
    if (!instance || !adapterCount) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }
    return GFX_RESULT_SUCCESS;
}

GfxResult validateAdapterCreateDevice(GfxAdapter adapter, const GfxDeviceDescriptor* descriptor, GfxDevice* outDevice)
{
    if (!adapter || !outDevice) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }
    return validateDeviceDescriptor(descriptor);
}

GfxResult validateAdapterGetInfo(GfxAdapter adapter, GfxAdapterInfo* outInfo)
{
    if (!adapter || !outInfo) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }
    return GFX_RESULT_SUCCESS;
}

GfxResult validateAdapterGetLimits(GfxAdapter adapter, GfxDeviceLimits* outLimits)
{
    if (!adapter || !outLimits) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }
    return GFX_RESULT_SUCCESS;
}

GfxResult validateAdapterEnumerateQueueFamilies(GfxAdapter adapter, uint32_t* queueFamilyCount)
{
    if (!adapter || !queueFamilyCount) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }
    return GFX_RESULT_SUCCESS;
}

GfxResult validateAdapterGetQueueFamilySurfaceSupport(GfxAdapter adapter, GfxSurface surface, bool* outSupported)
{
    if (!adapter || !surface || !outSupported) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }
    return GFX_RESULT_SUCCESS;
}

GfxResult validateAdapterEnumerateExtensions(GfxAdapter adapter, uint32_t* extensionCount)
{
    if (!adapter || !extensionCount) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }
    return GFX_RESULT_SUCCESS;
}

GfxResult validateDeviceGetQueue(GfxDevice device, GfxQueue* outQueue)
{
    if (!device || !outQueue) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }
    return GFX_RESULT_SUCCESS;
}

GfxResult validateDeviceGetQueueByIndex(GfxDevice device, GfxQueue* outQueue)
{
    if (!device || !outQueue) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }
    return GFX_RESULT_SUCCESS;
}

GfxResult validateDeviceCreateSurface(GfxDevice device, const GfxSurfaceDescriptor* descriptor, GfxSurface* outSurface)
{
    if (!device || !descriptor || !outSurface) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }
    return GFX_RESULT_SUCCESS;
}

GfxResult validateDeviceCreateSwapchain(GfxDevice device, GfxSurface surface, const GfxSwapchainDescriptor* descriptor, GfxSwapchain* outSwapchain)
{
    if (!device || !surface || !descriptor || !outSwapchain) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }
    return validateSwapchainDescriptor(descriptor);
}

GfxResult validateDeviceCreateBuffer(GfxDevice device, const GfxBufferDescriptor* descriptor, GfxBuffer* outBuffer)
{
    if (!device || !descriptor || !outBuffer) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }
    return validateBufferDescriptor(descriptor);
}

GfxResult validateDeviceImportBuffer(GfxDevice device, const GfxBufferImportDescriptor* descriptor, GfxBuffer* outBuffer)
{
    if (!device || !outBuffer) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }
    return validateBufferImportDescriptor(descriptor);
}

GfxResult validateDeviceCreateTexture(GfxDevice device, const GfxTextureDescriptor* descriptor, GfxTexture* outTexture)
{
    if (!device || !descriptor || !outTexture) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }
    return validateTextureDescriptor(descriptor);
}

GfxResult validateDeviceImportTexture(GfxDevice device, const GfxTextureImportDescriptor* descriptor, GfxTexture* outTexture)
{
    if (!device || !outTexture) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }
    return validateTextureImportDescriptor(descriptor);
}

GfxResult validateDeviceCreateSampler(GfxDevice device, const GfxSamplerDescriptor* descriptor, GfxSampler* outSampler)
{
    if (!device || !descriptor || !outSampler) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }
    return validateSamplerDescriptor(descriptor);
}

GfxResult validateDeviceCreateShader(GfxDevice device, const GfxShaderDescriptor* descriptor, GfxShader* outShader)
{
    if (!device || !descriptor || !outShader) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }
    return validateShaderDescriptor(descriptor);
}

GfxResult validateDeviceCreateBindGroupLayout(GfxDevice device, const GfxBindGroupLayoutDescriptor* descriptor, GfxBindGroupLayout* outLayout)
{
    if (!device || !descriptor || !outLayout) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }
    return validateBindGroupLayoutDescriptor(descriptor);
}

GfxResult validateDeviceCreateBindGroup(GfxDevice device, const GfxBindGroupDescriptor* descriptor, GfxBindGroup* outBindGroup)
{
    if (!device || !descriptor || !outBindGroup) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }
    return validateBindGroupDescriptor(descriptor);
}

GfxResult validateDeviceCreateRenderPipeline(GfxDevice device, const GfxRenderPipelineDescriptor* descriptor, GfxRenderPipeline* outPipeline)
{
    if (!device || !descriptor || !outPipeline) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }
    return validateRenderPipelineDescriptor(descriptor);
}

GfxResult validateDeviceCreateComputePipeline(GfxDevice device, const GfxComputePipelineDescriptor* descriptor, GfxComputePipeline* outPipeline)
{
    if (!device || !descriptor || !outPipeline) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }
    return validateComputePipelineDescriptor(descriptor);
}

GfxResult validateDeviceCreateRenderPass(GfxDevice device, const GfxRenderPassDescriptor* descriptor, GfxRenderPass* outRenderPass)
{
    if (!device || !descriptor || !outRenderPass) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }
    return validateRenderPassDescriptor(descriptor);
}

GfxResult validateDeviceCreateFramebuffer(GfxDevice device, const GfxFramebufferDescriptor* descriptor, GfxFramebuffer* outFramebuffer)
{
    if (!device || !descriptor || !outFramebuffer) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }
    return validateFramebufferDescriptor(descriptor);
}

GfxResult validateDeviceCreateCommandEncoder(GfxDevice device, const GfxCommandEncoderDescriptor* descriptor, GfxCommandEncoder* outEncoder)
{
    if (!device || !descriptor || !outEncoder) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }
    return GFX_RESULT_SUCCESS;
}

GfxResult validateDeviceCreateFence(GfxDevice device, const GfxFenceDescriptor* descriptor, GfxFence* outFence)
{
    if (!device || !outFence) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }
    return validateFenceDescriptor(descriptor);
}

GfxResult validateDeviceCreateSemaphore(GfxDevice device, const GfxSemaphoreDescriptor* descriptor, GfxSemaphore* outSemaphore)
{
    if (!device || !outSemaphore) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }
    return validateSemaphoreDescriptor(descriptor);
}

GfxResult validateDeviceCreateQuerySet(GfxDevice device, const GfxQuerySetDescriptor* descriptor, GfxQuerySet* outQuerySet)
{
    if (!device || !descriptor || !outQuerySet) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }
    if (descriptor->count == 0) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }
    return GFX_RESULT_SUCCESS;
}

GfxResult validateDeviceGetLimits(GfxDevice device, GfxDeviceLimits* outLimits)
{
    if (!device || !outLimits) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }
    return GFX_RESULT_SUCCESS;
}

GfxResult validateSurfaceEnumerateSupportedFormats(GfxSurface surface, uint32_t* formatCount)
{
    if (!surface || !formatCount) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }
    return GFX_RESULT_SUCCESS;
}

GfxResult validateSurfaceEnumerateSupportedPresentModes(GfxSurface surface, uint32_t* presentModeCount)
{
    if (!surface || !presentModeCount) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }
    return GFX_RESULT_SUCCESS;
}

GfxResult validateSwapchainGetInfo(GfxSwapchain swapchain, GfxSwapchainInfo* outInfo)
{
    if (!swapchain || !outInfo) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }
    return GFX_RESULT_SUCCESS;
}

GfxResult validateSwapchainAcquireNextImage(GfxSwapchain swapchain, uint32_t* outImageIndex)
{
    if (!swapchain || !outImageIndex) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }
    return GFX_RESULT_SUCCESS;
}

GfxResult validateSwapchainGetTextureView(GfxSwapchain swapchain, GfxTextureView* outView)
{
    if (!swapchain || !outView) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }
    return GFX_RESULT_SUCCESS;
}

GfxResult validateSwapchainGetCurrentTextureView(GfxSwapchain swapchain, GfxTextureView* outView)
{
    if (!swapchain || !outView) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }
    return GFX_RESULT_SUCCESS;
}

GfxResult validateSwapchainPresent(GfxSwapchain swapchain)
{
    if (!swapchain) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }
    return GFX_RESULT_SUCCESS;
}

GfxResult validateBufferGetInfo(GfxBuffer buffer, GfxBufferInfo* outInfo)
{
    if (!buffer || !outInfo) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }
    return GFX_RESULT_SUCCESS;
}

GfxResult validateBufferGetNativeHandle(GfxBuffer buffer, void** outHandle)
{
    if (!buffer || !outHandle) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }
    return GFX_RESULT_SUCCESS;
}

GfxResult validateBufferMap(GfxBuffer buffer, void** outMappedPointer)
{
    if (!buffer || !outMappedPointer) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }
    return GFX_RESULT_SUCCESS;
}

GfxResult validateTextureGetInfo(GfxTexture texture, GfxTextureInfo* outInfo)
{
    if (!texture || !outInfo) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }
    return GFX_RESULT_SUCCESS;
}

GfxResult validateTextureGetNativeHandle(GfxTexture texture, void** outHandle)
{
    if (!texture || !outHandle) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }
    return GFX_RESULT_SUCCESS;
}

GfxResult validateTextureGetLayout(GfxTexture texture, GfxTextureLayout* outLayout)
{
    if (!texture || !outLayout) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }
    return GFX_RESULT_SUCCESS;
}

GfxResult validateTextureCreateView(GfxTexture texture, const GfxTextureViewDescriptor* descriptor, GfxTextureView* outView)
{
    if (!texture || !outView) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }
    return validateTextureViewDescriptor(descriptor);
}

GfxResult validateQueueSubmit(GfxQueue queue, const GfxSubmitDescriptor* submitInfo)
{
    if (!queue || !submitInfo) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }
    return GFX_RESULT_SUCCESS;
}

GfxResult validateQueueWriteBuffer(GfxQueue queue, GfxBuffer buffer, const void* data)
{
    if (!queue || !buffer || !data) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }
    return GFX_RESULT_SUCCESS;
}

GfxResult validateQueueWriteTexture(GfxQueue queue, GfxTexture texture, const GfxOrigin3D* origin, const GfxExtent3D* extent, const void* data)
{
    if (!queue || !texture || !origin || !extent || !data) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }
    return GFX_RESULT_SUCCESS;
}

GfxResult validateCommandEncoderBeginRenderPass(GfxCommandEncoder commandEncoder, const GfxRenderPassBeginDescriptor* beginDescriptor, GfxRenderPassEncoder* outRenderPass)
{
    if (!commandEncoder || !outRenderPass) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }
    return validateRenderPassBeginDescriptor(beginDescriptor);
}

GfxResult validateCommandEncoderBeginComputePass(GfxCommandEncoder commandEncoder, const GfxComputePassBeginDescriptor* beginDescriptor, GfxComputePassEncoder* outComputePass)
{
    if (!commandEncoder || !outComputePass) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }
    return validateComputePassBeginDescriptor(beginDescriptor);
}

GfxResult validateCommandEncoderCopyBufferToBuffer(GfxCommandEncoder commandEncoder, const GfxCopyBufferToBufferDescriptor* descriptor)
{
    if (!commandEncoder) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }
    return validateCopyBufferToBufferDescriptor(descriptor);
}

GfxResult validateCommandEncoderCopyBufferToTexture(GfxCommandEncoder commandEncoder, const GfxCopyBufferToTextureDescriptor* descriptor)
{
    if (!commandEncoder) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }
    return validateCopyBufferToTextureDescriptor(descriptor);
}

GfxResult validateCommandEncoderCopyTextureToBuffer(GfxCommandEncoder commandEncoder, const GfxCopyTextureToBufferDescriptor* descriptor)
{
    if (!commandEncoder) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }
    return validateCopyTextureToBufferDescriptor(descriptor);
}

GfxResult validateCommandEncoderCopyTextureToTexture(GfxCommandEncoder commandEncoder, const GfxCopyTextureToTextureDescriptor* descriptor)
{
    if (!commandEncoder) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }
    return validateCopyTextureToTextureDescriptor(descriptor);
}

GfxResult validateCommandEncoderBlitTextureToTexture(GfxCommandEncoder commandEncoder, const GfxBlitTextureToTextureDescriptor* descriptor)
{
    if (!commandEncoder) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }
    return validateBlitTextureToTextureDescriptor(descriptor);
}

GfxResult validateCommandEncoderPipelineBarrier(GfxCommandEncoder commandEncoder, const GfxPipelineBarrierDescriptor* descriptor)
{
    if (!commandEncoder) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }
    return validatePipelineBarrierDescriptor(descriptor);
}

GfxResult validateCommandEncoderGenerateMipmaps(GfxCommandEncoder commandEncoder, GfxTexture texture)
{
    if (!commandEncoder || !texture) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }
    return GFX_RESULT_SUCCESS;
}

GfxResult validateCommandEncoderGenerateMipmapsRange(GfxCommandEncoder commandEncoder, GfxTexture texture)
{
    if (!commandEncoder || !texture) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }
    return GFX_RESULT_SUCCESS;
}

GfxResult validateCommandEncoderWriteTimestamp(GfxCommandEncoder commandEncoder, GfxQuerySet querySet)
{
    if (!commandEncoder || !querySet) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }
    return GFX_RESULT_SUCCESS;
}

GfxResult validateCommandEncoderResolveQuerySet(GfxCommandEncoder commandEncoder, GfxQuerySet querySet, GfxBuffer destinationBuffer)
{
    if (!commandEncoder || !querySet || !destinationBuffer) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }
    return GFX_RESULT_SUCCESS;
}

GfxResult validateRenderPassEncoderSetPipeline(GfxRenderPassEncoder renderPassEncoder, GfxRenderPipeline pipeline)
{
    if (!renderPassEncoder || !pipeline) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }
    return GFX_RESULT_SUCCESS;
}

GfxResult validateRenderPassEncoderSetBindGroup(GfxRenderPassEncoder renderPassEncoder, GfxBindGroup bindGroup)
{
    if (!renderPassEncoder || !bindGroup) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }
    return GFX_RESULT_SUCCESS;
}

GfxResult validateRenderPassEncoderSetVertexBuffer(GfxRenderPassEncoder renderPassEncoder, GfxBuffer buffer)
{
    if (!renderPassEncoder || !buffer) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }
    return GFX_RESULT_SUCCESS;
}

GfxResult validateRenderPassEncoderSetIndexBuffer(GfxRenderPassEncoder renderPassEncoder, GfxBuffer buffer)
{
    if (!renderPassEncoder || !buffer) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }
    return GFX_RESULT_SUCCESS;
}

GfxResult validateRenderPassEncoderSetViewport(GfxRenderPassEncoder renderPassEncoder, const GfxViewport* viewport)
{
    if (!renderPassEncoder || !viewport) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }
    return GFX_RESULT_SUCCESS;
}

GfxResult validateRenderPassEncoderSetScissorRect(GfxRenderPassEncoder renderPassEncoder, const GfxScissorRect* scissor)
{
    if (!renderPassEncoder || !scissor) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }
    return GFX_RESULT_SUCCESS;
}

GfxResult validateRenderPassEncoderDrawIndirect(GfxRenderPassEncoder renderPassEncoder, GfxBuffer indirectBuffer)
{
    if (!renderPassEncoder || !indirectBuffer) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }
    return GFX_RESULT_SUCCESS;
}

GfxResult validateRenderPassEncoderDrawIndexedIndirect(GfxRenderPassEncoder renderPassEncoder, GfxBuffer indirectBuffer)
{
    if (!renderPassEncoder || !indirectBuffer) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }
    return GFX_RESULT_SUCCESS;
}

GfxResult validateRenderPassEncoderBeginOcclusionQuery(GfxRenderPassEncoder renderPassEncoder, GfxQuerySet querySet)
{
    if (!renderPassEncoder || !querySet) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }
    return GFX_RESULT_SUCCESS;
}

GfxResult validateRenderPassEncoderEndOcclusionQuery(GfxRenderPassEncoder renderPassEncoder)
{
    if (!renderPassEncoder) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }
    return GFX_RESULT_SUCCESS;
}

GfxResult validateComputePassEncoderSetPipeline(GfxComputePassEncoder computePassEncoder, GfxComputePipeline pipeline)
{
    if (!computePassEncoder || !pipeline) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }
    return GFX_RESULT_SUCCESS;
}

GfxResult validateComputePassEncoderSetBindGroup(GfxComputePassEncoder computePassEncoder, GfxBindGroup bindGroup)
{
    if (!computePassEncoder || !bindGroup) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }
    return GFX_RESULT_SUCCESS;
}

GfxResult validateComputePassEncoderDispatchIndirect(GfxComputePassEncoder computePassEncoder, GfxBuffer indirectBuffer)
{
    if (!computePassEncoder || !indirectBuffer) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }
    return GFX_RESULT_SUCCESS;
}

GfxResult validateFenceGetStatus(GfxFence fence, bool* isSignaled)
{
    if (!fence || !isSignaled) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }
    return GFX_RESULT_SUCCESS;
}

GfxResult validateSemaphoreGetType(GfxSemaphore semaphore, GfxSemaphoreType* outType)
{
    if (!semaphore || !outType) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }
    return GFX_RESULT_SUCCESS;
}

GfxResult validateSemaphoreGetValue(GfxSemaphore semaphore, uint64_t* outValue)
{
    if (!semaphore || !outValue) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }
    return GFX_RESULT_SUCCESS;
}

// ============================================================================
// Simple validation functions (destroy, wait, etc.)
// ============================================================================

GfxResult validateInstanceDestroy(GfxInstance instance)
{
    if (!instance) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }
    return GFX_RESULT_SUCCESS;
}

GfxResult validateAdapterDestroy(GfxAdapter adapter)
{
    if (!adapter) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }
    return GFX_RESULT_SUCCESS;
}

GfxResult validateDeviceDestroy(GfxDevice device)
{
    if (!device) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }
    return GFX_RESULT_SUCCESS;
}

GfxResult validateDeviceWaitIdle(GfxDevice device)
{
    if (!device) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }
    return GFX_RESULT_SUCCESS;
}

GfxResult validateSurfaceDestroy(GfxSurface surface)
{
    if (!surface) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }
    return GFX_RESULT_SUCCESS;
}

GfxResult validateSwapchainDestroy(GfxSwapchain swapchain)
{
    if (!swapchain) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }
    return GFX_RESULT_SUCCESS;
}

GfxResult validateBufferDestroy(GfxBuffer buffer)
{
    if (!buffer) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }
    return GFX_RESULT_SUCCESS;
}

GfxResult validateBufferUnmap(GfxBuffer buffer)
{
    if (!buffer) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }
    return GFX_RESULT_SUCCESS;
}

GfxResult validateBufferFlushMappedRange(GfxBuffer buffer)
{
    if (!buffer) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }
    return GFX_RESULT_SUCCESS;
}

GfxResult validateBufferInvalidateMappedRange(GfxBuffer buffer)
{
    if (!buffer) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }
    return GFX_RESULT_SUCCESS;
}

GfxResult validateTextureDestroy(GfxTexture texture)
{
    if (!texture) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }
    return GFX_RESULT_SUCCESS;
}

GfxResult validateTextureViewDestroy(GfxTextureView textureView)
{
    if (!textureView) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }
    return GFX_RESULT_SUCCESS;
}

GfxResult validateSamplerDestroy(GfxSampler sampler)
{
    if (!sampler) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }
    return GFX_RESULT_SUCCESS;
}

GfxResult validateShaderDestroy(GfxShader shader)
{
    if (!shader) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }
    return GFX_RESULT_SUCCESS;
}

GfxResult validateBindGroupLayoutDestroy(GfxBindGroupLayout bindGroupLayout)
{
    if (!bindGroupLayout) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }
    return GFX_RESULT_SUCCESS;
}

GfxResult validateBindGroupDestroy(GfxBindGroup bindGroup)
{
    if (!bindGroup) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }
    return GFX_RESULT_SUCCESS;
}

GfxResult validateRenderPipelineDestroy(GfxRenderPipeline renderPipeline)
{
    if (!renderPipeline) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }
    return GFX_RESULT_SUCCESS;
}

GfxResult validateComputePipelineDestroy(GfxComputePipeline computePipeline)
{
    if (!computePipeline) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }
    return GFX_RESULT_SUCCESS;
}

GfxResult validateRenderPassDestroy(GfxRenderPass renderPass)
{
    if (!renderPass) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }
    return GFX_RESULT_SUCCESS;
}

GfxResult validateFramebufferDestroy(GfxFramebuffer framebuffer)
{
    if (!framebuffer) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }
    return GFX_RESULT_SUCCESS;
}

GfxResult validateQuerySetDestroy(GfxQuerySet querySet)
{
    if (!querySet) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }
    return GFX_RESULT_SUCCESS;
}

GfxResult validateQueueWaitIdle(GfxQueue queue)
{
    if (!queue) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }
    return GFX_RESULT_SUCCESS;
}

GfxResult validateCommandEncoderDestroy(GfxCommandEncoder commandEncoder)
{
    if (!commandEncoder) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }
    return GFX_RESULT_SUCCESS;
}

GfxResult validateCommandEncoderEnd(GfxCommandEncoder commandEncoder)
{
    if (!commandEncoder) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }
    return GFX_RESULT_SUCCESS;
}

GfxResult validateCommandEncoderBegin(GfxCommandEncoder commandEncoder)
{
    if (!commandEncoder) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }
    return GFX_RESULT_SUCCESS;
}

GfxResult validateRenderPassEncoderDraw(GfxRenderPassEncoder renderPassEncoder)
{
    if (!renderPassEncoder) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }
    return GFX_RESULT_SUCCESS;
}

GfxResult validateRenderPassEncoderDrawIndexed(GfxRenderPassEncoder renderPassEncoder)
{
    if (!renderPassEncoder) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }
    return GFX_RESULT_SUCCESS;
}

GfxResult validateRenderPassEncoderEnd(GfxRenderPassEncoder renderPassEncoder)
{
    if (!renderPassEncoder) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }
    return GFX_RESULT_SUCCESS;
}

GfxResult validateComputePassEncoderDispatch(GfxComputePassEncoder computePassEncoder)
{
    if (!computePassEncoder) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }
    return GFX_RESULT_SUCCESS;
}

GfxResult validateComputePassEncoderEnd(GfxComputePassEncoder computePassEncoder)
{
    if (!computePassEncoder) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }
    return GFX_RESULT_SUCCESS;
}

GfxResult validateFenceDestroy(GfxFence fence)
{
    if (!fence) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }
    return GFX_RESULT_SUCCESS;
}

GfxResult validateFenceWait(GfxFence fence)
{
    if (!fence) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }
    return GFX_RESULT_SUCCESS;
}

GfxResult validateFenceReset(GfxFence fence)
{
    if (!fence) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }
    return GFX_RESULT_SUCCESS;
}

GfxResult validateSemaphoreDestroy(GfxSemaphore semaphore)
{
    if (!semaphore) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }
    return GFX_RESULT_SUCCESS;
}

GfxResult validateSemaphoreSignal(GfxSemaphore semaphore)
{
    if (!semaphore) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }
    return GFX_RESULT_SUCCESS;
}

GfxResult validateSemaphoreWait(GfxSemaphore semaphore)
{
    if (!semaphore) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }
    return GFX_RESULT_SUCCESS;
}

} // namespace gfx::backend::webgpu::validator
