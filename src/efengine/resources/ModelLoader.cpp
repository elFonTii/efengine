#include "ModelLoader.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <efengine/core/Assert.h>
#include <efengine/core/Log.h>

#include <vector>
#include <utility>

// https://the-asset-importer-lib-documentation.readthedocs.io/en/latest/usage/use_the_lib.html

namespace efengine {
namespace resources {

    std::optional<renderer::Model> ModelLoader::Load(const char* path) {
        EF_ASSERT(path != null, "ModelLoader::Load: path no puede ser null");

        Assimp::Importer importer;
        const aiScene* scene = importer.ReadFile(path,
            aiProcess_Triangulate |
            aiProcess_GenSmoothNormals |
            aiProcess_CalcTangentSpace |
            aiProcess_JoinIdenticalVertices |
            aiProcess_PreTransformVertices
        );

        if (scene == null || (scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) || scene->mRootNode == null) {
            EF_LOG_ERROR("ModelLoader::Load: %s", importer.GetErrorString());
            return std::nullopt;
        }

        std::vector<renderer::Mesh> meshes;
        meshes.reserve(scene->mNumMeshes);

        for (u32 i = 0; i < scene->mNumMeshes; ++i) {
            const aiMesh* m = scene->mMeshes[i];

            std::vector<renderer::Vertex> vertices;
            vertices.reserve(m->mNumVertices);

            for (u32 v = 0; v < m->mNumVertices; ++v) {
                renderer::Vertex vert;

                vert.position = { m->mVertices[v].x, m->mVertices[v].y, m->mVertices[v].z };
                vert.normal   = { m->mNormals[v].x,  m->mNormals[v].y,  m->mNormals[v].z  };

                if (m->mTangents != null) {
                    vert.tangent = { m->mTangents[v].x, m->mTangents[v].y, m->mTangents[v].z };
                } else {
                    vert.tangent = { 0.0f, 0.0f, 0.0f };
                }

                // mTextureCoords[0] = canal 0 de UVs. assimp guarda 3
                // solo se usa x,y.
                if (m->mTextureCoords[0] != null) {
                    vert.uv = { m->mTextureCoords[0][v].x, m->mTextureCoords[0][v].y };
                } else {
                    vert.uv = { 0.0f, 0.0f };
                }

                vertices.push_back(vert);
            }

            std::vector<u32> indices;
            indices.reserve(m->mNumFaces * 3);

            for (u32 f = 0; f < m->mNumFaces; ++f) {
                const aiFace& face = m->mFaces[f];
                for (u32 j = 0; j < face.mNumIndices; ++j) {
                    indices.push_back(face.mIndices[j]);
                }
            }

            meshes.emplace_back(vertices, indices);
        }

        return renderer::Model(std::move(meshes));
    }

}
}