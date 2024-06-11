#pragma once

struct DrawProperties
{
    bool skyboxEnabled;
    bool wireframeModeEnabled;
    bool diffuseEnabled;
    bool specularEnabled;
    int selectedModelIndex;
    float fov;
    float backgroundColor[3];
    float modelRotation[3];
    float modelColor[3];
    float lightDirection[3];
};
