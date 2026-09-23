#include "efengine/resources/MaterialImport.h"

#include <efengine/core/Log.h>
#include <efengine/renderer/Model.h>
#include <efengine/resources/MaterialBuilder.h>

#include <optional>
#include <utility>

namespace efengine {
namespace resources {

    renderer::MaterialMap ImportMaterials(ResourceManager& rm, SceneAssets& assets,
                                          const char* modelPath) {
        renderer::MaterialMap mapa;

        if (rm.GetModel(modelPath) == null) return mapa;

        const std::vector<renderer::MaterialDef>* defs = rm.GetModelMaterials(modelPath);
        if (defs == null || defs->empty()) return mapa;

        u32 nuevos   = 0u;
        u32 reusados = 0u;

        for (const renderer::MaterialDef& def : *defs) {
            u32 indice = assets.IndexOfMaterialNamed(def.name);

            if (indice != SceneAssets::kInvalidIndex) {
                ++reusados;
            } else {
                std::optional<renderer::Material> mat = BuildMaterial(def, rm);
                if (!mat) {
                    EF_LOG_WARNING("ImportMaterials: no se pudo construir '%s'", def.name.c_str());
                    continue;
                }
                indice = assets.AddMaterial(def, std::move(*mat));
                ++nuevos;
            }

            const renderer::Material* material = assets.MaterialAt(indice);
            if (material == null) continue;

            // La clave es el materialName() de la submalla, que ModelLoader saca
            // del mismo AI_MATKEY_NAME que def.name.
            mapa[def.name] = material;
        }

        EF_LOG_INFO("ImportMaterials: '%s' - %u materiales nuevos, %u reusados",
                    modelPath, nuevos, reusados);

        return mapa;
    }

}
}
