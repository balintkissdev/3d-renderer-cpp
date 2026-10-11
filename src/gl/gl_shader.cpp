#include "gl_shader.hpp"

#include "utils.hpp"

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

namespace
{
const char* GLSL_PREAMBLE
    = "#if __VERSION__ <= 310\n"
      "precision mediump float;\n"
      "#endif\n"
      "#if 460 <= __VERSION__\n"
      "#define OPENGL_4_6\n"
      "#endif\n";
}

std::optional<GLShader> GLShader::createFromFile(const Params& params
#ifndef __EMSCRIPTEN__
                                                 ,
                                                 const RenderingAPI api
#endif
)
{
    const fs::path glslShaderDirPath("assets/shaders/glsl/");
    const fs::path glslShaderBasePath
        = glslShaderDirPath / params.shaderBaseName;
    const fs::path includePath
        = params.includeBaseName.empty()
            ? fs::path()
            : (glslShaderDirPath / params.includeBaseName).concat(".glsli");

    // Compile vertex shader
    const auto vertexShader = compile(glslShaderBasePath,
#ifndef __EMSCRIPTEN__
                                      api,
#endif
                                      GL_VERTEX_SHADER,
                                      includePath,
                                      params.defines);
    if (!vertexShader)
    {
        return std::nullopt;
    }
    DEFER(glDeleteShader(vertexShader.value()));

    // Compile fragment shader
    const auto fragmentShader = compile(glslShaderBasePath,
#ifndef __EMSCRIPTEN__
                                        api,
#endif
                                        GL_FRAGMENT_SHADER,
                                        includePath,
                                        params.defines);
    if (!fragmentShader)
    {
        return std::nullopt;
    }
    DEFER(glDeleteShader(fragmentShader.value()));

    // Link shader program
    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader.value());
    glAttachShader(shaderProgram, fragmentShader.value());
    glLinkProgram(shaderProgram);
    // Vertex and fragment shader not needed anymore after linked as part for
    // program.
    if (!checkLinkerErrors(shaderProgram))
    {
        return std::nullopt;
    }

    return GLShader{shaderProgram};
}

GLShader::GLShader()
    : m_shaderProgram{0}
{
}

GLShader::GLShader(const GLuint shaderProgram)
    : m_shaderProgram{shaderProgram}
{
    if (shaderProgram)
    {
        cacheActiveUniforms();
    }
}

GLShader::~GLShader()
{
    cleanup();
}

GLShader::GLShader(GLShader&& other) noexcept
    : m_shaderProgram{std::exchange(other.m_shaderProgram, 0)}
    , m_uniformCache{std::move(other.m_uniformCache)}
#ifndef __EMSCRIPTEN__
    , m_subroutineIndices{std::move(other.m_subroutineIndices)}
#endif
{
}

GLShader& GLShader::operator=(GLShader&& other) noexcept
{
    std::swap(m_shaderProgram, other.m_shaderProgram);
    m_uniformCache = std::move(other.m_uniformCache);
#ifndef __EMSCRIPTEN__
    m_subroutineIndices = std::move(other.m_subroutineIndices);
#endif
    return *this;
}

void GLShader::use() const
{
    glUseProgram(m_shaderProgram);
}

void GLShader::cleanup()
{
    if (m_shaderProgram != 0)
    {
        glDeleteShader(m_shaderProgram);
        m_shaderProgram = 0;
    }
}

template <>
void GLShader::setUniform(const std::string& name, const int& v)
{
    ASSERT_UNIFORM(name);
    glUniform1i(m_uniformCache.at(name), v);
}

template <>
void GLShader::setUniform(const std::string& name, const float& v)
{
    ASSERT_UNIFORM(name);
    glUniform1f(m_uniformCache.at(name), v);
}

template <>
void GLShader::setUniform(const std::string& name, const bool& v)
{
    ASSERT_UNIFORM(name);
    glUniform1i(m_uniformCache.at(name), v);
}

template <>
void GLShader::setUniform(const std::string& name,
                          const std::array<float, 3>& v)
{
    ASSERT_UNIFORM(name);
    glUniform3fv(m_uniformCache.at(name), 1, v.data());
}

template <>
void GLShader::setUniform(const std::string& name, const glm::vec3& v)
{
    ASSERT_UNIFORM(name);
    glUniform3fv(m_uniformCache.at(name), 1, glm::value_ptr(v));
}

template <>
void GLShader::setUniform(const std::string& name, const glm::mat3& v)
{
    ASSERT_UNIFORM(name);
    glUniformMatrix3fv(m_uniformCache.at(name), 1, GL_FALSE, glm::value_ptr(v));
}

template <>
void GLShader::setUniform(const std::string& name, const glm::mat4& v)
{
    ASSERT_UNIFORM(name);
    glUniformMatrix4fv(m_uniformCache.at(name), 1, GL_FALSE, glm::value_ptr(v));
}

std::optional<GLuint> GLShader::compile(const fs::path& shaderPath,
#ifndef __EMSCRIPTEN__
                                        const RenderingAPI api,
#endif
                                        const GLenum shaderTpye,
                                        const fs::path& includePath,
                                        const std::vector<std::string>& defines)
{
    std::string shaderSrc = utils::RenderingAPIToGLSLDirective(
#ifndef __EMSCRIPTEN__
                                api
#endif
                                )
                          + std::string(GLSL_PREAMBLE);

    for (const std::string& d : defines)
    {
        shaderSrc += "#define ";
        shaderSrc += d;
        shaderSrc += '\n';
    }

    if (!includePath.empty())
    {
        shaderSrc += readFile(includePath);
    }

    fs::path shaderFilePath = shaderPath;
    shaderFilePath.concat(shaderTpye == GL_VERTEX_SHADER ? ".vert.glsl"
                                                         : ".frag.glsl");
    shaderSrc += readFile(shaderFilePath);

    const GLchar* shaderGlSrc = shaderSrc.c_str();
    const GLuint shader = glCreateShader(shaderTpye);
    glShaderSource(shader, 1, &shaderGlSrc, nullptr);
    glCompileShader(shader);
    return checkCompileErrors(shader, shaderTpye)
             ? std::optional<GLuint>{shader}
             : std::nullopt;
}

std::string GLShader::readFile(const fs::path& shaderPath)
{
    std::ifstream file(shaderPath, std::ios::binary | std::ios::ate);
    std::streamsize fileSize(file.tellg());
    file.seekg(0);
    std::string shaderSrc(fileSize, '\0');
    file.read(shaderSrc.data(), fileSize);
    return shaderSrc;
}

bool GLShader::checkCompileErrors(const GLuint shaderID,
                                  const GLenum shaderType)
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

bool GLShader::checkLinkerErrors(const GLuint shaderID)
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
void GLShader::updateSubroutines(const GLenum shaderType,
                                 const std::vector<std::string>& names)
{
    // TODO: Clearing subroutine indices on every frame update is slow
    m_subroutineIndices.clear();
    for (const std::string& name : names)
    {
        m_subroutineIndices.push_back(
            glGetSubroutineIndex(m_shaderProgram, shaderType, name.c_str()));
    }
    glUniformSubroutinesuiv(GL_FRAGMENT_SHADER,
                            static_cast<GLsizei>(m_subroutineIndices.size()),
                            m_subroutineIndices.data());
}
#endif

void GLShader::cacheActiveUniforms()
{
    GLint uniformCount = 0;
    glGetProgramiv(m_shaderProgram, GL_ACTIVE_UNIFORMS, &uniformCount);
    if (uniformCount <= 0)
    {
        // No active uniforms present, skip
        return;
    }

    constexpr size_t maxBufferSize = 64;
    std::array<char, maxBufferSize> uniformNameBuffer;
    m_uniformCache.reserve(uniformCount);
    for (GLint i = 0; i < uniformCount; ++i)
    {
        GLsizei bufferSize;
        GLsizei uniformVarSize;
        GLenum uniformDataType;
        glGetActiveUniform(m_shaderProgram,
                           i,
                           maxBufferSize,
                           &bufferSize,
                           &uniformVarSize,
                           &uniformDataType,
                           uniformNameBuffer.data());
        const GLint uniformLocation
            = glGetUniformLocation(m_shaderProgram, uniformNameBuffer.data());
        m_uniformCache.emplace(
            std::string(uniformNameBuffer.data(), bufferSize),
            uniformLocation);
    }
}

#ifndef NDEBUG
void GLShader::assertUniform(const std::string& name)
{
    if (!m_uniformCache.contains(name))
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
