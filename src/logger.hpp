#ifndef LOGGER_HPP_
#define LOGGER_HPP_

#include <format>

namespace logger
{
template <typename... Args>
inline void trace(std::format_string<Args...> fmt, Args&&... args)
{
    std::string msg = std::format(fmt, std::forward<Args>(args)...);
    msg += '\n';
    // TODO: Only for Windows yet
    OutputDebugStringA(msg.c_str());
}
}  // namespace logger

#endif
