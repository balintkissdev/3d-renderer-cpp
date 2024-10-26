#include "drawproperties.hpp"

#include <cstdlib>

namespace
{
constexpr size_t STANFORD_BUNNY_MODEL_INDEX = 2;
}

DrawProperties DrawProperties::createDefault()
{
    return DrawProperties{
        .backgroundColor{0.5F, 0.5F, 0.5F},
        .modelRotation{0.0F, 0.0F, 0.0F},
        .modelColor{0.0F, 0.8F, 1.0F},
        .lightDirection{-0.5F, -1.0F, 0.0F},
        .fieldOfView = 60.0F,
        .selectedModelIndex = STANFORD_BUNNY_MODEL_INDEX,
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
