#pragma once

#include <array>

struct DrawProperties
{
    bool skyboxEnabled;
    bool wireframeModeEnabled;
    bool diffuseEnabled;
    bool specularEnabled;
    int selectedModelIndex;
    float fov;
    std::array<float, 3> backgroundColor;
    std::array<float, 3> modelRotation;
    std::array<float, 3> modelColor;
    std::array<float, 3> lightDirection;
};
