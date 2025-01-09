#ifndef RENDERER_HPP_
#define RENDERER_HPP_

#include "drawproperties.hpp"
#include "shader.hpp"
#include "utils.hpp"

#include "glm/mat4x4.hpp"

#include <vector>

class Camera;
class Skybox;
class Model;
class Scene;
class Window;
struct DrawProperties;
struct GLFWwindow;

/// Separation of graphics API-dependent rendering mechanisms.
class Renderer
{
public:
    Renderer(const Window& window,
             const DrawProperties& drawProps,
             const Camera& camera);
    DISABLE_COPY_AND_MOVE(Renderer)

    /// Create required shaders and set OpenGL capabilities.
    bool init(
#ifndef __EMSCRIPTEN__
        const RenderingAPI renderingAPI
#endif
    );
    void cleanup();
    void draw(const Scene& scene,
              const std::vector<Model>& models,
              const Skybox& skybox);

    // Screen update and buffer swap is responsibility of window

private:
    enum class ShaderInstance : uint8_t
    {
        ModelShader,
        SkyboxShader,
    };

    const Window& window_;
#ifndef __EMSCRIPTEN__
    RenderingAPI renderingAPI_;
#endif
    glm::mat4 view_;
    glm::mat4 projection_;
    std::vector<Shader> shaders_;
    const DrawProperties& drawProps_;
    const Camera& camera_;

    void drawModels(const Scene& scene, const std::vector<Model>& models);
    void drawSkybox(const Skybox& skybox);
};
#endif
