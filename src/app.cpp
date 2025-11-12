#include "app.hpp"

#include "gl/gl_renderer.hpp"
#include "utils.hpp"

#if defined(WINDOW_PLATFORM_WIN32)
#include "direct3d12/d3d12_renderer.hpp"
#endif

#ifndef __EMSCRIPTEN__
#include <chrono>
#endif

#include <filesystem>

namespace fs = std::filesystem;

namespace
{
constexpr uint16_t WINDOW_WIDTH = 1024;
constexpr uint16_t WINDOW_HEIGHT = 768;

// NOLINTNEXTLINE(readability-identifier-naming)
const char* WINDOW_TITLE = "3D Renderer by Bálint Kiss";

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
    : window_{*this}
    , renderer_{nullptr}
    // Positioning and rotation accidentally imitates a right-handed 3D
    // coordinate system with positive Z going farther from model, but this
    // setting is done because of initial orientation of the loaded Stanford
    // Bunny mesh.
    , camera_{{1.7f, 1.3f, 4.0f}, {240.0f, -15.0f}}
    , drawProps_{DrawProperties::createDefault()}
#ifndef __EMSCRIPTEN__
    , frameRateInfo_{.framesPerSecond = 0.0f, .msPerFrame = 0.0f}
    , vsyncEnabled_{false}
    , currentRenderingAPI_{drawProps_.renderingAPI}
#endif
{
    // TODO: Rename to "Stanford Bunny" once scene node label renaming is
    // functional
    scene_.add({"Model", Scene::Bunny});
}

bool App::init()
{
    return
#ifdef __EMSCRIPTEN__
        initSystems()
#else
        initSystems(currentRenderingAPI_)
#endif
            ;
}

#ifndef __EMSCRIPTEN__
bool App::reinit(const RenderingAPI previousRenderingAPI,
                 const RenderingAPI newRenderingAPI)
{
    window_.hide();
    // Important to release resources using current graphics context before
    // destroying it
    gui_.cleanup(previousRenderingAPI);
    renderer_.reset();
    // TODO: Avoid full re-init, see TODO in win32_window.hpp
    window_.cleanup();

    // TODO: Reloading assets on rendering backend change would normally not be
    // necessary, but in this current architecture the GPU buffer and texture
    // resources are bound to the assets themselves.
    return initSystems(newRenderingAPI);
}

bool App::initSystems(const RenderingAPI newRenderingAPI)
{
    if (createWindow(newRenderingAPI))
    {
        currentRenderingAPI_ = newRenderingAPI;
    }
    else
    {
        utils::showErrorMessage(
            "Selected graphics API is not supported on your system. Falling "
            "back to more compatible OpenGL 3.3.");
        gui_.disallowRenderingAPIOption(newRenderingAPI);
        currentRenderingAPI_ = RenderingAPI::OpenGL33;
        drawProps_.renderingAPI = currentRenderingAPI_;
        const bool fallbackGLVersionSuccess
            = createWindow(currentRenderingAPI_);
        if (!fallbackGLVersionSuccess)
        {
            utils::showErrorMessage("unable to create graphics context. ",
                                    GPU_REQUIREMENTS_MESSAGE);
            return false;
        }
    }

    switch (currentRenderingAPI_)
    {
        case RenderingAPI::OpenGL33:
        case RenderingAPI::OpenGL46:
            renderer_ = std::make_unique<GLRenderer>(window_,
                                                     drawProps_,
                                                     camera_,
                                                     currentRenderingAPI_);
            break;
#if defined(WINDOW_PLATFORM_WIN32)
        case RenderingAPI::Direct3D12:
            renderer_
                = std::make_unique<D3D12Renderer>(window_, drawProps_, camera_);
            break;
#endif
        default:
            assert(
                "illegal operation during init: non-existent "
                "graphics backend");
    }

    if (!renderer_->init())
    {
        utils::showErrorMessage("unable to initialize renderer. ",
                                GPU_REQUIREMENTS_MESSAGE);
        return false;
    }

    gui_.init(*renderer_, currentRenderingAPI_);

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

    renderer_ = std::make_unique<GLRenderer>(window_, drawProps_, camera_);
    if (!renderer_->init())
    {
        utils::showErrorMessage("unable to initialize renderer. ",
                                GPU_REQUIREMENTS_MESSAGE);
        return false;
    }

    gui_.init(*renderer_);

    return true;
}
#endif

#ifdef __EMSCRIPTEN__
bool App::createWindow()
#else
bool App::createWindow(const RenderingAPI renderingAPI)
#endif
{
    if (!window_.init(WINDOW_WIDTH,
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

    window_.setOnMouseMove(MouseOffsetCallback);

    return true;
}

void App::cleanup()
{
    gui_.cleanup(
#ifndef __EMSCRIPTEN__
        currentRenderingAPI_
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
    while (!window_.shouldQuit())
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
            if ((currentRenderingAPI_ != drawProps_.renderingAPI)
                && !reinit(currentRenderingAPI_, drawProps_.renderingAPI))
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
            frameRateInfo_.framesPerSecond
                = static_cast<float>(frameCount) / elapsedFrameTime;
            frameRateInfo_.msPerFrame
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
    if (app.window_.rightMouseButtonPressed())
    {
        app.camera_.look(offsetX, offsetY);
    }
}

void App::processInput()
{
    window_.poll();
    // No need to exit application from a web browser.
#ifndef __EMSCRIPTEN__
    // Exiting here instead of update() avoids delay.
    // BUG: Pressing ESC while editing ImGui textbox should cancel, not exit
    // app.
    if (window_.keyPressed(Key::ESCAPE))
    {
        window_.setShouldQuit(true);
    }
#endif
}

void App::update()
{
    window_.setCursorEnabled(!window_.rightMouseButtonPressed());

    // Update camera here instead of processInput(), otherwise camera movement
    // will be too fast on fast computers.
    if (window_.keyPressed(Key::W))
    {
        camera_.moveForward(FIXED_UPDATE_TIMESTEP);
    }
    if (window_.keyPressed(Key::S))
    {
        camera_.moveBackward(FIXED_UPDATE_TIMESTEP);
    }
    if (window_.keyPressed(Key::A))
    {
        camera_.strafeLeft(FIXED_UPDATE_TIMESTEP);
    }
    if (window_.keyPressed(Key::D))
    {
        camera_.strafeRight(FIXED_UPDATE_TIMESTEP);
    }

    if (window_.keyPressed(Key::SPACE))
    {
        camera_.ascend(FIXED_UPDATE_TIMESTEP);
    }
    if (window_.keyPressed(Key::C))
    {
        camera_.descend(FIXED_UPDATE_TIMESTEP);
    }

#ifndef __EMSCRIPTEN__
    // Update VSync option if changed from GUI
    if (vsyncEnabled_ != drawProps_.vsyncEnabled)
    {
        vsyncEnabled_ = drawProps_.vsyncEnabled;
        renderer_->setVSyncEnabled(vsyncEnabled_);
    }
#endif
}

void App::render()
{
    gui_.prepareDraw(
#ifndef __EMSCRIPTEN__
        frameRateInfo_,
#endif
        camera_,
        drawProps_,
        scene_);
    renderer_->draw(scene_);
}

