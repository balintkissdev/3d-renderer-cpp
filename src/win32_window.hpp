#ifndef WIN32_WINDOW_HPP_
#define WIN32_WINDOW_HPP_

#include "drawproperties.hpp"

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
/// Windows out of the box only supports OpenGL 1.1 API with opengl32.dll
/// without installed graphics drivers and later versions are implemented by GPU
/// manufacturer drivers. This is due to Microsoft leaving the OpenGL
/// architecture board back in 2003
/// (https://www.theregister.com/2003/03/03/microsoft_quits_opengl_board/) in
/// order to pool all their resources into DirectX instead. The Windows SDK also
/// only provides gl.h for OpenGL 1.1 only and modern OpenGL functions are
/// usually imported by manually downloading wglext.h headers from the Khronos
/// OpenGL Registry. This is not needed thanks to usage of GLAD in this project.
///
/// In order to access modern OpenGL versions, the WGL extensions have to be
/// loaded, set the pixel format of the OpenGL surface and then create the
/// proper OpenGL Rendering Context.
///
/// The provisions of OpenGL 1.1 in Windows 98, ME and 2000 are done using a
/// software version of OpenGL 1.1. On Windows XP/Vista/7/10, a Direct3D wrapper
/// supporting OpenGL 1.1 does this.
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
    [[nodiscard]] bool keyPressed(const char key) const;
    // TODO: Could handle more mouse buttons, but right now that would be YAGNI
    bool rightMouseButtonPressed() const;
    void setOnMouseMove(const MouseOffsetCallback& callback);
    void setCursorEnabled(const bool enabled);
    [[nodiscard]] std::pair<int, int> frameBufferSize() const;
    // Needed by ImGUI
    [[nodiscard]] Raw raw() const;

private:
    static const wchar_t* APPLICATION_NAME;
    static HMODULE s_openGLDLL;

    // ANSI code pages on Windows can be different between computers, leading to
    // string corruption when only relying on Win32 API calls that only accept
    // ANSI string parameters (suffixed with "A"). The wide UTF-16 parameter
    // ("W") variants of Win32 API calls are used for consistency between
    // different systems. This converter turns a basic UTF-8 string into
    // a 2-byte UTF-16 string.
    //
    // std::u16string was considered for the sake of explicitness, but the
    // resulting occurrences of reinterpret_cast<LPWSTR>() resulted in
    // boilterplate code and std::wstring is guaranteed to be 2 bytes on Windows
    // anyway (only platform difference is that wchar_t is 4 bytes on Linux,
    // but that platform is irrelevant here).
    static std::wstring ToWideString(std::string_view str);
    static void* GLFuncLoader(const char* name);
    static LRESULT CALLBACK WndProc(HWND hWnd,
                                    UINT uMsg,
                                    WPARAM wParam,
                                    LPARAM lParam);

    App& app_;
    bool shouldQuit_;
    HWND hWnd_;
    HDC deviceContext_;
    HGLRC renderingContext_;
    // Key map is filled from the Window Procedure from WM_KEYDOWN and WM_KEYUP
    // messages. WM_KEYDOWN messages in Win32 event queue only come in
    // periodically when being held and are not continous, hence the separate
    // array.
    //
    // There's alternatively GetAsyncKeystate, but that can miss
    // important key presses and can introduce bugs by listening on key presses
    // even when the window is not focused or minimized to system tray
    // (disencouraged by the book Game Coding Complete, but encouraged by
    // Tricks of the Windows Game Programming Gurus book).
    std::array<bool, 256> keys_;
    bool rightMouseButton_;
    bool cursorVisible_;

    MouseOffsetCallback onMouseMove_;

    bool enableWglExtensions(HINSTANCE hInstance);
    bool createWindow(HINSTANCE hInstance,
                      const uint16_t width,
                      const uint16_t height,
                      std::string_view title);
    bool createGLContext(const RenderingAPI renderingAPI);
    void setCursorVisible(const bool visible);
};

inline void Window::swapBuffers()
{
    ::SwapBuffers(deviceContext_);
}

inline void Window::hide()
{
    ::ShowWindow(hWnd_, SW_HIDE);
}

inline bool Window::shouldQuit() const
{
    return shouldQuit_;
}

inline void Window::setShouldQuit(const bool quit)
{
    shouldQuit_ = quit;
}

inline bool Window::keyPressed(const char key) const
{
    return keys_[key];
}

inline bool Window::rightMouseButtonPressed() const
{
    return rightMouseButton_;
}

inline void Window::setOnMouseMove(const MouseOffsetCallback& callback)
{
    onMouseMove_ = callback;
}

inline Window::Raw Window::raw() const
{
    return hWnd_;
}

#endif

