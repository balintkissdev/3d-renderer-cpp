#ifndef APP_HPP_
#define APP_HPP_

#include "camera.hpp"
#include "drawproperties.hpp"
#include "gui.hpp"
#include "renderer.hpp"
#include "scene.hpp"
#include "utils.hpp"
#include "window.hpp"

#include <memory>

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
    std::unique_ptr<Renderer> renderer_;
    Gui gui_;
    Camera camera_;
    DrawProperties drawProps_;
#ifndef __EMSCRIPTEN__
    FrameRateInfo frameRateInfo_;
    bool vsyncEnabled_;
    RenderingAPI currentRenderingAPI_;
#endif
    Scene scene_;

#ifdef __EMSCRIPTEN__
    bool initSystems();
    bool createWindow();
#else
    /// When rendering backend is changed during runtime, restart renderer and
    /// reinitialize the systems of the application.
    ///
    /// New context requires destroying the existing window and creating
    /// a new one.
    bool reinit(const RenderingAPI previousRenderingAPI,
                const RenderingAPI newRenderingAPI);
    bool initSystems(const RenderingAPI newRenderingAPI);
    bool createWindow(const RenderingAPI newRenderingAPI);
#endif
    void processInput();
    void update();
    void render();
};

#endif
