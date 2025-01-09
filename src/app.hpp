#ifndef APP_HPP_
#define APP_HPP_

#include "camera.hpp"
#include "drawproperties.hpp"
#include "gui.hpp"
#include "model.hpp"
#include "renderer.hpp"
#include "scene.hpp"
#include "skybox.hpp"
#include "window.hpp"
#include "utils.hpp"

/// Encapsulation of renderer application lifecycle and logic update to avoid
/// polluting main().
class App
{
public:
    App();
    DISABLE_COPY_AND_MOVE(App)

    /// Controlled initialization for explicit error handling.
    bool init();

    /// Execute main loop until user exits application.
    void run();

    /// Controlled deinitialization instead of relying on RAII to avoid
    /// surprises.
    void cleanup();

private:


#ifdef __EMSCRIPTEN__
    static void emscriptenMainLoopCallback(void* arg);
#endif
    static void MouseOffsetCallback(App& app,
                                    const float offsetX,
                                    const float offsetY);

    Window window_;
#ifndef __EMSCRIPTEN__
    FrameRateInfo frameRateInfo_;
    RenderingAPI currentRenderingAPI_;
    bool vsyncEnabled_;
#endif
    Renderer renderer_;
    Gui gui_;
    Camera camera_;
    DrawProperties drawProps_;
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
