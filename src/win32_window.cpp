#include "win32_window.hpp"

#include "app.hpp"
#include "utils.hpp"

#include "glad/gl.h"
#include "glad/wgl.h"
#include "imgui_impl_win32.h"

#include <Libloaderapi.h>
#include <cassert>

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd,
                                                             UINT msg,
                                                             WPARAM wParam,
                                                             LPARAM lParam);

const wchar_t* Window::APPLICATION_NAME = L"3DRenderer";

HMODULE Window::s_openGLDLL = nullptr;

Window::Window(App& app)
    : app_{app}
    , shouldQuit_{false}
    , hWnd_{nullptr}
    , deviceContext_{nullptr}
    , renderingContext_{nullptr}
    , keys_{false}
    , rightMouseButton_{false}
    , cursorVisible_{true}
{
}

Window::~Window()
{
    cleanup();
}

bool Window::init(const uint16_t width,
                  const uint16_t height,
                  std::string_view title,
                  const RenderingAPI renderingAPI)
{
    // Register window
    WNDCLASSEXW windowClass = {};
    windowClass.cbSize = sizeof(windowClass);
    windowClass.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    windowClass.lpfnWndProc = Window::WndProc;
    // Returns handle to the current EXE, but be careful if code is running in
    // DLL instead.
    HINSTANCE hInstance = ::GetModuleHandle(nullptr);
    windowClass.hInstance = hInstance;
    windowClass.lpszClassName = APPLICATION_NAME;
    windowClass.hCursor = ::LoadCursorA(nullptr, IDC_ARROW);
    if (!::RegisterClassExW(&windowClass))
    {
        utils::showErrorMessage("unable to register Win32 window class");
        return false;
    }

    if (!enableWglExtensions(hInstance))
    {
        utils::showErrorMessage("unable enable WGL extensions for OpenGL");
        return false;
    }

    if (!createWindow(hInstance, width, height, title))
    {
        utils::showErrorMessage("unable create window");
        return false;
    }

    if (!createGLContext(renderingAPI))
    {
        utils::showErrorMessage("unable create OpenGL Rendering Context");
        return false;
    }

    // Register mouse as raw input device
    RAWINPUTDEVICE rid = {};
    rid.usUsagePage = 0x01;  // HID_USAGE_PAGE_GENERIC
    rid.usUsage = 0x02;  // HID_USAGE_GENERIC_MOUSE
    rid.dwFlags = RIDEV_INPUTSINK;
    rid.hwndTarget = hWnd_;
    ::RegisterRawInputDevices(&rid, 1, sizeof(rid));  // TODO: Error handling

    ::ShowWindow(hWnd_, SW_SHOW);
    ::UpdateWindow(hWnd_);

    return true;
}

// OpenGL context creation is not standardized by the Khronos group, the
// implementation of context creation is always specified by the platform API
// vendor. On Win32, a fake (also known as "temporary context", "helper
// context" or "empty context") OpenGL context has to be created before
// initializing the real one. The prerequisites for either a fake or real Win32
// OpenGL Rendering Context is having a window handle and a Device Context in
// the first place.
bool Window::enableWglExtensions(HINSTANCE hInstance)
{
    // Create fake window
    HWND fakeWindow = ::CreateWindowW(
        APPLICATION_NAME,
        nullptr,
        WS_OVERLAPPEDWINDOW | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
        0,
        0,
        1,
        1,
        nullptr,
        nullptr,
        hInstance,
        nullptr);

    // Get fake GDI Device Context
    HDC fakeDeviceContext = ::GetDC(fakeWindow);

    DEFER({
        ::ReleaseDC(fakeWindow, fakeDeviceContext);
        ::DestroyWindow(fakeWindow);
    });

    // Windows is still just Windows and still operates on palettized CLUT like
    // in the old days. Instead manually passing a pixel format to the
    // application, we ask the OS our desired pixel format specified by the
    // PIXELFORMATDESCRIPTOR from the system palette.
    PIXELFORMATDESCRIPTOR fakePFD = {};
    fakePFD.nSize = sizeof(fakePFD);
    fakePFD.nVersion = 1;
    fakePFD.dwFlags
        = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    fakePFD.iPixelType = PFD_TYPE_RGBA;
    fakePFD.cColorBits = 32;
    fakePFD.cDepthBits = 24;
    fakePFD.cStencilBits = 8;
    fakePFD.iLayerType = PFD_MAIN_PLANE;

    const int pixelFormat = ::ChoosePixelFormat(fakeDeviceContext, &fakePFD);
    // Set the requested pixel format to the Device Context.
    if (pixelFormat == 0
        || !::SetPixelFormat(fakeDeviceContext, pixelFormat, &fakePFD))
    {
        return false;
    }

    // Create fake OpenGL 1.1 context to use for loading WGL function addresses.
    HGLRC fakeRenderingContext = wglCreateContext(fakeDeviceContext);
    if (!fakeRenderingContext)
    {
        return false;
    }
    DEFER(wglDeleteContext(fakeRenderingContext));

    if (!wglMakeCurrent(fakeDeviceContext, fakeRenderingContext))
    {
        return false;
    }

    // Load WGL function addresses
    if (!gladLoadWGL(fakeDeviceContext,
                     reinterpret_cast<GLADloadfunc>(wglGetProcAddress)))
    {
        return false;
    }

    // Fake context was only needed to load WGL function pointers, can be
    // released now (done by DEFER).

    return true;
}

bool Window::createWindow(HINSTANCE hInstance,
                          const uint16_t width,
                          const uint16_t height,
                          std::string_view title)
{
    // Make sure to adjust the size to include the window frame with title bar
    // and borders, otherwise you get wrong framebuffer resolution.
    RECT windowFramedRect{0,
                          0,
                          static_cast<LONG>(width),
                          static_cast<LONG>(height)};
    ::AdjustWindowRect(&windowFramedRect, WS_OVERLAPPEDWINDOW, false);
    const int windowFramedWidth
        = windowFramedRect.right - windowFramedRect.left;
    const int windowFramedHeight
        = windowFramedRect.bottom - windowFramedRect.top;

    hWnd_ = ::CreateWindowExW(0,
                              APPLICATION_NAME,
                              ToWideString(title).data(),
                              WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU
                                  | WS_MINIMIZEBOX | WS_CLIPSIBLINGS
                                  | WS_CLIPCHILDREN,
                              CW_USEDEFAULT,
                              CW_USEDEFAULT,
                              windowFramedWidth,
                              windowFramedHeight,
                              nullptr,
                              nullptr,
                              hInstance,
                              nullptr);
    if (!hWnd_)
    {
        ::UnregisterClassW(APPLICATION_NAME, ::GetModuleHandle(nullptr));
        return false;
    }
    // Save for use inside window procedure
    ::SetWindowLongPtr(hWnd_, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));

    return true;
}

bool Window::createGLContext(const RenderingAPI renderingAPI)
{
    // Device Context contains the pixel format specification for the draw
    // buffer.
    deviceContext_ = ::GetDC(hWnd_);

    // Select pixel format that is suitable for an OpenGL surface. OpenGL
    // Context requires information about window color bit sizes, buffering
    // methods and buffer bit sizes. Win32 ChoosePixelFormat() is not compatible
    // with modern extensions, because that lacks support for multisampling,
    // sRGB pixel formats and floating-point framebuffers.

    // clang-format off
    const std::array pixelAttribs = {
        WGL_DRAW_TO_WINDOW_ARB, GL_TRUE,
        WGL_SUPPORT_OPENGL_ARB, GL_TRUE,
        WGL_DOUBLE_BUFFER_ARB,  GL_TRUE,
        WGL_PIXEL_TYPE_ARB,     WGL_TYPE_RGBA_ARB,
        WGL_ACCELERATION_ARB,   WGL_FULL_ACCELERATION_ARB,
        WGL_COLOR_BITS_ARB,     32,
        WGL_DEPTH_BITS_ARB,     24,
        WGL_STENCIL_BITS_ARB,   8,
        0
    };
    // clang-format on
    int pixelFormat;
    UINT foundFormatCount;
    bool ok = wglChoosePixelFormatARB(deviceContext_,
                                      pixelAttribs.data(),
                                      nullptr,
                                      1U,
                                      &pixelFormat,
                                      &foundFormatCount);
    if (!ok || foundFormatCount == 0)
    {
        return false;
    }

    // Fill PIXELFORMATDESCRIPTOR with information queried by WGL
    PIXELFORMATDESCRIPTOR pfd;
    ::DescribePixelFormat(deviceContext_, pixelFormat, sizeof(pfd), &pfd);
    // Set finalized pixelformat to Device Context
    if (!::SetPixelFormat(deviceContext_, pixelFormat, &pfd))
    {
        return false;
    }

    // Created OpenGL Rendering Context for correct API version
    const int requestedMajorGLVersion
        = (renderingAPI == RenderingAPI::OpenGL46) ? 4 : 3;
    const int requestedMinorGLVersion
        = (renderingAPI == RenderingAPI::OpenGL46) ? 6 : 3;
    // clang-format off
    const std::array contextAttribs = {
        WGL_CONTEXT_MAJOR_VERSION_ARB, requestedMajorGLVersion,
        WGL_CONTEXT_MINOR_VERSION_ARB, requestedMinorGLVersion,
        WGL_CONTEXT_PROFILE_MASK_ARB,  WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
        0
    };
    // clang-format on

    renderingContext_ = wglCreateContextAttribsARB(deviceContext_,
                                                   nullptr,
                                                   contextAttribs.data());
    if (!renderingContext_)
    {
        return false;
    }

    // An OpenGL state/Rendering Context is implicit and global to window
    // belonging to the current thread.
    if (!wglMakeCurrent(deviceContext_, renderingContext_))
    {
        return false;
    }

    // DLL with OpenGL 1.1 functions needed because for GLAD
    s_openGLDLL = ::LoadLibraryA("opengl32.dll");
    if (!s_openGLDLL)
    {
        return false;
    }
    DEFER(::FreeLibrary(s_openGLDLL));

    if (!gladLoadGL(reinterpret_cast<GLADloadfunc>(GLFuncLoader)))
    {
        return false;
    }

    DEBUG_ASSERT_GL_VERSION(requestedMajorGLVersion, requestedMinorGLVersion);

    return true;
}

void* Window::GLFuncLoader(const char* name)
{
    // The default Win32 way to load OpenGL functions that include modern OpenGL
    // without manually loading DLL files is wglGetProcAddress.
    void* glFunc = reinterpret_cast<void*>(wglGetProcAddress(name));

    // Fallback to manualy opengl32.dll symbol load when wglGetProcAddress can't
    // get an address. wglGetProcAddress can only retrieve function addresses
    // for extensions and modern OpenGL features (glCreateShader), but not old
    // functions available in OpenGL version 1.1 (glClearColor). Legacy
    // functions have to be accessed from opengl32.dll itself instead.
    //
    // Be careful, MSDN docs state that error return is supposed to be 0, but
    // certain implementations return 1, 2, 3 or -1 too.
    if (glFunc == nullptr || (glFunc == reinterpret_cast<void*>(1))
        || (glFunc == reinterpret_cast<void*>(2))
        || (glFunc == reinterpret_cast<void*>(3))
        // NOLINTNEXTLINE(performance-no-int-to-ptr)
        || (glFunc == reinterpret_cast<void*>(-1)))
    {
        glFunc = reinterpret_cast<void*>(::GetProcAddress(s_openGLDLL, name));
    }

    return glFunc;
}

void Window::cleanup()
{
    wglDeleteContext(renderingContext_);
    ::ReleaseDC(hWnd_, deviceContext_);
    ::DestroyWindow(hWnd_);
    ::UnregisterClassW(APPLICATION_NAME, ::GetModuleHandle(nullptr));
}

void Window::poll()
{
    MSG msg;
    while (::PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
    {
        // WM_QUIT is sent by PostQuitMessage() and should not be dispatched
        // with DispatchMessage(), as it already notifies the message loop to
        // end.
        if (msg.message == WM_QUIT)
        {
            shouldQuit_ = true;
            return;
        }

        ::TranslateMessage(&msg);
        ::DispatchMessage(&msg);
    }
}

void Window::setVSyncEnabled(const bool vsyncEnabled)
{
    wglSwapIntervalEXT(vsyncEnabled);
}

std::wstring Window::ToWideString(std::string_view str)
{
    if (str.empty())
    {
        return {};
    }

    const int byteCount = ::MultiByteToWideChar(CP_UTF8,
                                                0,
                                                str.data(),
                                                static_cast<int>(str.size()),
                                                nullptr,
                                                0);
    if (byteCount <= 0)
    {
        return {};
    }

    std::wstring wstr(byteCount, '0');
    const int ok = ::MultiByteToWideChar(CP_UTF8,
                                         0,
                                         str.data(),
                                         static_cast<int>(str.size()),
                                         wstr.data(),
                                         byteCount);

    return (0 < ok) ? wstr : std::wstring();
}

LRESULT CALLBACK Window::WndProc(HWND hWnd,
                                 UINT msg,
                                 WPARAM wParam,
                                 LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
    {
        return 0;
    }

    auto* impl
        // NOLINTNEXTLINE(performance-no-int-to-ptr)
        = reinterpret_cast<Window*>(::GetWindowLongPtr(hWnd, GWLP_USERDATA));
    switch (msg)
    {
        case WM_CLOSE:
            ::PostQuitMessage(0);
            break;
        case WM_DESTROY:
        {
            // Avoid issuing PostQuitMessage() calls in WM_DESTROY, because
            // temporary window creation is done due to WGL
            // extension loading and destroying a temp window sends
            // WM_DESTROY to message queue. Avoid temp window cleanup to trigger
            // application shutdown.
            break;
        }
        case WM_INPUT:
        {
            assert(impl->onMouseMove_);

            UINT dwSize = sizeof(RAWINPUT);
            std::array<BYTE, sizeof(RAWINPUT)> lpb;
            // NOLINTNEXTLINE(performance-no-int-to-ptr)
            ::GetRawInputData(reinterpret_cast<HRAWINPUT>(lParam),
                              RID_INPUT,
                              lpb.data(),
                              &dwSize,
                              sizeof(RAWINPUTHEADER));
            auto* raw = reinterpret_cast<RAWINPUT*>(lpb.data());
            if (raw->header.dwType == RIM_TYPEMOUSE)
            {
                const auto xOffset = static_cast<float>(raw->data.mouse.lLastX);
                // Inverted Y because it's bottom to up in OpenGL
                const auto yOffset
                    = static_cast<float>(-raw->data.mouse.lLastY);
                impl->onMouseMove_(impl->app_, xOffset, yOffset);
            }
            break;
        }
        case WM_KEYDOWN:
            impl->keys_[wParam] = true;
            break;
        case WM_KEYUP:
            impl->keys_[wParam] = false;
            break;
        case WM_RBUTTONDOWN:
            impl->rightMouseButton_ = true;
            break;
        case WM_RBUTTONUP:
            impl->rightMouseButton_ = false;
            break;
        case WM_SYSCOMMAND:
        {  // Disable ALT application menu
            if ((wParam & 0xfff0) == SC_KEYMENU)
            {
                return 0;
            }
        }
        break;
        default:
            break;
    }
    return ::DefWindowProcW(hWnd, msg, wParam, lParam);
}

void Window::setCursorEnabled(const bool enabled)
{
    setCursorVisible(enabled);
    if (enabled)
    {
        ::ClipCursor(nullptr);
    }
    else
    {
        // Keep mouse in place
        POINT cursorPos;
        ::GetCursorPos(&cursorPos);
        RECT rect{cursorPos.x, cursorPos.y, cursorPos.x + 1, cursorPos.y + 1};
        ::ClipCursor(&rect);
    }
}

std::pair<int, int> Window::frameBufferSize() const
{
    RECT renderArea;
    ::GetClientRect(hWnd_, &renderArea);
    return {renderArea.right, renderArea.bottom};
}

void Window::setCursorVisible(const bool visible)
{
    // Careful, there's a gotcha in Win32 API. ShowCursor doesn't
    // set a boolean flag internally, but maintains an incrementable counter
    // instead. Multiple ShowCursor(TRUE) calls increment the counter,
    // ShowCursor(FALSE) calls decrement it, and the cursor won't be hidden
    // until the internal counter reaches zero.
    //
    // To avoid bugs, the internal counter is only allowed to be incremented and
    // decremented once.
    //
    // See:
    // https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-showcursor#remarks
    if (visible != cursorVisible_)
    {
        ::ShowCursor(visible);
        cursorVisible_ = visible;
    }
}

