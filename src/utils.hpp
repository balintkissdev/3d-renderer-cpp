#ifndef UTILS_HPP_
#define UTILS_HPP_

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
}  // namespace utils

#endif
