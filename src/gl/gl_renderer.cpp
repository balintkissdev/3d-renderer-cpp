#include "gl_renderer.hpp"

#include "camera.hpp"
#include "gl_model.hpp"
#include "gl_skybox.hpp"
#include "scene.hpp"
#include "utils.hpp"
#include "window.hpp"

#include "glm/glm.hpp"
#include "glm/gtc/quaternion.hpp"
#if defined(WINDOW_PLATFORM_WIN32)
#include "imgui_impl_win32.h"
#elif defined(WINDOW_PLATFORM_GLFW)
#include "imgui_impl_glfw.h"
#endif
#include "imgui_impl_opengl3.h"

#ifdef __EMSCRIPTEN__
#include "glad/gles2.h"
#else
#include "glad/gl.h"
#endif

#include <filesystem>

namespace fs = std::filesystem;

// TODO: This preprocessor soup is getting ludicrous
GLRenderer::GLRenderer(Window& window,
                       const DrawProperties& drawProps,
                       const Camera& camera
#ifndef __EMSCRIPTEN__
                       ,
                       const RenderingAPI renderingAPI
#endif
                       )
    : Renderer{window, drawProps, camera}
#ifndef __EMSCRIPTEN__
    , glVersionAPI_{renderingAPI}
#endif
{
#ifndef __EMSCRIPTEN__
    assert(glVersionAPI_ != RenderingAPI::Direct3D12);
#endif
    shaders_.reserve(2);
    models_.reserve(3);
}

bool GLRenderer::init()
{
    if (!loadShaders() || !loadAssets())
    {
        return false;
    }

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

#ifndef __EMSCRIPTEN__
    setVSyncEnabled(drawProps_.vsyncEnabled);
#endif
    return true;
}

bool GLRenderer::loadShaders()
{
    const fs::path glslShaderBasePath("assets/shaders/glsl/");
#ifdef __EMSCRIPTEN__
    const fs::path modelVertexShaderPath
        = glslShaderBasePath / "model_gles3.vert.glsl";
    const fs::path modelFragmentShaderPath
        = glslShaderBasePath / "model_gles3.frag.glsl";
    const fs::path skyboxVertexShaderPath
        = glslShaderBasePath / "skybox_gles3.vert.glsl";
    const fs::path skyboxFragmentShaderPath
        = glslShaderBasePath / "skybox_gles3.frag.glsl";
#else
    fs::path modelVertexShaderPath;
    fs::path modelFragmentShaderPath;
    fs::path skyboxVertexShaderPath;
    fs::path skyboxFragmentShaderPath;
    if (glVersionAPI_ == RenderingAPI::OpenGL46)
    {
        modelVertexShaderPath = glslShaderBasePath / "model_gl4.vert.glsl";
        modelFragmentShaderPath = glslShaderBasePath / "model_gl4.frag.glsl";
        skyboxVertexShaderPath = glslShaderBasePath / "skybox_gl4.vert.glsl";
        skyboxFragmentShaderPath = glslShaderBasePath / "skybox_gl4.frag.glsl";
    }
    else
    {
        modelVertexShaderPath = glslShaderBasePath / "model_gl3.vert.glsl";
        modelFragmentShaderPath = glslShaderBasePath / "model_gl3.frag.glsl";
        skyboxVertexShaderPath = glslShaderBasePath / "skybox_gl3.vert.glsl";
        skyboxFragmentShaderPath = glslShaderBasePath / "skybox_gl3.frag.glsl";
    }
#endif
    std::optional<GLShader> modelShader
        = GLShader::createFromFile(modelVertexShaderPath,
                                   modelFragmentShaderPath);
    if (!modelShader)
    {
        return false;
    }

    std::optional<GLShader> skyboxShader
        = GLShader::createFromFile(skyboxVertexShaderPath,
                                   skyboxFragmentShaderPath);
    if (!skyboxShader)
    {
        return false;
    }
    shaders_.emplace_back(std::move(modelShader.value()));
    shaders_.emplace_back(std::move(skyboxShader.value()));

    return true;
}

bool GLRenderer::loadAssets()
{
    std::optional<GLSkybox> skybox = GLSkyboxBuilder()
                                         .setRight("assets/skybox/right.jpg")
                                         .setLeft("assets/skybox/left.jpg")
                                         .setTop("assets/skybox/top.jpg")
                                         .setBottom("assets/skybox/bottom.jpg")
                                         .setFront("assets/skybox/front.jpg")
                                         .setBack("assets/skybox/back.jpg")
                                         .build();
    if (!skybox)
    {
        utils::showErrorMessage("unable to create skybox for application");
        return false;
    }
    skybox_ = std::move(skybox.value());

    const std::array<fs::path, 3> modelPaths{"assets/meshes/cube.obj",
                                             "assets/meshes/teapot.obj",
                                             "assets/meshes/bunny.obj"};
    for (const auto& path : modelPaths)
    {
        std::optional<GLModel> model = GLModel::create(path);
        if (!model)
        {
            utils::showErrorMessage("unable to create model from path ", path);
            return false;
        }
        models_.emplace_back(std::move(model.value()));
    }

    return true;
}

void GLRenderer::initImGuiBackend()
{
#if defined(WINDOW_PLATFORM_WIN32)
    ImGui_ImplWin32_InitForOpenGL(window_.raw());
#elif defined(WINDOW_PLATFORM_GLFW)
    ImGui_ImplGlfw_InitForOpenGL(window_.raw(), true);
#endif

    const char* glslVersion =
#ifdef __EMSCRIPTEN__
        "#version 300 es"
#else
        glVersionAPI_ == RenderingAPI::OpenGL46 ? "#version 460 core"
                                                : "#version 330 core"
#endif
        ;
    ImGui_ImplOpenGL3_Init(glslVersion);
}

void GLRenderer::cleanup()
{
    shaders_.clear();
    skybox_.cleanup();
    models_.clear();
}

void GLRenderer::draw(const Scene& scene)
{
    // Viewport setup
    //
    // Always query framebuffer size even if the window is not resizable. You'll
    // never know how framebuffer size might differ from window size, especially
    // on high-DPI displays. Not doing so can lead to display bugs like clipping
    // top part of the view.
    const auto [frameBufferWidth, frameBufferHeight]
        = window_.frameBufferSize();
    if (frameBufferWidth <= 0 || frameBufferHeight <= 0)
    {
        // If frame buffer size is currently (0,0), that means window is
        // minimized (on Windows). Skip drawing.
        // Alternative method is checking on Win32 API side if window was
        // minimized:
        // https://github.com/ocornut/imgui/blob/13c4084362b35ce58a25be70b9f1710dfe3377e9/examples/example_win32_opengl3/main.cpp#L228
        return;
    }

    glViewport(0, 0, frameBufferWidth, frameBufferHeight);
    projection_
        = glm::perspectiveRH(glm::radians(drawProps_.fieldOfView),
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

    drawModels(scene);
    if (drawProps_.skyboxEnabled)
    {
        drawSkybox();
    }

    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    // Buffer swapping behavior is different between OpenGL and Direct3D. While
    // Direct3D manages its own API-specific Swap Chain, buffer swapping in this
    // OpenGL code architecture is delegated to the Win32/GLFW window.
    window_.swapBuffers();
}

void GLRenderer::drawModels(const Scene& scene)
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
    if (glVersionAPI_ == RenderingAPI::OpenGL46)
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
    const GLModel* model = nullptr;
    for (const SceneNode& sceneNode : scene.children())
    {
        // Only bind vertex array if model changes
        if (sceneNode.modelID != cachedModelID)
        {
            model = &models_[sceneNode.modelID];
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

void GLRenderer::drawSkybox()
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
    glBindVertexArray(skybox_.vertexArray());

    // Set skybox texture
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, skybox_.textureID());

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

void GLRenderer::setVSyncEnabled(const bool vsyncEnabled)
{
    window_.setVSyncEnabled(vsyncEnabled);
}

