#include "glfw_window.hpp"

#include "app.hpp"
#include "utils.hpp"

#ifdef __EMSCRIPTEN__
#include "glad/gles2.h"

#include <emscripten.h>
#else
#include "glad/gl.h"
#endif

Window::Window(App& app)
    : window_{nullptr}
    , app_{app}
{
}

Window::~Window()
{
    cleanup();
}

bool Window::init(const uint16_t width,
                  const uint16_t height,
                  std::string_view title
#ifndef __EMSCRIPTEN__
                  ,
                  const RenderingAPI renderingAPI
#endif
)
{
    // TODO: Init GLFW only once per application lifetime
    if (!glfwInit())
    {
        return false;
    }
    glfwSetErrorCallback([](int /*error*/, const char* description)
                         { utils::showErrorMessage(description); });

#ifdef __EMSCRIPTEN__
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
#else
    const int requestedMajorGLVersion
        = renderingAPI == RenderingAPI::OpenGL46 ? 4 : 3;
    const int requestedMinorGLVersion
        = renderingAPI == RenderingAPI::OpenGL46 ? 6 : 3;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, requestedMajorGLVersion);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, requestedMinorGLVersion);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#endif
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
    glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

    window_ = glfwCreateWindow(width, height, title.data(), nullptr, nullptr);
    if (!window_)
    {
        return false;
    }
    glfwMakeContextCurrent(window_);

#ifdef __EMSCRIPTEN__
    if (!gladLoadGLES2(glfwGetProcAddress))
#else
    if (!gladLoadGL(glfwGetProcAddress))
#endif
    {
        return false;
    }

#ifndef __EMSCRIPTEN__
    setVSyncEnabled(false);
#endif

    DEBUG_ASSERT_GL_VERSION(requestedMajorGLVersion, requestedMinorGLVersion);

    return true;
}

void Window::cleanup()
{
    glfwDestroyWindow(window_);
    window_ = nullptr;
    glfwTerminate();  // TODO: Like glfwInit(), only cleanup once per
                      // application lifetime
}

void Window::setOnMouseMove(const MouseOffsetCallback& callback)
{
    mouseMoveCallback_ = callback;
    glfwSetWindowUserPointer(window_, this);
    glfwSetCursorPosCallback(window_, OnMouseMove);
}

void Window::setCursorEnabled(const bool enabled)
{
    if (!enabled
        && glfwGetInputMode(window_, GLFW_CURSOR) == GLFW_CURSOR_NORMAL)
    {
#ifndef __EMSCRIPTEN__
        // HACK: Hide cursor to prevent flicker at center before disabling
        //
        // GLFW_CURSOR_HIDDEN is not implemented in JS Emscripten port
        // of GLFW, resulting in an error console.log() display.
        glfwSetInputMode(window_, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
#endif
        glfwSetInputMode(window_, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    }
    else if (enabled)
    {
        glfwSetInputMode(window_, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }
}

std::pair<int, int> Window::frameBufferSize() const
{
    int frameBufferWidth;
    int frameBufferHeight;
    glfwGetFramebufferSize(window_, &frameBufferWidth, &frameBufferHeight);
    return {frameBufferWidth, frameBufferHeight};
}

void Window::OnMouseMove(GLFWwindow* window,
                         double currentMousePosX,
                         double currentMousePosY)
{
    auto* impl = static_cast<Window*>(glfwGetWindowUserPointer(window));
    assert(impl->mouseMoveCallback_);

    const glm::vec2 currentMousePosFloat{
        static_cast<float>(currentMousePosX),
        static_cast<float>(currentMousePosY),
    };
    glm::vec2& lastMousePos = impl->lastMousePos_;
    const float xOffset = currentMousePosFloat.x - lastMousePos.x;
    // Reversed because Y is bottom to up in OpenGL
    const float yOffset = lastMousePos.y - currentMousePosFloat.y;

    impl->mouseMoveCallback_(impl->app_, xOffset, yOffset);

    lastMousePos = currentMousePosFloat;
}

