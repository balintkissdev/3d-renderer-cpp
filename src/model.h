#ifndef MODEL_H_
#define MODEL_H_

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
class Model
{
public:
    /// Factory method loading a model file and initializing buffers.
    static std::optional<Model> create(const std::filesystem::path& filePath);

    Model(const Model& other) = delete;
    Model& operator=(const Model& other) = delete;
    Model(Model&& other) noexcept;
    Model& operator=(Model&& other) noexcept;

    ~Model();

    // (Exposed as public variables instead of getters due to performance
    // concerns)
    GLuint vertexArray;
    std::vector<GLuint> indices;

private:
    /// Per-vertex data containing vertex attributes for each vertex.
    ///
    /// Texture UV coordinates are omitted because none of the bundled default
    /// models have textures.
    struct Vertex
    {
        glm::vec3 position;
        glm::vec3 normal;
    };

    static bool loadModelFromFile(const std::filesystem::path& filePath,
                                  std::vector<Vertex>& outVertices,
                                  std::vector<GLuint>& outIndices);

    Model();

    GLuint vertexBuffer_;
    GLuint indexBuffer_;  /// Index buffer avoids duplication of vertices in
                          /// vertex buffer
};
#endif
