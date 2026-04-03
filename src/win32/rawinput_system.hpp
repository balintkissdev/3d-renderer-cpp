#ifndef RAWINPUT_SYSTEM_HPP_
#define RAWINPUT_SYSTEM_HPP_

#include "win32_inpututils.hpp"

#include <Windows.h>
#include <array>

namespace RawInputSystem
{
bool init(const HWND hWnd);
void process(const LPARAM lParam,
             std::array<bool, 256>& keys,
             bool& rightMouseButton,
             const win32InputUtils::MouseOffsetFn& onMouseMove);
}  // namespace RawInputSystem

#endif
