#include "drawproperties.h"

DrawProperties DrawProperties::createDefault()
{
    return DrawProperties{.skyboxEnabled = true,
                          .wireframeModeEnabled = false,
                          .diffuseEnabled = true,
                          .specularEnabled = true,
                          .selectedModelIndex = 2,
                          .fov = 60.0F,
                          .backgroundColor = {0.5F, 0.5F, 0.5F},
                          .modelRotation = {0.0F, 0.0F, 0.0F},
                          .modelColor = {0.0F, 0.8F, 1.0F},
                          .lightDirection = {-0.5F, -1.0F, 0.0F}};
}
