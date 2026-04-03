#include "gameinput_system.hpp"

#include "GameInput.h"

#include <winrt/base.h>

using namespace win32InputUtils;
using namespace GameInput::v3;

namespace
{
winrt::com_ptr<IGameInput> s_gameInput{nullptr};
winrt::com_ptr<IGameInputReading> s_reading{nullptr};

GameInputMouseState s_mouseState{};
// Even the most high-end gaming keyboards rarely support more
// than 16 simultaneous keypresses at a time.
// If needed, max keycount can be queried through
// IGameInputDevice::GetDeviceInfo() as
// GameInputKeyboardInfo::keyCount.
constexpr size_t GAMEINPUT_MAX_KEYCOUNT = 16;
std::array<GameInputKeyState, GAMEINPUT_MAX_KEYCOUNT> s_keyState{0};
}  // namespace

namespace GameInputSystem
{
bool init()
{
    HRESULT hr = GameInputCreate(s_gameInput.put());
    return SUCCEEDED(hr) && s_gameInput;
}

bool valid()
{
    return s_gameInput != nullptr;
}

void process(std::array<bool, 256>& keys,
             bool& rightMouseButton,
             const MouseOffsetFn& onMouseMove)
{
    // Based on sample from
    // https://github.com/microsoft/Xbox-GDK-Samples/blob/main/Samples/System/GamepadKeyboardMouse/GamepadKeyboardMouse.cpp

    // Mouse
    if (SUCCEEDED(s_gameInput->GetCurrentReading(GameInputKindMouse,
                                                 nullptr,
                                                 s_reading.put())))
    {
        static int64_t s_previousOffsetX = 0;
        static int64_t s_previousOffsetY = 0;
        static GameInputMouseState s_previousMouseState{};

        if (s_reading->GetMouseState(&s_mouseState))
        {
            rightMouseButton = s_mouseState.buttons & GameInputMouseRightButton;

            // GameInput mouse position are not relative deltas, but
            // accumulated distances the mouse travelled since GameInput
            // initialization.
            const int64_t offsetX
                = s_mouseState.positionX - s_previousMouseState.positionX;
            const int64_t offsetY
                = s_mouseState.positionY - s_previousMouseState.positionY;

            s_previousOffsetX = offsetX ? offsetX : s_previousOffsetX;
            s_previousOffsetY = offsetY ? offsetY : s_previousOffsetY;
            s_previousMouseState = s_mouseState;

            if (offsetX != 0 || offsetY != 0)
            {
                onMouseMove(static_cast<float>(offsetX),
                            static_cast<float>(offsetY));
            }
        }
    }

    // Keyboard
    keys = {false};
    if (SUCCEEDED(s_gameInput->GetCurrentReading(GameInputKindKeyboard,
                                                 nullptr,
                                                 s_reading.put())))
    {
        const uint32_t keyCount
            = s_reading->GetKeyState(GAMEINPUT_MAX_KEYCOUNT, s_keyState.data());
        for (uint32_t i = 0; i < keyCount; ++i)
        {
            uint8_t virtualKey = s_keyState[i].virtualKey;
            if (virtualKey < INVALID_VIRTUALKEY)
            {
                normalizeRightShiftNumLock(s_keyState[i].scanCode, virtualKey);
                keys[virtualKey] = true;
            }
        }
    }
}
}  // namespace GameInputSystem
