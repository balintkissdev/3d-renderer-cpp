#include "renderer.hpp"

Renderer::Renderer(Window& window,
                   const DrawProperties& drawProps,
                   const Camera& camera)
    : window_{window}
    , drawProps_{drawProps}
    , camera_{camera}
{
}

Renderer::~Renderer() = default;
