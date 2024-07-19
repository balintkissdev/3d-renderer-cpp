#ifndef RENDERER_H_
#define RENDERER_H_

#include "shader.h"

#include "glm/mat4x4.hpp"

#include <array>
#include <memory>

class Camera;
class Skybox;
class Model;
struct DrawProperties;
struct GLFWwindow;

class Renderer
{
public:
    Renderer(const DrawProperties& drawProps, const Camera& camera);
    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;
    Renderer(Renderer&&) noexcept = delete;
    Renderer& operator=(Renderer&&) noexcept = delete;

    bool init(GLFWwindow* window);
    void prepareDraw();
    void drawModel(const Model& model);
    void drawSkybox(const Skybox& skybox);

private:
    enum class ShaderInstance : std::uint8_t
    {
        ModelShader,
        SkyboxShader,
    };

    GLFWwindow* window_;
    const DrawProperties& drawProps_;
    const Camera& camera_;
    std::array<std::unique_ptr<Shader>, 2> shaders_;
    glm::mat4 projection_;
};
#endif
