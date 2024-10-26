#include "app.hpp"

#include "utils.hpp"

#include "glm/gtc/matrix_transform.hpp"

#ifdef __EMSCRIPTEN__
#include "glad/gles2.h"

#include <GLFW/glfw3.h>  // Use GLFW port from Emscripten
#include <emscripten.h>
#else
#include "GLFW/glfw3.h"
#include "glad/gl.h"

#include <chrono>
#endif

#include <filesystem>

namespace fs = std::filesystem;

namespace
{
constexpr uint16_t SCREEN_WIDTH = 1024;
constexpr uint16_t SCREEN_HEIGHT = 768;

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
constexpr float MAX_LOGIC_UPDATE_PER_SECOND = 60.0F;
constexpr float FIXED_UPDATE_TIMESTEP = 1.0F / MAX_LOGIC_UPDATE_PER_SECOND;
}  // namespace

App::App()
    : window_{nullptr}
#ifndef __EMSCRIPTEN__
    , frameRateInfo_{0.0F, 0.0F}
    , currentRenderingAPI_(RenderingAPI::OpenGL46)
    , vsyncEnabled_{false}
#endif
    , renderer_(drawProps_, camera_)
    // Positioning and rotation accidentally imitates a right-handed 3D
    // coordinate system with positive Z going farther from model, but this
    // setting is done because of initial orientation of the loaded Stanford
    // Bunny mesh.
    , camera_({1.7F, 1.3F, 4.0F}, {240.0F, -15.0F})
    , drawProps_(DrawProperties::createDefault())
    , lastMousePos_{static_cast<float>(SCREEN_WIDTH) / 2.0F,
                    static_cast<float>(SCREEN_HEIGHT) / 2.0F}
{
    models_.reserve(3);
}

bool App::init()
{
    // Initialize windowing system
    if (!glfwInit())
    {
        utils::showErrorMessage("unable to initialize windowing system. ",
                                GPU_REQUIREMENTS_MESSAGE);
        return false;
    }
    glfwSetErrorCallback(errorCallback);

    return
#ifdef __EMSCRIPTEN__
        initSystems()
#else
        initSystems(currentRenderingAPI_)
#endif
        && loadAssets();
}

#ifndef __EMSCRIPTEN__
bool App::reinit(const RenderingAPI newRenderingAPI)
{
    glfwHideWindow(window_);
    // Important to release resources using current graphics context before
    // destroying it
    gui_.cleanup();
    skybox_.cleanup();
    models_.clear();
    renderer_.cleanup();

    glfwDestroyWindow(window_);
    window_ = nullptr;

    // TODO: Reloading assets on rendering backend change would normally not be
    // necessary, but in this current architecture the GPU buffer and texture
    // resources are bound to the assets themselves.
    return initSystems(newRenderingAPI) && loadAssets();
}

bool App::initSystems(const RenderingAPI newRenderingAPI)
{
    const bool desiredGLVersionSuccess = createWindow(newRenderingAPI);
    if (desiredGLVersionSuccess)
    {
        currentRenderingAPI_ = newRenderingAPI;
    }
    else
    {
        utils::showErrorMessage(
            "OpenGL 4.6 is not supported on your system. Falling back to more "
            "compatible OpenGL 3.3.");
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

    // Init GUI
    gui_.init(window_, currentRenderingAPI_);

    // Init renderer
    if (!renderer_.init(window_, currentRenderingAPI_))
    {
        utils::showErrorMessage("unable to initialize renderer. ",
                                GPU_REQUIREMENTS_MESSAGE);
        return false;
    }

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

    // Init GUI
    gui_.init(window_);

    // Init renderer
    if (!renderer_.init(window_))
    {
        utils::showErrorMessage("unable to initialize renderer. ",
                                GPU_REQUIREMENTS_MESSAGE);
        return false;
    }

    return true;
}
#endif

#ifdef __EMSCRIPTEN__
bool App::createWindow()
{
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
#else
bool App::createWindow(const RenderingAPI renderingAPI)
{
    const int majorGLversion = renderingAPI == RenderingAPI::OpenGL46 ? 4 : 3;
    const int minorGLversion = renderingAPI == RenderingAPI::OpenGL46 ? 6 : 3;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, majorGLversion);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, minorGLversion);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#endif
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
    // TODO: Make window and OpenGL framebuffer resizable
    glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
    window_ = glfwCreateWindow(SCREEN_WIDTH,
                               SCREEN_HEIGHT,
                               "3D renderer by Bálint Kiss",
                               nullptr,
                               nullptr);
    if (!window_)
    {
        return false;
    }

    // Setup event callbacks
    glfwSetWindowUserPointer(window_, this);
    glfwSetMouseButtonCallback(window_, mouseButtonCallback);
    glfwSetCursorPosCallback(window_, mouseMoveCallback);

    // Make GL context current
    glfwMakeContextCurrent(window_);
#ifndef __EMSCRIPTEN__
    // Set VSync
    glfwSwapInterval(vsyncEnabled_);
#endif

    return true;
}

bool App::loadAssets()
{
    std::optional<Skybox> skybox = SkyboxBuilder()
                                       .setRight("assets/skybox/right.jpg")
                                       .setLeft("assets/skybox/left.jpg")
                                       .setTop("assets/skybox/top.jpg")
                                       .setBottom("assets/skybox/bottom.jpg")
                                       .setFront("assets/skybox/front.jpg")
                                       .setBack("assets/skybox/back.jpg")
                                       .build();
    if (!skybox)
    {
        utils::showErrorMessage("unable to create skybox for application");
        return false;
    }
    skybox_ = std::move(skybox.value());

    const std::array<fs::path, 3> modelPaths{"assets/meshes/cube.obj",
                                             "assets/meshes/teapot.obj",
                                             "assets/meshes/bunny.obj"};
    for (const auto& path : modelPaths)
    {
        std::optional<Model> model = Model::create(path);
        if (!model)
        {
            utils::showErrorMessage("unable to create model from path ", path);
            return false;
        }
        models_.emplace_back(std::move(model.value()));
    }

    return true;
}

void App::cleanup()
{
    gui_.cleanup();
    glfwDestroyWindow(window_);
    glfwTerminate();
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

    float elapsedFrameTime = 0.0F;
    int frameCount = 0;

    // Prefer steady_clock over high_resolution_clock, because
    // high_resolution_clock could lie.
    auto previousTime = std::chrono::steady_clock::now();
    // How much application "clock" is behind real time. Also known as
    // "accumulator"
    float lag = 0.0F;
    while (!glfwWindowShouldClose(window_))
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
                && !reinit(drawProps_.renderingAPI))
            {
                // Exit on rendering context switch error
                break;
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
                = 1000.0F / static_cast<float>(frameCount);

            // Reset framerate counter
            elapsedFrameTime -= 1.0;
            frameCount = 0;
        }
    }
#endif
}

void App::errorCallback([[maybe_unused]] int error, const char* description)
{
    utils::showErrorMessage("GLFW error: ", description);
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

void App::mouseButtonCallback(GLFWwindow* window,
                              int button,
                              int action,
                              [[maybe_unused]] int mods)
{
    if (button == GLFW_MOUSE_BUTTON_RIGHT)
    {
        // Initiate mouse look on right mouse button press
        if (action == GLFW_PRESS)
        {
            if (glfwGetInputMode(window, GLFW_CURSOR) == GLFW_CURSOR_NORMAL)
            {
#ifndef __EMSCRIPTEN__
                // HACK: Prevent cursor flicker at center before disabling
                //
                // GLFW_CURSOR_HIDDEN is not implemented in JS Emscripten port
                // of GLFW, resulting in an error console.log() display.
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
#endif
                // Cursor disable is required to temporarily center it for
                // mouselook
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            }
        }
        // Stop mouse look on release, give cursor back. Cursor position stays
        // the same as before mouse look.
        else
        {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        }
    }
}

void App::mouseMoveCallback(GLFWwindow* window,
                            double currentMousePosX,
                            double currentMousePosY)
{
    const glm::vec2 currentMousePosFloat{
        static_cast<float>(currentMousePosX),
        static_cast<float>(currentMousePosY),
    };
    auto* impl = static_cast<App*>(glfwGetWindowUserPointer(window));
    glm::vec2& lastMousePos = impl->lastMousePos_;
    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_RELEASE)
    {
        // Always save position even when not holding down mouse button to avoid
        // sudden jumps when initiating turning
        lastMousePos.x = currentMousePosFloat.x;
        lastMousePos.y = currentMousePosFloat.y;
        return;
    }

    const float xOffset = currentMousePosFloat.x - lastMousePos.x;
    // Reversed because y is bottom to up
    const float yOffset = lastMousePos.y - currentMousePosFloat.y;
    lastMousePos = currentMousePosFloat;

    impl->camera_.look(xOffset, yOffset);
}

void App::processInput()
{
    glfwPollEvents();
#ifndef __EMSCRIPTEN__
    // Exiting here instead of update() avoids delay.
    // No need to exit application from a web browser.
    if (glfwGetKey(window_, GLFW_KEY_ESCAPE) == GLFW_PRESS)
    {
        glfwSetWindowShouldClose(window_, true);
    }
#endif
}

void App::update()
{
    // Update camera here instead of processInput(), otherwise camera movement
    // will be too fast on fast computers.
    if (glfwGetKey(window_, GLFW_KEY_W) == GLFW_PRESS)
    {
        camera_.moveForward(FIXED_UPDATE_TIMESTEP);
    }
    if (glfwGetKey(window_, GLFW_KEY_S) == GLFW_PRESS)
    {
        camera_.moveBackward(FIXED_UPDATE_TIMESTEP);
    }
    if (glfwGetKey(window_, GLFW_KEY_A) == GLFW_PRESS)
    {
        camera_.strafeLeft(FIXED_UPDATE_TIMESTEP);
    }
    if (glfwGetKey(window_, GLFW_KEY_D) == GLFW_PRESS)
    {
        camera_.strafeRight(FIXED_UPDATE_TIMESTEP);
    }

    if (glfwGetKey(window_, GLFW_KEY_SPACE) == GLFW_PRESS)
    {
        camera_.ascend(FIXED_UPDATE_TIMESTEP);
    }
    if (glfwGetKey(window_, GLFW_KEY_C) == GLFW_PRESS)
    {
        camera_.descend(FIXED_UPDATE_TIMESTEP);
    }

#ifndef __EMSCRIPTEN__
    // Update VSync option if changed from GUI
    if (vsyncEnabled_ != drawProps_.vsyncEnabled)
    {
        vsyncEnabled_ = drawProps_.vsyncEnabled;
        glfwSwapInterval(vsyncEnabled_);
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
        drawProps_);
    const Model& activeModel = models_[drawProps_.selectedModelIndex];
    renderer_.draw(activeModel, skybox_);
    gui_.draw();

    glfwSwapBuffers(window_);
}

