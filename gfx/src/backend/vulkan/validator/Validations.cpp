#include "Validations.h"
#include <cstdint>

namespace gfx::backend::vulkan::validator {

GfxResult validateInstanceDescriptor(const GfxInstanceDescriptor* descriptor)
{
    // Descriptor is optional
    if (!descriptor) {
        return GFX_RESULT_SUCCESS;
    }

    // All fields are optional - no specific validation needed
    // applicationName, applicationVersion, enabledFeatures are all optional
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

    // Validate enabledFeatures and enabledFeatureCount consistency
    if (descriptor->enabledFeatures != nullptr && descriptor->enabledFeatureCount == 0) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }
    if (descriptor->enabledFeatures == nullptr && descriptor->enabledFeatureCount != 0) {
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

    // Validate code pointer and size
    if (!descriptor->code || descriptor->codeSize == 0) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    // For SPIR-V, code size should be multiple of 4 bytes (for binary formats)
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

} // namespace gfx::backend::vulkan::validator
