#include "rawinput_system.hpp"

#include <hidusage.h>

using namespace win32InputUtils;

namespace RawInputSystem
{

bool init(const HWND hWnd)
{
    // Register mouse and keyboard as raw input devices
    std::array<RAWINPUTDEVICE, 2> rawDevices{};
    rawDevices[0].usUsagePage = HID_USAGE_PAGE_GENERIC;
    rawDevices[0].usUsage = HID_USAGE_GENERIC_MOUSE;
    rawDevices[0].dwFlags = RIDEV_INPUTSINK;
    rawDevices[0].hwndTarget = hWnd;

    rawDevices[1].usUsagePage = HID_USAGE_PAGE_GENERIC;
    rawDevices[1].usUsage = HID_USAGE_GENERIC_KEYBOARD;
    // As much as I would like to use RIDEV_NOLEGACY, ImGui does not handle
    // WM_INPUT messages yet, leading to not accepting key presses in text
    // boxes.
    rawDevices[1].hwndTarget = hWnd;

    return ::RegisterRawInputDevices(rawDevices.data(),
                                     rawDevices.size(),
                                     sizeof(rawDevices[0]));
}

void process(const LPARAM lParam,
             std::array<bool, 256>& keys,
             bool& rightMouseButton,
             const MouseOffsetFn& onMouseMove)
{
    UINT dwSize = sizeof(RAWINPUT);
    std::array<BYTE, sizeof(RAWINPUT)> lpb;
    // NOLINTNEXTLINE(performance-no-int-to-ptr)
    ::GetRawInputData(reinterpret_cast<HRAWINPUT>(lParam),
                      RID_INPUT,
                      lpb.data(),
                      &dwSize,
                      sizeof(RAWINPUTHEADER));
    auto* raw = reinterpret_cast<RAWINPUT*>(lpb.data());
    const RAWINPUTHEADER& header = raw->header;

    // Mouse
    if (header.dwType == RIM_TYPEMOUSE)
    {
        const RAWMOUSE& mouse = raw->data.mouse;

        // https://learn.microsoft.com/en-us/windows/win32/api/winuser/ns-winuser-rawmouse
        const USHORT mouseButtonFlags = mouse.usButtonFlags;
        if (mouseButtonFlags & RI_MOUSE_RIGHT_BUTTON_DOWN)
        {
            rightMouseButton = true;
        }
        if (mouseButtonFlags & RI_MOUSE_RIGHT_BUTTON_UP)
        {
            rightMouseButton = false;
        }

        const auto xOffset = static_cast<float>(mouse.lLastX);
        const auto yOffset = static_cast<float>(mouse.lLastY);
        if (xOffset != 0 || yOffset != 0)
        {
            // TODO: MOUSE_MOVE_RELATIVE (0) flag is
            // active and positions are still recorded. Callback is being
            // processed even when window is not focused. No behavior change due
            // to no right mouse click, but can be source of potential bugs.
            onMouseMove(xOffset, yOffset);
        }
        // If you want to implement mouse wheel, make sure to check for unsigned
        // negative overflow errors.
    }
    // Keyboard
    if (header.dwType == RIM_TYPEKEYBOARD)
    {
        // Raw Input API uses clean hardware events instead
        // of legacy WM_KEYDOWN and WM_KEYUP messages. Raw Input messages have
        // better timing and reliability
        // (~500ms initial delay on first WM_KEYDOWN and repeated ~30ms
        // intervals on helds). WM_KEYDOWN messages in Win32 event queue come in
        // periodically when being held, while Raw Input messages fire only
        // once.
        //
        // There's alternatively GetAsyncKeystate, but that can miss
        // important key presses and can introduce bugs by listening on key
        // presses even when the window is not focused or minimized to system
        // tray (disencouraged by the book Game Coding Complete, but encouraged
        // by Tricks of the Windows Game Programming Gurus book).
        const RAWKEYBOARD& keyboard = raw->data.keyboard;
        USHORT virtualKey = keyboard.VKey;
        if (virtualKey < INVALID_VIRTUALKEY)
        {
            uint8_t vk8 = static_cast<uint8_t>(virtualKey);
            normalizeRightShiftNumLock(keyboard.MakeCode, vk8);

            // The flag that is active during key press is RI_KEY_MAKE.
            // RI_KEY_E0 and RI_KEY_E1 are active both during key press and key
            // release.
            // - RI_KEY_E0 is used to distinguish between modifier keys
            //   from left/right and Numpad or regular keys.
            // - RI_KEY_E1 is very rare, either for Pause/Break or old or
            //   specialized keyboards.
            const bool keyPressed = !(keyboard.Flags & RI_KEY_BREAK);
            keys[vk8] = keyPressed;
        }
    }
}

}  // namespace RawInputSystem
