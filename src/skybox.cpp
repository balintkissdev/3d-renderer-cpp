#include "skybox.h"

#include "utils.h"

// clang-format off
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
// clang-format on

#include <array>
#include <cstdint>

Skybox::Skybox()
    : textureID{0}
    , vertexArray{0}
    , vertexBuffer_{0}
    , indexBuffer_{0}
{
}

Skybox::Skybox(Skybox&& other) noexcept
    : textureID(other.textureID)
    , vertexArray(other.vertexArray)
    , vertexBuffer_(other.vertexBuffer_)
    , indexBuffer_(other.indexBuffer_)
{
    other.textureID = 0;
    other.vertexArray = 0;
    other.vertexBuffer_ = 0;
    other.indexBuffer_ = 0;
}

Skybox& Skybox::operator=(Skybox&& other) noexcept
{
    if (this != &other)
    {
        if (textureID != 0)
        {
            glDeleteTextures(1, &textureID);
        }
        if (vertexArray != 0)
        {
            glDeleteVertexArrays(1, &vertexArray);
        }
        if (vertexBuffer_ != 0)
        {
            glDeleteBuffers(1, &vertexBuffer_);
        }
        if (indexBuffer_ != 0)
        {
            glDeleteBuffers(1, &indexBuffer_);
        }

        textureID = other.textureID;
        vertexArray = other.vertexArray;
        vertexBuffer_ = other.vertexBuffer_;
        indexBuffer_ = other.indexBuffer_;

        other.textureID = 0;
        other.vertexArray = 0;
        other.vertexBuffer_ = 0;
        other.indexBuffer_ = 0;
    }
    return *this;
}

Skybox::~Skybox()
{
    glDeleteTextures(1, &textureID);
    glDeleteVertexArrays(1, &vertexArray);
    glDeleteBuffers(1, &indexBuffer_);
    glDeleteBuffers(1, &vertexBuffer_);
}

SkyboxBuilder& SkyboxBuilder::setRight(const std::string& rightFacePath)
{
    rightFacePath_ = rightFacePath;
    return *this;
}

SkyboxBuilder& SkyboxBuilder::setLeft(const std::string& leftFacePath)
{
    leftFacePath_ = leftFacePath;
    return *this;
}

SkyboxBuilder& SkyboxBuilder::setTop(const std::string& topFacePath)
{
    topFacePath_ = topFacePath;
    return *this;
}

SkyboxBuilder& SkyboxBuilder::setBottom(const std::string& bottomFacePath)
{
    bottomFacePath_ = bottomFacePath;
    return *this;
}

SkyboxBuilder& SkyboxBuilder::setFront(const std::string& frontFacePath)
{
    frontFacePath_ = frontFacePath;
    return *this;
}

SkyboxBuilder& SkyboxBuilder::setBack(const std::string& backFacePath)
{
    backFacePath_ = backFacePath;
    return *this;
}

std::unique_ptr<Skybox> SkyboxBuilder::build()
{
    // Load textures
    const std::array textureFacePaths = {rightFacePath_.c_str(),
                                         leftFacePath_.c_str(),
                                         topFacePath_.c_str(),
                                         bottomFacePath_.c_str(),
                                         frontFacePath_.c_str(),
                                         backFacePath_.c_str()};

    auto skybox = std::unique_ptr<Skybox>(new Skybox);
    glGenTextures(1, &skybox->textureID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, skybox->textureID);

    for (size_t i = 0; i < textureFacePaths.size(); ++i)
    {
        int width, height, channelCount;
        uint8_t* imageData
            = stbi_load(textureFacePaths[i], &width, &height, &channelCount, 0);
        if (!imageData)
        {
            utils::showErrorMessage("unable to load skybox texture from",
                                    textureFacePaths[i]);
            return nullptr;
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
    const std::array skyboxVertices = {
        -1.0F,  1.0F, -1.0F,
        -1.0F, -1.0F, -1.0F,
         1.0F, -1.0F, -1.0F,
         1.0F,  1.0F, -1.0F,
        -1.0F,  1.0F,  1.0F,
        -1.0F, -1.0F,  1.0F,
         1.0F, -1.0F,  1.0F,
         1.0F,  1.0F,  1.0F
    };

    const std::array skyboxIndices = {
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
    glGenVertexArrays(1, &skybox->vertexArray);
    glBindVertexArray(skybox->vertexArray);

    // Create vertex buffer
    glGenBuffers(1, &skybox->vertexBuffer_);
    glBindBuffer(GL_ARRAY_BUFFER, skybox->vertexBuffer_);
    glBufferData(GL_ARRAY_BUFFER,
                 sizeof(skyboxVertices),
                 skyboxVertices.data(),
                 GL_STATIC_DRAW);

    // Create index buffer
    glGenBuffers(1, &skybox->indexBuffer_);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, skybox->indexBuffer_);
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

