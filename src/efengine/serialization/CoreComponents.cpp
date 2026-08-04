#include "efengine/serialization/CoreComponents.h"
#include <efengine/core/Log.h>
#include <efengine/serialization/ComponentRegistry.h>
#include <efengine/serialization/EfeFile.h>   // kInvalidIndex

#include <algorithm>
#include <memory>
#include <string>
#include <utility>

namespace efengine {
namespace serialization {

    bool DescribeMesh(ComponentWriter& ar, const scene::MeshAttachment& att, MeshPayload& out) {
        // Primero se pregunta si es una malla generada (la escena es su duena);
        // si no, tiene que venir de un path del ResourceManager.
        const u32 genIndex = ar.assets.IndexOfGenerated(att.model);
        if (genIndex != kInvalidIndex) {
            out.kind       = MeshKind::Generator;
            out.str        = ar.Intern(ar.assets.GeneratorNameAt(genIndex));
            out.genPayload = ar.assets.GeneratorPayloadAt(genIndex);
        } else if (const std::string* path = ar.rm.PathOf(att.model)) {
            out.kind = MeshKind::Path;
            out.str  = ar.Intern(*path);
        } else {
            EF_LOG_WARNING("SceneSerializer: malla sin path ni generador en el nodo '%s', "
                           "se omite", ar.nodeName);
            return false;
        }

        for (const auto& entry : att.materials) {
            const u32 matIndex = ar.assets.IndexOfMaterial(entry.second);
            if (matIndex == kInvalidIndex) {
                EF_LOG_WARNING("SceneSerializer: el material del submesh '%s' no esta "
                               "en SceneAssets, se omite", entry.first.c_str());
                continue;
            }
            out.bindings.push_back(MaterialBinding{ ar.Intern(entry.first), matIndex });
        }

        // MaterialMap es unordered: sin ordenar, dos guardados dan bytes distintos.
        std::sort(out.bindings.begin(), out.bindings.end(),
                  [&ar](const MaterialBinding& a, const MaterialBinding& b) {
                      return ar.strings.View(a.submeshNameStr)
                           < ar.strings.View(b.submeshNameStr);
                  });
        return true;
    }

    bool RebuildMesh(ComponentReader& ar, const MeshPayload& payload, scene::MeshAttachment& out) {
        const std::string ref(ar.View(payload.str));
        const renderer::Model* model = null;

        if (payload.kind == MeshKind::Generator) {
            MeshGeneratorRegistry::CreateFn fn = ar.generators.Find(ref);
            if (fn == null) {
                EF_LOG_WARNING("SceneSerializer: generador '%s' no registrado, "
                               "el nodo '%s' queda sin malla", ref.c_str(), ar.nodeName);
            } else {
                BinaryReader payloadReader(payload.genPayload);
                std::unique_ptr<renderer::Model> generado = fn(payloadReader);
                if (generado == nullptr) {
                    EF_LOG_WARNING("SceneSerializer: el generador '%s' no produjo "
                                   "malla para '%s'", ref.c_str(), ar.nodeName);
                } else {
                    const u32 idx = ar.assets.AddGenerated(
                        ref, payload.genPayload, std::move(generado));
                    model = ar.assets.GeneratedAt(idx);
                }
            }
        } else {
            model = ar.rm.GetModel(ref.c_str());
            if (model == null) {
                EF_LOG_WARNING("SceneSerializer: no se pudo cargar '%s', el nodo "
                               "'%s' queda sin malla", ref.c_str(), ar.nodeName);
            }
        }

        // Sin modelo no hay adjunto: el nodo se queda sin malla, pero el resto
        // del archivo se sigue cargando.
        if (model == null) return false;

        renderer::MaterialMap materials;
        for (const MaterialBinding& b : payload.bindings) {
            const renderer::Material* m = ar.assets.MaterialAt(b.materialIndex);
            if (m == null) {
                EF_LOG_WARNING("SceneSerializer: binding con material %u "
                               "invalido en '%s'", b.materialIndex, ar.nodeName);
                continue;
            }
            materials[std::string(ar.View(b.submeshNameStr))] = m;
        }

        out.model     = model;
        out.materials = std::move(materials);
        return true;
    }

    void RegisterCoreComponents(ComponentRegistry& registry) {
        // El orden de registro es el orden de emision en el archivo.
        registry.Register<scene::MeshAttachment>(kMeshComponentName);
        registry.Register<scene::LightAttachment>(kLightComponentName);
    }

}
}
