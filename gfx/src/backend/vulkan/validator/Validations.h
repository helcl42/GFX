#pragma once

#include "gfx/gfx.h"

namespace gfx::backend::vulkan::validator {

/**
 * @brief Validate instance descriptor (optional)
 * @param descriptor The instance descriptor to validate (NULL is valid)
 * @return GFX_RESULT_SUCCESS if valid, error code otherwise
 */
GfxResult validateInstanceDescriptor(const GfxInstanceDescriptor* descriptor);

/**
 * @brief Validate adapter descriptor (optional)
 * @param descriptor The adapter descriptor to validate (NULL is valid)
 * @return GFX_RESULT_SUCCESS if valid, error code otherwise
 */
GfxResult validateAdapterDescriptor(const GfxAdapterDescriptor* descriptor);

/**
 * @brief Validate swapchain descriptor
 * @param descriptor The swapchain descriptor to validate
 * @return GFX_RESULT_SUCCESS if valid, error code otherwise
 */
GfxResult validateSwapchainDescriptor(const GfxSwapchainDescriptor* descriptor);

/**
 * @brief Validate device descriptor (optional)
 * @param descriptor The device descriptor to validate (NULL is valid)
 * @return GFX_RESULT_SUCCESS if valid, error code otherwise
 */
GfxResult validateDeviceDescriptor(const GfxDeviceDescriptor* descriptor);

/**
 * @brief Validate buffer descriptor
 * @param descriptor The buffer descriptor to validate
 * @return GFX_RESULT_SUCCESS if valid, error code otherwise
 */
GfxResult validateBufferDescriptor(const GfxBufferDescriptor* descriptor);

/**
 * @brief Validate buffer import descriptor
 * @param descriptor The buffer import descriptor to validate
 * @return GFX_RESULT_SUCCESS if valid, error code otherwise
 */
GfxResult validateBufferImportDescriptor(const GfxBufferImportDescriptor* descriptor);

/**
 * @brief Validate texture descriptor
 * @param descriptor The texture descriptor to validate
 * @return GFX_RESULT_SUCCESS if valid, error code otherwise
 */
GfxResult validateTextureDescriptor(const GfxTextureDescriptor* descriptor);

/**
 * @brief Validate texture import descriptor
 * @param descriptor The texture import descriptor to validate
 * @return GFX_RESULT_SUCCESS if valid, error code otherwise
 */
GfxResult validateTextureImportDescriptor(const GfxTextureImportDescriptor* descriptor);

/**
 * @brief Validate sampler descriptor
 * @param descriptor The sampler descriptor to validate
 * @return GFX_RESULT_SUCCESS if valid, error code otherwise
 */
GfxResult validateSamplerDescriptor(const GfxSamplerDescriptor* descriptor);

/**
 * @brief Validate shader descriptor
 * @param descriptor The shader descriptor to validate
 * @return GFX_RESULT_SUCCESS if valid, error code otherwise
 */
GfxResult validateShaderDescriptor(const GfxShaderDescriptor* descriptor);

/**
 * @brief Validate texture view descriptor
 * @param descriptor The texture view descriptor to validate
 * @return GFX_RESULT_SUCCESS if valid, error code otherwise
 */
GfxResult validateTextureViewDescriptor(const GfxTextureViewDescriptor* descriptor);

/**
 * @brief Validate bind group layout descriptor
 * @param descriptor The bind group layout descriptor to validate
 * @return GFX_RESULT_SUCCESS if valid, error code otherwise
 */
GfxResult validateBindGroupLayoutDescriptor(const GfxBindGroupLayoutDescriptor* descriptor);

/**
 * @brief Validate bind group descriptor
 * @param descriptor The bind group descriptor to validate
 * @return GFX_RESULT_SUCCESS if valid, error code otherwise
 */
GfxResult validateBindGroupDescriptor(const GfxBindGroupDescriptor* descriptor);

/**
 * @brief Validate render pipeline descriptor
 * @param descriptor The render pipeline descriptor to validate
 * @return GFX_RESULT_SUCCESS if valid, error code otherwise
 */
GfxResult validateRenderPipelineDescriptor(const GfxRenderPipelineDescriptor* descriptor);

/**
 * @brief Validate compute pipeline descriptor
 * @param descriptor The compute pipeline descriptor to validate
 * @return GFX_RESULT_SUCCESS if valid, error code otherwise
 */
GfxResult validateComputePipelineDescriptor(const GfxComputePipelineDescriptor* descriptor);

/**
 * @brief Validate render pass descriptor
 * @param descriptor The render pass descriptor to validate
 * @return GFX_RESULT_SUCCESS if valid, error code otherwise
 */
GfxResult validateRenderPassDescriptor(const GfxRenderPassDescriptor* descriptor);

/**
 * @brief Validate framebuffer descriptor
 * @param descriptor The framebuffer descriptor to validate
 * @return GFX_RESULT_SUCCESS if valid, error code otherwise
 */
GfxResult validateFramebufferDescriptor(const GfxFramebufferDescriptor* descriptor);

/**
 * @brief Validate fence descriptor (optional)
 * @param descriptor The fence descriptor to validate (NULL is valid)
 * @return GFX_RESULT_SUCCESS if valid, error code otherwise
 */
GfxResult validateFenceDescriptor(const GfxFenceDescriptor* descriptor);

/**
 * @brief Validate semaphore descriptor (optional)
 * @param descriptor The semaphore descriptor to validate (NULL is valid)
 * @return GFX_RESULT_SUCCESS if valid, error code otherwise
 */
GfxResult validateSemaphoreDescriptor(const GfxSemaphoreDescriptor* descriptor);

/**
 * @brief Validate render pass begin descriptor
 * @param descriptor The render pass begin descriptor to validate
 * @return GFX_RESULT_SUCCESS if valid, error code otherwise
 */
GfxResult validateRenderPassBeginDescriptor(const GfxRenderPassBeginDescriptor* descriptor);

/**
 * @brief Validate compute pass begin descriptor
 * @param descriptor The compute pass begin descriptor to validate
 * @return GFX_RESULT_SUCCESS if valid, error code otherwise
 */
GfxResult validateComputePassBeginDescriptor(const GfxComputePassBeginDescriptor* descriptor);

/**
 * @brief Validate buffer to buffer copy descriptor
 * @param descriptor The copy descriptor to validate
 * @return GFX_RESULT_SUCCESS if valid, error code otherwise
 */
GfxResult validateCopyBufferToBufferDescriptor(const GfxCopyBufferToBufferDescriptor* descriptor);

/**
 * @brief Validate buffer to texture copy descriptor
 * @param descriptor The copy descriptor to validate
 * @return GFX_RESULT_SUCCESS if valid, error code otherwise
 */
GfxResult validateCopyBufferToTextureDescriptor(const GfxCopyBufferToTextureDescriptor* descriptor);

/**
 * @brief Validate texture to buffer copy descriptor
 * @param descriptor The copy descriptor to validate
 * @return GFX_RESULT_SUCCESS if valid, error code otherwise
 */
GfxResult validateCopyTextureToBufferDescriptor(const GfxCopyTextureToBufferDescriptor* descriptor);

/**
 * @brief Validate texture to texture copy descriptor
 * @param descriptor The copy descriptor to validate
 * @return GFX_RESULT_SUCCESS if valid, error code otherwise
 */
GfxResult validateCopyTextureToTextureDescriptor(const GfxCopyTextureToTextureDescriptor* descriptor);

/**
 * @brief Validate texture to texture blit descriptor
 * @param descriptor The blit descriptor to validate
 * @return GFX_RESULT_SUCCESS if valid, error code otherwise
 */
GfxResult validateBlitTextureToTextureDescriptor(const GfxBlitTextureToTextureDescriptor* descriptor);

/**
 * @brief Validate pipeline barrier descriptor
 * @param descriptor The pipeline barrier descriptor to validate
 * @return GFX_RESULT_SUCCESS if valid, error code otherwise
 */
GfxResult validatePipelineBarrierDescriptor(const GfxPipelineBarrierDescriptor* descriptor);

} // namespace gfx::backend::vulkan::validator
