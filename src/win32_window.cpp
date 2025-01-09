#include "win32_window.hpp"

#include "app.hpp"
#include "utils.hpp"

#include "imgui_impl_win32.h"

#include <cassert>

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd,
                                                             UINT msg,
                                                             WPARAM wParam,
                                                             LPARAM lParam);

const wchar_t* Window::APPLICATION_NAME = L"3DRenderer";

Window::Window(App& app)
    : app_{app}
    , shouldQuit_{false}
    , hWnd_{nullptr}
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

    if (!wglContext_.enableWglExtensions(hInstance, windowClass))
    {
        utils::showErrorMessage("unable enable WGL extensions for OpenGL");
        return false;
    }

    if (!createWindow(hInstance, width, height, title))
    {
        utils::showErrorMessage("unable create window");
        return false;
    }

    if (!wglContext_.create(hWnd_, renderingAPI))
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

void Window::cleanup()
{
    wglContext_.cleanup(hWnd_);
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

