#ifndef MODEL_H_
#define MODEL_H_

#ifdef __EMSCRIPTEN__
#include "glad/gles2.h"
#else
#include "glad/gl.h"
#endif
#include "glm/vec3.hpp"

#include <memory>
#include <string_view>
#include <vector>

class Model
{
public:
    static std::unique_ptr<Model> create(std::string_view filePath);

    // TODO: Debug why specifying Rule of Five and private constructor break
    // mesh display

    ~Model();

    GLuint vertexArray;
    std::vector<GLuint> indices;

private:
    struct Vertex
    {
        glm::vec3 position;
        glm::vec3 normal;
    };

    GLuint vertexBuffer_;
    GLuint indexBuffer_;
};
#endif
