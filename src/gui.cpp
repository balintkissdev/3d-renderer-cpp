#include "gui.hpp"

#include "camera.hpp"
#include "drawproperties.hpp"
#include "renderer.hpp"
#include "scene.hpp"

#include "glm/gtc/type_ptr.hpp"
#include "imgui_impl_opengl3.h"

#include <cassert>

#if defined(WINDOW_PLATFORM_WIN32)
#include "imgui_impl_dx12.h"
#include "imgui_impl_win32.h"

#define ImGui_Impl_NewFrame ImGui_ImplWin32_NewFrame
#define ImGui_Impl_Shutdown ImGui_ImplWin32_Shutdown

namespace
{
constexpr bool DIRECT3D12_SUPPORTED = true;
}

#elif defined(WINDOW_PLATFORM_GLFW)
#include "imgui_impl_glfw.h"

#define ImGui_Impl_NewFrame ImGui_ImplGlfw_NewFrame
#define ImGui_Impl_Shutdown ImGui_ImplGlfw_Shutdown

#ifndef __EMSCRIPTEN__
namespace
{
constexpr bool DIRECT3D12_SUPPORTED = false;
}
#endif
#endif

namespace
{
const std::array SELECTABLE_MODELS{"Cube", "Utah Teapot", "Stanford Bunny"};
}

// TODO: This preprocessor soup is getting ludicrous
Gui::Gui()
    : selectedSceneItem_{-1}
#ifndef __EMSCRIPTEN__
    , supportedRenderingAPIs_{true, true, DIRECT3D12_SUPPORTED}
#endif
{
}

void Gui::init(Renderer& renderer
#ifndef __EMSCRIPTEN__
               ,
               const RenderingAPI newRenderingAPI
#endif
)
{
#ifndef __EMSCRIPTEN__
    currentRenderingAPI_ = newRenderingAPI;
    selectedRenderingAPI_ = newRenderingAPI;
#endif
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    renderer.initImGuiBackend();

    // Disable ImGUI overriding GLFW cursor appearance for right-click mouselook
    ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;

    auto& style = ImGui::GetStyle();
    ImVec4* colors = style.Colors;
    const ImVec4 transparentBackgroundColor{0.1F, 0.1F, 0.1F, 0.5F};
    colors[ImGuiCol_WindowBg] = transparentBackgroundColor;
    colors[ImGuiCol_ChildBg] = transparentBackgroundColor;
    colors[ImGuiCol_TitleBg] = transparentBackgroundColor;
}

void Gui::prepareDraw(
#ifndef __EMSCRIPTEN__
    const FrameRateInfo& frameRateInfo,
#endif
    const Camera& camera,
    DrawProperties& drawProps,
    Scene& scene)
{
#ifndef __EMSCRIPTEN__
    // HACK
    switch (currentRenderingAPI_)
    {
        case RenderingAPI::OpenGL33:
        case RenderingAPI::OpenGL46:
            ImGui_ImplOpenGL3_NewFrame();
            break;
#if defined(WINDOW_PLATFORM_WIN32)
        case RenderingAPI::Direct3D12:
            ImGui_ImplDX12_NewFrame();
            break;
#endif
        default:
            assert(
                "illegal operation during ImGui NewFrame(): non-existent "
                "graphics backend");
    }
#else
    ImGui_ImplOpenGL3_NewFrame();
#endif
    ImGui_Impl_NewFrame();
    ImGui::NewFrame();

#ifdef __EMSCRIPTEN__
    propertiesDialog(camera, drawProps, scene);
#else
    if (selectedRenderingAPI_ != drawProps.renderingAPI)
    {
        confirmRestartDialog(drawProps.renderingAPI);
    }
    else
    {
        propertiesDialog(frameRateInfo, camera, drawProps, scene);
    }
#endif

    ImGui::Render();
}

void Gui::cleanup(
#ifndef __EMSCRIPTEN__
    const RenderingAPI previousRenderingAPI
#endif
)
{
#ifndef __EMSCRIPTEN__
    // HACK
    switch (previousRenderingAPI)
    {
        case RenderingAPI::OpenGL33:
        case RenderingAPI::OpenGL46:
            ImGui_ImplOpenGL3_Shutdown();
            break;
#if defined(WINDOW_PLATFORM_WIN32)
        case RenderingAPI::Direct3D12:
            ImGui_ImplDX12_Shutdown();
            break;
#endif
        default:
            assert(
                "illegal operation during ImGui shutdown: non-existent "
                "graphics backend");
    }
#else
    ImGui_ImplOpenGL3_Shutdown();
#endif
    ImGui_Impl_Shutdown();
    ImGui::DestroyContext();
}

void Gui::propertiesDialog(
#ifndef __EMSCRIPTEN__
    const FrameRateInfo& frameRateInfo,
#endif
    const Camera& camera,
    DrawProperties& drawProps,
    Scene& scene)
{
    ImGui::Begin("Properties", nullptr, ImGuiWindowFlags_NoTitleBar);

    if (ImGui::CollapsingHeader("Help", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::BulletText("Movement: W, A, S, D");
        ImGui::BulletText("Mouse look: Right-click and drag");
        ImGui::BulletText("Ascend: Spacebar");
        ImGui::BulletText("Descend: C");
    }

#ifndef __EMSCRIPTEN__
    rendererSection(frameRateInfo, drawProps);
#endif

    if (ImGui::CollapsingHeader("Camera", ImGuiTreeNodeFlags_DefaultOpen))
    {
        const glm::vec3& cameraPosition = camera.position();
        ImGui::Text("X:%.3f Y:%.3f Z:%.3f",
                    cameraPosition.x,
                    cameraPosition.y,
                    cameraPosition.z);
        const glm::vec2& cameraRotation = camera.rotation();
        ImGui::Text("Yaw:%.1f° Pitch:%.1f°",
                    cameraRotation.x,
                    cameraRotation.y);
        ImGui::SliderFloat("##FOV",
                           &drawProps.fieldOfView,
                           45.0F,
                           120.0F,
                           "FOV = %.1f°");
    }

    sceneOutline(drawProps, scene);
    sceneNodeSection(drawProps, scene);

    ImGui::End();
}

#ifndef __EMSCRIPTEN__
void Gui::disallowRenderingAPIOption(const RenderingAPI renderingAPI)
{
    supportedRenderingAPIs_[static_cast<size_t>(renderingAPI)] = false;
}

void Gui::rendererSection(const FrameRateInfo& frameRateInfo,
                          DrawProperties& drawProps)
{
    if (ImGui::CollapsingHeader("Renderer", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::Text("%.2F FPS, %.6F ms/frame",
                    frameRateInfo.framesPerSecond,
                    frameRateInfo.msPerFrame);
        constexpr std::array selectableAPIs{
            "OpenGL 4.6",
            "OpenGL 3.3",
            "Direct3D 12",
        };
        auto renderingAPI = static_cast<size_t>(selectedRenderingAPI_);
        if (ImGui::BeginCombo("##Rendering API", selectableAPIs[renderingAPI]))
        {
            for (size_t i = 0; i < static_cast<size_t>(RenderingAPI::Count);
                 ++i)
            {
                // Display unsupported APIs as unselectable
                if (!supportedRenderingAPIs_[i])
                {
                    ImGui::PushStyleColor(
                        ImGuiCol_Text,
                        ImGui::GetStyle().Colors[ImGuiCol_TextDisabled]);
                    ImGui::Text("%s (Unsupported)", selectableAPIs[i]);
                    ImGui::PopStyleColor();
                    continue;
                }

                // Handle apply changes on selection
                const bool selected = (renderingAPI == i);
                if (ImGui::Selectable(selectableAPIs[i], selected))
                {
                    renderingAPI = i;
                    selectedRenderingAPI_
                        = static_cast<RenderingAPI>(renderingAPI);
                }

                // Set initial focus when opening
                if (selected)
                {
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }
        ImGui::Checkbox("Vertical sync", &drawProps.vsyncEnabled);
        // TODO: Wireframe fill mode requires creation of a separate PSO on
        // D3D12
        const bool direct3DEnabled
            = currentRenderingAPI_ == RenderingAPI::Direct3D12;
        ImGui::BeginDisabled(direct3DEnabled);
        bool tmpDisabled = false;
        ImGui::Checkbox(
            "Wireframe mode",
            direct3DEnabled ? &tmpDisabled : &drawProps.wireframeModeEnabled);
        ImGui::EndDisabled();
    }
}

void Gui::confirmRestartDialog(RenderingAPI& renderingAPI)
{
    ImGuiWindowFlags flags = ImGuiWindowFlags_Modal | ImGuiWindowFlags_NoMove
                           | ImGuiWindowFlags_NoDecoration
                           | ImGuiWindowFlags_AlwaysAutoResize
                           | ImGuiWindowFlags_NoSavedSettings;
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->GetCenter(),
                            ImGuiCond_Always,
                            ImVec2(0.5F, 0.5F));
    ImGui::Begin("##Confirm renderer restart", nullptr, flags);
    ImGui::Text(
        "Changing rendering API requires restarting the renderer. Application "
        "state and settings will be unaffected.");
    ImGui::Spacing();
    ImGui::Text("Are you sure you want to restart the renderer?");
    ImGui::Spacing();
    const float buttonWidth = 120.0F;
    const float totalWidth
        = (buttonWidth * 2) + ImGui::GetStyle().ItemSpacing.x;
    const float windowWidth = ImGui::GetWindowSize().x;
    const float startX = (windowWidth - totalWidth) * 0.5F;
    ImGui::SetCursorPosX(startX);

    if (ImGui::Button("Yes", ImVec2(buttonWidth, 0)))
    {
        renderingAPI = selectedRenderingAPI_;
    }

    ImGui::SameLine();

    if (ImGui::Button("No", ImVec2(buttonWidth, 0)))
    {
        selectedRenderingAPI_ = renderingAPI;
    }
    ImGui::End();
}
#endif

void Gui::sceneOutline(DrawProperties& drawProps, Scene& scene)
{
    if (ImGui::CollapsingHeader("Scene outline",
                                ImGuiTreeNodeFlags_DefaultOpen))
    {
        if (ImGui::TreeNodeEx("Scene", ImGuiTreeNodeFlags_DefaultOpen))
        {
            sceneContextMenu(scene);

            // TODO: Skybox is not a real scene graph yet
            if (ImGui::TreeNodeEx("Skybox",
                                  highlightIfSelected(SkyboxTreeIndex)))
            {
                selectIfClicked(SkyboxTreeIndex);
                if (ImGui::BeginPopupContextItem())
                {
                    if (ImGui::MenuItem(drawProps.skyboxEnabled ? "Hide"
                                                                : "Show"))
                    {
                        drawProps.skyboxEnabled = !drawProps.skyboxEnabled;
                    }
                    ImGui::EndPopup();
                }
                ImGui::TreePop();
            }

            // TODO: Lighting is not a real scene graph yet
            if (ImGui::TreeNodeEx("Directional light",
                                  highlightIfSelected(LightingTreeIndex)))
            {
                selectIfClicked(LightingTreeIndex);
                ImGui::TreePop();
            }

            populateTreeFromSceneNodes(scene);
        }
    }
}

void Gui::sceneContextMenu(Scene& scene)
{
    if (ImGui::BeginPopupContextItem())
    {
        if (ImGui::BeginMenu("Add model"))
        {
            for (size_t i = 0; i < SELECTABLE_MODELS.size(); ++i)
            {
                if (ImGui::MenuItem(SELECTABLE_MODELS[i]))
                {
                    scene.add({"Model", i});
                    selectedSceneItem_
                        = static_cast<int>(scene.children().size())
                        + LightingTreeIndex;
                }
            }
            ImGui::EndMenu();
        }
        ImGui::EndPopup();
    }
}

void Gui::populateTreeFromSceneNodes(Scene& scene)
{
    constexpr int selectionStart = LightingTreeIndex + 1;
    for (size_t sceneNodeIndex = 0, selectionIndex = selectionStart;
         sceneNodeIndex < scene.children().size();)
    {
        if (ImGui::TreeNodeEx(scene.children()[sceneNodeIndex].label.c_str(),
                              highlightIfSelected(selectionIndex)))
        {
            selectIfClicked(selectionIndex);

            bool deleteFlag = false;

            // BUG: Repeated Delete menu for scene nodes with same name
            if (ImGui::BeginPopupContextItem())
            {
                if (ImGui::MenuItem("Delete"))
                {
                    deleteFlag = true;
                }
                ImGui::EndPopup();
            }

            ImGui::TreePop();

            if (deleteFlag)
            {
                if (selectionStart <= selectedSceneItem_
                    && selectedSceneItem_ == static_cast<int>(selectionIndex))
                {
                    selectedSceneItem_ = -selectionStart;
                }
                scene.remove(sceneNodeIndex);
                continue;  // Avoid accidentally advancing iterator
            }
        }
        ++sceneNodeIndex;
        ++selectionIndex;
    }

    ImGui::TreePop();
}

ImGuiTreeNodeFlags Gui::highlightIfSelected(const size_t selectionIndex) const
{
    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_Leaf;
    if (selectedSceneItem_ == static_cast<int>(selectionIndex))
    {
        flags |= ImGuiTreeNodeFlags_Selected;
    }
    return flags;
}

void Gui::selectIfClicked(const size_t selectionIndex)
{
    if (ImGui::IsItemClicked())
    {
        selectedSceneItem_ = static_cast<int>(selectionIndex);
    }
}

void Gui::sceneNodeSection(DrawProperties& drawProps, Scene& scene) const
{
    // Skybox
    if (selectedSceneItem_ == SkyboxTreeIndex
        && ImGui::CollapsingHeader("Skybox/Background",
                                   ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::Checkbox("Enable skybox", &drawProps.skyboxEnabled);
        ImGui::Text("Background clear color");
        ImGui::ColorEdit3("##Background clear color",
                          drawProps.backgroundColor.data());
    }

    // Lighting
    if (selectedSceneItem_ == LightingTreeIndex
        && ImGui::CollapsingHeader("Lighting", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::SliderFloat3("Direction",
                            drawProps.lightDirection.data(),
                            -1.0F,
                            1.0F);
        ImGui::Checkbox("Diffuse", &drawProps.diffuseEnabled);
        ImGui::Checkbox("Specular", &drawProps.specularEnabled);
    }

    // Model
    if (LightingTreeIndex < selectedSceneItem_)
    {
        SceneNode& sceneNode = scene.get(selectedSceneItem_ - 2);
        if (ImGui::CollapsingHeader("Transform",
                                    ImGuiTreeNodeFlags_DefaultOpen))
        {
            constexpr float itemWidth = 80.0F;
            constexpr float minPosition = -100.0F;
            constexpr float maxPosition = 100.0F;
            constexpr float positionStep = 0.1F;
            ImGui::Text("Position");
            ImGui::SetNextItemWidth(itemWidth);
            ImGui::DragFloat("##Translate X",
                             &sceneNode.position.x,
                             positionStep,
                             minPosition,
                             maxPosition,
                             "X: %.3f");
            ImGui::SameLine();
            ImGui::SetNextItemWidth(itemWidth);
            ImGui::DragFloat("##Translate Y",
                             &sceneNode.position.y,
                             positionStep,
                             minPosition,
                             maxPosition,
                             "Y: %.3f");
            ImGui::SameLine();
            ImGui::SetNextItemWidth(itemWidth);
            ImGui::DragFloat("##Translate Z",
                             &sceneNode.position.z,
                             positionStep,
                             minPosition,
                             maxPosition,
                             "Z: %.3f");

            constexpr float minRotation = 0.0F;
            constexpr float maxRotation = 360.0F;
            constexpr float rotationStep = 1.0F;
            ImGui::Text("Rotation");
            ImGui::SetNextItemWidth(itemWidth);
            ImGui::DragFloat("##Rotate X",
                             &sceneNode.rotation.x,
                             rotationStep,
                             minRotation,
                             maxRotation,
                             "X: %.0f°");
            ImGui::SameLine();
            ImGui::SetNextItemWidth(itemWidth);
            ImGui::DragFloat("##Rotate Y",
                             &sceneNode.rotation.y,
                             rotationStep,
                             minRotation,
                             maxRotation,
                             "Y: %.0f°");
            ImGui::SameLine();
            ImGui::SetNextItemWidth(itemWidth);
            ImGui::DragFloat("##Rotate Z",
                             &sceneNode.rotation.z,
                             rotationStep,
                             minRotation,
                             maxRotation,
                             "Z: %.0f°");
        }

        if (ImGui::CollapsingHeader("Model", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::Combo("##Selected Model",
                         reinterpret_cast<int*>(&sceneNode.modelID),
                         SELECTABLE_MODELS.data(),
                         static_cast<int>(SELECTABLE_MODELS.size()));
        }

        if (ImGui::CollapsingHeader("Material", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::ColorEdit3("##Solid Color", glm::value_ptr(sceneNode.color));
        }
    }
}
