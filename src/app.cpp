#include "app.h"

#include "drawproperties.h"
#include "gui.h"
#include "utils.h"

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
}

bool App::init()
{
    const char* gpuRequirementsMessage
#ifdef __EMSCRIPTEN__
        = "Browser needs to support at least "
          "OpenGL ES 3.0 (WebGL2)";
#else
        = "Graphics card needs to support at least "
          "OpenGL 4.3";
#endif

    // Initialize windowing system
    if (!glfwInit())
    {
        utils::showErrorMessage("unable to initialize windowing system. ",
                                gpuRequirementsMessage);
        return false;
    }
    glfwSetErrorCallback(errorCallback);

    // Create window
#ifdef __EMSCRIPTEN__
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
#else
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
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
        utils::showErrorMessage("unable to create window. ",
                                gpuRequirementsMessage);
        return false;
    }

    // Setup event callbacks
    glfwSetWindowUserPointer(window_, this);
    glfwSetMouseButtonCallback(window_, mouseButtonCallback);
    glfwSetCursorPosCallback(window_, mouseMoveCallback);
    glfwMakeContextCurrent(window_);

    // Init GUI
    Gui::init(window_);

    // Init renderer
    if (!renderer_.init(window_))
    {
        utils::showErrorMessage("unable to initialize renderer. ",
                                gpuRequirementsMessage);
        return false;
    }

    // Load resources
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
    Gui::cleanup();
    glfwDestroyWindow(window_);
    glfwTerminate();
}

void App::run()
{
#ifdef __EMSCRIPTEN__
    // Web build main loop does not rely on a timestep algorithm or passing
    // deltaTime. Instead, the browser calls and iterates the update function
    // frequently using the requestAnimationFrame mechanism. This approach
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
    //
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

        handleInput();

        while (lag >= FIXED_UPDATE_TIMESTEP)
        {
            lag -= FIXED_UPDATE_TIMESTEP;
        }

        render();
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
    app->handleInput();
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
    lastMousePos.x = currentMousePosFloat.x;
    lastMousePos.y = currentMousePosFloat.y;

    impl->camera_.look(xOffset, yOffset);
}

void App::handleInput()
{
    glfwPollEvents();

#ifndef __EMSCRIPTEN__
    // No need to quit application from a web browser
    if (glfwGetKey(window_, GLFW_KEY_ESCAPE) == GLFW_PRESS)
    {
        glfwSetWindowShouldClose(window_, true);
    }
#endif

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
}

void App::render()
{
    Gui::prepareDraw(camera_, drawProps_);
    const Model& activeModel = models_[drawProps_.selectedModelIndex];
    renderer_.prepareDraw();
    renderer_.drawModel(activeModel);
    if (drawProps_.skyboxEnabled)
    {
        renderer_.drawSkybox(skybox_);
    }
    Gui::draw();

    glfwSwapBuffers(window_);
}

