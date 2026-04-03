#ifndef INPUTUTILS_HPP_
#define INPUTUTILS_HPP_

#include <Windows.h>
#include <cstdint>
#include <functional>

namespace win32InputUtils
{
// 255 (0xff) is invalid key that shows up for Shift+Home or
// NumLock off. Above this are very specific OEM keys.
constexpr uint8_t INVALID_VIRTUALKEY = 0xff;

using MouseOffsetFn = std::function<void(const float, const float)>;

inline void normalizeRightShiftNumLock(const uint8_t scanCode,
                                       uint8_t& virtualKey)
{
    if (virtualKey == 0)
    {
        switch (scanCode)
        {
            case 0xe036:
                virtualKey = VK_RSHIFT;
                break;
            case 0xe045:
                virtualKey = VK_NUMLOCK;
                break;
            default:
                break;
        }
    }
}

}  // namespace win32InputUtils

#endif
