#ifndef SHADER_H_
#define SHADER_H_

#include "glad/gl.h"
#include "glm/mat4x3.hpp"
#include "glm/mat4x4.hpp"
#include "glm/vec3.hpp"

#include <array>
#include <memory>
#include <string_view>
#include <vector>

class Shader
{
public:
    static std::unique_ptr<Shader> createFromFile(
        std::string_view vertexShaderPath,
        std::string_view fragmentShaderPath);

    Shader(const Shader&) = delete;
    Shader& operator=(const Shader&) = delete;

    ~Shader();

    void use() const;

    template <typename T>
    void setUniform(std::string_view name, const T& v);

    void updateSubroutines(const GLenum shaderType,
                           const std::vector<std::string>& names);

private:
    static std::string readFile(std::string_view shaderPath);
    static bool checkCompileErrors(const GLuint shaderID,
                                   const GLenum shaderType);
    static bool checkLinkerErrors(const GLuint shaderID);

    Shader();

    GLuint shaderProgram_;
    std::vector<GLuint> subroutineIndices_;
};

template <>
void Shader::setUniform(std::string_view name, const int& v);
template <>
void Shader::setUniform(std::string_view name, const std::array<float, 3>& v);
template <>
void Shader::setUniform(std::string_view name, const glm::vec3& v);
template <>
void Shader::setUniform(std::string_view name, const glm::mat3& v);
template <>
void Shader::setUniform(std::string_view name, const glm::mat4& v);

#endif
