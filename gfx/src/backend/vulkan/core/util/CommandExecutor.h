#ifndef GFX_VULKAN_COMMAND_EXECUTOR_H
#define GFX_VULKAN_COMMAND_EXECUTOR_H

#include "../../common/Common.h"

#include <functional>

namespace gfx::backend::vulkan::core {

class Queue;

/**
 * @brief Utility for executing single-time Vulkan commands synchronously
 *
 * Creates a transient command pool on construction and reuses it for multiple
 * command buffer submissions. Each execute() call allocates a new command buffer,
 * records commands, submits to queue, and waits for completion.
 *
 * Example usage:
 *   CommandExecutor executor(queue);
 *   executor.execute([&](VkCommandBuffer cmd) {
 *       vkCmdCopyBuffer(cmd, srcBuffer, dstBuffer, 1, &copyRegion);
 *   });
 *   executor.execute([&](VkCommandBuffer cmd) {
 *       vkCmdCopyBufferToImage(cmd, buffer, image, ...);
 *   });
 */
class CommandExecutor {
public:
    explicit CommandExecutor(Queue* queue);
    ~CommandExecutor();

    // Non-copyable
    CommandExecutor(const CommandExecutor&) = delete;
    CommandExecutor& operator=(const CommandExecutor&) = delete;

    /**
     * @brief Execute commands synchronously on the queue
     *
     * @param recordFunc Lambda/function to record commands into the command buffer
     */
    void execute(const std::function<void(VkCommandBuffer)>& recordFunc);

private:
    Queue* m_queue;
    VkDevice m_device;
    VkCommandPool m_commandPool;
};

} // namespace gfx::backend::vulkan::core

#endif // GFX_VULKAN_COMMAND_EXECUTOR_H
