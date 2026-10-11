#include "renderer.hpp"

Renderer::Renderer(Window& window,
                   const DrawProperties& drawProps,
                   const Camera& camera)
    : m_window{window}
    , m_drawProps{drawProps}
    , m_camera{camera}
{
}

Renderer::~Renderer() = default;
