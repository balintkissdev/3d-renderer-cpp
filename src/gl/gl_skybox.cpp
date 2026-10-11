#include "gl_skybox.hpp"

#include "utils.hpp"

// clang-format off
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
// clang-format on

#include <array>
#include <cstdint>
#include <utility>

namespace fs = std::filesystem;

GLSkybox::GLSkybox()
    : m_textureID{0}
    , m_vertexArray{0}
    , m_vertexBuffer{0}
    , m_indexBuffer{0}
{}

GLSkybox::GLSkybox(GLSkybox&& other) noexcept
    : m_textureID{std::exchange(other.m_textureID, 0)}
    , m_vertexArray{std::exchange(other.m_vertexArray, 0)}
    , m_vertexBuffer{std::exchange(other.m_vertexBuffer, 0)}
    , m_indexBuffer{std::exchange(other.m_indexBuffer, 0)}
{}

GLSkybox& GLSkybox::operator=(GLSkybox&& other) noexcept
{
    std::swap(m_textureID, other.m_textureID);
    std::swap(m_vertexArray, other.m_vertexArray);
    std::swap(m_vertexBuffer, other.m_vertexBuffer);
    std::swap(m_indexBuffer, other.m_indexBuffer);
    return *this;
}

GLSkybox::~GLSkybox()
{
    cleanup();
}

void GLSkybox::cleanup()
{
    // Existing 0s are silently ignored
    glDeleteTextures(1, &m_textureID);
    glDeleteVertexArrays(1, &m_vertexArray);
    glDeleteBuffers(1, &m_indexBuffer);
    glDeleteBuffers(1, &m_vertexBuffer);
    m_textureID = 0;
    m_vertexArray = 0;
    m_vertexBuffer = 0;
    m_indexBuffer = 0;
}

GLSkyboxBuilder& GLSkyboxBuilder::setRight(const fs::path& rightFacePath)
{
    m_rightFacePath = rightFacePath;
    return *this;
}

GLSkyboxBuilder& GLSkyboxBuilder::setLeft(const fs::path& leftFacePath)
{
    m_leftFacePath = leftFacePath;
    return *this;
}

GLSkyboxBuilder& GLSkyboxBuilder::setTop(const fs::path& topFacePath)
{
    m_topFacePath = topFacePath;
    return *this;
}

GLSkyboxBuilder& GLSkyboxBuilder::setBottom(const fs::path& bottomFacePath)
{
    m_bottomFacePath = bottomFacePath;
    return *this;
}

GLSkyboxBuilder& GLSkyboxBuilder::setFront(const fs::path& frontFacePath)
{
    m_frontFacePath = frontFacePath;
    return *this;
}

GLSkyboxBuilder& GLSkyboxBuilder::setBack(const fs::path& backFacePath)
{
    m_backFacePath = backFacePath;
    return *this;
}

std::optional<GLSkybox> GLSkyboxBuilder::build()
{
    GLSkybox skybox;
    // Create texture
    glGenTextures(1, &skybox.m_textureID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, skybox.m_textureID);

    // Load texture images
    const std::array textureFacePaths{m_rightFacePath,
                                      m_leftFacePath,
                                      m_topFacePath,
                                      m_bottomFacePath,
                                      m_frontFacePath,
                                      m_backFacePath};
    for (size_t i = 0; i < textureFacePaths.size(); ++i)
    {
        int width, height, channelCount;
        uint8_t* imageData = stbi_load(textureFacePaths[i].string().c_str(),
                                       &width,
                                       &height,
                                       &channelCount,
                                       0);
        if (!imageData)
        {
            utils::showErrorMessage("unable to load skybox texture from",
                                    textureFacePaths[i]);
            return std::nullopt;
        }

        glTexImage2D(static_cast<GLenum>(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i),
                     0,
                     GL_RGB,
                     width,
                     height,
                     0,
                     GL_RGB,
                     GL_UNSIGNED_BYTE,
                     imageData);
        stbi_image_free(imageData);
    }

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    // Create buffers

    // clang-format off
    const std::array skyboxVertices{
        -1.0f,  1.0f, -1.0f,
        -1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f,  1.0f,
        -1.0f, -1.0f,  1.0f,
         1.0f, -1.0f,  1.0f,
         1.0f,  1.0f,  1.0f
    };

    const std::array skyboxIndices{
        // Front face
        0, 1, 2,
        2, 3, 0,
        // Back face
        4, 5, 6,
        6, 7, 4,
        // Left face
        4, 5, 1,
        1, 0, 4,
        // Right face
        3, 2, 6,
        6, 7, 3,
        // Top face
        4, 0, 3,
        3, 7, 4,
        // Bottom face
        1, 5, 6,
        6, 2, 1
    };
    // clang-format on

    // Create vertex array
    glGenVertexArrays(1, &skybox.m_vertexArray);
    glBindVertexArray(skybox.m_vertexArray);

    // Create vertex buffer
    glGenBuffers(1, &skybox.m_vertexBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, skybox.m_vertexBuffer);
    glBufferData(GL_ARRAY_BUFFER,
                 sizeof(skyboxVertices),
                 skyboxVertices.data(),
                 GL_STATIC_DRAW);

    // Create index buffer
    glGenBuffers(1, &skybox.m_indexBuffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, skybox.m_indexBuffer);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 sizeof(skyboxIndices),
                 skyboxIndices.data(),
                 GL_STATIC_DRAW);

    // Setup vertex array layout (just vertex positions)
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0,
                          3,
                          GL_FLOAT,
                          GL_FALSE,
                          3 * sizeof(float),
                          reinterpret_cast<GLvoid*>(0));

    return skybox;
}

