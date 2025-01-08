#ifndef RENDERER_HPP_
#define RENDERER_HPP_

#include "drawproperties.hpp"
#include "utils.hpp"

class Camera;
class Scene;
class Window;

/// Abstract base to be implemented by different rendering backends to enable
/// runtime switching between them, acting as interface API contract for other
/// components.
///
/// Current window, global drawing properties and camera behavior is shared.
/// For renderable entities (model meshes, shaders, skyboxes), it is best to
/// keep virtual inheritance hierarchy to a minimum.
class Renderer
{
public:
    Renderer(Window& window,
             const DrawProperties& drawProps,
             const Camera& camera);
    virtual ~Renderer();
    DISABLE_COPY_AND_MOVE(Renderer)

    virtual bool init() = 0;
    virtual void initImGuiBackend() = 0;
    virtual void cleanup() = 0;
    virtual void draw(const Scene& scene) = 0;
    virtual void setVSyncEnabled(const bool vsyncEnabled) = 0;

protected:
    Window& window_;
    const DrawProperties& drawProps_;
    const Camera& camera_;
};

#endif
