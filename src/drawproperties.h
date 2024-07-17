#ifndef DRAW_PROPERTIES_H_
#define DRAW_PROPERTIES_H_

#include <array>

struct DrawProperties
{
    static DrawProperties createDefault();

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

#endif
