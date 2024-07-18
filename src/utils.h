#ifndef UTILS_H_
#define UTILS_H_

#include <iostream>
#include <sstream>

#ifdef _WIN32
#include <Windows.h>
#elif __EMSCRIPTEN__
#include <emscripten.h>
#endif

namespace utils
{
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
