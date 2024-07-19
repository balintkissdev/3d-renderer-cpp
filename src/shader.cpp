#include "shader.h"

#include "utils.h"

#include "glm/gtc/type_ptr.hpp"

#include <cstdio>
#include <fstream>
#include <ios>

#ifndef NDEBUG
#define ASSERT_UNIFORM(name) assertUniform((name))
#else
#define ASSERT_UNIFORM(name)
#endif

namespace
{
constexpr size_t SHADER_COMPILE_MESSAGE_MAX_LENGTH = 512;
}

std::unique_ptr<Shader> Shader::createFromFile(
    std::string_view vertexShaderPath,
    std::string_view fragmentShaderPath)
{
    const auto vertexShader = compile(vertexShaderPath, GL_VERTEX_SHADER);
    if (!vertexShader)
    {
        return nullptr;
    }

    const auto fragmentShader = compile(fragmentShaderPath, GL_FRAGMENT_SHADER);
    if (!fragmentShader)
    {
        return nullptr;
    }

    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader.value());
    glAttachShader(shaderProgram, fragmentShader.value());
    glLinkProgram(shaderProgram);
    glDeleteShader(vertexShader.value());
    glDeleteShader(fragmentShader.value());

    if (!checkLinkerErrors(shaderProgram))
    {
        return nullptr;
    }

    auto shader = std::unique_ptr<Shader>(new Shader);
    shader->shaderProgram_ = shaderProgram;
    shader->cacheUniforms();
    return shader;
}

Shader::Shader()
    : shaderProgram_{0}
{
}

Shader::~Shader()
{
    glDeleteShader(shaderProgram_);
}

void Shader::use() const
{
    glUseProgram(shaderProgram_);
}

template <>
void Shader::setUniform(const std::string& name, const int& v)
{
    ASSERT_UNIFORM(name);
    glUniform1i(uniformCache_.at(name), v);
}

template <>
void Shader::setUniform(const std::string& name, const bool& v)
{
    ASSERT_UNIFORM(name);
    glUniform1i(uniformCache_.at(name), v);
}

template <>
void Shader::setUniform(const std::string& name, const std::array<float, 3>& v)
{
    ASSERT_UNIFORM(name);
    glUniform3fv(uniformCache_.at(name), 1, v.data());
}

template <>
void Shader::setUniform(const std::string& name, const glm::vec3& v)
{
    ASSERT_UNIFORM(name);
    glUniform3fv(uniformCache_.at(name), 1, glm::value_ptr(v));
}

template <>
void Shader::setUniform(const std::string& name, const glm::mat3& v)
{
    ASSERT_UNIFORM(name);
    glUniformMatrix3fv(uniformCache_.at(name), 1, GL_FALSE, glm::value_ptr(v));
}

template <>
void Shader::setUniform(const std::string& name, const glm::mat4& v)
{
    ASSERT_UNIFORM(name);
    glUniformMatrix4fv(uniformCache_.at(name), 1, GL_FALSE, glm::value_ptr(v));
}

std::optional<GLuint> Shader::compile(std::string_view shaderPath,
                                      const GLenum shaderTpye)
{
    const std::string shaderSrc = readFile(shaderPath);
    const GLchar* shaderGlSrc = shaderSrc.c_str();
    const GLuint shader = glCreateShader(shaderTpye);
    glShaderSource(shader, 1, &shaderGlSrc, nullptr);
    glCompileShader(shader);
    return checkCompileErrors(shader, shaderTpye)
             ? std::optional<GLuint>{shader}
             : std::nullopt;
}

std::string Shader::readFile(std::string_view shaderPath)
{
    std::ifstream file(shaderPath.data(), std::ios::binary);
    file.seekg(0, std::istream::end);
    std::streamsize fileSize(file.tellg());
    file.seekg(0, std::istream::beg);
    std::string shaderSrc(fileSize, 0);
    file.read(shaderSrc.data(), fileSize);
    return shaderSrc;
}

bool Shader::checkCompileErrors(const GLuint shaderID, const GLenum shaderType)
{
    int success;
    std::array<char, SHADER_COMPILE_MESSAGE_MAX_LENGTH> message;
    glGetShaderiv(shaderID, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(shaderID,
                           SHADER_COMPILE_MESSAGE_MAX_LENGTH,
                           nullptr,
                           message.data());
        utils::showErrorMessage(
            shaderType == GL_VERTEX_SHADER ? "vertex" : "fragment",
            " shader compile error: ",
            message.data());
    }

    return success;
}

bool Shader::checkLinkerErrors(const GLuint shaderID)
{
    int success;
    std::array<char, SHADER_COMPILE_MESSAGE_MAX_LENGTH> message;
    glGetProgramiv(shaderID, GL_LINK_STATUS, &success);
    if (!success)
    {
        glGetProgramInfoLog(shaderID,
                            SHADER_COMPILE_MESSAGE_MAX_LENGTH,
                            nullptr,
                            message.data());
        utils::showErrorMessage("shader link error: ", message.data());
    }

    return success;
}

#ifndef __EMSCRIPTEN__
void Shader::updateSubroutines(const GLenum shaderType,
                               const std::vector<std::string>& names)
{
    subroutineIndices_.clear();
    for (const std::string& name : names)
    {
        subroutineIndices_.push_back(
            glGetSubroutineIndex(shaderProgram_, shaderType, name.c_str()));
    }
    glUniformSubroutinesuiv(GL_FRAGMENT_SHADER,
                            static_cast<GLsizei>(subroutineIndices_.size()),
                            subroutineIndices_.data());
}
#endif

void Shader::cacheUniforms()
{
    GLint uniformCount = 0;
    glGetProgramiv(shaderProgram_, GL_ACTIVE_UNIFORMS, &uniformCount);
    if (uniformCount <= 0)
    {
        // No uniforms present, skip
        return;
    }

    constexpr size_t maxBufferSize = 64;
    std::array<char, maxBufferSize> uniformNameBuffer;
    uniformCache_.reserve(uniformCount);
    for (GLint i = 0; i < uniformCount; ++i)
    {
        GLsizei bufferSize;
        GLsizei uniformVarSize;
        GLenum uniformDataType;
        glGetActiveUniform(shaderProgram_,
                           i,
                           maxBufferSize,
                           &bufferSize,
                           &uniformVarSize,
                           &uniformDataType,
                           uniformNameBuffer.data());
        const GLint uniformLocation
            = glGetUniformLocation(shaderProgram_, uniformNameBuffer.data());
        uniformCache_.emplace(std::string(uniformNameBuffer.data(), bufferSize),
                              uniformLocation);
    }
}

#ifndef NDEBUG
void Shader::assertUniform(const std::string& name)
{
    if (!uniformCache_.contains(name))
    {
        std::cerr << "uniform is not in compiled shader code: " << name << '\n';
        assert(false);
    }
}
#endif
