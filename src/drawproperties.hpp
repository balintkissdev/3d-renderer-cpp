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
    float fieldOfView;
    int selectedModelIndex;
#ifndef __EMSCRIPTEN__
    bool vsyncEnabled;
#endif
    bool skyboxEnabled;
    bool wireframeModeEnabled;
    bool diffuseEnabled;
    bool specularEnabled;
};

#ifndef __EMSCRIPTEN__
/// Information for displaying framerate measurements
struct FrameRateInfo
{
    /// Average number of rendered frames for 1 second
    float framesPerSecond;
    /// Number of milliseconds spent during rendering a single frame. More
    /// useful metric for performance measurement than simple FPS.
    float msPerFrame;
};
#endif

#endif
