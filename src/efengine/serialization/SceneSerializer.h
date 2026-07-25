#pragma once
#include <efengine/core/Types.h>
#include <efengine/resources/ResourceManager.h>
#include <efengine/resources/SceneAssets.h>
#include <efengine/scene/SceneGraph.h>
#include <efengine/serialization/SceneDocument.h>
#include <efengine/serialization/SceneRegistry.h>

#include <vector>

namespace efengine {
namespace serialization {

    // Las dos conversiones impuras entre el grafo vivo y el documento del archivo.
    // El resto del formato vive en SceneDocument; aca solo se traduce.
    class SceneSerializer {
        public:
            // Grafo -> documento. Recorre en PRE-ORDEN desde Root(), asi parent < i.
            // Necesita el ResourceManager para el lookup inverso Model* -> path.
            static bool Extract(const scene::SceneGraph& graph,
                                const resources::SceneAssets& assets,
                                const resources::ResourceManager& rm,
                                const SceneRegistry& reg,
                                SceneDocument& out);

            // Documento -> grafo. Arranca limpiando grafo y assets; resuelve materiales
            // y modelos contra el ResourceManager y los registries.
            static bool Resolve(const SceneDocument& doc,
                                scene::SceneGraph& outGraph,
                                resources::SceneAssets& outAssets,
                                resources::ResourceManager& rm,
                                const SceneRegistry& reg);

            // Nivel bytes: sin tocar el disco (es lo que testean los tests).
            static bool SaveToBytes(const scene::SceneGraph& graph,
                                    const resources::SceneAssets& assets,
                                    const resources::ResourceManager& rm,
                                    const SceneRegistry& reg,
                                    std::vector<u8>& out);
            static bool LoadFromBytes(const u8* data, usize size,
                                      scene::SceneGraph& outGraph,
                                      resources::SceneAssets& outAssets,
                                      resources::ResourceManager& rm,
                                      const SceneRegistry& reg);

            // Nivel archivo: envoltorios finos sobre los de arriba.
            static bool Save(const char* path,
                             const scene::SceneGraph& graph,
                             const resources::SceneAssets& assets,
                             const resources::ResourceManager& rm,
                             const SceneRegistry& reg);
            static bool Load(const char* path,
                             scene::SceneGraph& outGraph,
                             resources::SceneAssets& outAssets,
                             resources::ResourceManager& rm,
                             const SceneRegistry& reg);
    };

}
}
