#include "shader.h"

#include "glm/gtc/type_ptr.hpp"

#include <cstdio>
#include <fstream>

// TODO: Remove "C-ism"
#define SHADER_TYPE_VERTEX ("Vertex shader")
#define SHADER_TYPE_FRAGMENT ("Fragment shader")
#define SHADER_TYPE_PROGRAM ("Shader program")
#define SHADER_COMPILE_MESSAGE_MAX_LENGTH (512)

std::string Shader::readFile(const char* shaderPath)
{
    std::ifstream file(shaderPath, std::ios::binary);
    file.seekg(0, std::istream::end);
    std::size_t fileSize(file.tellg());
    file.seekg(0, std::istream::beg);
    std::string shaderSrc(fileSize, 0);
    file.read(&shaderSrc[0], fileSize);
    return shaderSrc;
}

std::unique_ptr<Shader> Shader::createFromFile(const char* vertexShaderPath,
                                               const char* fragmentShaderPath)
{
    std::string shaderSrc = readFile(vertexShaderPath);
    const GLchar* shaderGlSrc = shaderSrc.c_str();
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &shaderGlSrc, nullptr);
    glCompileShader(vertexShader);
    if (!checkCompileErrors(vertexShader, GL_VERTEX_SHADER))
    {
        return nullptr;
    }

    shaderSrc = readFile(fragmentShaderPath);
    shaderGlSrc = shaderSrc.c_str();
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &shaderGlSrc, nullptr);
    glCompileShader(fragmentShader);
    if (!checkCompileErrors(fragmentShader, GL_FRAGMENT_SHADER))
    {
        return nullptr;
    }

    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    if (!checkLinkerErrors(shaderProgram))
    {
        return nullptr;
    }

    auto shader = std::unique_ptr<Shader>(new Shader);
    shader->shaderProgram_ = shaderProgram;
    return shader;
}

Shader::Shader()
    : shaderProgram_(0)
{
}

Shader::~Shader()
{
    glDeleteShader(shaderProgram_);
}

void Shader::use()
{
    glUseProgram(shaderProgram_);
}

template <>
void Shader::setUniform(const char* name, const int& v)
{
    glUniform1i(glGetUniformLocation(shaderProgram_, name), v);
}

template <>
void Shader::setUniform(const char* name, const float (&v)[3])
{
    glUniform3fv(glGetUniformLocation(shaderProgram_, name), 1, v);
}

template <>
void Shader::setUniform(const char* name, const glm::vec3& v)
{
    glUniform3fv(glGetUniformLocation(shaderProgram_, name),
                 1,
                 glm::value_ptr(v));
}

template <>
void Shader::setUniform(const char* name, const glm::mat3& v)
{
    glUniformMatrix3fv(glGetUniformLocation(shaderProgram_, name),
                       1,
                       GL_FALSE,
                       glm::value_ptr(v));
}

template <>
void Shader::setUniform(const char* name, const glm::mat4& v)
{
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram_, name),
                       1,
                       GL_FALSE,
                       glm::value_ptr(v));
}

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
                            subroutineIndices_.size(),
                            subroutineIndices_.data());
}

bool Shader::checkCompileErrors(GLuint shaderID, const GLenum shaderType)
{
    int success;
    char message[SHADER_COMPILE_MESSAGE_MAX_LENGTH];
    glGetShaderiv(shaderID, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(shaderID,
                           SHADER_COMPILE_MESSAGE_MAX_LENGTH,
                           NULL,
                           message);
        fprintf(stderr,
                "%s shader compile error: %s\n",
                shaderType == GL_VERTEX_SHADER ? "Vertex" : "Fragment",
                message);
    }

    return success;
}

bool Shader::checkLinkerErrors(GLuint shaderID)
{
    int success;
    char message[SHADER_COMPILE_MESSAGE_MAX_LENGTH];
    glGetProgramiv(shaderID, GL_LINK_STATUS, &success);
    if (!success)
    {
        glGetProgramInfoLog(shaderID,
                            SHADER_COMPILE_MESSAGE_MAX_LENGTH,
                            NULL,
                            message);
        fprintf(stderr, "Shader link error: %s\n", message);
    }

    return success;
}
