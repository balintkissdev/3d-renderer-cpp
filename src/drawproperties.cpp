#include "drawproperties.hpp"

#include <cstdlib>

DrawProperties DrawProperties::createDefault()
{
    return DrawProperties{
        .backgroundColor{0.5F, 0.5F, 0.5F},
        .lightDirection{-0.5F, -1.0F, 0.0F},
        .fieldOfView = 60.0F,
#ifndef __EMSCRIPTEN__
        .renderingAPI = RenderingAPI::OpenGL46,
        .vsyncEnabled = false,
#endif
        .skyboxEnabled = true,
        .wireframeModeEnabled = false,
        .diffuseEnabled = true,
        .specularEnabled = true,
    };
}
