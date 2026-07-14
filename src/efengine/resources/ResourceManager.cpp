#include "ResourceManager.h"
#include <efengine/resources/ModelLoader.h>
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
}
}