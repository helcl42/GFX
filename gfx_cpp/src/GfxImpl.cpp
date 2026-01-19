#include <gfx_cpp/gfx.hpp>

#include <gfx/gfx.h>

// Include all implementation headers
#include "converter/Conversions.h"
#include "core/util/HandleExtractor.h"
#include "core/util/Utilities.h"

// Resource implementations
#include "core/resource/BindGroup.h"
#include "core/resource/BindGroupLayout.h"
#include "core/resource/Buffer.h"
#include "core/resource/Sampler.h"
#include "core/resource/Shader.h"
#include "core/resource/Texture.h"
#include "core/resource/TextureView.h"

// Command implementations
#include "core/command/CommandEncoder.h"
#include "core/command/ComputePassEncoder.h"
#include "core/command/RenderPassEncoder.h"

// Render implementations
#include "core/render/Framebuffer.h"
#include "core/render/RenderPass.h"
#include "core/render/RenderPipeline.h"

// Compute implementations
#include "core/compute/ComputePipeline.h"

// Presentation implementations
#include "core/presentation/Surface.h"
#include "core/presentation/Swapchain.h"

// Sync implementations
#include "core/sync/Fence.h"
#include "core/sync/Semaphore.h"

// System implementations
#include "core/system/Adapter.h"
#include "core/system/Device.h"
#include "core/system/Instance.h"
#include "core/system/Queue.h"

#include <cstring>
#include <memory>
#include <stdexcept>

namespace gfx {

// ============================================================================
// Factory Function and Utilities
// ============================================================================

std::shared_ptr<Instance> createInstance(const InstanceDescriptor& descriptor)
{
    // Load the backend first (required by the C API)
    GfxBackend cBackend = cppBackendToCBackend(descriptor.backend);
    if (gfxLoadBackend(cBackend) != GFX_RESULT_SUCCESS) {
        throw std::runtime_error("Failed to load graphics backend");
    }

    // Convert enabled features
    std::vector<GfxInstanceFeatureType> cFeatures;
    cFeatures.reserve(descriptor.enabledFeatures.size());
    for (const auto& feature : descriptor.enabledFeatures) {
        cFeatures.push_back(cppInstanceFeatureTypeToCInstanceFeatureType(feature));
    }

    GfxInstanceDescriptor cDesc = {};
    cDesc.backend = cBackend;
    cDesc.enableValidation = descriptor.enableValidation;
    cDesc.applicationName = descriptor.applicationName.c_str();
    cDesc.applicationVersion = descriptor.applicationVersion;
    cDesc.enabledFeatures = cFeatures.empty() ? nullptr : cFeatures.data();
    cDesc.enabledFeatureCount = static_cast<uint32_t>(cFeatures.size());

    GfxInstance instance = nullptr;
    GfxResult result = gfxCreateInstance(&cDesc, &instance);
    if (result != GFX_RESULT_SUCCESS || !instance) {
        throw std::runtime_error("Failed to create instance");
    }

    return std::make_shared<InstanceImpl>(instance);
}

// Global log callback storage (needed because gfxSetLogCallback requires a C function pointer)
void setLogCallback(LogCallback callback)
{
    static LogCallback logCallback = callback;
    if (callback) {
        gfxSetLogCallback([](GfxLogLevel level, const char* message, void* userData) {
            (void)userData;
            if (logCallback) {
                logCallback(cLogLevelToCppLogLevel(level), std::string(message));
            }
        },
            nullptr);
    } else {
        gfxSetLogCallback(nullptr, nullptr);
    }
}

} // namespace gfx
