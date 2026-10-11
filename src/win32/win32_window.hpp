#ifndef WIN32_WINDOW_HPP_
#define WIN32_WINDOW_HPP_

#include "drawproperties.hpp"
#include "gl/wgl_context.hpp"
#include "utils.hpp"

#include <Windows.h>
#include <array>
#include <functional>
#include <string>
#include <string_view>

class App;

// Only the necessary keys used by app
namespace Key
{
constexpr char A = 'A';
constexpr char C = 'C';
constexpr char D = 'D';
constexpr char ESCAPE = VK_ESCAPE;
constexpr char S = 'S';
constexpr char SPACE = VK_SPACE;
constexpr char W = 'W';
}  // namespace Key

/// Win32 API implementation of application window for Windows platform. Primary
/// motivation to be separate from GLFW is the introduction of Direct3D API.
///
/// TODO: Reuse the same Device Context for switching between OpenGL versions.
///
/// Win32 window actually does not require a window and Device Context
/// recreation to switch between different OpenGL rendering contexts, the
/// same already existing DC can be reused. Keeping it for temporary
/// compatibility with GLFW, but should be optimized to avoid unnecessary
/// window recreations.
/// Keep in mind that GLAD still needs to reload OpenGL function pointers on
/// context recreation.
///
/// NOTE about using Unicode (UTF-16) instead of ANSI: A rule of thumb is to
/// only use ANSI if the application is expected to work on old Windows systems
/// that do not support Unicode (Windows 95, 98, ME). ANSI code pages are a
/// legacy feature and the code pages on Windows depend on system settings that
/// can be different between computers, leading to string corruption between
/// machines and unreliable behavior when relying on Win32 API calls that only
/// accept ANSI string parameters (suffixed with "A"). The wide UTF-16 parameter
/// ("W") variants of Win32 API calls are expected to work consistently between
/// different systems and new Windows applications should use that instead. In
/// fact, there are even newer Windows API functions that don't even have an
/// ANSI variant at all. To learn more about how ANSI code pages used to work,
/// see: https://learn.microsoft.com/en-us/windows/win32/intl/code-pages
///
/// TODO: Unlike GLFW, this implementation does not take DPI scaling into
/// account when creating raw window.
/// TODO: Prepare for Direct3D API.
class Window
{
public:
    using Raw = HWND;

    /// Callback parameters are for raw mouse movement amount from last mouse
    /// position since previous frame (also known as "offset" or "mouse delta").
    using MouseOffsetCallback = std::function<
        void(App& app, const float offsetX, const float offsetY)>;

    Window(App& app);
    ~Window();
    DISABLE_COPY_AND_MOVE(Window)

    bool init(const uint16_t width,
              const uint16_t height,
              std::string_view title,
              const RenderingAPI renderingAPI);
    void cleanup();
    void poll();
    void swapBuffers();
    void hide();

    [[nodiscard]] bool shouldQuit() const;
    void setShouldQuit(const bool quit);
    void setVSyncEnabled(const bool vsyncEnabled);
    // BUG: When window loses focus (ALT+TAB) while key is held down, it gets
    // stuck until same key is pressed again. Present in both legacy WM messages
    // and Raw Input API. Check also with GLFW implementation.
    [[nodiscard]] bool keyPressed(const char key) const;
    // TODO: Could handle more mouse buttons, but right now that would be YAGNI
    [[nodiscard]] bool rightMouseButtonPressed() const;
    void setOnMouseMove(const MouseOffsetCallback& callback);
    void setCursorEnabled(const bool enabled);
    [[nodiscard]] std::pair<int, int> frameBufferSize() const;
    // Needed by ImGUI
    [[nodiscard]] Raw raw() const;

private:
    static LRESULT CALLBACK WndProc(HWND hWnd,
                                    UINT uMsg,
                                    WPARAM wParam,
                                    LPARAM lParam);

    App& m_app;
    bool m_shouldQuit;
    HWND m_hWnd;
    WGLContext m_wglContext;
    std::array<bool, 256> m_keys;
    bool m_rightMouseButton;
    bool m_cursorVisible;

    MouseOffsetCallback m_onMouseMove;

    bool createWindow(HINSTANCE hInstance,
                      const uint16_t width,
                      const uint16_t height,
                      std::string_view title);
    void handleRawInput(const LPARAM lParam);
    void setCursorVisible(const bool visible);
};

inline void Window::swapBuffers()
{
    m_wglContext.swapBuffers();
}

inline void Window::hide()
{
    ::ShowWindow(m_hWnd, SW_HIDE);
}

inline bool Window::shouldQuit() const
{
    return m_shouldQuit;
}

inline void Window::setShouldQuit(const bool quit)
{
    m_shouldQuit = quit;
}

inline void Window::setVSyncEnabled(const bool vsyncEnabled)
{
    m_wglContext.setVSyncEnabled(vsyncEnabled);
}

inline bool Window::keyPressed(const char key) const
{
    return m_keys[key];
}

inline bool Window::rightMouseButtonPressed() const
{
    return m_rightMouseButton;
}

inline void Window::setOnMouseMove(const MouseOffsetCallback& callback)
{
    m_onMouseMove = callback;
}

inline Window::Raw Window::raw() const
{
    return m_hWnd;
}

#endif
