#pragma once
#include <efengine/core/Types.h>
#include <efengine/renderer/Model.h>
#include <efengine/serialization/BinaryReader.h>
#include <efengine/serialization/BinaryWriter.h>

#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace efengine {
namespace serialization {

    // Mallas que no vienen de un archivo (el plano procedural del sandbox). El .efe
    // guarda el nombre del generador y sus params; el cliente registra el generador.
    class MeshGeneratorRegistry {
        public:
            using CreateFn = std::unique_ptr<renderer::Model> (*)(BinaryReader&);

            void     Register(const char* name, CreateFn fn);
            CreateFn Find(std::string_view name) const;

        private:
            std::unordered_map<std::string, CreateFn> m_byName;
    };

    // Serializa 'params', invoca el generador y devuelve el modelo. 'outPayload' queda
    // con los bytes exactos que hay que guardar en el .efe para poder recrearlo.
    // Params tiene que tener 'template <class Ar> void Serialize(Ar&)'.
    template <class Params>
    std::unique_ptr<renderer::Model> CreateGenerated(const MeshGeneratorRegistry& reg,
                                                     const char* name,
                                                     Params params,
                                                     std::vector<u8>& outPayload) {
        outPayload.clear();

        MeshGeneratorRegistry::CreateFn fn = reg.Find(name);
        if (fn == null) return nullptr;

        BinaryWriter w;
        params.Serialize(w);
        std::vector<u8> bytes = w.Take();

        BinaryReader r(bytes);
        std::unique_ptr<renderer::Model> model = fn(r);
        // Distingue "fallo leyendo sus params" de "devolvio null a proposito".
        if (model == nullptr && !r.Ok()) return nullptr;

        outPayload = std::move(bytes);
        return model;
    }

}
}
