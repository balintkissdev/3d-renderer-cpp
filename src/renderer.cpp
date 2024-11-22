#include "renderer.hpp"

#include "camera.hpp"
#include "model.hpp"
#include "scene.hpp"
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
    shaders_.reserve(2);
}

bool Renderer::init(GLFWwindow* window
#ifndef __EMSCRIPTEN__
                    ,
                    const RenderingAPI renderingAPI
#endif
)
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
    fs::path modelVertexShaderPath;
    fs::path modelFragmentShaderPath;
    fs::path skyboxVertexShaderPath;
    fs::path skyboxFragmentShaderPath;
    if (renderingAPI == RenderingAPI::OpenGL46)
    {
        modelVertexShaderPath = "assets/shaders/model_gl4.vert.glsl";
        modelFragmentShaderPath = "assets/shaders/model_gl4.frag.glsl";
        skyboxVertexShaderPath = "assets/shaders/skybox_gl4.vert.glsl";
        skyboxFragmentShaderPath = "assets/shaders/skybox_gl4.frag.glsl";
    }
    else
    {
        modelVertexShaderPath = "assets/shaders/model_gl3.vert.glsl";
        modelFragmentShaderPath = "assets/shaders/model_gl3.frag.glsl";
        skyboxVertexShaderPath = "assets/shaders/skybox_gl3.vert.glsl";
        skyboxFragmentShaderPath = "assets/shaders/skybox_gl3.frag.glsl";
    }
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
    shaders_.emplace_back(std::move(modelShader.value()));
    shaders_.emplace_back(std::move(skyboxShader.value()));

    // Customize OpenGL capabilities
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    window_ = window;
#ifndef __EMSCRIPTEN__
    renderingAPI_ = renderingAPI;
#endif

    return true;
}

void Renderer::cleanup()
{
    shaders_.clear();
}

void Renderer::draw(const Scene& scene,
                    const std::vector<Model>& models,
                    const Skybox& skybox)
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

    view_ = camera_.calculateViewMatrix();

    // Clear screen
    glClearColor(drawProps_.backgroundColor[0],
                 drawProps_.backgroundColor[1],
                 drawProps_.backgroundColor[2],
                 1.0F);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    drawModels(scene, models);
    if (drawProps_.skyboxEnabled)
    {
        drawSkybox(skybox);
    }
}

void Renderer::drawModels(const Scene& scene, const std::vector<Model>& models)
{
    if (scene.children().empty())
    {
        return;
    }

    // Set model draw shader
    auto& shader = shaders_[static_cast<uint8_t>(ShaderInstance::ModelShader)];
    shader.use();

    // Setup uniform values shared by all scene nodes, avoiding doing
    // unnecessary work during iteration
    shader.setUniform("u_light.direction", drawProps_.lightDirection);
    shader.setUniform("u_viewPos", camera_.position());
#ifdef __EMSCRIPTEN__
    shader.setUniform("u_adsProps.diffuseEnabled", drawProps_.diffuseEnabled);
    shader.setUniform("u_adsProps.specularEnabled", drawProps_.specularEnabled);
#else
    if (renderingAPI_ == RenderingAPI::OpenGL46)
    {
        // GLSL subroutines only became supported starting from OpenGL 4.0
        shader.updateSubroutines(
            GL_FRAGMENT_SHADER,
            {drawProps_.diffuseEnabled ? "DiffuseEnabled" : "Disabled",
             drawProps_.specularEnabled ? "SpecularEnabled" : "Disabled"});
    }
    else
    {
        shader.setUniform("u_adsProps.diffuseEnabled",
                          drawProps_.diffuseEnabled);
        shader.setUniform("u_adsProps.specularEnabled",
                          drawProps_.specularEnabled);
    }

    // glPolygonMode is not supported in OpenGL ES 3.0
    glPolygonMode(GL_FRONT_AND_BACK,
                  drawProps_.wireframeModeEnabled ? GL_LINE : GL_FILL);
#endif

    // TODO: Introduce instanced rendering
    size_t cachedModelID = std::numeric_limits<size_t>::max();
    const Model* model = nullptr;
    for (const SceneNode& sceneNode : scene.children())
    {
        // Only bind vertex array if model changes
        if (sceneNode.modelID != cachedModelID)
        {
            model = &models[sceneNode.modelID];
            glBindVertexArray(model->vertexArray());
        }

        // Model transform
        // Translate
        glm::mat4 modelMatrix
            = glm::translate(glm::mat4(1.0F), sceneNode.position);

        // Avoid Gimbal-lock by converting Euler angles to quaternions
        const glm::quat quatX
            = glm::angleAxis(glm::radians(sceneNode.rotation.x),
                             glm::vec3(1.0F, 0.0F, 0.0F));
        const glm::quat quatY
            = glm::angleAxis(glm::radians(sceneNode.rotation.y),
                             glm::vec3(0.0F, 1.0F, 0.0F));
        const glm::quat quatZ
            = glm::angleAxis(glm::radians(sceneNode.rotation.z),
                             glm::vec3(0.0F, 0.0F, 1.0F));
        const glm::quat quat = quatZ * quatY * quatX;
        modelMatrix *= glm::mat4_cast(quat);

        // Concat matrix transformations on CPU to avoid unnecessary
        // multiplications in GLSL. Results would be the same for all vertices.
        const glm::mat4 mvp = projection_ * view_ * modelMatrix;
        const glm::mat3 normalMatrix
            = glm::mat3(glm::transpose(glm::inverse(modelMatrix)));

        // Scene node-specific uniforms
        shader.setUniform("u_model", modelMatrix);
        shader.setUniform("u_mvp", mvp);
        shader.setUniform("u_normalMatrix", normalMatrix);
        shader.setUniform("u_color", sceneNode.color);

        // Issue draw call
        glDrawElements(GL_TRIANGLES,
                       static_cast<GLsizei>(model->indices().size()),
                       GL_UNSIGNED_INT,
                       nullptr);
    }

    // Reset state
#ifndef __EMSCRIPTEN__
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
#endif
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
    glm::mat4 normalizedView = view_;
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
    glDepthFunc(GL_LESS);  // Reset depth testing to default
}
