#ifndef MODEL_H_
#define MODEL_H_

#include "shader.h"

#ifdef __EMSCRIPTEN__
#include "glad/gles2.h"
#else
#include "glad/gl.h"
#endif
#include "glm/mat4x4.hpp"

#include <memory>
#include <string_view>
#include <vector>

class Camera;
struct DrawProperties;

class Model
{
public:
    static std::unique_ptr<Model> create(std::string_view filePath);

    // TODO: Debug why specifying Rule of Five and private constructor break
    // mesh display

    ~Model();

    void draw(const glm::mat4& projection,
              const Camera& camera,
              const DrawProperties& drawProps);

private:
    struct Vertex
    {
        glm::vec3 position;
        glm::vec3 normal;
    };

    std::unique_ptr<Shader> shader_;
    GLuint vertexBuffer_;
    GLuint indexBuffer_;
    GLuint vertexArray_;
    std::vector<GLuint> indices_;
};

#endif
