#ifndef SCENE_HPP
#define SCENE_HPP

#include "glm/vec3.hpp"

#include <string>
#include <unordered_map>
#include <vector>

/// Node element that can be added or removed from the scene using the GUI.
/// TODO: Split down into ECS components
struct SceneNode
{
    std::string label;  /// Label used for display in GUI
    glm::vec3 position;
    glm::vec3 rotation;
    glm::vec3 color;
    size_t modelID;

    SceneNode(const char* label, const size_t modelID)
        : label{label}
        , position{0.0F}
        , rotation{0.0F}
        , color{0.0F, 0.8F, 1.0F}
        , modelID{modelID}
    {
    }
};

using SceneNodeCollection = std::vector<SceneNode>;

/// Scene tree containing list of entities the user is able to interact with and
/// the renderer can iterate on. A user is able to add or remove nodes from the
/// GUI.
///
/// TODO: This is not a real ECS yet.
/// TODO: Currently only supports adding models only
class Scene
{
public:
    enum Model : uint8_t
    {
        Cube,
        Teapot,
        Bunny,
    };

    /// Adding a node with the same label resolves duplicate labels and appends
    /// an occurence number to avoid name collisions.
    void add(SceneNode node);
    void remove(const size_t nodeIndex);
    SceneNode& get(const size_t nodeIndex);
    [[nodiscard]] const SceneNodeCollection& children() const;

private:
    SceneNodeCollection children_;
    std::unordered_map<std::string, size_t> duplicateLabelResolver_;
};

inline SceneNode& Scene::get(const size_t nodeIndex)
{
    return children_[nodeIndex];
}

inline const SceneNodeCollection& Scene::children() const
{
    return children_;
}

#endif

