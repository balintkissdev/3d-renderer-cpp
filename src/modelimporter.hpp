#ifndef MODELIMPORTER_HPP_
#define MODELIMPORTER_HPP_

#include <array>
#include <filesystem>
#include <vector>

/// Per-vertex data containing vertex attributes for each vertex.
///
/// Texture UV coordinates are omitted because none of the bundled default
/// models have textures.
struct Vertex
{
    std::array<float, 3> position;
    std::array<float, 3> normal;
};

namespace ModelImporter
{
// Careful with winding order differences between OpenGL/Vulkan and
// Direct3D. (alternatively use rasterizerDesc.FrontCounterClockwise
// = TRUE;)
enum class Winding : uint8_t
{
    Clockwise,
    CounterClockwise,
};

bool loadFromFile(const std::filesystem::path& filePath,
                  std::vector<Vertex>& outVertices,
                  std::vector<uint32_t>& outIndices,
                  const Winding windingOrder);
}  // namespace ModelImporter

#endif
