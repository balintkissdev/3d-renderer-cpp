#ifndef SKYBOX_H_
#define SKYBOX_H_

#ifdef __EMSCRIPTEN__
#include "glad/gles2.h"
#else
#include "glad/gl.h"
#endif

#include <memory>
#include <string>

/// Skybox containing cube-mapped texture and vertex positions for skybox
/// cube.
///
/// Cube-map is represented by six subtextures that must be square and the same
/// size. Sampling from cube-map is done as direction from origin. Skybox is an
/// application of cube-mapping where entire scene is wrapped in a large cube
/// surrounding the viewer and model. A unit cube is rendered centered
/// at the origin and uses the object space position as a texture coordinate
/// from which to sample the cube map texture.
class Skybox
{
public:
    friend class SkyboxBuilder;

    Skybox(const Skybox&) = delete;
    Skybox& operator=(const Skybox&) = delete;
    Skybox(Skybox&& other) noexcept;
    Skybox& operator=(Skybox&& other) noexcept;

    ~Skybox();

    GLuint textureID;
    GLuint vertexArray;

private:
    Skybox();

    GLuint vertexBuffer_;
    GLuint indexBuffer_;
};

/// Builder pattern for skybox creation, avoiding mistakes from specifying
/// skybox face texture parameters.
class SkyboxBuilder
{
public:
    SkyboxBuilder& setRight(const std::string& rightFacePath);
    SkyboxBuilder& setLeft(const std::string& leftFacePath);
    SkyboxBuilder& setTop(const std::string& topFacePath);
    SkyboxBuilder& setBottom(const std::string& bottomFacePath);
    SkyboxBuilder& setFront(const std::string& frontFacePath);
    SkyboxBuilder& setBack(const std::string& backFacePath);

    /// Load texture faces and generate vertex and index buffers.
    std::unique_ptr<Skybox> build();

private:
    std::string rightFacePath_;
    std::string leftFacePath_;
    std::string topFacePath_;
    std::string bottomFacePath_;
    std::string frontFacePath_;
    std::string backFacePath_;
};
#endif
