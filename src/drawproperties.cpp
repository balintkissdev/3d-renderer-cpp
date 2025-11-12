#include "drawproperties.hpp"

#ifndef __EMSCRIPTEN__
namespace
{
constexpr RenderingAPI DEFAULT_RENDERING_API =
#ifdef WINDOW_PLATFORM_WIN32
    RenderingAPI::Direct3D12
#else
    RenderingAPI::OpenGL46
#endif
    ;
}  // namespace
#endif

DrawProperties DrawProperties::createDefault()
{
    return DrawProperties{
        .backgroundColor{0.5f, 0.5f, 0.5f},
        .lightDirection{-0.5f, -1.0f, 0.0f},
        .fieldOfView = 60.0f,
#ifndef __EMSCRIPTEN__
        .renderingAPI = DEFAULT_RENDERING_API,
        .vsyncEnabled = false,
#endif
        .skyboxEnabled = true,
        .wireframeModeEnabled = false,
        .diffuseEnabled = true,
        .specularEnabled = true,
    };
}
