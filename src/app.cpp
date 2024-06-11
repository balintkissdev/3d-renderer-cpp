#include "app.h"

#include "drawproperties.h"
#include "utils.h"

#include "glad/gl.h"
#include "GLFW/glfw3.h"
#include "glm/gtc/matrix_transform.hpp"

#include <chrono>
#include <iostream>

App::App()
    : window_(nullptr)
    // Positioning and rotation accidentally imitates a right-handed 3D coordinate
    // system with positive Z going farther from model, but this setting is done
    // because of initial orientation of the loaded Stanford Bunny mesh.
    , camera_({1.7f, 1.3f, 4.0f}, {240.0f, -15.0f})
    , windowCallbackData_{
        .camera = camera_,
        .lastMousePos{SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2}
    }
{}

bool App::init()
{
    if (!glfwInit())
    {
        utils::errorMessage("Unable to initialize windowing system. Graphics card needs to support at least OpenGL 4.3.");
        return false;
    }
    glfwSetErrorCallback(ErrorCallback);

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
    glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

    window_ = glfwCreateWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "3D renderer by Bálint Kiss", nullptr, nullptr);
    if (!window_)
    {
        utils::errorMessage("Unable to create window. Graphics card needs to support at least OpenGL 4.3.");
        return false;
    }
    glfwSetWindowUserPointer(window_, &windowCallbackData_);
    glfwSetMouseButtonCallback(window_, MouseButtonCallback);
    glfwSetCursorPosCallback(window_, MouseCursorCallback);
    glfwMakeContextCurrent(window_);

    if (!gladLoadGL(glfwGetProcAddress))
    {
        utils::errorMessage("Unable to load OpenGL extensions. Graphics card needs to support at least OpenGL 4.3.");
        return false;
    }

    gui_.init(window_);

    skybox_ = SkyboxBuilder().setRight("assets/skybox/right.jpg")
                             .setLeft("assets/skybox/left.jpg")
                             .setTop("assets/skybox/top.jpg")
                             .setBottom("assets/skybox/bottom.jpg")
                             .setFront("assets/skybox/front.jpg")
                             .setBack("assets/skybox/back.jpg")
                             .build();

    models_.emplace_back(Model::create("assets/meshes/cube.obj"));
    models_.emplace_back(Model::create("assets/meshes/teapot.obj"));
    models_.emplace_back(Model::create("assets/meshes/bunny.obj"));

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

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
    DrawProperties drawProps = {
        .skyboxEnabled = true,
        .wireframeModeEnabled = false,
        .diffuseEnabled = true,
        .specularEnabled = true,
        .selectedModelIndex = 2,
        .fov = 60.0f,
        .backgroundColor = {0.5f, 0.5f, 0.5f},
        .modelRotation = {0.0f, 0.0f, 0.0f},
        .modelColor = {0.0f, 0.8f, 1.0f},
        .lightDirection = {-0.5f, -1.0f, 0.0f}
    };

    // Frame-rate independent loop with fixed update, variable rendering time.
    //
    // A naive calculation and passing of deltaTime introduces floating point
    // precision errors, leading to choppy movement even on high framerate.
    //
    // Prefer steady_clock over high_resolution_clock, because
    // high_resolution_clock could lie.
    auto previousTime = std::chrono::steady_clock::now();
    float lag = 0.0f;
    while (!glfwWindowShouldClose(window_))
    {
        const auto currentTime = std::chrono::steady_clock::now();
        const float elapsedTime = std::chrono::duration<float>(currentTime - previousTime).count();
        previousTime = currentTime;
        lag += elapsedTime;

        handleInput();

        while (lag >= FIXED_UPDATE_TIMESTEP)
        {
            lag -= FIXED_UPDATE_TIMESTEP;
        }

        gui_.preRender(camera_, drawProps);
        Model *activeModel = models_[drawProps.selectedModelIndex].get();

        int frameBufferWidth = SCREEN_WIDTH;
        int frameBufferHeight = SCREEN_HEIGHT;
        glViewport(0, 0, frameBufferWidth, frameBufferHeight);
        glClearColor(drawProps.backgroundColor[0], drawProps.backgroundColor[1], drawProps.backgroundColor[2], 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        const glm::mat4 projection = glm::perspective(glm::radians(drawProps.fov), (float)frameBufferWidth/(float)frameBufferHeight, 0.1f, 100.0f);

        // TODO: Abstract away renderer implementation when starting working on Direct3D11 and Vulkan
        activeModel->draw(projection, camera_, drawProps);
        if (drawProps.skyboxEnabled)
        {
            skybox_->draw(projection, camera_);
        }
        gui_.draw();

        glfwSwapBuffers(window_);
        glfwPollEvents();
    }
}

void App::ErrorCallback(int error, const char *description)
{
    utils::errorMessage("GLFW error: ", description);
}

void App::MouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
    if (button == GLFW_MOUSE_BUTTON_RIGHT)
    {
        if (action == GLFW_PRESS)
        {
            if (glfwGetInputMode(window, GLFW_CURSOR) == GLFW_CURSOR_NORMAL)
            {
                // HACK: Prevent cursor flicker at center before disabling
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
                // Cursor disable is required to temporarily center it for mouselook
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            }
        }
        else
        {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        }
    }
}

void App::MouseCursorCallback(GLFWwindow* window, double currentMousePosX, double currentMousePosY)
{
    auto *callbackData = static_cast<WindowCallbackData*>(glfwGetWindowUserPointer(window));
    glm::vec2 &lastMousePos = callbackData->lastMousePos;
    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_RELEASE)
    {
        // Avoid sudden jumps when initiating turning
        lastMousePos.x = currentMousePosX;
        lastMousePos.y = currentMousePosY;
        return;
    }

    float xOffset = currentMousePosX - lastMousePos.x;
    float yOffset = lastMousePos.y - currentMousePosY;  // Reversed because y is bottom to up
    lastMousePos.x = currentMousePosX;
    lastMousePos.y = currentMousePosY;

    callbackData->camera.look(xOffset, yOffset);
}

void App::handleInput()
{
    if (glfwGetKey(window_, GLFW_KEY_ESCAPE) == GLFW_PRESS)
    {
        glfwSetWindowShouldClose(window_, true);
    }

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
    if (glfwGetKey(window_, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS)
    {
        camera_.descend(FIXED_UPDATE_TIMESTEP);
    }
}

