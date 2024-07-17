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
inline void wrap(T& x, const T min, const T max)
{
    if (max < x)
    {
        x = min;
    }
    else if (x < min)
    {
        x = max;
    }
}

template <typename T>
inline void clamp(T& x, const T min, const T max)
{
    if (max < x)
    {
        x = max;
    }
    else if (x < min)
    {
        x = min;
    }
}
}  // namespace utils
