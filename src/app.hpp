#ifndef APP_HPP_
#define APP_HPP_

#include "camera.hpp"
#include "drawproperties.hpp"
#include "gui.hpp"
#include "model.hpp"
#include "renderer.hpp"
#include "scene.hpp"
#include "skybox.hpp"

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
#ifndef __EMSCRIPTEN__
    FrameRateInfo frameRateInfo_;
    RenderingAPI currentRenderingAPI_;
    bool vsyncEnabled_;
#endif
    Renderer renderer_;
    Gui gui_;
    Camera camera_;
    DrawProperties drawProps_;
    glm::vec2 lastMousePos_;
    Skybox skybox_;
    std::vector<Model> models_;
    Scene scene_;

#ifdef __EMSCRIPTEN__
    bool initSystems();
    bool createWindow();
#else
    /// When rendering backend is changed during runtime, restart renderer and
    /// reinitialize the systems of the application.
    ///
    /// New OpenGL context requires destroying the existing window and creating
    /// a new one.
    bool reinit(const RenderingAPI newRenderingAPI);
    bool initSystems(const RenderingAPI newRenderingAPI);
    bool createWindow(const RenderingAPI newRenderingAPI);
#endif
    bool loadAssets();
    void processInput();
    void update();
    void render();
};

#endif
