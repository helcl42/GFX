#include "Instance.h"

#include "Adapter.h"

#include "common/Logger.h"

#include <algorithm>

namespace gfx::backend::webgpu::core {

Instance::Instance(const InstanceCreateInfo& createInfo)
{
    (void)createInfo; // Descriptor not used in current implementation

    // Request TimedWaitAny feature for proper async callback handling
    // For native Dawn, also request ShaderSourceSPIRV for SPIR-V shader support
    static const WGPUInstanceFeatureName requiredFeatures[] = {
        WGPUInstanceFeatureName_TimedWaitAny,
#ifndef __EMSCRIPTEN__
        WGPUInstanceFeatureName_ShaderSourceSPIRV
#endif
    };

#ifndef __EMSCRIPTEN__
    // Enable Dawn toggles to allow SPIR-V at instance level (Dawn-specific)
    static const char* enabledToggles[] = { "allow_unsafe_apis" };
    WGPUDawnTogglesDescriptor instanceTogglesDesc = {};
    instanceTogglesDesc.chain.sType = WGPUSType_DawnTogglesDescriptor;
    instanceTogglesDesc.enabledToggleCount = 1;
    instanceTogglesDesc.enabledToggles = enabledToggles;
    instanceTogglesDesc.disabledToggleCount = 0;
    instanceTogglesDesc.disabledToggles = nullptr;

    gfx::common::Logger::instance().logDebug("WebGPU Instance: Enabling allow_unsafe_apis toggle and requesting ShaderSourceSPIRV feature");
#endif

    WGPUInstanceDescriptor wgpuDesc = WGPU_INSTANCE_DESCRIPTOR_INIT;
    wgpuDesc.requiredFeatureCount = sizeof(requiredFeatures) / sizeof(requiredFeatures[0]);
    wgpuDesc.requiredFeatures = requiredFeatures;
#ifndef __EMSCRIPTEN__
    wgpuDesc.nextInChain = reinterpret_cast<WGPUChainedStruct*>(&instanceTogglesDesc);
#endif
    m_instance = wgpuCreateInstance(&wgpuDesc);

    // Enumerate and cache adapters by trying different power preferences and fallback
    // WebGPU doesn't have a true enumerate API, so we request with different preferences
    // and deduplicate based on adapter handle
    std::vector<WGPUAdapter> discoveredAdapters;

    const WGPUPowerPreference preferences[] = {
        WGPUPowerPreference_HighPerformance,
        WGPUPowerPreference_LowPower,
        WGPUPowerPreference_Undefined
    };

    for (auto preference : preferences) {
        for (auto forceFallback : { WGPU_FALSE, WGPU_TRUE }) {
            WGPURequestAdapterOptions options = WGPU_REQUEST_ADAPTER_OPTIONS_INIT;
            options.powerPreference = preference;
            options.forceFallbackAdapter = forceFallback;

            WGPUAdapter adapter = nullptr;
            bool completed = false;

            WGPURequestAdapterCallbackInfo callbackInfo = WGPU_REQUEST_ADAPTER_CALLBACK_INFO_INIT;
            callbackInfo.mode = WGPUCallbackMode_WaitAnyOnly;
            callbackInfo.callback = [](WGPURequestAdapterStatus status, WGPUAdapter adapterResult, WGPUStringView message, void* userdata1, void* userdata2) {
                auto* adapterPtr = static_cast<WGPUAdapter*>(userdata1);
                auto* completedPtr = static_cast<bool*>(userdata2);

                if (status == WGPURequestAdapterStatus_Success && adapterResult) {
                    *adapterPtr = adapterResult;
                }
                *completedPtr = true;
                (void)message;
            };
            callbackInfo.userdata1 = &adapter;
            callbackInfo.userdata2 = &completed;

            WGPUFuture future = wgpuInstanceRequestAdapter(m_instance, &options, callbackInfo);
            WGPUFutureWaitInfo waitInfo = WGPU_FUTURE_WAIT_INFO_INIT;
            waitInfo.future = future;
            wgpuInstanceWaitAny(m_instance, 1, &waitInfo, UINT64_MAX);

            if (completed && adapter) {
                // Check if we already have this adapter (deduplicate)
                auto isDuplicate = std::any_of(discoveredAdapters.begin(), discoveredAdapters.end(),
                    [adapter](WGPUAdapter existing) { return existing == adapter; });

                if (isDuplicate) {
                    wgpuAdapterRelease(adapter); // Release duplicate
                } else {
                    discoveredAdapters.push_back(adapter);
                }
            }
        }
    }

    // Convert to wrapped adapters
    for (auto adapter : discoveredAdapters) {
        m_adapters.push_back(std::make_unique<Adapter>(adapter, this));
    }
}

Instance::~Instance()
{
    // Clear adapters before destroying instance
    m_adapters.clear();

    if (m_instance) {
        wgpuInstanceRelease(m_instance);
    }
}

WGPUInstance Instance::handle() const
{
    return m_instance;
}

std::vector<const char*> Instance::enumerateSupportedExtensions()
{
    static const std::vector<const char*> supportedExtensions = {
        extensions::SURFACE,
        extensions::DEBUG
    };
    return supportedExtensions;
}

Adapter* Instance::requestAdapter(const AdapterCreateInfo& createInfo) const
{
    if (m_adapters.empty()) {
        throw std::runtime_error("No adapters available");
    }

    // If specific adapter index requested, return that adapter
    if (createInfo.adapterIndex != UINT32_MAX) {
        if (createInfo.adapterIndex >= m_adapters.size()) {
            throw std::out_of_range("Adapter index out of range");
        }
        return m_adapters[createInfo.adapterIndex].get();
    }

    // Match based on power preference
    WGPUAdapterType preferredType;

    switch (createInfo.powerPreference) {
    case WGPUPowerPreference_HighPerformance:
        preferredType = WGPUAdapterType_DiscreteGPU;
        break;
    case WGPUPowerPreference_LowPower:
        preferredType = WGPUAdapterType_IntegratedGPU;
        break;
    case WGPUPowerPreference_Undefined:
    default:
        // Prefer discrete GPU if available
        preferredType = WGPUAdapterType_DiscreteGPU;
        break;
    }

    // Search for preferred adapter type
    for (const auto& adapter : m_adapters) {
        if (adapter->getInfo().adapterType == preferredType) {
            return adapter.get();
        }
    }

    // Fallback to first available if no match
    return m_adapters[0].get();
}

const std::vector<std::unique_ptr<Adapter>>& Instance::getAdapters() const
{
    return m_adapters;
}

} // namespace gfx::backend::webgpu::core