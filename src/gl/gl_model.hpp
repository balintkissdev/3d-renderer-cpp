#ifndef GL_MODEL_HPP_
#define GL_MODEL_HPP_

#include "utils.hpp"

#ifdef __EMSCRIPTEN__
#include "glad/gles2.h"
#else
#include "glad/gl.h"
#endif
#include "glm/vec3.hpp"

#include <filesystem>
#include <optional>
#include <vector>

/// Representation of 3D model (currently mesh only).
///
/// Non-copyable, move-only. Mesh face vertices reside in GPU memory.
/// Vertices are referred by indices to avoid storing duplicated vertices.
class GLModel
{
public:
    /// Factory method loading a model file and initializing buffers.
    static std::optional<GLModel> create(const std::filesystem::path& filePath);

    DISABLE_COPY(GLModel)
    GLModel(GLModel&& other) noexcept;
    GLModel& operator=(GLModel&& other) noexcept;

    ~GLModel();

    void cleanup();

    [[nodiscard]] GLuint vertexArray() const;
    [[nodiscard]] const std::vector<GLuint>& indices() const;

private:
    GLModel();

    GLuint vertexArray_;
    std::vector<GLuint> indices_;
    GLuint vertexBuffer_;
    GLuint indexBuffer_;  /// Index buffer avoids duplication of vertices in
                          /// vertex buffer
};

inline GLuint GLModel::vertexArray() const
{
    return vertexArray_;
}

inline const std::vector<GLuint>& GLModel::indices() const
{
    return indices_;
}

#endif
