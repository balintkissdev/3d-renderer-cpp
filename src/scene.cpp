#include "scene.hpp"

void Scene::add(SceneNode node)
{
    // On duplicate labels, counting starts from 2
    const size_t duplicateCount = ++m_duplicateLabelResolver[node.label];
    if (1 < duplicateCount)
    {
        node.label += ' ' + std::to_string(duplicateCount);
    }

    m_children.push_back(node);
}

void Scene::remove(const size_t nodeIndex)
{
    m_children.erase(m_children.begin() + static_cast<long>(nodeIndex));
}

