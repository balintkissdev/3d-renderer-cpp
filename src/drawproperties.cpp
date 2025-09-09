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
        .backgroundColor{0.5F, 0.5F, 0.5F},
        .lightDirection{-0.5F, -1.0F, 0.0F},
        .fieldOfView = 60.0F,
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
