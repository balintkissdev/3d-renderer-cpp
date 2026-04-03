#ifndef GAMEINPUT_SYSTEM_HPP_
#define GAMEINPUT_SYSTEM_HPP_

#include "win32_inpututils.hpp"

#include <array>

namespace GameInputSystem
{
bool init();
bool valid();
void process(std::array<bool, 256>& keys,
             bool& rightMouseButton,
             const win32InputUtils::MouseOffsetFn& onMouseMove);
};  // namespace GameInputSystem

#endif
