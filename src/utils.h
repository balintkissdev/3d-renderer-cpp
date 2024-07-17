#pragma once

#include <iostream>
#include <sstream>

#ifdef _WIN32
#include <Windows.h>
#endif

namespace utils
{
template <typename... Args>
inline void errorMessage(Args... args)
{
    std::ostringstream oss;
    (oss << ... << args);
    std::cerr << oss.str() << '\n';
#ifdef _WIN32
    MessageBox(nullptr, oss.str().c_str(), "Error", MB_ICONERROR);
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
