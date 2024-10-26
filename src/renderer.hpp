#ifndef RENDERER_HPP_
#define RENDERER_HPP_

#include "drawproperties.hpp"
#include "shader.hpp"

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
    bool init(GLFWwindow* window
#ifndef __EMSCRIPTEN__
              ,
              const RenderingAPI renderingAPI
#endif
    );
    void cleanup();
    void draw(const Model& model, const Skybox& skybox);
    void drawModel(const Model& model);
    void drawSkybox(const Skybox& skybox);

    // Screen update and buffer swap is responsibility of window

private:
    enum class ShaderInstance : uint8_t
    {
        ModelShader,
        SkyboxShader,
    };

    GLFWwindow* window_;
#ifndef __EMSCRIPTEN__
    RenderingAPI renderingAPI_;
#endif
    glm::mat4 projection_;
    std::vector<Shader> shaders_;
    const DrawProperties& drawProps_;
    const Camera& camera_;
};
#endif
