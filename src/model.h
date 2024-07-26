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

/// Representation of 3D model (currently mesh only).
class Model
{
public:
    /// Factory method loading a model file and initializing buffers.
    static std::unique_ptr<Model> create(std::string_view filePath);

    // TODO: Debug why specifying Rule of Five and private constructor break
    // mesh display

    ~Model();

    // (exposed as public due to performance concerns)
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

    static bool loadModelFromFile(std::string_view filePath,
                                  std::vector<Vertex>& outVertices,
                                  std::vector<GLuint>& outIndices);

    GLuint vertexBuffer_;
    GLuint indexBuffer_;  /// Index buffer avoids duplication of vertices in
                          /// vertex buffer
};
#endif
