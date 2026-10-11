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
    : m_window{nullptr}
    , m_app{app}
{
}

Window::~Window()
{
    cleanup();
}

bool Window::init(const uint16_t width,
                  const uint16_t height,
                  const char* title
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

    m_window = glfwCreateWindow(width, height, title, nullptr, nullptr);
    if (!m_window)
    {
        return false;
    }
    glfwMakeContextCurrent(m_window);

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
    DEBUG_ASSERT_GL_VERSION(requestedMajorGLVersion, requestedMinorGLVersion);
#endif

    return true;
}

void Window::cleanup()
{
    glfwDestroyWindow(m_window);
    m_window = nullptr;
    glfwTerminate();  // TODO: Like glfwInit(), only cleanup once per
                      // application lifetime
}

void Window::setOnMouseMove(const MouseOffsetCallback& callback)
{
    m_mouseMoveCallback = callback;
    glfwSetWindowUserPointer(m_window, this);
    glfwSetCursorPosCallback(m_window, OnMouseMove);
}

void Window::setCursorEnabled(const bool enabled)
{
    if (!enabled
        && glfwGetInputMode(m_window, GLFW_CURSOR) == GLFW_CURSOR_NORMAL)
    {
#ifndef __EMSCRIPTEN__
        // HACK: Hide cursor to prevent flicker at center before disabling
        //
        // GLFW_CURSOR_HIDDEN is not implemented in JS Emscripten port
        // of GLFW, resulting in an error console.log() display.
        glfwSetInputMode(m_window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
#endif
        glfwSetInputMode(m_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    }
    else if (enabled)
    {
        glfwSetInputMode(m_window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }
}

std::pair<int, int> Window::frameBufferSize() const
{
    int frameBufferWidth;
    int frameBufferHeight;
    glfwGetFramebufferSize(m_window, &frameBufferWidth, &frameBufferHeight);
    return {frameBufferWidth, frameBufferHeight};
}

void Window::OnMouseMove(GLFWwindow* window,
                         double currentMousePosX,
                         double currentMousePosY)
{
    auto* impl = static_cast<Window*>(glfwGetWindowUserPointer(window));
    assert(impl->m_mouseMoveCallback);

    const glm::vec2 currentMousePosFloat{
        static_cast<float>(currentMousePosX),
        static_cast<float>(currentMousePosY),
    };
    glm::vec2& lastMousePos = impl->m_lastMousePos;
    const float xOffset = currentMousePosFloat.x - lastMousePos.x;
    // Reversed because Y is bottom to up in OpenGL
    const float yOffset = lastMousePos.y - currentMousePosFloat.y;

    impl->m_mouseMoveCallback(impl->m_app, xOffset, yOffset);

    lastMousePos = currentMousePosFloat;
}

