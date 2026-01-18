#include "../system/Device.h"

#include "Adapter.h"
#include "Instance.h"
#include "Queue.h"

#include "../util/Blit.h"

#include "common/Logger.h"

#include <stdexcept>

namespace gfx::backend::webgpu::core {

// Constructor 1: Request device from adapter with createInfo
Device::Device(Adapter* adapter, const DeviceCreateInfo& createInfo)
    : m_device(nullptr)
    , m_adapter(adapter)
{
    if (!adapter) {
        throw std::runtime_error("Invalid adapter for device creation");
    }

    WGPUUncapturedErrorCallbackInfo errorCallbackInfo = WGPU_UNCAPTURED_ERROR_CALLBACK_INFO_INIT;
    errorCallbackInfo.callback = [](WGPUDevice const*, WGPUErrorType type, WGPUStringView message, void*, void*) {
        gfx::common::Logger::instance().logError("[WebGPU Uncaptured Error] Type: {}, Message: {}",
            static_cast<int>(type), std::string_view(message.data, message.length));
    };

    WGPUDeviceLostCallbackInfo deviceLostCallbackInfo = WGPU_DEVICE_LOST_CALLBACK_INFO_INIT;
    deviceLostCallbackInfo.mode = WGPUCallbackMode_AllowSpontaneous;
    deviceLostCallbackInfo.callback = [](WGPUDevice const*, WGPUDeviceLostReason reason, WGPUStringView message, void*, void*) {
        gfx::common::Logger::instance().logError("[WebGPU Device Lost] Reason: {}, Message: {}",
            static_cast<int>(reason), std::string_view(message.data, message.length));
    };

    WGPUDeviceDescriptor wgpuDesc = WGPU_DEVICE_DESCRIPTOR_INIT;
    wgpuDesc.uncapturedErrorCallbackInfo = errorCallbackInfo;
    wgpuDesc.deviceLostCallbackInfo = deviceLostCallbackInfo;
    // DeviceCreateInfo is currently empty, but we keep it for future extensibility
    (void)createInfo;

    struct DeviceRequestContext {
        WGPUDevice* outDevice;
        bool completed;
        WGPURequestDeviceStatus status;
    } context = { &m_device, false, WGPURequestDeviceStatus_Error };

    WGPURequestDeviceCallbackInfo callbackInfo = WGPU_REQUEST_DEVICE_CALLBACK_INFO_INIT;
    callbackInfo.mode = WGPUCallbackMode_WaitAnyOnly;
    callbackInfo.callback = [](WGPURequestDeviceStatus status, WGPUDevice device, WGPUStringView message, void* userdata1, void* userdata2) {
        auto* ctx = static_cast<DeviceRequestContext*>(userdata1);
        ctx->status = status;
        ctx->completed = true;

        if (status == WGPURequestDeviceStatus_Success && device) {
            *ctx->outDevice = device;
        } else if (message.data) {
            gfx::common::Logger::instance().logError("Error: Failed to request device: {}",
                std::string_view(message.data, message.length));
        }
        (void)userdata2; // Unused
    };
    callbackInfo.userdata1 = &context;

    WGPUFuture future = wgpuAdapterRequestDevice(adapter->handle(), &wgpuDesc, callbackInfo);

    // Use WaitAny to properly wait for the callback
    if (adapter->getInstance()) {
        WGPUFutureWaitInfo waitInfo = WGPU_FUTURE_WAIT_INFO_INIT;
        waitInfo.future = future;
        wgpuInstanceWaitAny(adapter->getInstance()->handle(), 1, &waitInfo, UINT64_MAX);
    }

    if (!context.completed) {
        throw std::runtime_error("Device request timed out");
    }

    if (!m_device) {
        throw std::runtime_error("Failed to request device");
    }

    // Create queue
    WGPUQueue wgpuQueue = wgpuDeviceGetQueue(m_device);
    if (!wgpuQueue) {
        throw std::runtime_error("Failed to get default queue from WGPUDevice");
    }
    m_queue = std::make_unique<Queue>(wgpuQueue, this);

    // Create blit helper
    m_blit = std::make_unique<Blit>(m_device);
}

Device::~Device()
{
    if (m_device) {
        // Release queue first
        m_queue.reset();
        // Destroy device to ensure proper cleanup of internal resources
        wgpuDeviceDestroy(m_device);
        wgpuDeviceRelease(m_device);
    }
}

WGPUDevice Device::handle() const
{
    return m_device;
}

Queue* Device::getQueue()
{
    return m_queue.get();
}

Adapter* Device::getAdapter()
{
    return m_adapter;
}

WGPULimits Device::getLimits() const
{
    WGPULimits limits = WGPU_LIMITS_INIT;
    WGPUStatus status = wgpuDeviceGetLimits(m_device, &limits);
    if (status != WGPUStatus_Success) {
        throw std::runtime_error("Failed to get device limits");
    }
    return limits;
}

void Device::waitIdle() const
{
    WGPUQueueWorkDoneCallbackInfo callbackInfo = WGPU_QUEUE_WORK_DONE_CALLBACK_INFO_INIT;
    callbackInfo.mode = WGPUCallbackMode_WaitAnyOnly;
    callbackInfo.callback = [](WGPUQueueWorkDoneStatus status, WGPUStringView message, void* userdata1, void* userdata2) {
        (void)status;
        (void)message;
        (void)userdata1;
        (void)userdata2;
    };
    WGPUFuture future = wgpuQueueOnSubmittedWorkDone(m_queue->handle(), callbackInfo);

    // Wait for the work to complete
    WGPUInstance instance = m_adapter->getInstance()->handle();
    WGPUFutureWaitInfo waitInfo = WGPU_FUTURE_WAIT_INFO_INIT;
    waitInfo.future = future;
    wgpuInstanceWaitAny(instance, 1, &waitInfo, UINT64_MAX);
}

Blit* Device::getBlit()
{
    return m_blit.get();
}

} // namespace gfx::backend::webgpu::core