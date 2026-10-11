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
                                    model.m_indices,
                                    MeshImporter::Winding::CounterClockwise))
    {
        return std::nullopt;
    }

    // Vertex array
    glGenVertexArrays(1, &model.m_vertexArray);
    glBindVertexArray(model.m_vertexArray);

    // Vertex buffer
    glGenBuffers(1, &model.m_vertexBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, model.m_vertexBuffer);
    glBufferData(GL_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(sizeof(Vertex) * vertices.size()),
                 vertices.data(),
                 GL_STATIC_DRAW);

    // Index buffer
    glGenBuffers(1, &model.m_indexBuffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, model.m_indexBuffer);
    glBufferData(
        GL_ELEMENT_ARRAY_BUFFER,
        static_cast<GLsizeiptr>(sizeof(GLuint) * model.m_indices.size()),
        model.m_indices.data(),
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
    : m_vertexArray{0}
    , m_vertexBuffer{0}
    , m_indexBuffer{0}
{
}

GLModel::GLModel(GLModel&& other) noexcept
    : m_vertexArray{std::exchange(other.m_vertexArray, 0)}
    , m_indices{std::move(other.m_indices)}
    , m_vertexBuffer{std::exchange(other.m_vertexBuffer, 0)}
    , m_indexBuffer{std::exchange(other.m_indexBuffer, 0)}
{
}

GLModel& GLModel::operator=(GLModel&& other) noexcept
{
    std::swap(m_vertexArray, other.m_vertexArray);
    m_indices = std::move(other.m_indices);
    std::swap(m_vertexBuffer, other.m_vertexBuffer);
    std::swap(m_indexBuffer, other.m_indexBuffer);
    return *this;
}

GLModel::~GLModel()
{
    cleanup();
}

void GLModel::cleanup()
{
    // Existing 0s are silently ignored
    glDeleteVertexArrays(1, &m_vertexArray);
    glDeleteBuffers(1, &m_indexBuffer);
    glDeleteBuffers(1, &m_vertexBuffer);
    m_vertexArray = 0;
    m_vertexBuffer = 0;
    m_indexBuffer = 0;
}
