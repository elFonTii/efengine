#include "ResourceManager.h"
#include <efengine/resources/ModelLoader.h>
#include <efengine/resources/FileIO.h>
#include <efengine/core/Assert.h>
#include <utility>

namespace efengine {
namespace resources {
    renderer::Model* ResourceManager::GetModel(const char* path){
        EF_ASSERT(path != null, "ResourceManager::GetModel: Intentando acceder a un directorio nulo");
        
        if (auto it = m_models.find(path); it != m_models.end()) {
            // found
            return &it->second;
        } else {
            // not found, not cached -> load the model.
            auto result = resources::ModelLoader::Load(path);
            if(!result) return null;

            auto [inserted, ok] = m_models.emplace(path, std::move(*result));
            EF_ASSERT(ok, "ResourceManager::GetModel: emplace no insertó tras un miss");

            // insertó, retornar
            return &inserted->second;
        }
    }

    renderer::Texture* ResourceManager::GetTexture(const char* path, renderer::ColorSpace space){
        EF_ASSERT(path != null, "ResourceManager::GetTexture: Intentando acceder a un directorio nulo");
        
        if (auto it = m_textures.find(path); it != m_textures.end()) {
            // found
            if(it->second.space != space) {
                EF_LOG_WARNING("ResourceManager: '%s' ya cacheada con otro ColorSpace, se reutiliza", path);
            }

            return &it->second.texture;
        } else {
            // not found, not cached -> load the texture.
            auto result = renderer::Texture::Create(path, space);
            if(!result) return null;

            auto [inserted, ok] = m_textures.emplace(path, TextureSlot{ std::move(*result), space });
            EF_ASSERT(ok, "ResourceManager::GetTexture: emplace no insertó tras un miss");

            // insertó, retornar
            return &inserted->second.texture;
        }
    }

    renderer::Shader* ResourceManager::GetShader(const char* name, const char* vertPath, const char* fragPath) {
        EF_ASSERT(vertPath != null && fragPath !=null, "ResourceManager::GetShader: Intentando acceder a un directorio nulo");
        EF_ASSERT(name != null, "ResourceManager::GetShader: Intentando almacenar un shader sin nombre");

        if (auto it = m_shaders.find(name); it != m_shaders.end()) {
            // found
            return &it->second;
        } else {
            // cargar vertex y fragment
            auto vertex = FileIO::ReadText(vertPath);
            auto fragment = FileIO::ReadText(fragPath);
            if(!vertex || !fragment) return null;

            // crear shader
            auto result = renderer::Shader::Create(vertex->c_str(), fragment->c_str());
            if(!result) return null;

            auto [inserted, ok] = m_shaders.emplace(name, std::move(*result));
            EF_ASSERT(ok, "ResourceManager::GetShader: emplace no insertó tras un miss");

            return &inserted->second;
        }
    }

    renderer::Shader* ResourceManager::GetComputeShader(const char* name, const char* computePath) {
        EF_ASSERT(computePath != null, "ResourceManager::GetComputeShader: Intentando acceder a un directorio nulo");
        EF_ASSERT(name != null, "ResourceManager::GetComputeShader: Intentando almacenar un shader sin nombre");

        if (auto it = m_shaders.find(name); it != m_shaders.end()) {
            // found
            return &it->second;
        } else {
            // cargar vertex y fragment
            auto compute = FileIO::ReadText(computePath);
            if(!compute) return null;

            // crear compute shader
            auto result = renderer::Shader::CreateCompute(compute->c_str());
            if(!result) return null;

            auto [inserted, ok] = m_shaders.emplace(name, std::move(*result));
            EF_ASSERT(ok, "ResourceManager::GetComputeShader: emplace no insertó tras un miss");

            return &inserted->second;
        }
    }
}
}