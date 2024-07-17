#pragma once

class Camera;
struct DrawProperties;
struct GLFWwindow;

class Gui
{
public:
    static void init(GLFWwindow* window);
    static void preRender(const Camera& camera, DrawProperties& drawProps);
    static void draw();
    static void cleanup();
};
