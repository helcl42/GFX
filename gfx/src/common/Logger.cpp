#include "Logger.h"

namespace gfx::common {

Logger& Logger::instance()
{
    static Logger s_instance;
    return s_instance;
}

Logger::Logger()
    : m_callback(nullptr)
    , m_userData(nullptr)
{
}

void Logger::setCallback(GfxLogCallback callback, void* userData)
{
    m_callback = callback;
    m_userData = userData;
}

} // namespace gfx::common
