#include "gl_model.hpp"

#include "meshimporter.hpp"
#include "utils.hpp"

#include <cstddef>
#include <utility>

namespace fs = std::filesystem;

std::optional<GLModel> GLModel::create(const fs::path& filePath)
{
    std::vector<Vertex> vertices;
    GLModel model;
    if (!MeshImporter::loadFromFile(filePath,
                                    vertices,
                                    model.indices_,
                                    MeshImporter::Winding::CounterClockwise))
    {
        return std::nullopt;
    }

    // Vertex array
    glGenVertexArrays(1, &model.vertexArray_);
    glBindVertexArray(model.vertexArray_);

    // Vertex buffer
    glGenBuffers(1, &model.vertexBuffer_);
    glBindBuffer(GL_ARRAY_BUFFER, model.vertexBuffer_);
    glBufferData(GL_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(sizeof(Vertex) * vertices.size()),
                 vertices.data(),
                 GL_STATIC_DRAW);

    // Index buffer
    glGenBuffers(1, &model.indexBuffer_);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, model.indexBuffer_);
    glBufferData(
        GL_ELEMENT_ARRAY_BUFFER,
        static_cast<GLsizeiptr>(sizeof(GLuint) * model.indices_.size()),
        model.indices_.data(),
        GL_STATIC_DRAW);

    // Vertex Attribute layout
    constexpr GLuint positionVertexAttribute = 0;
    glEnableVertexAttribArray(positionVertexAttribute);
    glVertexAttribPointer(positionVertexAttribute,
                          3,
                          GL_FLOAT,
                          GL_FALSE,
                          sizeof(Vertex),
                          reinterpret_cast<GLvoid*>(0));

    constexpr GLuint normalVertexAttribute = 1;
    glEnableVertexAttribArray(normalVertexAttribute);
    glVertexAttribPointer(normalVertexAttribute,
                          3,
                          GL_FLOAT,
                          GL_FALSE,
                          sizeof(Vertex),
                          // NOLINTNEXTLINE(performance-no-int-to-ptr)
                          reinterpret_cast<GLvoid*>(offsetof(Vertex, normal)));

    glBindVertexArray(0);

    return model;
}

GLModel::GLModel()
    : vertexArray_{0}
    , vertexBuffer_{0}
    , indexBuffer_{0}
{
}

GLModel::GLModel(GLModel&& other) noexcept
    : vertexArray_{std::exchange(other.vertexArray_, 0)}
    , indices_{std::move(other.indices_)}
    , vertexBuffer_{std::exchange(other.vertexBuffer_, 0)}
    , indexBuffer_{std::exchange(other.indexBuffer_, 0)}
{
}

GLModel& GLModel::operator=(GLModel&& other) noexcept
{
    std::swap(vertexArray_, other.vertexArray_);
    indices_ = std::move(other.indices_);
    std::swap(vertexBuffer_, other.vertexBuffer_);
    std::swap(indexBuffer_, other.indexBuffer_);
    return *this;
}

GLModel::~GLModel()
{
    cleanup();
}

void GLModel::cleanup()
{
    // Existing 0s are silently ignored
    glDeleteVertexArrays(1, &vertexArray_);
    glDeleteBuffers(1, &indexBuffer_);
    glDeleteBuffers(1, &vertexBuffer_);
    vertexArray_ = 0;
    vertexBuffer_ = 0;
    indexBuffer_ = 0;
}
