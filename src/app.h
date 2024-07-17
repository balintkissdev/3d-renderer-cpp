#pragma once

#include "camera.h"
#include "model.h"
#include "skybox.h"

#include <cstdint>
#include <memory>

struct GLFWwindow;

class App
{
public:
    App();

    bool init();
    void run();
    void cleanup();

private:
    struct WindowCallbackData
    {
        Camera& camera;
        glm::vec2 lastMousePos;
    };

    static constexpr uint16_t SCREEN_WIDTH = 1024;
    static constexpr uint16_t SCREEN_HEIGHT = 768;
    static constexpr float MAX_LOGIC_UPDATE_PER_SECOND = 60.0F;
    static constexpr float FIXED_UPDATE_TIMESTEP
        = 1.0F / MAX_LOGIC_UPDATE_PER_SECOND;

    static void ErrorCallback(int error, const char* description);
    static void MouseButtonCallback(GLFWwindow* window,
                                    int button,
                                    int action,
                                    int mods);
    static void MouseCursorCallback(GLFWwindow* window,
                                    double currentMousePosX,
                                    double currentMousePosY);

    // TODO: Abstract away window implementation once starting work on native
    // Win32 window
    GLFWwindow* window_;
    Camera camera_;
    std::unique_ptr<Skybox> skybox_;
    std::vector<std::unique_ptr<Model>> models_;
    WindowCallbackData windowCallbackData_;

    void handleInput();
};
