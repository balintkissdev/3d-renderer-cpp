#ifndef UTILS_H_
#define UTILS_H_

#include <iostream>
#include <sstream>

#ifdef _WIN32
#include <Windows.h>
#endif

namespace utils
{
template <typename... Args>
inline void showErrorMessage(Args... args)
{
    std::ostringstream oss;
    (oss << ... << args);
    std::cerr << "ERROR: " << oss.str() << '\n';
#ifdef _WIN32
    MessageBox(nullptr, oss.str().c_str(), "ERROR", MB_ICONERROR);
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
