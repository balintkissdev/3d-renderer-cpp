#pragma once

#include <iostream>
#include <sstream>

namespace utils
{
template <typename... Args>
inline void errorMessage(Args... args) {
    std::ostringstream oss;
    (oss << ... << args);
    std::cerr << oss.str() << '\n';
}

template <typename T>
inline void wrap(T &x, const T min, const T max)
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
inline void clamp(T &x, const T min, const T max)
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
}
