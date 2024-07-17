#include "skybox.h"

#include "camera.h"
#include "shader.h"
#include "utils.h"

#include "glad/gl.h"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"
// clang-format off
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
// clang-format on

#include <cstdint>

Skybox::Skybox()
    : textureID_{0}
    , vertexArray_{0}
    , vertexBuffer_{0}
    , indexBuffer_{0}
    , shader_{nullptr}
{
}

Skybox::Skybox(Skybox&& other) noexcept
    : textureID_(other.textureID_)
    , vertexArray_(other.vertexArray_)
    , vertexBuffer_(other.vertexBuffer_)
    , indexBuffer_(other.indexBuffer_)
    , shader_(std::move(other.shader_))
{
    other.textureID_ = 0;
    other.vertexArray_ = 0;
    other.vertexBuffer_ = 0;
    other.indexBuffer_ = 0;
}

Skybox& Skybox::operator=(Skybox&& other) noexcept
{
    if (this != &other)
    {
        if (textureID_ != 0)
        {
            glDeleteTextures(1, &textureID_);
        }
        if (vertexArray_ != 0)
        {
            glDeleteVertexArrays(1, &vertexArray_);
        }
        if (vertexBuffer_ != 0)
        {
            glDeleteBuffers(1, &vertexBuffer_);
        }
        if (indexBuffer_ != 0)
        {
            glDeleteBuffers(1, &indexBuffer_);
        }

        textureID_ = other.textureID_;
        vertexArray_ = other.vertexArray_;
        vertexBuffer_ = other.vertexBuffer_;
        indexBuffer_ = other.indexBuffer_;
        shader_ = std::move(other.shader_);

        other.textureID_ = 0;
        other.vertexArray_ = 0;
        other.vertexBuffer_ = 0;
        other.indexBuffer_ = 0;
    }
    return *this;
}

Skybox::~Skybox()
{
    glDeleteTextures(1, &textureID_);
    glDeleteVertexArrays(1, &vertexArray_);
    glDeleteBuffers(1, &indexBuffer_);
    glDeleteBuffers(1, &vertexBuffer_);
}

void Skybox::draw(const glm::mat4& projection, const Camera& camera)
{
    glDepthFunc(GL_LEQUAL);
    shader_->use();
    glBindVertexArray(vertexArray_);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID_);

    // Remove camera position transformations but keep rotation by recreating
    // view matrix, then converting to mat3 and back. If you don't do this,
    // skybox will be shown as a shrinked down cube around model.
    const glm::mat4 normalizedView
        = glm::mat4(glm::mat3(camera.makeViewMatrix()));
    // Concat matrix transformations on CPU to avoid unnecessary multiplications
    // in GLSL. Results would be the same for all vertices.
    const glm::mat4 projectionView = projection * normalizedView;

    shader_->setUniform("projectionView", projectionView);
    shader_->setUniform("skybox", 0);

    glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, nullptr);
    glDepthFunc(GL_LESS);
    glBindVertexArray(0);
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
    const std::array textureFacePaths = {rightFacePath_.c_str(),
                                         leftFacePath_.c_str(),
                                         topFacePath_.c_str(),
                                         bottomFacePath_.c_str(),
                                         frontFacePath_.c_str(),
                                         backFacePath_.c_str()};

    auto skybox = std::unique_ptr<Skybox>(new Skybox);
    glGenTextures(1, &skybox->textureID_);
    glBindTexture(GL_TEXTURE_CUBE_MAP, skybox->textureID_);

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

    skybox->shader_ = Shader::createFromFile("assets/shaders/skybox.vert.glsl",
                                             "assets/shaders/skybox.frag.glsl");
    if (!skybox->shader_)
    {
        return nullptr;
    }

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

    glGenVertexArrays(1, &skybox->vertexArray_);
    glBindVertexArray(skybox->vertexArray_);

    glGenBuffers(1, &skybox->vertexBuffer_);
    glBindBuffer(GL_ARRAY_BUFFER, skybox->vertexBuffer_);
    glBufferData(GL_ARRAY_BUFFER,
                 sizeof(skyboxVertices),
                 skyboxVertices.data(),
                 GL_STATIC_DRAW);

    glGenBuffers(1, &skybox->indexBuffer_);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, skybox->indexBuffer_);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 sizeof(skyboxIndices),
                 skyboxIndices.data(),
                 GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0,
                          3,
                          GL_FLOAT,
                          GL_FALSE,
                          3 * sizeof(float),
                          reinterpret_cast<GLvoid*>(0));

    return skybox;
}
