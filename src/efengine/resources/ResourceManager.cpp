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
}
}