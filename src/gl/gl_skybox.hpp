#ifndef GL_SKYBOX_HPP_
#define GL_SKYBOX_HPP_

#include "utils.hpp"

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
class GLSkybox
{
public:
    friend class GLSkyboxBuilder;

    GLSkybox();  // HACK: Allowing as member variable in App without
                 // std::unique_ptr
    DISABLE_COPY(GLSkybox)
    GLSkybox(GLSkybox&& other) noexcept;
    GLSkybox& operator=(GLSkybox&& other) noexcept;

    ~GLSkybox();

    void cleanup();

    [[nodiscard]] GLuint textureID() const;
    [[nodiscard]] GLuint vertexArray() const;

private:
    GLuint m_textureID;
    GLuint m_vertexArray;
    GLuint m_vertexBuffer;
    GLuint m_indexBuffer;
};

inline GLuint GLSkybox::textureID() const
{
    return m_textureID;
}

inline GLuint GLSkybox::vertexArray() const
{
    return m_vertexArray;
}

/// Builder pattern for skybox creation, avoiding mistakes from specifying
/// skybox face texture parameters out of order.
class GLSkyboxBuilder
{
public:
    GLSkyboxBuilder& setRight(const std::filesystem::path& rightFacePath);
    GLSkyboxBuilder& setLeft(const std::filesystem::path& leftFacePath);
    GLSkyboxBuilder& setTop(const std::filesystem::path& topFacePath);
    GLSkyboxBuilder& setBottom(const std::filesystem::path& bottomFacePath);
    GLSkyboxBuilder& setFront(const std::filesystem::path& frontFacePath);
    GLSkyboxBuilder& setBack(const std::filesystem::path& backFacePath);

    /// Load texture faces and generate vertex and index buffers.
    std::optional<GLSkybox> build();

private:
    std::filesystem::path m_rightFacePath;
    std::filesystem::path m_leftFacePath;
    std::filesystem::path m_topFacePath;
    std::filesystem::path m_bottomFacePath;
    std::filesystem::path m_frontFacePath;
    std::filesystem::path m_backFacePath;
};

#endif
