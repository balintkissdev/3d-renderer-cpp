#ifndef SHADER_HPP_
#define SHADER_HPP_

#ifdef __EMSCRIPTEN__
#include "glad/gles2.h"
#else
#include "glad/gl.h"
#endif
#include "glm/mat4x3.hpp"
#include "glm/mat4x4.hpp"
#include "glm/vec3.hpp"

#include <array>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

/// Wrapper around shader with helper operations
/// for loading, compiling, binding, uniform value update.
///
/// Non-copyable, move-only. Because this already contains a handle to the
/// compiled shader binary in GPU memory, there's no point in separate heap
/// allocation if not necessary.
class Shader
{
public:
    /// Factory method compiling vertex and fragment shaders from GLSL files.
    static std::optional<Shader> createFromFile(
        const std::filesystem::path& vertexShaderPath,
        const std::filesystem::path& fragmentShaderPath);

    Shader(const Shader& other) = delete;
    Shader& operator=(const Shader& other) = delete;
    Shader(Shader&& other) noexcept;
    Shader& operator=(Shader&& other) noexcept;

    ~Shader();

    /// Bind shader to graphics pipeline to use for draw calls.
    void use() const;
    void cleanup();

    template <typename T>
    void setUniform(const std::string& name, const T& v);

#ifndef __EMSCRIPTEN__
    /// Change subroutines to use in shader based on list of subroutine names.
    ///
    /// Subroutines are analogous to C function pointers and is an efficient way
    /// to customize parts of the shader program to execute.
    ///
    /// Shader subroutines are only supported from OpenGL 4.0+ and are not
    /// available in OpenGL ES 3.0.
    void updateSubroutines(const GLenum shaderType,
                           const std::vector<std::string>& names);
#endif

private:
    static std::optional<GLuint> compile(
        const std::filesystem::path& shaderPath,
        const GLenum shaderTpye);
    static std::string readFile(const std::filesystem::path& shaderPath);
    static bool checkCompileErrors(const GLuint shaderID,
                                   const GLenum shaderType);
    static bool checkLinkerErrors(const GLuint shaderID);

    Shader();
    explicit Shader(const GLuint shaderProgram);

    GLuint shaderProgram_;
    std::unordered_map<std::string, GLint> uniformCache_;

#ifndef __EMSCRIPTEN__
    std::vector<GLuint> subroutineIndices_;
#endif

    /// Query all active uniforms on shader creation and cache uniform locations
    /// for access by name. This is done to avoid repeated calls to
    /// glGetUniformLocation during rendering loop.
    ///
    /// Can only be done once shader linking was successful. Only active
    /// uniforms are cached, meaning only uniforms that are actually used by
    /// shader operations. Because GPU drivers optimize shader compilation,
    /// only active uniforms are compiled into the shader program and unused
    /// uniforms are discarded.
    void cacheActiveUniforms();
#ifndef NDEBUG
    /// Catch non-existent active uniform
    /// errors during development in debug build (disabled release build to
    /// avoid overhead of uniform existence checks).
    void assertUniform(const std::string& name);
#endif
};

template <>
void Shader::setUniform(const std::string& name, const int& v);
template <>
void Shader::setUniform(const std::string& name, const bool& v);
template <>
void Shader::setUniform(const std::string& name, const std::array<float, 3>& v);
template <>
void Shader::setUniform(const std::string& name, const glm::vec3& v);
template <>
void Shader::setUniform(const std::string& name, const glm::mat3& v);
template <>
void Shader::setUniform(const std::string& name, const glm::mat4& v);

#endif
