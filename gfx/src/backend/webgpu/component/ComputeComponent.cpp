#include "ComputeComponent.h"

#include "common/Logger.h"

#include "../common/Common.h"
#include "../converter/Conversions.h"
#include "../validator/Validations.h"

#include "../core/compute/ComputePipeline.h"
#include "../core/system/Device.h"

#include <stdexcept>
#include <vector>

namespace gfx::backend::webgpu::component {

// ComputePipeline functions
GfxResult ComputeComponent::deviceCreateComputePipeline(GfxDevice device, const GfxComputePipelineDescriptor* descriptor, GfxComputePipeline* outPipeline) const
{
    GfxResult validationResult = validator::validateDeviceCreateComputePipeline(device, descriptor, outPipeline);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    try {
        auto* devicePtr = converter::toNative<core::Device>(device);
        auto createInfo = converter::gfxDescriptorToWebGPUComputePipelineCreateInfo(descriptor);
        auto* pipeline = new core::ComputePipeline(devicePtr, createInfo);
        *outPipeline = converter::toGfx<GfxComputePipeline>(pipeline);
        return GFX_RESULT_SUCCESS;
    } catch (const std::exception& e) {
        gfx::common::Logger::instance().logError("Failed to create compute pipeline: {}", e.what());
        return GFX_RESULT_ERROR_UNKNOWN;
    }
}

GfxResult ComputeComponent::computePipelineDestroy(GfxComputePipeline computePipeline) const
{
    GfxResult validationResult = validator::validateComputePipelineDestroy(computePipeline);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    delete converter::toNative<core::ComputePipeline>(computePipeline);
    return GFX_RESULT_SUCCESS;
}

} // namespace gfx::backend::webgpu::component
