#ifndef GL_RENDERER_HPP_
#define GL_RENDERER_HPP_

#include "gl_model.hpp"
#include "gl_shader.hpp"
#include "gl_skybox.hpp"
#include "renderer.hpp"
#include "utils.hpp"

#include "glm/mat4x4.hpp"

#include <vector>

class Camera;
class GLSkybox;
class GLModel;
class Scene;
class Window;
struct DrawProperties;
struct GLFWwindow;

class GLRenderer : public Renderer
{
public:
    GLRenderer(Window& window,
               const DrawProperties& drawProps,
               const Camera& camera
#ifndef __EMSCRIPTEN__
               ,
               const RenderingAPI renderingAPI
#endif
    );
    DISABLE_COPY_AND_MOVE(GLRenderer)

    bool init() final;
    void initImGuiBackend() final;
    void cleanup() final;
    void draw(const Scene& scene) final;
    void setVSyncEnabled(const bool vsyncEnabled) final;

private:
    enum class ShaderInstance : uint8_t
    {
        Gouraud,
        Phong,
        BlinnPhong,
        Skybox,

        Count,
    };

#ifndef __EMSCRIPTEN__
    RenderingAPI m_glVersionAPI;
#endif
    glm::mat4 m_view;
    glm::mat4 m_projection;

    std::vector<GLShader> m_shaders;
    std::vector<GLModel> m_models;
    GLSkybox m_skybox;

    bool loadShaders();
    bool loadAssets();
    void drawModels(const Scene& scene);
    void drawSkybox();
    void screenshot();
};

#endif

