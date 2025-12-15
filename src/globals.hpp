#ifndef GLOBALS_HPP_
#define GLOBALS_HPP_

#include <cstdint>
#include <string_view>

// NOLINTNEXTLINE(readability-identifier-naming)
constexpr std::string_view WINDOW_TITLE = "3D Renderer by Bálint Kiss";

constexpr uint16_t WINDOW_WIDTH = 1024;
constexpr uint16_t WINDOW_HEIGHT = 768;
constexpr float ASPECT_RATIO
    = static_cast<float>(WINDOW_WIDTH) / static_cast<float>(WINDOW_HEIGHT);

constexpr float NEAR_CLIP_DISTANCE_Z = 0.1f;
constexpr float FAR_CLIP_DISTANCE_Z = 100.0f;

namespace Globals
{
extern bool takingScreenshot;
}

#endif
