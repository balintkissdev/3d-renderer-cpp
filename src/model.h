#pragma once

#include "shader.h"

#include "glad/gl.h"
#include "glm/mat4x4.hpp"

#include <memory>
#include <vector>

class Camera;
class DrawProperties;

class Model
{
public:
    static std::unique_ptr<Model> create(const char* filePath);
    ~Model();

    void draw(const glm::mat4& projection,
              const Camera& camera,
              const DrawProperties& drawProps);

private:
    std::unique_ptr<Shader> shader_;
    GLuint vertexBuffer_;
    GLuint indexBuffer_;
    GLuint vertexArray_;
    std::vector<GLuint> indices_;
};
