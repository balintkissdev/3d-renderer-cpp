#ifndef UTILS_HPP_
#define UTILS_HPP_

#include "pch.hpp"

#include <iostream>
#include <sstream>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

/// Collection of miscellaneous utility operations shared to all components
namespace utils
{
/// Macro string concatenation during preprocessing phase.
#define STR_CONCAT(a, b) _STR_CONCAT(a, b)
#define _STR_CONCAT(a, b) a##b

/// Log a formatted error message accepting any number of arguments to console.
/// An error message box is displayed on Windows and WebAssembly builds.
template <typename... Args>
inline void showErrorMessage(Args... args)
{
    std::ostringstream messageStream;
    messageStream << "ERROR: ";
    (messageStream << ... << args);
    const std::string errorMessage = messageStream.str();
    std::cerr << errorMessage << '\n';

#ifdef _WIN32
    MessageBox(nullptr, errorMessage.c_str(), "ERROR", MB_ICONERROR);
#elif __EMSCRIPTEN__
    std::ostringstream alertStream;
    alertStream << "alert('" << errorMessage << "');";
    emscripten_run_script(alertStream.str().c_str());
#endif
}

/// Wrap around value to min if overflows max and to max if underflows min.
template <typename T>
inline void wrap(T& v, const T min, const T max)
{
    if (max < v)
    {
        v = min;
    }
    else if (v < min)
    {
        v = max;
    }
}

/// Implementation of "defer" keyword similar in Go and Zig to automatically
/// call resource cleanup at end of function scope without copy-pasted cleanup
/// statements or separate RAII wrapper data types. Alternative to wrapping
/// functor into std::unique_ptr, but without the unnecessary heap allocations.
#define DEFER(x)                                     \
    const auto STR_CONCAT(tmpDeferVarName, __LINE__) \
        = utils::_ScopedDefer([&]() { x; })

template <typename F>
struct _ScopedDefer
{
    _ScopedDefer(F f)
        : f(f)
    {
    }

    ~_ScopedDefer() { f(); }

    F f;
};
}  // namespace utils

#ifndef NDEBUG
#define DEBUG_ASSERT_GL_VERSION(requestedMajorGLVersion,           \
                                requestedMinorGLVersion)           \
    do                                                             \
    {                                                              \
        int actualMajorGLVersion = 0;                              \
        int actualMinorGLVersion = 0;                              \
        glGetIntegerv(GL_MAJOR_VERSION, &actualMajorGLVersion);    \
        glGetIntegerv(GL_MINOR_VERSION, &actualMinorGLVersion);    \
        assert(actualMajorGLVersion == (requestedMajorGLVersion)); \
        assert(actualMinorGLVersion == (requestedMinorGLVersion)); \
    } while (0)
#else
#define DEBUG_ASSERT_GL_VERSION(requestedMajorGLVersion, \
                                requestedMinorGLVersion)
#endif

#endif
