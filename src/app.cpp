#include "app.hpp"

#include "direct3d12/d3d12_renderer.hpp"
#include "gl/gl_renderer.hpp"
#include "globals.hpp"
#include "utils.hpp"

#ifndef __EMSCRIPTEN__
#include <chrono>
#endif

#include <filesystem>

namespace fs = std::filesystem;

namespace
{
// NOLINTNEXTLINE(readability-identifier-naming)
const char* GPU_REQUIREMENTS_MESSAGE =
#ifdef __EMSCRIPTEN__
    "Browser needs to support at least "
    "WebGL2";
#else
    "Graphics card needs to support at least "
    "OpenGL 3.3";
#endif

// This is the granularity of how often to update logic and not to be confused
// with framerate limiting or 60 frames per second, because the main loop
// implementation uses a fixed update, variable framerate timestep algorithm.
//
// 60 logic updates per second is a common value used in games.
// - Higher update rate (120) can lead to smoother gameplay, more precise
// control, at the cost of CPU load. Keep mobile devices in mind.
// - Lower update rate (30) reduces CPU load, runs game logic less frequently,
// but can make game less responsive.
constexpr float MAX_LOGIC_UPDATE_PER_SECOND = 60.0f;
constexpr float FIXED_UPDATE_TIMESTEP = 1.0f / MAX_LOGIC_UPDATE_PER_SECOND;
}  // namespace

App::App()
    : m_window{*this}
    , m_renderer{nullptr}
    // Positioning and rotation accidentally imitates a right-handed 3D
    // coordinate system with positive Z going farther from model, but this
    // setting is done because of initial orientation of the loaded Stanford
    // Bunny mesh.
    , m_camera{{1.7f, 1.3f, 4.0f}, {240.0f, -15.0f}}
    , m_drawProps{DrawProperties::createDefault()}
#ifndef __EMSCRIPTEN__
    , m_frameRateInfo{.framesPerSecond = 0.0f, .msPerFrame = 0.0f}
    , m_vsyncEnabled{false}
    , m_currentRenderingAPI{m_drawProps.renderingAPI}
#endif
{
    // TODO: Rename to "Stanford Bunny" once scene node label renaming is
    // functional
    m_scene.add({"Model", Scene::Bunny});
}

bool App::init()
{
    return
#ifdef __EMSCRIPTEN__
        initSystems()
#else
        initSystems(m_currentRenderingAPI)
#endif
            ;
}

#ifndef __EMSCRIPTEN__
bool App::reinit(const RenderingAPI previousRenderingAPI,
                 const RenderingAPI newRenderingAPI)
{
    m_window.hide();
    // Important to release resources using current graphics context before
    // destroying it
    m_gui.cleanup(previousRenderingAPI);
    m_renderer.reset();
    // TODO: Avoid full re-init, see TODO in win32_window.hpp
    m_window.cleanup();

    // TODO: Reloading assets on rendering backend change would normally not be
    // necessary, but in this current architecture the GPU buffer and texture
    // resources are bound to the assets themselves.
    return initSystems(newRenderingAPI);
}

bool App::initSystems(const RenderingAPI newRenderingAPI)
{
    if (createWindow(newRenderingAPI))
    {
        m_currentRenderingAPI = newRenderingAPI;
    }
    else
    {
        utils::showErrorMessage(
            "Selected graphics API is not supported on your system. Falling "
            "back to more compatible OpenGL 3.3.");
        m_gui.disallowRenderingAPIOption(newRenderingAPI);
        m_currentRenderingAPI = RenderingAPI::OpenGL33;
        m_drawProps.renderingAPI = m_currentRenderingAPI;
        const bool fallbackGLVersionSuccess
            = createWindow(m_currentRenderingAPI);
        if (!fallbackGLVersionSuccess)
        {
            utils::showErrorMessage("unable to create graphics context. ",
                                    GPU_REQUIREMENTS_MESSAGE);
            return false;
        }
    }

    switch (m_currentRenderingAPI)
    {
        case RenderingAPI::OpenGL33:
        case RenderingAPI::OpenGL46:
            m_renderer = std::make_unique<GLRenderer>(m_window,
                                                      m_drawProps,
                                                      m_camera,
                                                      m_currentRenderingAPI);
            break;
#ifdef WINDOW_PLATFORM_WIN32
        case RenderingAPI::Direct3D12:
            m_renderer = std::make_unique<D3D12Renderer>(m_window,
                                                         m_drawProps,
                                                         m_camera);
            break;
#endif
        default:
            assert(
                "illegal operation during init: non-existent "
                "graphics backend");
    }

    if (!m_renderer->init())
    {
        utils::showErrorMessage("unable to initialize renderer. ",
                                GPU_REQUIREMENTS_MESSAGE);
        return false;
    }

    m_gui.init(*m_renderer, m_currentRenderingAPI);

    return true;
}
#else
bool App::initSystems()
{
    if (!createWindow())
    {
        utils::showErrorMessage("unable to create graphics context. ",
                                GPU_REQUIREMENTS_MESSAGE);
        return false;
    }

    m_renderer = std::make_unique<GLRenderer>(m_window, m_drawProps, m_camera);
    if (!m_renderer->init())
    {
        utils::showErrorMessage("unable to initialize renderer. ",
                                GPU_REQUIREMENTS_MESSAGE);
        return false;
    }

    m_gui.init(*m_renderer);

    return true;
}
#endif

#ifdef __EMSCRIPTEN__
bool App::createWindow()
#else
bool App::createWindow(const RenderingAPI renderingAPI)
#endif
{
    if (!m_window.init(WINDOW_WIDTH,
                       WINDOW_HEIGHT,
                       WINDOW_TITLE
#ifndef __EMSCRIPTEN__
                       ,
                       renderingAPI
#endif
                       ))
    {
        return false;
    }

    m_window.setOnMouseMove(MouseOffsetCallback);

    return true;
}

void App::cleanup()
{
    m_gui.cleanup(
#ifndef __EMSCRIPTEN__
        m_currentRenderingAPI
#endif
    );
}

void App::run()
{
#ifdef __EMSCRIPTEN__
    // Web build main loop does not rely on a timestep algorithm or passing
    // deltaTime. Instead, the browser calls and iterates the update function
    // frequently using the requestAnimationFrame() mechanism. This approach
    // prevents blocking the browser's event loop, allowing the browser thread
    // to remain responsive to user interactions.
    emscripten_set_main_loop_arg(emscriptenMainLoopCallback, this, 0, 1);
#else
    // Frame-rate independent loop with fixed update, variable framerate.
    //
    // A naive calculation and passing of a deltaTime introduces floating point
    // precision errors, leading to choppy camera movement and unstable logic
    // even on high framerate. Here, think of it as renderer dictating time, and
    // logic update adapting to it.

    float elapsedFrameTime = 0.0f;
    int frameCount = 0;

    // Prefer steady_clock over high_resolution_clock, because
    // high_resolution_clock could lie.
    auto previousTime = std::chrono::steady_clock::now();
    // How much application "clock" is behind real time. Also known as
    // "accumulator"
    float lag = 0.0f;
    while (!m_window.shouldQuit())
    {
        const auto currentTime = std::chrono::steady_clock::now();
        const float elapsedTime
            = std::chrono::duration<float>(currentTime - previousTime).count();
        previousTime = currentTime;
        lag += elapsedTime;

        // Increment framerate counter
        elapsedFrameTime += elapsedTime;
        ++frameCount;

        processInput();

        while (lag >= FIXED_UPDATE_TIMESTEP)
        {
            // Switch rendering context and reinitialize if changed from GUI
            if ((m_currentRenderingAPI != m_drawProps.renderingAPI)
                && !reinit(m_currentRenderingAPI, m_drawProps.renderingAPI))
            {
                // Exit on rendering context switch error
                return;
            }
            update();
            lag -= FIXED_UPDATE_TIMESTEP;
        }

        render();

        // Update framerate display every 1 second
        if (1.0 <= elapsedFrameTime)
        {
            m_frameRateInfo.framesPerSecond
                = static_cast<float>(frameCount) / elapsedFrameTime;
            m_frameRateInfo.msPerFrame
                = 1000.0f / static_cast<float>(frameCount);

            // Reset framerate counter
            elapsedFrameTime -= 1.0f;
            frameCount = 0;
        }
    }
#endif
}

#ifdef __EMSCRIPTEN__
void App::emscriptenMainLoopCallback(void* arg)
{
    auto* app = static_cast<App*>(arg);
    app->processInput();
    app->update();
    app->render();
}
#endif

void App::MouseOffsetCallback(App& app,
                              const float offsetX,
                              const float offsetY)
{
    if (app.m_window.rightMouseButtonPressed())
    {
        app.m_camera.look(offsetX, offsetY);
    }
}

void App::processInput()
{
    m_window.poll();
    // No need to exit application from a web browser.
#ifndef __EMSCRIPTEN__
    // Exiting here instead of update() avoids delay.
    // BUG: Pressing ESC while editing ImGui textbox should cancel, not exit
    // app.
    if (m_window.keyPressed(Key::ESCAPE))
    {
        m_window.setShouldQuit(true);
    }
#endif
}

void App::update()
{
    m_window.setCursorEnabled(!m_window.rightMouseButtonPressed());

    // Update camera here instead of processInput(), otherwise camera movement
    // will be too fast on fast computers.
    if (m_window.keyPressed(Key::W))
    {
        m_camera.moveForward(FIXED_UPDATE_TIMESTEP);
    }
    if (m_window.keyPressed(Key::S))
    {
        m_camera.moveBackward(FIXED_UPDATE_TIMESTEP);
    }
    if (m_window.keyPressed(Key::A))
    {
        m_camera.strafeLeft(FIXED_UPDATE_TIMESTEP);
    }
    if (m_window.keyPressed(Key::D))
    {
        m_camera.strafeRight(FIXED_UPDATE_TIMESTEP);
    }

    if (m_window.keyPressed(Key::SPACE))
    {
        m_camera.ascend(FIXED_UPDATE_TIMESTEP);
    }
    if (m_window.keyPressed(Key::C))
    {
        m_camera.descend(FIXED_UPDATE_TIMESTEP);
    }

#ifndef __EMSCRIPTEN__
    // Update VSync option if changed from GUI
    if (m_vsyncEnabled != m_drawProps.vsyncEnabled)
    {
        m_vsyncEnabled = m_drawProps.vsyncEnabled;
        m_renderer->setVSyncEnabled(m_vsyncEnabled);
    }
#endif
}

void App::render()
{
    m_gui.prepareDraw(
#ifndef __EMSCRIPTEN__
        m_frameRateInfo,
#endif
        m_camera,
        m_drawProps,
        m_scene);
    m_renderer->draw(m_scene);
}

