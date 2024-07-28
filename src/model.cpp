#include "model.h"

#include "utils.h"

#include "assimp/Importer.hpp"
#include "assimp/postprocess.h"
#include "assimp/scene.h"

#include <cstddef>
#include <utility>

std::optional<Model> Model::create(std::string_view filePath)
{
    std::vector<Vertex> vertices;
    Model model;
    if (!loadModelFromFile(filePath, vertices, model.indices))
    {
        return std::nullopt;
    }

    // Create vertex array
    glGenVertexArrays(1, &model.vertexArray);
    glBindVertexArray(model.vertexArray);

    // Create vertex buffer
    glGenBuffers(1, &model.vertexBuffer_);
    glBindBuffer(GL_ARRAY_BUFFER, model.vertexBuffer_);
    glBufferData(GL_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(sizeof(Vertex) * vertices.size()),
                 vertices.data(),
                 GL_STATIC_DRAW);

    // Create index buffer
    glGenBuffers(1, &model.indexBuffer_);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, model.indexBuffer_);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(sizeof(GLuint) * model.indices.size()),
                 model.indices.data(),
                 GL_STATIC_DRAW);

    // Setup vertex array layout
    // Vertex Attribute 0: position
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0,
                          3,
                          GL_FLOAT,
                          GL_FALSE,
                          sizeof(Vertex),
                          reinterpret_cast<GLvoid*>(0));

    // Vertex Attribute 1: normal
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1,
                          3,
                          GL_FLOAT,
                          GL_FALSE,
                          sizeof(Vertex),
                          // NOLINTNEXTLINE(performance-no-int-to-ptr)
                          reinterpret_cast<GLvoid*>(offsetof(Vertex, normal)));

    glBindVertexArray(0);

    return model;
}

bool Model::loadModelFromFile(std::string_view filePath,
                              std::vector<Vertex>& outVertices,
                              std::vector<GLuint>& outIndices)
{
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(
        filePath.data(),
        aiProcess_Triangulate | aiProcess_GenNormals | aiProcess_FlipUVs);
    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE
        || !scene->mRootNode)
    {
        utils::showErrorMessage("unable to load 3D model file at ",
                                filePath,
                                ": ",
                                importer.GetErrorString());
        return false;
    }

    // aiScene contains all mesh vertices themselves.
    // aiNodes contain indices to meshes in aiScene, but no vertices.
    //
    // Sometimes you get a mesh file with just a single mesh and no nodes.
    // The bundled default files are such meshes.
    for (size_t i = 0; i < scene->mNumMeshes; ++i)
    {
        aiMesh* mesh = scene->mMeshes[i];
        for (size_t j = 0; j < mesh->mNumVertices; ++j)
        {
            Vertex vertex;
            vertex.position.x = mesh->mVertices[j].x;
            vertex.position.y = mesh->mVertices[j].y;
            vertex.position.z = mesh->mVertices[j].z;
            vertex.normal.x = mesh->mNormals[j].x;
            vertex.normal.y = mesh->mNormals[j].y;
            vertex.normal.z = mesh->mNormals[j].z;
            outVertices.push_back(vertex);
        }

        for (size_t j = 0; j < mesh->mNumFaces; ++j)
        {
            aiFace face = mesh->mFaces[j];
            for (size_t k = 0; k < face.mNumIndices; ++k)
            {
                outIndices.push_back(face.mIndices[k]);
            }
        }
    }

    return true;
}

Model::Model()
    : vertexArray{0}
    , vertexBuffer_{0}
    , indexBuffer_{0}
{
}

Model::Model(Model&& other) noexcept
    : vertexArray{std::exchange(other.vertexArray, 0)}
    , indices{std::move(other.indices)}
    , vertexBuffer_{std::exchange(other.vertexBuffer_, 0)}
    , indexBuffer_{std::exchange(other.indexBuffer_, 0)}
{
}

Model& Model::operator=(Model&& other) noexcept
{
    std::swap(vertexArray, other.vertexArray);
    indices = std::move(other.indices);
    std::swap(vertexBuffer_, other.vertexBuffer_);
    std::swap(indexBuffer_, other.indexBuffer_);
    return *this;
}

Model::~Model()
{
    glDeleteVertexArrays(1, &vertexArray);
    glDeleteBuffers(1, &indexBuffer_);
    glDeleteBuffers(1, &vertexBuffer_);
}

