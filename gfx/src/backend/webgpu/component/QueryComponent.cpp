#include "QueryComponent.h"

#include "common/Logger.h"

#include "../common/Common.h"
#include "../converter/Conversions.h"
#include "../validator/Validations.h"

#include "../core/query/QuerySet.h"
#include "../core/system/Device.h"

#include <stdexcept>
#include <vector>

namespace gfx::backend::webgpu::component {

// QuerySet functions
GfxResult QueryComponent::deviceCreateQuerySet(GfxDevice device, const GfxQuerySetDescriptor* descriptor, GfxQuerySet* outQuerySet) const
{
    GfxResult validationResult = validator::validateDeviceCreateQuerySet(device, descriptor, outQuerySet);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    try {
        auto* dev = converter::toNative<core::Device>(device);
        auto createInfo = converter::gfxDescriptorToWebGPUQuerySetCreateInfo(descriptor);
        auto* querySet = new core::QuerySet(dev, createInfo);
        *outQuerySet = converter::toGfx<GfxQuerySet>(querySet);
        return GFX_RESULT_SUCCESS;
    } catch (const std::exception& e) {
        gfx::common::Logger::instance().logError("Failed to create query set: {}", e.what());
        return GFX_RESULT_ERROR_UNKNOWN;
    }
}

GfxResult QueryComponent::querySetDestroy(GfxQuerySet querySet) const
{
    GfxResult validationResult = validator::validateQuerySetDestroy(querySet);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    delete converter::toNative<core::QuerySet>(querySet);
    return GFX_RESULT_SUCCESS;
}

} // namespace gfx::backend::webgpu::component
