#ifndef WINDOW_HPP_
#define WINDOW_HPP_

// Inspiration for compile-time platform-dependent implementation is
// Boost.Thread
// (https://github.com/boostorg/thread/blob/develop/include/boost/thread/thread_only.hpp)

#if defined(WINDOW_PLATFORM_GLFW)
#include "glfw_window.hpp"
#elif defined(WINDOW_PLATFORM_WIN32)
#include "win32_window.hpp"
#else
#error "Invalid case: no window platform selected"
#endif

#endif
