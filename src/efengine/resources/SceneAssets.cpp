#include "efengine/resources/SceneAssets.h"
#include <efengine/core/Log.h>
#include <utility>

namespace efengine {
namespace resources {

    u32 SceneAssets::AddMaterial(renderer::MaterialDef def, renderer::Material material) {
        const u32 index = MaterialCount();
        m_materials.push_back(MaterialSlot{
            std::move(def),
            std::make_unique<renderer::Material>(std::move(material))
        });
        return index;
    }

    const renderer::MaterialDef* SceneAssets::DefAt(u32 index) const {
        if (index >= MaterialCount()) return null;
        return &m_materials[index].def;
    }

    const renderer::Material* SceneAssets::MaterialAt(u32 index) const {
        if (index >= MaterialCount()) return null;
        return m_materials[index].material.get();
    }

    u32 SceneAssets::IndexOfMaterial(const renderer::Material* material) const {
        if (material == null) return kInvalidIndex;
        for (u32 i = 0u; i < MaterialCount(); ++i) {
            if (m_materials[i].material.get() == material) return i;
        }
        return kInvalidIndex;
    }

    u32 SceneAssets::AddGenerated(std::string generatorName, std::vector<u8> payload,
                                  std::unique_ptr<renderer::Model> model) {
        const u32 index = GeneratedCount();
        m_generated.push_back(GeneratedSlot{
            std::move(generatorName), std::move(payload), std::move(model)
        });
        return index;
    }

    const renderer::Model* SceneAssets::GeneratedAt(u32 index) const {
        if (index >= GeneratedCount()) return null;
        return m_generated[index].model.get();
    }

    const std::string& SceneAssets::GeneratorNameAt(u32 index) const {
        static const std::string vacio;
        if (index >= GeneratedCount()) {
            EF_LOG_WARNING("SceneAssets::GeneratorNameAt: indice %u fuera de rango", index);
            return vacio;
        }
        return m_generated[index].name;
    }

    const std::vector<u8>& SceneAssets::GeneratorPayloadAt(u32 index) const {
        static const std::vector<u8> vacio;
        if (index >= GeneratedCount()) {
            EF_LOG_WARNING("SceneAssets::GeneratorPayloadAt: indice %u fuera de rango", index);
            return vacio;
        }
        return m_generated[index].payload;
    }

    u32 SceneAssets::IndexOfGenerated(const renderer::Model* model) const {
        if (model == null) return kInvalidIndex;
        for (u32 i = 0u; i < GeneratedCount(); ++i) {
            if (m_generated[i].model.get() == model) return i;
        }
        return kInvalidIndex;
    }

    void SceneAssets::Clear() {
        m_materials.clear();
        m_generated.clear();
    }

}
}