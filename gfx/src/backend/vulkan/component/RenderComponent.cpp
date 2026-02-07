#include "RenderComponent.h"

#include "common/Logger.h"

#include "backend/vulkan/common/Common.h"
#include "backend/vulkan/converter/Conversions.h"
#include "backend/vulkan/validator/Validations.h"

#include "backend/vulkan/core/render/Framebuffer.h"
#include "backend/vulkan/core/render/RenderPass.h"
#include "backend/vulkan/core/render/RenderPipeline.h"
#include "backend/vulkan/core/system/Device.h"

#include <stdexcept>

namespace gfx::backend::vulkan::component {

// RenderPass functions
GfxResult RenderComponent::deviceCreateRenderPass(GfxDevice device, const GfxRenderPassDescriptor* descriptor, GfxRenderPass* outRenderPass) const
{
    GfxResult validationResult = validator::validateDeviceCreateRenderPass(device, descriptor, outRenderPass);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    try {
        auto* dev = converter::toNative<core::Device>(device);
        auto createInfo = converter::gfxRenderPassDescriptorToRenderPassCreateInfo(descriptor);
        auto* renderPass = new core::RenderPass(dev, createInfo);
        *outRenderPass = converter::toGfx<GfxRenderPass>(renderPass);
        return GFX_RESULT_SUCCESS;
    } catch (const std::exception& e) {
        gfx::common::Logger::instance().logError("Failed to create render pass: {}", e.what());
        return GFX_RESULT_ERROR_UNKNOWN;
    }
}

GfxResult RenderComponent::renderPassDestroy(GfxRenderPass renderPass) const
{
    GfxResult validationResult = validator::validateRenderPassDestroy(renderPass);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    delete converter::toNative<core::RenderPass>(renderPass);
    return GFX_RESULT_SUCCESS;
}

// Framebuffer functions
GfxResult RenderComponent::deviceCreateFramebuffer(GfxDevice device, const GfxFramebufferDescriptor* descriptor, GfxFramebuffer* outFramebuffer) const
{
    GfxResult validationResult = validator::validateDeviceCreateFramebuffer(device, descriptor, outFramebuffer);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    try {
        auto* dev = converter::toNative<core::Device>(device);
        auto createInfo = converter::gfxFramebufferDescriptorToFramebufferCreateInfo(descriptor);
        auto* framebuffer = new core::Framebuffer(dev, createInfo);
        *outFramebuffer = converter::toGfx<GfxFramebuffer>(framebuffer);
        return GFX_RESULT_SUCCESS;
    } catch (const std::exception& e) {
        gfx::common::Logger::instance().logError("Failed to create framebuffer: {}", e.what());
        return GFX_RESULT_ERROR_UNKNOWN;
    }
}

GfxResult RenderComponent::framebufferDestroy(GfxFramebuffer framebuffer) const
{
    GfxResult validationResult = validator::validateFramebufferDestroy(framebuffer);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    delete converter::toNative<core::Framebuffer>(framebuffer);
    return GFX_RESULT_SUCCESS;
}

// RenderPipeline functions
GfxResult RenderComponent::deviceCreateRenderPipeline(GfxDevice device, const GfxRenderPipelineDescriptor* descriptor, GfxRenderPipeline* outPipeline) const
{
    GfxResult validationResult = validator::validateDeviceCreateRenderPipeline(device, descriptor, outPipeline);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    try {
        auto* dev = converter::toNative<core::Device>(device);
        auto createInfo = converter::gfxDescriptorToRenderPipelineCreateInfo(descriptor);
        auto* pipeline = new core::RenderPipeline(dev, createInfo);
        *outPipeline = converter::toGfx<GfxRenderPipeline>(pipeline);
        return GFX_RESULT_SUCCESS;
    } catch (const std::exception& e) {
        gfx::common::Logger::instance().logError("Failed to create render pipeline: {}", e.what());
        return GFX_RESULT_ERROR_UNKNOWN;
    }
}

GfxResult RenderComponent::renderPipelineDestroy(GfxRenderPipeline renderPipeline) const
{
    GfxResult validationResult = validator::validateRenderPipelineDestroy(renderPipeline);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    delete converter::toNative<core::RenderPipeline>(renderPipeline);
    return GFX_RESULT_SUCCESS;
}

} // namespace gfx::backend::vulkan::component
