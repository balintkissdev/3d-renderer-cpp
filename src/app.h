#ifndef APP_H_
#define APP_H_

#include "camera.h"
#include "drawproperties.h"
#include "model.h"
#include "renderer.h"
#include "skybox.h"

#include <memory>

struct GLFWwindow;

/// Encapsulation of renderer application lifecycle and logic update to avoid
/// polluting main().
class App
{
public:
    App();
    App(const App&) = delete;
    App& operator=(const App&) = delete;
    App(App&&) = delete;
    App& operator=(App&&) = delete;

    /// Controlled initialization for explicit error handling.
    bool init();

    /// Execute main loop until user exits application.
    void run();

    /// Controlled deinitialization instead of relying on RAII to avoid
    /// surprises.
    void cleanup();

private:
    static void errorCallback(int error, const char* description);
#ifdef __EMSCRIPTEN__
    static void emscriptenMainLoopCallback(void* arg);
#endif
    static void mouseButtonCallback(GLFWwindow* window,
                                    int button,
                                    int action,
                                    int mods);
    static void mouseMoveCallback(GLFWwindow* window,
                                  double currentMousePosX,
                                  double currentMousePosY);

    // TODO: Abstract away window implementation once starting work on native
    // Win32 window
    GLFWwindow* window_;
    Renderer renderer_;
    Camera camera_;
    DrawProperties drawProps_;
    glm::vec2 lastMousePos_;
    std::unique_ptr<Skybox> skybox_;
    std::vector<std::unique_ptr<Model>> models_;

    void handleInput();
    void render();
};

#endif
