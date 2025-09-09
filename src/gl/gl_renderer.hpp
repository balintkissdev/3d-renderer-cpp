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
        ModelShader,
        SkyboxShader,
    };

#ifndef __EMSCRIPTEN__
    RenderingAPI glVersionAPI_;
#endif
    glm::mat4 view_;
    glm::mat4 projection_;

    std::vector<GLShader> shaders_;
    std::vector<GLModel> models_;
    GLSkybox skybox_;

    bool loadShaders();
    bool loadAssets();
    void drawModels(const Scene& scene);
    void drawSkybox();
};

#endif

