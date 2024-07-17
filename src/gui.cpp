#include "gui.h"

#include "camera.h"
#include "drawproperties.h"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

void Gui::init(GLFWwindow* window)
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init();

    auto& style = ImGui::GetStyle();
    ImVec4* colors = style.Colors;
    const ImVec4 transparentBackgroundColor = ImVec4(0.1F, 0.1F, 0.1F, 0.5F);
    colors[ImGuiCol_WindowBg] = transparentBackgroundColor;
    colors[ImGuiCol_ChildBg] = transparentBackgroundColor;
    colors[ImGuiCol_TitleBg] = transparentBackgroundColor;
}

void Gui::preRender(const Camera& camera, DrawProperties& drawProps)
{
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    ImGui::Begin("Properties", nullptr, ImGuiWindowFlags_NoTitleBar);

    if (ImGui::CollapsingHeader("Help", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::BulletText("Movement: W, A, S, D");
        ImGui::BulletText("Mouse look: right-click and drag");
        ImGui::BulletText("Ascend: Spacebar");
        ImGui::BulletText("Descend: Right CTRL");
    }

    if (ImGui::CollapsingHeader("Camera", ImGuiTreeNodeFlags_DefaultOpen))
    {
        const glm::vec3& cameraPosition = camera.position();
        const glm::vec2& cameraRotation = camera.rotation();
        ImGui::Text("X:%.3f Y:%.3f Z:%.3f",
                    cameraPosition.x,
                    cameraPosition.y,
                    cameraPosition.z);
        ImGui::Text("Yaw:%.1f° Pitch:%.1f°",
                    cameraRotation.x,
                    cameraRotation.y);
        ImGui::SliderFloat("##FOV",
                           &drawProps.fov,
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
        static const std::array modelItems
            = {"Blender Cube", "Utah Teapot", "Stanford Bunny"};
        ImGui::Combo("##Selected Model",
                     &drawProps.selectedModelIndex,
                     modelItems.data(),
                     static_cast<int>(modelItems.size()));
        ImGui::Checkbox("Wireframe mode", &drawProps.wireframeModeEnabled);
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
        ImGui::SliderFloat3("Sun direction",
                            drawProps.lightDirection.data(),
                            -1.0F,
                            1.0F);
        ImGui::Checkbox("Diffuse", &drawProps.diffuseEnabled);
        ImGui::Checkbox("Specular", &drawProps.specularEnabled);
    }

    ImGui::End();
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

