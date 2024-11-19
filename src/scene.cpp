#include "scene.hpp"

void Scene::add(SceneNode node)
{
    // On duplicate labels, counting starts from 2
    const size_t duplicateCount = ++duplicateLabelResolver_[node.label];
    if (1 < duplicateCount)
    {
        node.label += ' ' + std::to_string(duplicateCount);
    }

    children_.push_back(node);
}

void Scene::remove(const size_t nodeIndex)
{
    children_.erase(children_.begin() + static_cast<long>(nodeIndex));
}

