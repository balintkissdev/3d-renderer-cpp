#include "modelimporter.hpp"

#include "utils.hpp"

#include "assimp/Importer.hpp"
#include "assimp/postprocess.h"
#include "assimp/scene.h"

namespace fs = std::filesystem;

namespace ModelImporter
{
bool loadFromFile(const fs::path& filePath,
                  std::vector<Vertex>& outVertices,
                  std::vector<uint32_t>& outIndices,
                  const Winding windingOrder)
{
    uint32_t importFlags
        = aiProcess_Triangulate | aiProcess_GenNormals | aiProcess_FlipUVs;
    if (windingOrder == Winding::Clockwise)
    {
        importFlags |= aiProcess_FlipWindingOrder;
    }
    Assimp::Importer importer;
    const aiScene* scene
        = importer.ReadFile(filePath.string().c_str(), importFlags);
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
            vertex.position[0] = mesh->mVertices[j].x;
            vertex.position[1] = mesh->mVertices[j].y;
            vertex.position[2] = mesh->mVertices[j].z;
            vertex.normal[0] = mesh->mNormals[j].x;
            vertex.normal[1] = mesh->mNormals[j].y;
            vertex.normal[2] = mesh->mNormals[j].z;
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
}  // namespace ModelImporter

