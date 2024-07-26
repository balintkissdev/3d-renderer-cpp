#include "drawproperties.h"

#include <cstdlib>

namespace
{
constexpr size_t STANFORD_BUNNY_MODEL_INDEX = 2;
}

DrawProperties DrawProperties::createDefault()
{
    return DrawProperties{
        .backgroundColor = {0.5F, 0.5F, 0.5F},
        .modelRotation = {0.0F, 0.0F, 0.0F},
        .modelColor = {0.0F, 0.8F, 1.0F},
        .lightDirection = {-0.5F, -1.0F, 0.0F},
        .fov = 60.0F,
        .selectedModelIndex = STANFORD_BUNNY_MODEL_INDEX,
        .skyboxEnabled = true,
        .wireframeModeEnabled = false,
        .diffuseEnabled = true,
        .specularEnabled = true,
    };
}
