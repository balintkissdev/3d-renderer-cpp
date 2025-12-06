#ifndef UTILS_HPP_
#define UTILS_HPP_

#include "drawproperties.hpp"

#include <cassert>
#include <iostream>
#include <sstream>

#ifdef _WIN32
#include <Windows.h>
#elif __EMSCRIPTEN__
#include <emscripten.h>
#endif

/// Collection of miscellaneous utility operations shared to all components
namespace utils
{
/// Macro string concatenation during preprocessing phase.
#define STR_CONCAT(a, b) _STR_CONCAT(a, b)
#define _STR_CONCAT(a, b) a##b

#define DISABLE_COPY(T)   \
    T(const T&) = delete; \
    T& operator=(const T&) = delete;
#define DISABLE_MOVE(T) \
    T(T&&) = delete;    \
    T& operator=(T&&) = delete;
#define DISABLE_COPY_AND_MOVE(T) \
    DISABLE_COPY(T)              \
    DISABLE_MOVE(T)

#ifdef _WIN32
// This converter turns a basic UTF-8 string into a 2-byte UTF-16 string.
//
// See Window doc comment about reasoning to stick with UTF-16.
//
// std::u16string was considered for the sake of explicitness, but the
// resulting occurrences of reinterpret_cast<LPWSTR>() resulted in
// boilerplate code and std::wstring is guaranteed to be 2 bytes on Windows
// anyway (only platform difference is that wchar_t is 4 bytes on Linux).
std::wstring ToWideString(std::string_view str);
#endif  // _WIN32

/// Log a formatted error message accepting any number of arguments to console.
/// An error message box is displayed on Windows and WebAssembly builds.
template <typename... Args>
inline void showErrorMessage(Args... args)
{
    std::ostringstream msgStream;
    msgStream << "ERROR: ";
    (msgStream << ... << args);
    const std::string errorMsg = msgStream.str();
    std::cerr << errorMsg << '\n';

#ifdef _WIN32
    const std::wstring wideErrorMsg = ToWideString(errorMsg);
    ::MessageBoxW(nullptr, wideErrorMsg.c_str(), L"ERROR", MB_ICONERROR);
#elif __EMSCRIPTEN__
    std::ostringstream alertStream;
    alertStream << "alert('" << errorMsg << "');";
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

const char* RenderingAPIToGLSLDirective(
#ifndef __EMSCRIPTEN__
    const RenderingAPI api
#endif
);

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
#endif  // NDEBUG

#ifdef PIX_ENABLED
#define PIX_EVENT(commandList, str) \
    PIXScopedEvent(commandList, PIX_COLOR_INDEX(0), str);
#else
#define PIX_EVENT(commandList, str)
#endif  // PIX_ENABLED

#endif
