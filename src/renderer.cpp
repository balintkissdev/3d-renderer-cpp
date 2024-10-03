#include "renderer.hpp"

#include "camera.hpp"
#include "drawproperties.hpp"
#include "model.hpp"
#include "shader.hpp"
#include "skybox.hpp"
#include "utils.hpp"

#include "glm/glm.hpp"
#include "glm/gtc/quaternion.hpp"

#ifdef __EMSCRIPTEN__
#include "glad/gles2.h"

#include <GLFW/glfw3.h>  // Use GLFW port from Emscripten
#else
#include "GLFW/glfw3.h"
#include "glad/gl.h"
#endif

#include <filesystem>

namespace fs = std::filesystem;

Renderer::Renderer(const DrawProperties& drawProps, const Camera& camera)
    : window_{nullptr}
    , drawProps_(drawProps)
    , camera_(camera)
{
}

bool Renderer::init(GLFWwindow* window)
{
    // Set OpenGL function addresses
#ifdef __EMSCRIPTEN__
    if (!gladLoadGLES2(glfwGetProcAddress))
#else
    if (!gladLoadGL(glfwGetProcAddress))
#endif
    {
        utils::showErrorMessage("unable to load OpenGL extensions");
        return false;
    }

    // Load shaders
#ifdef __EMSCRIPTEN__
    const fs::path modelVertexShaderPath(
        "assets/shaders/model_gles3.vert.glsl");
    const fs::path modelFragmentShaderPath(
        "assets/shaders/model_gles3.frag.glsl");
    const fs::path skyboxVertexShaderPath(
        "assets/shaders/skybox_gles3.vert.glsl");
    const fs::path skyboxFragmentShaderPath(
        "assets/shaders/skybox_gles3.frag.glsl");
#else
    const fs::path modelVertexShaderPath("assets/shaders/model_gl4.vert.glsl");
    const fs::path modelFragmentShaderPath(
        "assets/shaders/model_gl4.frag.glsl");
    const fs::path skyboxVertexShaderPath(
        "assets/shaders/skybox_gl4.vert.glsl");
    const fs::path skyboxFragmentShaderPath(
        "assets/shaders/skybox_gl4.frag.glsl");
#endif
    std::optional<Shader> modelShader
        = Shader::createFromFile(modelVertexShaderPath,
                                 modelFragmentShaderPath);
    if (!modelShader)
    {
        return false;
    }

    std::optional<Shader> skyboxShader
        = Shader::createFromFile(skyboxVertexShaderPath,
                                 skyboxFragmentShaderPath);
    if (!skyboxShader)
    {
        return false;
    }
    shaders_.reserve(2);
    shaders_.emplace_back(std::move(modelShader.value()));
    shaders_.emplace_back(std::move(skyboxShader.value()));

    // Customize OpenGL capabilities
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    window_ = window;

    return true;
}

void Renderer::draw(const Model& model, const Skybox& skybox)
{
    // Viewport setup
    //
    // Always query framebuffer size even if the window is not resizable. You'll
    // never know how framebuffer size might differ from window size, especially
    // on high-DPI displays. Not doing so can lead to display bugs like clipping
    // top part of the view.
    int frameBufferWidth, frameBufferHeight;
    glfwGetFramebufferSize(window_, &frameBufferWidth, &frameBufferHeight);
    glViewport(0, 0, frameBufferWidth, frameBufferHeight);
    projection_ = glm::perspective(glm::radians(drawProps_.fieldOfView),
                                   static_cast<float>(frameBufferWidth)
                                       / static_cast<float>(frameBufferHeight),
                                   0.1F,
                                   100.0F);

    // Clear screen
    glClearColor(drawProps_.backgroundColor[0],
                 drawProps_.backgroundColor[1],
                 drawProps_.backgroundColor[2],
                 1.0F);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    drawModel(model);
    if (drawProps_.skyboxEnabled)
    {
        drawSkybox(skybox);
    }
}

void Renderer::drawModel(const Model& model)
{
    // Set model draw shader
    auto& shader
        = shaders_[static_cast<std::uint8_t>(ShaderInstance::ModelShader)];
    shader.use();
    // Set vertex input
    glBindVertexArray(model.vertexArray());

    // Model transform
    // Avoid Gimbal-lock by converting Euler angles to quaternions
    const glm::quat quatX
        = glm::angleAxis(glm::radians(drawProps_.modelRotation[0]),
                         glm::vec3(1.0F, 0.0F, 0.0F));
    const glm::quat quatY
        = glm::angleAxis(glm::radians(drawProps_.modelRotation[1]),
                         glm::vec3(0.0F, 1.0F, 0.0F));
    const glm::quat quatZ
        = glm::angleAxis(glm::radians(drawProps_.modelRotation[2]),
                         glm::vec3(0.0F, 0.0F, 1.0F));
    const glm::quat quat = quatZ * quatY * quatX;
    const auto modelMatrix = glm::mat4_cast(quat);

    // Concat matrix transformations on CPU to avoid unnecessary multiplications
    // in GLSL. Results would be the same for all vertices.
    const glm::mat4 view = camera_.calculateViewMatrix();
    const glm::mat4 mvp = projection_ * view * modelMatrix;
    const glm::mat3 normalMatrix
        = glm::mat3(glm::transpose(glm::inverse(modelMatrix)));

    // Transfer uniforms
    shader.setUniform("u_model", modelMatrix);
    shader.setUniform("u_mvp", mvp);
    shader.setUniform("u_normalMatrix", normalMatrix);
    shader.setUniform("u_color", drawProps_.modelColor);
    shader.setUniform("u_light.direction", drawProps_.lightDirection);
    shader.setUniform("u_viewPos", camera_.position());
#ifdef __EMSCRIPTEN__
    // GLSL subroutines are not supported in OpenGL ES 3.0
    shader.setUniform("u_adsProps.diffuseEnabled", drawProps_.diffuseEnabled);
    shader.setUniform("u_adsProps.specularEnabled", drawProps_.specularEnabled);
#else
    shader.updateSubroutines(
        GL_FRAGMENT_SHADER,
        {drawProps_.diffuseEnabled ? "DiffuseEnabled" : "Disabled",
         drawProps_.specularEnabled ? "SpecularEnabled" : "Disabled"});
    // glPolygonMode is not supported in OpenGL ES 3.0
    glPolygonMode(GL_FRONT_AND_BACK,
                  drawProps_.wireframeModeEnabled ? GL_LINE : GL_FILL);
#endif

    // Issue draw call
    glDrawElements(GL_TRIANGLES,
                   static_cast<GLsizei>(model.indices().size()),
                   GL_UNSIGNED_INT,
                   nullptr);

    // Reset state
#ifndef __EMSCRIPTEN__
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
#endif
    glBindVertexArray(0);
}

void Renderer::drawSkybox(const Skybox& skybox)
{
    // Skybox needs to be drawn at the end of the rendering pipeline for
    // efficiency, not the other way around before objects (like in Painter's
    // Algorithm).
    //
    // Allow skybox pixel depths to pass depth test even when depth buffer is
    // filled with maximum 1.0 depth values. Everything drawn before skybox
    // will be displayed in front of skybox.
    glDepthFunc(GL_LEQUAL);
    // Set skybox shader
    auto& shader
        = shaders_[static_cast<std::uint8_t>(ShaderInstance::SkyboxShader)];
    shader.use();
    glBindVertexArray(skybox.vertexArray());

    // Set skybox texture
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, skybox.textureID());

    // Remove camera position transformations by nullifying column 4, but keep
    // rotation in the view matrix. If you don't do this, skybox will be shown
    // as a shrinked down cube around model.
    glm::mat4 normalizedView = camera_.calculateViewMatrix();
    normalizedView[3] = glm::vec4(0.0F, 0.0F, 0.0F, 0.0F);
    // Concat matrix transformations on CPU to avoid unnecessary
    // multiplications in GLSL. Results would be the same for all vertices.
    const glm::mat4 projectionView = projection_ * normalizedView;

    // Transfer uniforms
    shader.setUniform("u_projectionView", projectionView);
    constexpr int textureUnit = 0;
    shader.setUniform("u_skyboxTexture", textureUnit);

    // Issue draw call
    glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, nullptr);

    // Reset state
    glBindVertexArray(0);
    glDepthFunc(GL_LESS);  // Reset depth testing to default
}
