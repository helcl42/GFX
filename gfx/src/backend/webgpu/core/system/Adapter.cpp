#include "Adapter.h"

#include "Instance.h"

#include "common/Logger.h"

#include <stdexcept>

namespace gfx::backend::webgpu::core {

Adapter::Adapter(Instance* instance, const AdapterCreateInfo& createInfo)
    : m_adapter(nullptr)
    , m_instance(instance)
{
    if (!instance) {
        throw std::runtime_error("Invalid instance for adapter creation");
    }

    // If adapter index is specified, enumerate and select by index
    if (createInfo.adapterIndex != UINT32_MAX) {
        // Enumerate adapters (WebGPU only returns 1 adapter typically)
        Adapter* adapters[16];
        uint32_t count = enumerate(instance, adapters, 16);

        if (createInfo.adapterIndex >= count) {
            throw std::runtime_error("Adapter index out of range");
        }

        // Use the adapter at the specified index
        m_adapter = adapters[createInfo.adapterIndex]->m_adapter;
        wgpuAdapterAddRef(m_adapter); // Add reference since we're sharing

        // Clean up other enumerated adapters
        for (uint32_t i = 0; i < count; ++i) {
            if (i != createInfo.adapterIndex) {
                delete adapters[i];
            } else {
                // Don't delete the one we're using, but we need to release it later
                delete adapters[i]; // This releases the reference but we already added one
            }
        }
    } else {
        // Otherwise, use preference-based selection (default behavior)
        WGPURequestAdapterOptions options = WGPU_REQUEST_ADAPTER_OPTIONS_INIT;
        options.powerPreference = createInfo.powerPreference;
        options.forceFallbackAdapter = createInfo.forceFallbackAdapter ? WGPU_TRUE : WGPU_FALSE;

        // Use a struct to track callback completion
        struct AdapterRequestContext {
            WGPUAdapter* outAdapter;
            bool completed;
            WGPURequestAdapterStatus status;
        } context = { &m_adapter, false, WGPURequestAdapterStatus_Error };

        WGPURequestAdapterCallbackInfo callbackInfo = WGPU_REQUEST_ADAPTER_CALLBACK_INFO_INIT;
        callbackInfo.mode = WGPUCallbackMode_WaitAnyOnly;
        callbackInfo.callback = [](WGPURequestAdapterStatus status, WGPUAdapter adapter, WGPUStringView message, void* userdata1, void* userdata2) {
            auto* ctx = static_cast<AdapterRequestContext*>(userdata1);
            ctx->status = status;
            ctx->completed = true;

            if (status == WGPURequestAdapterStatus_Success && adapter) {
                *ctx->outAdapter = adapter;
            } else if (message.data) {
                gfx::common::Logger::instance().logError("Error: Failed to request adapter: {}",
                    std::string_view(message.data, message.length));
            }
            (void)userdata2; // Unused
        };
        callbackInfo.userdata1 = &context;

        WGPUFuture future = wgpuInstanceRequestAdapter(instance->handle(), &options, callbackInfo);

        // Use WaitAny to properly wait for the callback
        WGPUFutureWaitInfo waitInfo = WGPU_FUTURE_WAIT_INFO_INIT;
        waitInfo.future = future;
        wgpuInstanceWaitAny(instance->handle(), 1, &waitInfo, UINT64_MAX);

        if (!context.completed) {
            throw std::runtime_error("Adapter request timed out");
        }

        if (!m_adapter) {
            throw std::runtime_error("Failed to request adapter");
        }
    }

    m_info = createAdapterInfo();
}

Adapter::Adapter(WGPUAdapter adapter, Instance* instance)
    : m_adapter(adapter)
    , m_instance(instance)
    , m_info(createAdapterInfo())
{
}

uint32_t Adapter::enumerate(Instance* instance, Adapter** outAdapters, uint32_t maxAdapters)
{
    if (!instance) {
        return 0;
    }

    // Try to request the default adapter
    try {
        AdapterCreateInfo createInfo{};
        createInfo.powerPreference = WGPUPowerPreference_Undefined;
        createInfo.forceFallbackAdapter = false;

        auto* adapter = new Adapter(instance, createInfo);

        // If just querying the count (no output array), clean up and return count
        if (!outAdapters || maxAdapters == 0) {
            delete adapter;
            return 1;
        }

        // Return the adapter
        outAdapters[0] = adapter;
        return 1;
    } catch (...) {
        return 0;
    }
}

Adapter::~Adapter()
{
    if (m_adapter) {
        wgpuAdapterRelease(m_adapter);
    }
}

WGPUAdapter Adapter::handle() const
{
    return m_adapter;
}

Instance* Adapter::getInstance() const
{
    return m_instance;
}

const AdapterInfo& Adapter::getInfo() const
{
    return m_info;
}

WGPULimits Adapter::getLimits() const
{
    WGPULimits limits = WGPU_LIMITS_INIT;
    WGPUStatus status = wgpuAdapterGetLimits(m_adapter, &limits);
    if (status != WGPUStatus_Success) {
        throw std::runtime_error("Failed to get adapter limits");
    }
    return limits;
}

std::vector<QueueFamilyProperties> Adapter::getQueueFamilyProperties() const
{
    // WebGPU has a single unified queue family with all capabilities
    QueueFamilyProperties props{};
    props.queueCount = 1;
    props.supportsGraphics = true;
    props.supportsCompute = true;
    props.supportsTransfer = true;
    return { props };
}

bool Adapter::supportsPresentation(uint32_t queueFamilyIndex) const
{
    // WebGPU's single queue family (index 0) always supports presentation
    return queueFamilyIndex == 0;
}

AdapterInfo Adapter::createAdapterInfo() const
{
    AdapterInfo adapterInfo{};

#ifdef GFX_HAS_EMSCRIPTEN
    // Emscripten's wgpuAdapterGetInfo has issues - provide reasonable defaults
    adapterInfo.name = "WebGPU Adapter";
    adapterInfo.driverDescription = "WebGPU";
    adapterInfo.vendorID = 0;
    adapterInfo.deviceID = 0;
    adapterInfo.adapterType = WGPUAdapterType_Unknown;
#else
    WGPUAdapterInfo info = WGPU_ADAPTER_INFO_INIT;
    wgpuAdapterGetInfo(m_adapter, &info);

    adapterInfo.name = info.device.data ? std::string(info.device.data, info.device.length) : "Unknown";
    adapterInfo.driverDescription = info.description.data ? std::string(info.description.data, info.description.length) : "";
    adapterInfo.vendorID = info.vendorID;
    adapterInfo.deviceID = info.deviceID;
    adapterInfo.adapterType = info.adapterType;

    wgpuAdapterInfoFreeMembers(info);
#endif
    return adapterInfo;
}

std::vector<const char*> Adapter::enumerateSupportedExtensions() const
{
    static const std::vector<const char*> supportedExtensions = {
        extensions::SWAPCHAIN,
        extensions::TIMELINE_SEMAPHORE
    };
    return supportedExtensions;
}

} // namespace gfx::backend::webgpu::core