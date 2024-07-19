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

namespace
{
constexpr uint16_t SCREEN_WIDTH = 1024;
constexpr uint16_t SCREEN_HEIGHT = 768;

constexpr float MAX_LOGIC_UPDATE_PER_SECOND = 60.0F;
constexpr float FIXED_UPDATE_TIMESTEP = 1.0F / MAX_LOGIC_UPDATE_PER_SECOND;
}  // namespace

App::App()
    : window_{nullptr}
    , drawProps_(DrawProperties::createDefault())
    // Positioning and rotation accidentally imitates a right-handed 3D
    // coordinate system with positive Z going farther from model, but this
    // setting is done because of initial orientation of the loaded Stanford
    // Bunny mesh.
    , camera_({1.7F, 1.3F, 4.0F}, {240.0F, -15.0F})
    , renderer_(drawProps_, camera_)
    , windowCallbackData_{
          .camera = camera_,
          .lastMousePos{static_cast<float>(SCREEN_WIDTH) / 2.0F,
                        static_cast<float>(SCREEN_HEIGHT) / 2.0F}}
{
}

bool App::init()
{
#ifdef __EMSCRIPTEN__
    const char* gpuRequirementsMessage
        = "Browser needs to support at least "
          "OpenGL ES 3.0 (WebGL2)";
#else
    const char* gpuRequirementsMessage
        = "Graphics card needs to support at least "
          "OpenGL 4.3";
#endif
    if (!glfwInit())
    {
        utils::showErrorMessage("unable to initialize windowing system. ",
                                gpuRequirementsMessage);
        return false;
    }
    glfwSetErrorCallback(errorCallback);

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
    glfwSetWindowUserPointer(window_, &windowCallbackData_);
    glfwSetMouseButtonCallback(window_, mouseButtonCallback);
    glfwSetCursorPosCallback(window_, mouseCursorCallback);
    glfwMakeContextCurrent(window_);

    Gui::init(window_);

    if (!renderer_.init(window_))
    {
        utils::showErrorMessage("unable to initialize renderer. ",
                                gpuRequirementsMessage);
        return false;
    }

    skybox_ = SkyboxBuilder()
                  .setRight("assets/skybox/right.jpg")
                  .setLeft("assets/skybox/left.jpg")
                  .setTop("assets/skybox/top.jpg")
                  .setBottom("assets/skybox/bottom.jpg")
                  .setFront("assets/skybox/front.jpg")
                  .setBack("assets/skybox/back.jpg")
                  .build();
    if (!skybox_)
    {
        utils::showErrorMessage("unable to create skybox for application");
        return false;
    }

    const std::array modelPaths = {"assets/meshes/cube.obj",
                                   "assets/meshes/teapot.obj",
                                   "assets/meshes/bunny.obj"};
    for (std::string_view path : modelPaths)
    {
        auto model = Model::create(path);
        if (!model)
        {
            utils::showErrorMessage("unable to create model from path ", path);
            return false;
        }
        models_.emplace_back(std::move(model));
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
    emscripten_set_main_loop_arg(emscriptenMainLoopCallback, this, 0, 1);
#else
    // Frame-rate independent loop with fixed update, variable rendering time.
    //
    // A naive calculation and passing of deltaTime introduces floating point
    // precision errors, leading to choppy movement even on high framerate.
    //
    // Prefer steady_clock over high_resolution_clock, because
    // high_resolution_clock could lie.
    auto previousTime = std::chrono::steady_clock::now();
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
        if (action == GLFW_PRESS)
        {
            if (glfwGetInputMode(window, GLFW_CURSOR) == GLFW_CURSOR_NORMAL)
            {
#ifndef __EMSCRIPTEN__
                // HACK: Prevent cursor flicker at center before disabling
                //
                // GLFW_CURSOR_HIDDEN is not implemented in JS Emscripten port
                // of GLFW
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
#endif
                // Cursor disable is required to temporarily center it for
                // mouselook
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            }
        }
        else
        {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        }
    }
}

void App::mouseCursorCallback(GLFWwindow* window,
                              double currentMousePosX,
                              double currentMousePosY)
{
    const glm::vec2 currentMousePosFloat = {
        static_cast<float>(currentMousePosX),
        static_cast<float>(currentMousePosY),
    };
    auto* callbackData
        = static_cast<WindowCallbackData*>(glfwGetWindowUserPointer(window));
    glm::vec2& lastMousePos = callbackData->lastMousePos;
    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_RELEASE)
    {
        // Avoid sudden jumps when initiating turning
        lastMousePos.x = currentMousePosFloat.x;
        lastMousePos.y = currentMousePosFloat.y;
        return;
    }

    const float xOffset = currentMousePosFloat.x - lastMousePos.x;
    // Reversed because y is bottom to up
    const float yOffset = lastMousePos.y - currentMousePosFloat.y;
    lastMousePos.x = currentMousePosFloat.x;
    lastMousePos.y = currentMousePosFloat.y;

    callbackData->camera.look(xOffset, yOffset);
}

void App::handleInput()
{
    glfwPollEvents();

#ifndef __EMSCRIPTEN__
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
    const Model& activeModel = *models_[drawProps_.selectedModelIndex];
    renderer_.prepareDraw();
    renderer_.drawModel(activeModel);
    if (drawProps_.skyboxEnabled)
    {
        renderer_.drawSkybox(*skybox_);
    }
    Gui::draw();

    glfwSwapBuffers(window_);
}

