#include "renderer.h"

#include "camera.h"
#include "drawproperties.h"
#include "model.h"
#include "shader.h"
#include "skybox.h"
#include "utils.h"

#include "glm/glm.hpp"
#include "glm/gtc/quaternion.hpp"

#ifdef __EMSCRIPTEN__
#include "glad/gles2.h"

#include <GLFW/glfw3.h>  // Use GLFW port from Emscripten
#else
#include "GLFW/glfw3.h"
#include "glad/gl.h"
#endif

Renderer::Renderer(const DrawProperties& drawProps, const Camera& camera)
    : drawProps_(drawProps)
    , camera_(camera)
{
}

bool Renderer::init(GLFWwindow* window)
{
#ifdef __EMSCRIPTEN__
    if (!gladLoadGLES2(glfwGetProcAddress))
#else
    if (!gladLoadGL(glfwGetProcAddress))
#endif
    {
        utils::showErrorMessage("unable to load OpenGL extensions");
        return false;
    }

#ifdef __EMSCRIPTEN__
    shaders_[static_cast<size_t>(ShaderInstance::ModelShader)]
        = Shader::createFromFile("assets/shaders/model_gles3.vert.glsl",
                                 "assets/shaders/model_gles3.frag.glsl");
#else
    shaders_[static_cast<size_t>(ShaderInstance::ModelShader)]
        = Shader::createFromFile("assets/shaders/model_gl4.vert.glsl",
                                 "assets/shaders/model_gl4.frag.glsl");
#endif
    if (!shaders_[static_cast<size_t>(ShaderInstance::ModelShader)])
    {
        return false;
    }

#ifdef __EMSCRIPTEN__
    shaders_[static_cast<size_t>(ShaderInstance::SkyboxShader)]
        = Shader::createFromFile("assets/shaders/skybox_gles3.vert.glsl",
                                 "assets/shaders/skybox_gles3.frag.glsl");
#else
    shaders_[static_cast<size_t>(ShaderInstance::SkyboxShader)]
        = Shader::createFromFile("assets/shaders/skybox_gl4.vert.glsl",
                                 "assets/shaders/skybox_gl4.frag.glsl");
#endif
    if (!shaders_[static_cast<size_t>(ShaderInstance::SkyboxShader)])
    {
        return false;
    }

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    window_ = window;

    return true;
}

void Renderer::prepareDraw()
{
    int frameBufferWidth, frameBufferHeight;
    glfwGetFramebufferSize(window_, &frameBufferWidth, &frameBufferHeight);
    glViewport(0, 0, frameBufferWidth, frameBufferHeight);
    projection_ = glm::perspective(glm::radians(drawProps_.fov),
                                   static_cast<float>(frameBufferWidth)
                                       / static_cast<float>(frameBufferHeight),
                                   0.1F,
                                   100.0F);

    glClearColor(drawProps_.backgroundColor[0],
                 drawProps_.backgroundColor[1],
                 drawProps_.backgroundColor[2],
                 1.0F);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Renderer::drawModel(const Model& model)
{
    const auto& shader
        = shaders_[static_cast<std::uint8_t>(ShaderInstance::ModelShader)];
    shader->use();
    glBindVertexArray(model.vertexArray);

    // Avoid Gimbal-lock
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
    const glm::mat4 view = camera_.makeViewMatrix();
    const glm::mat4 mvp = projection_ * view * modelMatrix;
    const glm::mat3 normalMatrix
        = glm::mat3(glm::transpose(glm::inverse(modelMatrix)));

    shader->setUniform("u_model", modelMatrix);
    shader->setUniform("u_mvp", mvp);
    shader->setUniform("u_normalMatrix", normalMatrix);
    shader->setUniform("u_color", drawProps_.modelColor);
    shader->setUniform("u_light.direction", drawProps_.lightDirection);
    shader->setUniform("u_viewPos", camera_.position());

#ifdef __EMSCRIPTEN__
    shader->setUniform("u_adsProps.diffuseEnabled", drawProps.diffuseEnabled);
    shader->setUniform("u_adsProps.specularEnabled", drawProps.specularEnabled);
#else
    // GLSL subroutines and glPolygonMode are not supported in OpenGL ES3
    shader->updateSubroutines(
        GL_FRAGMENT_SHADER,
        {drawProps_.diffuseEnabled ? "DiffuseEnabled" : "Disabled",
         drawProps_.specularEnabled ? "SpecularEnabled" : "Disabled"});

    glPolygonMode(GL_FRONT_AND_BACK,
                  drawProps_.wireframeModeEnabled ? GL_LINE : GL_FILL);
#endif

    glDrawElements(GL_TRIANGLES,
                   static_cast<GLsizei>(model.indices.size()),
                   GL_UNSIGNED_INT,
                   nullptr);

#ifndef __EMSCRIPTEN__
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
#endif
    glBindVertexArray(0);
}

void Renderer::drawSkybox(const Skybox& skybox)
{
    glDepthFunc(GL_LEQUAL);
    const auto& shader
        = shaders_[static_cast<std::uint8_t>(ShaderInstance::SkyboxShader)];
    shader->use();
    glBindVertexArray(skybox.vertexArray);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, skybox.textureID);

    // Remove camera position transformations but keep rotation by recreating
    // view matrix, then converting to mat3 and back. If you don't do this,
    // skybox will be shown as a shrinked down cube around model.
    const glm::mat4 normalizedView
        = glm::mat4(glm::mat3(camera_.makeViewMatrix()));
    // Concat matrix transformations on CPU to avoid unnecessary multiplications
    // in GLSL. Results would be the same for all vertices.
    const glm::mat4 projectionView = projection_ * normalizedView;

    shader->setUniform("u_projectionView", projectionView);
    constexpr int textureUnit0 = 0;
    shader->setUniform("u_skyboxTexture", textureUnit0);

    glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, nullptr);
    glDepthFunc(GL_LESS);
    glBindVertexArray(0);
}
