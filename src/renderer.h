#ifndef RENDERER_H_
#define RENDERER_H_

#include "shader.h"

#include "glm/mat4x4.hpp"

#include <vector>

class Camera;
class Skybox;
class Model;
struct DrawProperties;
struct GLFWwindow;

/// Separation of graphics API-dependent rendering mechanisms.
class Renderer
{
public:
    Renderer(const DrawProperties& drawProps, const Camera& camera);
    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;
    Renderer(Renderer&&) noexcept = delete;
    Renderer& operator=(Renderer&&) noexcept = delete;

    /// Load OpenGL function addresses, required shaders and set OpenGL
    /// capabilities.
    bool init(GLFWwindow* window);
    /// Setup viewport and clear screen
    void prepareDraw();
    void drawModel(const Model& model);
    void drawSkybox(const Skybox& skybox);

    // Screen update and buffer swap is responsibility of window

private:
    enum class ShaderInstance : size_t
    {
        ModelShader,
        SkyboxShader,
    };

    GLFWwindow* window_;
    glm::mat4 projection_;
    std::vector<Shader> shaders_;
    const DrawProperties& drawProps_;
    const Camera& camera_;
};
#endif
