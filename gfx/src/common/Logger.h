#ifndef GFX_COMMON_LOGGER_H
#define GFX_COMMON_LOGGER_H

#include <gfx/gfx.h>

#include <format>
#include <string>

namespace gfx::common {

class Logger {
public:
    // Get singleton instance
    static Logger& instance();

    // Delete copy/move constructors and assignment operators
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;
    Logger(Logger&&) = delete;
    Logger& operator=(Logger&&) = delete;

    // Set the logging callback
    void setCallback(GfxLogCallback callback, void* userData);

    // Log functions
    template <typename... Args>
    void logError(std::format_string<Args...> fmt, Args&&... args)
    {
        logMessage(GFX_LOG_LEVEL_ERROR, fmt, std::forward<Args>(args)...);
    }

    template <typename... Args>
    void logWarning(std::format_string<Args...> fmt, Args&&... args)
    {
        logMessage(GFX_LOG_LEVEL_WARNING, fmt, std::forward<Args>(args)...);
    }

    template <typename... Args>
    void logInfo(std::format_string<Args...> fmt, Args&&... args)
    {
        logMessage(GFX_LOG_LEVEL_INFO, fmt, std::forward<Args>(args)...);
    }

    template <typename... Args>
    void logDebug(std::format_string<Args...> fmt, Args&&... args)
    {
        logMessage(GFX_LOG_LEVEL_DEBUG, fmt, std::forward<Args>(args)...);
    }

private:
    Logger();
    ~Logger() = default;

    template <typename... Args>
    void logMessage(GfxLogLevel level, std::format_string<Args...> fmt, Args&&... args)
    {
        if (!m_callback) {
            return;
        }

        std::string message = std::format(fmt, std::forward<Args>(args)...);
        m_callback(level, message.c_str(), m_userData);
    }

    GfxLogCallback m_callback;
    void* m_userData;
};

} // namespace gfx::common

#endif // GFX_COMMON_LOGGER_H