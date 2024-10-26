#include "gui.hpp"

#include "camera.hpp"
#include "drawproperties.hpp"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

Gui::Gui()
#ifndef __EMSCRIPTEN__
    : supportedRenderingAPIs_{true, true}
#endif
{
}

void Gui::init(GLFWwindow* window
#ifndef __EMSCRIPTEN__
               ,
               const RenderingAPI renderingAPI
#endif
)
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
#ifdef __EMSCRIPTEN__
    const char* glslVersion = "#version 300 es";
    ImGui_ImplGlfw_InstallEmscriptenCanvasResizeCallback("#canvas");
#else
    const char* glslVersion = renderingAPI == RenderingAPI::OpenGL46
                                ? "#version 460 core"
                                : "#version 330 core";
    selectedRenderingAPI_ = renderingAPI;
#endif
    ImGui_ImplOpenGL3_Init(glslVersion);

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
    DrawProperties& drawProps)
{
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

#ifdef __EMSCRIPTEN__
    propertiesDialog(camera, drawProps);
#else
    if (selectedRenderingAPI_ != drawProps.renderingAPI)
    {
        confirmRestartDialog(drawProps.renderingAPI);
    }
    else
    {
        propertiesDialog(frameRateInfo, camera, drawProps);
    }
#endif

    ImGui::Render();
}

void Gui::draw()
{
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void Gui::cleanup()
{
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

void Gui::propertiesDialog(
#ifndef __EMSCRIPTEN__
    const FrameRateInfo& frameRateInfo,
#endif
    const Camera& camera,
    DrawProperties& drawProps)
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
        ImGui::Checkbox("Skybox", &drawProps.skyboxEnabled);
        if (!drawProps.skyboxEnabled)
        {
            ImGui::ColorEdit3("Background", drawProps.backgroundColor.data());
        }
    }

    if (ImGui::CollapsingHeader("Model", ImGuiTreeNodeFlags_DefaultOpen))
    {
        static const std::array modelItems{"Blender Cube",
                                           "Utah Teapot",
                                           "Stanford Bunny"};
        ImGui::Combo("##Selected Model",
                     &drawProps.selectedModelIndex,
                     modelItems.data(),
                     static_cast<int>(modelItems.size()));
#ifndef __EMSCRIPTEN__
        ImGui::Checkbox("Wireframe mode", &drawProps.wireframeModeEnabled);
#endif
    }

    if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen))
    {
        constexpr float minModelRotation = 0.0F;
        constexpr float maxModelRotation = 360.0F;
        ImGui::SliderFloat("##Rotate X",
                           drawProps.modelRotation.data(),
                           minModelRotation,
                           maxModelRotation,
                           "X rotation = %.0f°");
        ImGui::SliderFloat("##Rotate Y",
                           &drawProps.modelRotation[1],
                           minModelRotation,
                           maxModelRotation,
                           "Y rotation = %.0f°");
        ImGui::SliderFloat("##Rotate Z",
                           &drawProps.modelRotation[2],
                           minModelRotation,
                           maxModelRotation,
                           "Z rotation = %.0f°");
    }

    if (ImGui::CollapsingHeader("Material", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::ColorEdit3("##Solid Color", drawProps.modelColor.data());
    }

    if (ImGui::CollapsingHeader("Lighting", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::SliderFloat3("Direction",
                            drawProps.lightDirection.data(),
                            -1.0F,
                            1.0F);
        ImGui::Checkbox("Diffuse", &drawProps.diffuseEnabled);
        ImGui::Checkbox("Specular", &drawProps.specularEnabled);
    }
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
        static constexpr std::array selectableAPIs{"OpenGL 4.6", "OpenGL 3.3"};
        // Suppress clang-tidy out of enum bounds warning
        static constexpr size_t selectableAPIsCount
            = static_cast<size_t>(RenderingAPI::OpenGL33) + 1;
        auto renderingAPI = static_cast<size_t>(selectedRenderingAPI_);
        if (ImGui::BeginCombo("##Rendering API", selectableAPIs[renderingAPI]))
        {
            for (size_t i = 0; i < selectableAPIsCount; ++i)
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
    const float totalWidth = buttonWidth * 2 + ImGui::GetStyle().ItemSpacing.x;
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

