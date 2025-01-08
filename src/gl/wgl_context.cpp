#include "wgl_context.hpp"

#include "utils.hpp"

#include "glad/gl.h"
#include "glad/wgl.h"

#include <Libloaderapi.h>

HMODULE WGLContext::s_openGLDLL = nullptr;

WGLContext::WGLContext()
    : deviceContext_{nullptr}
    , renderingContext_{nullptr}
{
}

bool WGLContext::enableWglExtensions(HINSTANCE hInstance,
                                     const WNDCLASSEXW& windowClass)
{
    // Create fake window
    // NOTE: It's possible to have a WGL context with only the same window, but
    // that single window cannot be used to retrieve pixel format that support
    // features like sRGB and multisample framebuffers.
    HWND fakeWindow = ::CreateWindowW(
        windowClass.lpszClassName,
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

bool WGLContext::create(const HWND hWnd, const RenderingAPI renderingAPI)
{
    // Device Context contains the pixel format specification for the draw
    // buffer.
    deviceContext_ = ::GetDC(hWnd);

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

void WGLContext::cleanup(HWND hWnd)
{
    wglDeleteContext(renderingContext_);
    ::ReleaseDC(hWnd, deviceContext_);
}

void* WGLContext::GLFuncLoader(const char* name)
{
    // The default Win32 way to load OpenGL functions that include modern OpenGL
    // without manually loading DLL files is wglGetProcAddress.
    void* glFunc = reinterpret_cast<void*>(wglGetProcAddress(name));

    // WGL extensions are not made for retrieving function pointers that are
    // from OpenGL 1.1. wglGetProcAddress can only retrieve function addresses
    // for extensions and modern OpenGL features (glCreateShader) from the
    // actual drivers made by GPU vendors, but not old functions
    // available in OpenGL version 1.1 (glClearColor). Legacy functions have to
    // be accessed manually from opengl32.dll itself instead.
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

void WGLContext::setVSyncEnabled(const bool vsyncEnabled)
{
    wglSwapIntervalEXT(vsyncEnabled);
}

