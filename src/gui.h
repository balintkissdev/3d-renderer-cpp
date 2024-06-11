#pragma once

class Camera;
class DrawProperties;
struct GLFWwindow;

class Gui
{
public:
    void init(GLFWwindow *window);
    void preRender(const Camera &camera, DrawProperties &drawProps);
    void draw();
    void cleanup();
};
