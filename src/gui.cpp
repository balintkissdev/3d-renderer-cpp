#include "gui.h"

#include "camera.h"
#include "drawproperties.h"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

void Gui::init(GLFWwindow *window)
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init();

    auto &style = ImGui::GetStyle();
    ImVec4 *colors = style.Colors;
    const ImVec4 transparentBackgroundColor = ImVec4(0.1, 0.1, 0.1, 0.5);
    colors[ImGuiCol_WindowBg] = transparentBackgroundColor;
    colors[ImGuiCol_ChildBg] = transparentBackgroundColor;
    colors[ImGuiCol_TitleBg] = transparentBackgroundColor;
}

void Gui::preRender(const Camera &camera, DrawProperties &drawProps)
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
        const glm::vec3 &cameraPosition = camera.position();
        const glm::vec2 &cameraRotation = camera.rotation();
        ImGui::Text("X:%.3f Y:%.3f Z:%.3f", cameraPosition.x, cameraPosition.y, cameraPosition.z);
        ImGui::Text("Yaw:%.1f° Pitch:%.1f°", cameraRotation.x, cameraRotation.y);
        ImGui::SliderFloat("##FOV", &drawProps.fov, 45.0f, 120.0f, "FOV = %.1f°");
        ImGui::Checkbox("Skybox", &drawProps.skyboxEnabled);
        if (!drawProps.skyboxEnabled)
        {
            ImGui::ColorEdit3("Background", drawProps.backgroundColor);
        }
    }

    if (ImGui::CollapsingHeader("Model", ImGuiTreeNodeFlags_DefaultOpen))
    {
        static const char* modelItems[] = {"Blender Cube", "Utah Teapot", "Stanford Bunny"};
        ImGui::Combo("##Selected Model", &drawProps.selectedModelIndex, modelItems, IM_ARRAYSIZE(modelItems));
        ImGui::Checkbox("Wireframe mode", &drawProps.wireframeModeEnabled);
    }

    if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen))
    {
        static constexpr float MIN_MODEL_ROTATION = 0.0f;
        static constexpr float MAX_MODEL_ROTATION = 360.0f;
        ImGui::SliderFloat("##Rotate X", &drawProps.modelRotation[0], MIN_MODEL_ROTATION, MAX_MODEL_ROTATION, "X rotation = %.0f°");
        ImGui::SliderFloat("##Rotate Y", &drawProps.modelRotation[1], MIN_MODEL_ROTATION, MAX_MODEL_ROTATION, "Y rotation = %.0f°");
        ImGui::SliderFloat("##Rotate Z", &drawProps.modelRotation[2], MIN_MODEL_ROTATION, MAX_MODEL_ROTATION, "Z rotation = %.0f°");
    }

    if (ImGui::CollapsingHeader("Material", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::ColorEdit3("##Solid Color", drawProps.modelColor);
    }

    static constexpr float MAX_LIGHT_VALUE = 99.9f;
    static constexpr float MIN_LIGHT_VALUE = -MAX_LIGHT_VALUE;
    if (ImGui::CollapsingHeader("Directional Light", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::SliderFloat3("Direction", drawProps.lightDirection, -1.0f, 1.0f);
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

