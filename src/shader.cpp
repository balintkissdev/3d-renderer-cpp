#include "shader.h"

#include "utils.h"

#include "glm/gtc/type_ptr.hpp"

#include <cstdio>
#include <fstream>
#include <ios>
#include <utility>

#ifndef NDEBUG
#define ASSERT_UNIFORM(name) assertUniform((name))
#else
#define ASSERT_UNIFORM(name)
#endif

namespace fs = std::filesystem;

std::optional<Shader> Shader::createFromFile(const fs::path& vertexShaderPath,
                                             const fs::path& fragmentShaderPath)
{
    // Compile vertex shader
    const auto vertexShader = compile(vertexShaderPath, GL_VERTEX_SHADER);
    if (!vertexShader)
    {
        return std::nullopt;
    }

    // Compile fragment shader
    const auto fragmentShader = compile(fragmentShaderPath, GL_FRAGMENT_SHADER);
    if (!fragmentShader)
    {
        glDeleteShader(vertexShader.value());
        return std::nullopt;
    }

    // Link shader program
    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader.value());
    glAttachShader(shaderProgram, fragmentShader.value());
    glLinkProgram(shaderProgram);
    // Vertex and fragment shader not needed anymore after linked as part for
    // program.
    glDeleteShader(vertexShader.value());
    glDeleteShader(fragmentShader.value());
    if (!checkLinkerErrors(shaderProgram))
    {
        return std::nullopt;
    }

    return Shader{shaderProgram};
}

Shader::Shader()
    : shaderProgram_{0}
{
}

Shader::Shader(const GLuint shaderProgram)
    : shaderProgram_{shaderProgram}
{
    if (shaderProgram)
    {
        cacheActiveUniforms();
    }
}

Shader::~Shader()
{
    glDeleteShader(shaderProgram_);
}

Shader::Shader(Shader&& other) noexcept
    : shaderProgram_{std::exchange(other.shaderProgram_, 0)}
    , uniformCache_{std::move(other.uniformCache_)}
#ifndef __EMSCRIPTEN__
    , subroutineIndices_{std::move(other.subroutineIndices_)}
#endif
{
}

Shader& Shader::operator=(Shader&& other) noexcept
{
    std::swap(shaderProgram_, other.shaderProgram_);
    uniformCache_ = std::move(other.uniformCache_);
#ifndef __EMSCRIPTEN__
    subroutineIndices_ = std::move(other.subroutineIndices_);
#endif
    return *this;
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

std::optional<GLuint> Shader::compile(const fs::path& shaderPath,
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

std::string Shader::readFile(const fs::path& shaderPath)
{
    std::ifstream file(shaderPath, std::ios::binary | std::ios::ate);
    std::streamsize fileSize(file.tellg());
    file.seekg(0);
    std::string shaderSrc(fileSize, '\0');
    file.read(shaderSrc.data(), fileSize);
    return shaderSrc;
}

bool Shader::checkCompileErrors(const GLuint shaderID, const GLenum shaderType)
{
    int success;
    glGetShaderiv(shaderID, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        GLint messageLength = 0;
        glGetShaderiv(shaderID, GL_INFO_LOG_LENGTH, &messageLength);

        char* message
            = static_cast<char*>(alloca(messageLength * sizeof(char)));
        glGetShaderInfoLog(shaderID, messageLength, nullptr, message);
        utils::showErrorMessage(
            shaderType == GL_VERTEX_SHADER ? "vertex" : "fragment",
            " shader compile error: ",
            message);
    }

    return success;
}

bool Shader::checkLinkerErrors(const GLuint shaderID)
{
    int success;
    glGetProgramiv(shaderID, GL_LINK_STATUS, &success);
    if (!success)
    {
        GLint messageLength = 0;
        glGetProgramiv(shaderID, GL_INFO_LOG_LENGTH, &messageLength);

        char* message
            = static_cast<char*>(alloca(messageLength * sizeof(char)));
        glGetProgramInfoLog(shaderID, messageLength, nullptr, message);
        utils::showErrorMessage("shader link error: ", message);
    }

    return success;
}

#ifndef __EMSCRIPTEN__
void Shader::updateSubroutines(const GLenum shaderType,
                               const std::vector<std::string>& names)
{
    // TODO: Clearing subroutine indices on every frame update is slow
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

void Shader::cacheActiveUniforms()
{
    GLint uniformCount = 0;
    glGetProgramiv(shaderProgram_, GL_ACTIVE_UNIFORMS, &uniformCount);
    if (uniformCount <= 0)
    {
        // No active uniforms present, skip
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
        std::cerr
            << "uniform is not present in compiled shader code. Either "
               "does not exists in original GLSL source code or the uniform is "
               "not active and was optimized out by shader compiler: "
            << name << '\n';
        assert(false);
    }
}
#endif
