#ifndef GUI_HPP_
#define GUI_HPP_

class Camera;
struct DrawProperties;
struct FrameRateInfo;
struct GLFWwindow;

/// UI overlay on top of rendered scene to manipulate rendering properties.
///
/// Immediate mode UI does not contain internal state, as it is the
/// application's responsibility to provide that in the form of DrawProperties.
/// Widgets are redrawn for each frame to integrate well into the loop of
/// real-time graphics and game applications.
class Gui
{
public:
    static void init(GLFWwindow* window);
    /// Setup UI widgets before submitting to draw call.
    static void prepareDraw(
#ifndef __EMSCRIPTEN__
        const FrameRateInfo& frameRateInfo,
#endif
        const Camera& camera,
        DrawProperties& drawProps);
    static void draw();
    static void cleanup();

    // Kept as class instead of utility function collection like "utils"
    // namespace, as there might be need for storing state in the future.
    Gui() = default;
    Gui(const Gui&) = delete;
    Gui& operator=(const Gui&) = delete;
    Gui(Gui&&) = delete;
    Gui& operator=(Gui&&) = delete;
};

#endif
