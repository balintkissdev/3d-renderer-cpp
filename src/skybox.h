#ifndef SKYBOX_H_
#define SKYBOX_H_

#ifdef __EMSCRIPTEN__
#include "glad/gles2.h"
#else
#include "glad/gl.h"
#endif

#include <filesystem>
#include <optional>

/// Skybox containing cube-mapped texture and vertex positions for skybox
/// cube.
///
/// Cube-map is represented by six subtextures that must be square and the same
/// size. Sampling from cube-map is done as direction from origin. Skybox is an
/// application of cube-mapping where entire scene is wrapped in a large cube
/// surrounding the viewer and model. A unit cube is rendered centered
/// at the origin and uses the object space position as a texture coordinate
/// from which to sample the cube map texture.
///
/// Non-copyable. Move-only. Texture and vertex data are stored in GPU memory.
class Skybox
{
public:
    friend class SkyboxBuilder;

    Skybox();  // HACK: Allowing as member variable in App without
               // std::unique_ptr
    Skybox(const Skybox&) = delete;
    Skybox& operator=(const Skybox&) = delete;
    Skybox(Skybox&& other) noexcept;
    Skybox& operator=(Skybox&& other) noexcept;

    ~Skybox();

    [[nodiscard]] GLuint textureID() const;
    [[nodiscard]] GLuint vertexArray() const;

private:
    GLuint textureID_;
    GLuint vertexArray_;
    GLuint vertexBuffer_;
    GLuint indexBuffer_;
};

inline GLuint Skybox::textureID() const
{
    return textureID_;
}

inline GLuint Skybox::vertexArray() const
{
    return vertexArray_;
}

/// Builder pattern for skybox creation, avoiding mistakes from specifying
/// skybox face texture parameters out of order.
class SkyboxBuilder
{
public:
    SkyboxBuilder& setRight(const std::filesystem::path& rightFacePath);
    SkyboxBuilder& setLeft(const std::filesystem::path& leftFacePath);
    SkyboxBuilder& setTop(const std::filesystem::path& topFacePath);
    SkyboxBuilder& setBottom(const std::filesystem::path& bottomFacePath);
    SkyboxBuilder& setFront(const std::filesystem::path& frontFacePath);
    SkyboxBuilder& setBack(const std::filesystem::path& backFacePath);

    /// Load texture faces and generate vertex and index buffers.
    std::optional<Skybox> build();

private:
    std::filesystem::path rightFacePath_;
    std::filesystem::path leftFacePath_;
    std::filesystem::path topFacePath_;
    std::filesystem::path bottomFacePath_;
    std::filesystem::path frontFacePath_;
    std::filesystem::path backFacePath_;
};
#endif
