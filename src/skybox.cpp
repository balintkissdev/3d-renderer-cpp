#include "skybox.h"

#include "utils.h"

// clang-format off
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
// clang-format on

#include <array>
#include <cstdint>
#include <utility>

namespace fs = std::filesystem;

Skybox::Skybox()
    : textureID_{0}
    , vertexArray_{0}
    , vertexBuffer_{0}
    , indexBuffer_{0}
{
}

Skybox::Skybox(Skybox&& other) noexcept
    : textureID_{std::exchange(other.textureID_, 0)}
    , vertexArray_{std::exchange(other.vertexArray_, 0)}
    , vertexBuffer_{std::exchange(other.vertexBuffer_, 0)}
    , indexBuffer_{std::exchange(other.indexBuffer_, 0)}
{
}

Skybox& Skybox::operator=(Skybox&& other) noexcept
{
    std::swap(textureID_, other.textureID_);
    std::swap(vertexArray_, other.vertexArray_);
    std::swap(vertexBuffer_, other.vertexBuffer_);
    std::swap(indexBuffer_, other.indexBuffer_);
    return *this;
}

Skybox::~Skybox()
{
    glDeleteTextures(1, &textureID_);
    glDeleteVertexArrays(1, &vertexArray_);
    glDeleteBuffers(1, &indexBuffer_);
    glDeleteBuffers(1, &vertexBuffer_);
}

SkyboxBuilder& SkyboxBuilder::setRight(const fs::path& rightFacePath)
{
    rightFacePath_ = rightFacePath;
    return *this;
}

SkyboxBuilder& SkyboxBuilder::setLeft(const fs::path& leftFacePath)
{
    leftFacePath_ = leftFacePath;
    return *this;
}

SkyboxBuilder& SkyboxBuilder::setTop(const fs::path& topFacePath)
{
    topFacePath_ = topFacePath;
    return *this;
}

SkyboxBuilder& SkyboxBuilder::setBottom(const fs::path& bottomFacePath)
{
    bottomFacePath_ = bottomFacePath;
    return *this;
}

SkyboxBuilder& SkyboxBuilder::setFront(const fs::path& frontFacePath)
{
    frontFacePath_ = frontFacePath;
    return *this;
}

SkyboxBuilder& SkyboxBuilder::setBack(const fs::path& backFacePath)
{
    backFacePath_ = backFacePath;
    return *this;
}

std::optional<Skybox> SkyboxBuilder::build()
{
    // Load textures
    const std::array textureFacePaths{rightFacePath_,
                                      leftFacePath_,
                                      topFacePath_,
                                      bottomFacePath_,
                                      frontFacePath_,
                                      backFacePath_};

    Skybox skybox;
    glGenTextures(1, &skybox.textureID_);
    glBindTexture(GL_TEXTURE_CUBE_MAP, skybox.textureID_);

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
        -1.0F,  1.0F, -1.0F,
        -1.0F, -1.0F, -1.0F,
         1.0F, -1.0F, -1.0F,
         1.0F,  1.0F, -1.0F,
        -1.0F,  1.0F,  1.0F,
        -1.0F, -1.0F,  1.0F,
         1.0F, -1.0F,  1.0F,
         1.0F,  1.0F,  1.0F
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
    glGenVertexArrays(1, &skybox.vertexArray_);
    glBindVertexArray(skybox.vertexArray_);

    // Create vertex buffer
    glGenBuffers(1, &skybox.vertexBuffer_);
    glBindBuffer(GL_ARRAY_BUFFER, skybox.vertexBuffer_);
    glBufferData(GL_ARRAY_BUFFER,
                 sizeof(skyboxVertices),
                 skyboxVertices.data(),
                 GL_STATIC_DRAW);

    // Create index buffer
    glGenBuffers(1, &skybox.indexBuffer_);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, skybox.indexBuffer_);
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

