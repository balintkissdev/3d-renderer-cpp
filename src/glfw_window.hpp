#ifndef GLFW_WINDOW_HPP_
#define GLFW_WINDOW_HPP_

#include "drawproperties.hpp"

#include "glm/vec2.hpp"

#define GLFW_INCLUDE_NONE  // Avoid GLFW including its own gl.h, it conflicts
                           // with glad/gl.h
#ifdef __EMSCRIPTEN__
#include <GLFW/glfw3.h>  // Use GLFW port from Emscripten
#else
#include "GLFW/glfw3.h"
#endif

#include <functional>
#include <string_view>

class App;
struct GLFWwindow;

namespace Key
{
constexpr int A = GLFW_KEY_A;
constexpr int C = GLFW_KEY_C;
constexpr int D = GLFW_KEY_D;
constexpr int ESCAPE = GLFW_KEY_ESCAPE;
constexpr int S = GLFW_KEY_S;
constexpr int SPACE = GLFW_KEY_SPACE;
constexpr int W = GLFW_KEY_W;
}  // namespace Key

/// GLFW window implementation for use on Linux, Windows (optional) and
/// WebAssembly platforms.
class Window
{
public:
    using Raw = GLFWwindow*;

    /// Callback parameters are for raw mouse movement amount from last mouse
    /// position since previous frame (also known as "offset" or "mouse delta").
    using MouseOffsetCallback = std::function<
        void(App& app, const float offsetX, const float offsetY)>;

    Window(App& app);
    ~Window();

    bool init(const uint16_t width,
              const uint16_t height,
              std::string_view title
#ifndef __EMSCRIPTEN__
              ,
              const RenderingAPI renderingAPI
#endif
    );
    void cleanup();
    void poll();
    void swapBuffers();
    void hide();

    [[nodiscard]] bool shouldQuit() const;
    void setShouldQuit(const bool quit);
    void setVSyncEnabled(const bool vsyncEnabled);
    [[nodiscard]] bool keyPressed(const int key) const;
    // TODO: Could handle more mouse buttons, but right now that would be YAGNI
    [[nodiscard]] bool rightMouseButtonPressed() const;
    void setOnMouseMove(const MouseOffsetCallback& callback);
    void setCursorEnabled(const bool enabled);
    [[nodiscard]] std::pair<int, int> frameBufferSize() const;
    // Needed by ImGUI
    [[nodiscard]] Raw raw() const;

private:
    GLFWwindow* window_;
    App& app_;
    MouseOffsetCallback mouseMoveCallback_;
    glm::vec2 lastMousePos_;

    static void OnMouseMove(GLFWwindow* window,
                            double currentMousePosX,
                            double currentMousePosY);
};

inline void Window::poll()
{
    glfwPollEvents();
}

inline void Window::swapBuffers()
{
    glfwSwapBuffers(window_);
}

inline void Window::hide()
{
    glfwHideWindow(window_);
}

inline bool Window::shouldQuit() const
{
    return glfwWindowShouldClose(window_);
}

inline void Window::setShouldQuit(const bool quit)
{
    glfwSetWindowShouldClose(window_, quit);
}

inline void Window::setVSyncEnabled(const bool vSyncEnabled)
{
    glfwSwapInterval(vSyncEnabled);
}

inline bool Window::keyPressed(const int key) const
{
    return glfwGetKey(window_, key) == GLFW_PRESS;
}

inline bool Window::rightMouseButtonPressed() const
{
    return glfwGetMouseButton(window_, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS;
}

inline Window::Raw Window::raw() const
{
    return window_;
}

#endif

