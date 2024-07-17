#include "model.h"

#include "camera.h"
#include "drawproperties.h"
#include "shader.h"
#include "utils.h"

#include "assimp/Importer.hpp"
#include "assimp/postprocess.h"
#include "assimp/scene.h"
#include "glad/gl.h"
#include "glm/gtc/type_ptr.hpp"

#include <cstddef>

struct Vertex
{
    glm::vec3 position;
    glm::vec3 normal;
};

std::unique_ptr<Model> Model::create(const char* filePath)
{
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(
        filePath,
        aiProcess_Triangulate | aiProcess_GenNormals | aiProcess_FlipUVs);
    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE
        || !scene->mRootNode)
    {
        utils::errorMessage("Unable to load 3D model file at ",
                            filePath,
                            ": ",
                            importer.GetErrorString());
        return nullptr;
    }

    // aiScene contains all mesh vertices themselves.
    // aiNodes contain indices to meshes in aiScene, but no vertices.
    //
    // Sometimes you get a mesh file with just a single mesh and no nodes.
    // The bundled default files are such meshes.
    auto model = std::make_unique<Model>();
    std::vector<Vertex> vertices;
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
            vertices.push_back(vertex);
        }

        for (size_t j = 0; j < mesh->mNumFaces; ++j)
        {
            aiFace face = mesh->mFaces[j];
            for (size_t k = 0; k < face.mNumIndices; ++k)
            {
                model->indices_.push_back(face.mIndices[k]);
            }
        }
    }

    glGenVertexArrays(1, &model->vertexArray_);
    glBindVertexArray(model->vertexArray_);

    glGenBuffers(1, &model->vertexBuffer_);
    glBindBuffer(GL_ARRAY_BUFFER, model->vertexBuffer_);
    glBufferData(GL_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(sizeof(Vertex) * vertices.size()),
                 vertices.data(),
                 GL_STATIC_DRAW);

    glGenBuffers(1, &model->indexBuffer_);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, model->indexBuffer_);
    glBufferData(
        GL_ELEMENT_ARRAY_BUFFER,
        static_cast<GLsizeiptr>(sizeof(GLuint) * model->indices_.size()),
        model->indices_.data(),
        GL_STATIC_DRAW);

    model->shader_ = Shader::createFromFile("assets/shaders/model.vert.glsl",
                                            "assets/shaders/model.frag.glsl");

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

Model::~Model()
{
    glDeleteVertexArrays(1, &vertexArray_);
    glDeleteBuffers(1, &indexBuffer_);
    glDeleteBuffers(1, &vertexBuffer_);
}

void Model::draw(const glm::mat4& projection,
                 const Camera& camera,
                 const DrawProperties& drawProps)
{
    shader_->use();
    glBindVertexArray(vertexArray_);

    glm::mat4 model(1.0F);

    // Avoid Gimbal-lock
    const glm::quat quatX
        = glm::angleAxis(glm::radians(drawProps.modelRotation[0]),
                         glm::vec3(1.0F, 0.0F, 0.0F));
    const glm::quat quatY
        = glm::angleAxis(glm::radians(drawProps.modelRotation[1]),
                         glm::vec3(0.0F, 1.0F, 0.0F));
    const glm::quat quatZ
        = glm::angleAxis(glm::radians(drawProps.modelRotation[2]),
                         glm::vec3(0.0F, 0.0F, 1.0F));
    const glm::quat quat = quatZ * quatY * quatX;
    model = glm::mat4_cast(quat);

    // Concat matrix transformations on CPU to avoid unnecessary multiplications
    // in GLSL. Results would be the same for all vertices.
    const glm::mat4 view = camera.makeViewMatrix();
    const glm::mat4 mvp = projection * view * model;
    const glm::mat3 normalMatrix
        = glm::mat3(glm::transpose(glm::inverse(model)));

    shader_->setUniform("model", model);
    shader_->setUniform("mvp", mvp);
    shader_->setUniform("normalMatrix", normalMatrix);
    shader_->setUniform("u_color", drawProps.modelColor);
    shader_->setUniform("light.direction", drawProps.lightDirection);
    shader_->setUniform("viewPos", camera.position());

    shader_->updateSubroutines(
        GL_FRAGMENT_SHADER,
        {drawProps.diffuseEnabled ? "DiffuseEnabled" : "Disabled",
         drawProps.specularEnabled ? "SpecularEnabled" : "Disabled"});

    glPolygonMode(GL_FRONT_AND_BACK,
                  drawProps.wireframeModeEnabled ? GL_LINE : GL_FILL);

    glDrawElements(GL_TRIANGLES,
                   static_cast<GLsizei>(indices_.size()),
                   GL_UNSIGNED_INT,
                   nullptr);

    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glBindVertexArray(0);
}
