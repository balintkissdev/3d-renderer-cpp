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

    Window m_window;
    std::unique_ptr<Renderer> m_renderer;
    Gui m_gui;
    Camera m_camera;
    DrawProperties m_drawProps;
#ifndef __EMSCRIPTEN__
    FrameRateInfo m_frameRateInfo;
    bool m_vsyncEnabled;
    RenderingAPI m_currentRenderingAPI;
#endif
    Scene m_scene;

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
