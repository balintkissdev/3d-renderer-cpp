#ifndef APP_H_
#define APP_H_

#include "camera.h"
#include "drawproperties.h"
#include "model.h"
#include "renderer.h"
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
#ifdef __EMSCRIPTEN__
    static void emscriptenMainLoopCallback(void* arg);
#endif
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
    DrawProperties drawProps_;
    Camera camera_;
    Renderer renderer_;
    std::unique_ptr<Skybox> skybox_;
    std::vector<std::unique_ptr<Model>> models_;
    WindowCallbackData windowCallbackData_;

    void handleInput();
    void render();
};

#endif
