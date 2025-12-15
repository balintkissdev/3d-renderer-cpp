#ifndef GUI_HPP_
#define GUI_HPP_

#include "drawproperties.hpp"
#include "utils.hpp"
#include "window.hpp"

#include "imgui.h"

#ifdef WINDOW_PLATFORM_GLFW
struct GLFWwindow;
#endif

class Camera;
class Scene;
class Renderer;

/// UI overlay on top of rendered scene to manipulate rendering properties.
///
/// Immediate mode UI does not contain internal state, as it is the
/// application's responsibility to provide that in the form of DrawProperties.
/// Widgets are redrawn for each frame to integrate well into the loop of
/// real-time graphics and game applications.
///
/// TODO: Add ability to rename scene tree node labels.
class Gui
{
public:
    Gui();
    DISABLE_COPY_AND_MOVE(Gui)

    void init(Renderer& renderer
#ifndef __EMSCRIPTEN__
              ,
              const RenderingAPI newRenderingAPI
#endif
    );

    /// Setup UI widgets before submitting to draw call.
    void prepareDraw(
#ifndef __EMSCRIPTEN__
        const FrameRateInfo& frameRateInfo,
#endif
        const Camera& camera,
        DrawProperties& drawProps,
        Scene& scene);
    void cleanup(
#ifndef __EMSCRIPTEN__
        const RenderingAPI previousRenderingAPI
#endif
    );

#ifndef __EMSCRIPTEN__
    void disallowRenderingAPIOption(const RenderingAPI renderingAPI);
#endif

private:
    enum SceneTreeIndices : uint8_t
    {
        SkyboxTreeIndex,
        LightingTreeIndex,
    };

    int selectedSceneItem_;  // -1 means "unselected"

    void propertiesDialog(
#ifndef __EMSCRIPTEN__
        const FrameRateInfo& frameRateInfo,
#endif
        const Camera& camera,
        DrawProperties& drawProps,
        Scene& scene);

#ifndef __EMSCRIPTEN__
    /// Keeping track which rendering API should be selectable in the dropdown
    /// list. Opted for enum-indexed array instead of std::map.
    std::array<bool, 3> supportedRenderingAPIs_;

    // Maintain separate selected and currently used rendering API to avoid
    // crash by calling prepareDraw() with new selectedRenderingAPI_ that is not
    // even initialized yet.
    RenderingAPI currentRenderingAPI_;
    RenderingAPI selectedRenderingAPI_;
    void rendererSection(const FrameRateInfo& frameRateInfo,
                         DrawProperties& drawProps);
    void confirmRestartDialog(RenderingAPI& renderingAPI);
    [[nodiscard]] bool isDirect3DEnabled() const;
#endif

    // BUG: Jank on right-click context menu because camera is also activated at
    // the same time
    void sceneOutline(DrawProperties& drawProps, Scene& scene);
    void sceneContextMenu(Scene& scene);
    void populateTreeFromSceneNodes(Scene& scene);
    [[nodiscard]] ImGuiTreeNodeFlags highlightIfSelected(
        const size_t selectionIndex) const;
    void selectIfClicked(const size_t selectionIndex);
    void sceneNodeSection(DrawProperties& drawProps, Scene& scene) const;
};

#endif
