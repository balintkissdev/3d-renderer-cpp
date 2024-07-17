#ifndef APP_H_
#define APP_H_

#include "camera.h"
#include "model.h"
#include "skybox.h"

#include <memory>

struct GLFWwindow;

class App
{
public:
    App();
    App(const App&) = delete;
    App& operator=(const App&) = delete;
    App(App&&) = delete;
    App& operator=(App&&) = delete;

    bool init();
    void run();
    void cleanup();

private:
    struct WindowCallbackData
    {
        Camera& camera;
        glm::vec2 lastMousePos;
    };

    static void errorCallback(int error, const char* description);
    static void mouseButtonCallback(GLFWwindow* window,
                                    int button,
                                    int action,
                                    int mods);
    static void mouseCursorCallback(GLFWwindow* window,
                                    double currentMousePosX,
                                    double currentMousePosY);

    // TODO: Abstract away window implementation once starting work on native
    // Win32 window
    GLFWwindow* window_;
    Camera camera_;
    std::unique_ptr<Skybox> skybox_;
    std::vector<std::unique_ptr<Model>> models_;
    WindowCallbackData windowCallbackData_;

    void handleInput();
};

#endif
