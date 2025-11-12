#include "utils.hpp"

namespace utils
{

#ifdef _WIN32
std::wstring ToWideString(std::string_view str)
{
    if (str.empty())
    {
        return {};
    }

    const int byteCount = ::MultiByteToWideChar(CP_UTF8,
                                                0,
                                                str.data(),
                                                static_cast<int>(str.size()),
                                                nullptr,
                                                0);
    if (byteCount <= 0)
    {
        return {};
    }

    std::wstring wstr(byteCount, '0');
    const int ok = ::MultiByteToWideChar(CP_UTF8,
                                         0,
                                         str.data(),
                                         static_cast<int>(str.size()),
                                         wstr.data(),
                                         byteCount);

    return (0 < ok) ? wstr : std::wstring();
}
#endif

}  // namespace utils
