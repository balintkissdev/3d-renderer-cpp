#ifndef GUI_H_
#define GUI_H_

class Camera;
struct DrawProperties;
struct GLFWwindow;

class Gui
{
public:
    static void init(GLFWwindow* window);
    static void prepareDraw(const Camera& camera, DrawProperties& drawProps);
    static void draw();
    static void cleanup();

    Gui() = default;
    Gui(const Gui&) = delete;
    Gui& operator=(const Gui&) = delete;
    Gui(Gui&&) = delete;
    Gui& operator=(Gui&&) = delete;
};

#endif
