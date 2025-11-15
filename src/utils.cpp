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

const char* RenderingAPIToGLSLDirective(const RenderingAPI api)
{
#ifdef __EMSCRIPTEN__
    return "#version 300 es\n";
#else
    switch (api)
    {
        case RenderingAPI::OpenGL33:
            return "#version 330\n";
        case RenderingAPI::OpenGL46:
            return "#version 460\n";
        default:
            assert("illegal API to GLSL directive conversion");
            return {};
    }
#endif
}

}  // namespace utils
