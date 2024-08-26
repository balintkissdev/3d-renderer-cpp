#ifndef DRAW_PROPERTIES_HPP_
#define DRAW_PROPERTIES_HPP_

#include <array>

/// Parameter object for user to customize selected model, model transformations
/// and rendering properties from UI.
struct DrawProperties
{
    /// Factory method creating properties that are default on application
    /// startup.
    static DrawProperties createDefault();

    std::array<float, 3> backgroundColor;
    std::array<float, 3> modelRotation;
    std::array<float, 3> modelColor;
    std::array<float, 3> lightDirection;
    float fov;
    int selectedModelIndex;
    bool skyboxEnabled;
    bool wireframeModeEnabled;
    bool diffuseEnabled;
    bool specularEnabled;
};

#endif
