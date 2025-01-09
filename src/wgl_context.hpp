#ifndef WGL_CONTEXT_HPP_
#define WGL_CONTEXT_HPP_

#include "drawproperties.hpp"
#include "utils.hpp"

#include <Windows.h>

/// Win32-specific OpenGL Rendering Context, otherwise WGL context helper.
/// Separated from Win32 window to avoid mixing different rendering APIs in
/// the same windowing code.
///
/// Windows out of the box only supports OpenGL 1.1 API with opengl32.dll
/// without installed graphics drivers and later versions are implemented by GPU
/// manufacturer drivers. This is due to Microsoft leaving the OpenGL
/// architecture board back in 2003
/// (https://www.theregister.com/2003/03/03/microsoft_quits_opengl_board/) in
/// order to pool all their resources into DirectX instead.
///
/// Besides the legacy OpenGL functionality, opengl32.dll also contains the
/// mechanism to load the GPU vendor's OpenGL Installable Client Driver (ICD).
/// ICD serves as the OpenGL API interface for the GPU vendor's User Mode Driver
/// (UMD). UMD contains the actual implementation of vendor-specific GPU
/// commands that get sent to the GPU vendor's Kernel Mode Driver (KMD). UMD
/// drivers will typically have the names "nvd3dum.dll" (NVidia), "atiumd*.dll"
/// (AMD) or "igd10umd32.dll" (Intel). There can be multiple running UMD
/// instances, one for each application instance, but only one KMD is active
/// during an OS run. UMD failures result in application crash, while an error
/// happening in KMD crashes the whole OS.
///
/// The Windows SDK provides gl.h header with functions for OpenGL 1.1 only and
/// modern OpenGL function declarations are usually imported to code by manually
/// downloading wglext.h headers from the Khronos OpenGL Registry. This is not
/// needed thanks to usage of GLAD extension loader in this project.
///
/// In order to access modern OpenGL versions in application code, the WGL
/// extensions have to be loaded first, select the pixel format suitable for the
/// OpenGL surface and then create the proper OpenGL Rendering Context.
///
/// The provisions of OpenGL 1.1 in Windows 98, ME and 2000 are done using a
/// software version of OpenGL 1.1. On Windows XP/Vista/7/10, a Direct3D wrapper
/// supporting OpenGL 1.1 does this.
class WGLContext
{
public:
    WGLContext();
    DISABLE_COPY_AND_MOVE(WGLContext)

    /// OpenGL context creation is not standardized by the Khronos group, the
    /// implementation of context creation is always specified by the platform
    /// API vendor. On Win32, a fake (also known as "temporary context", "helper
    /// context" or "empty context") OpenGL context has to be created before
    /// initializing the real one. The prerequisites for either a fake or real
    /// Win32 OpenGL Rendering Context is having a window handle and a Device
    /// Context in the first place.
    bool enableWglExtensions(HINSTANCE hInstance,
                             const WNDCLASSEXW& windowClass);
    bool create(const HWND hWnd, const RenderingAPI renderingAPI);
    void cleanup(HWND hWnd);

    void swapBuffers();
    void setVSyncEnabled(const bool vsyncEnabled);

private:
    static HMODULE s_openGLDLL;

    static void* GLFuncLoader(const char* name);

    HDC deviceContext_;
    HGLRC renderingContext_;
};

inline void WGLContext::swapBuffers()
{
    ::SwapBuffers(deviceContext_);
}

#endif
