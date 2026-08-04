#include "efengine/serialization/SceneDocument.h"
#include <efengine/core/Log.h>
#include <efengine/serialization/ComponentPayloads.h>
#include <optional>
#include <utility>

namespace efengine {
namespace serialization {

    // ---------------------------------------------------------------------------
    // LEGACY v<=4 -- NO EXTENDER
    //
    // Hasta v4 el NodeRecord llevaba un bitfield de flags y los cuerpos de mesh y
    // light incrustados, posicionales y sin tamano. Desde v5 son ComponentRecords
    // como cualquier otro adjunto.
    //
    // Este es el unico lugar del motor que todavia sabe que existieron mesh y
    // light como campos del formato. Decodifica el layout viejo y lo sube a
    // components; de ahi en adelante el resto del pipeline no distingue un .efe
    // v3 de uno v5. Un componente NUEVO no se agrega aca: se registra en el
    // ComponentRegistry y ya.
    //
    // Los payloads se re-encodean con el mismo SerializePayload que usa el
    // componente vivo, asi que salen byte a byte iguales al cuerpo original.
    // ---------------------------------------------------------------------------
    namespace legacy {

        namespace NodeFlags {
            inline constexpr u32 HasMesh  = 1u << 0;
            inline constexpr u32 HasLight = 1u << 1;
        }

        struct NodeRecordV4 {
            u32 nameStr = 0u;
            u32 parent  = kInvalidIndex;
            math::Transform local;
            std::optional<MeshPayload>  mesh;
            std::optional<LightPayload> light;
            std::vector<BehaviorRecord> behaviors;
        };

        // Solo lectura: el writer nunca emite este layout.
        inline void Serialize(BinaryReader& ar, NodeRecordV4& n) {
            ar.Field(n.nameStr);
            ar.Field(n.parent);

            u32 flags = 0u;
            ar.Field(flags);

            serialization::Serialize(ar, n.local);

            if (flags & NodeFlags::HasMesh) {
                n.mesh.emplace();
                SerializePayload(ar, *n.mesh);
            }
            if (flags & NodeFlags::HasLight) {
                n.light.emplace();
                SerializePayload(ar, *n.light);
            }

            SerializeVector(ar, n.behaviors, kMinEncodedBehavior);
        }

        // Re-encodea un payload viejo como el blob de un ComponentRecord.
        template <class Payload>
        ComponentRecord toComponent(Payload payload, const char* typeName, StringTable& strings) {
            BinaryWriter w;
            SerializePayload(w, payload);

            ComponentRecord rec;
            rec.typeNameStr = strings.Intern(typeName);
            rec.payload     = w.Take();
            return rec;
        }

        NodeRecord upgrade(NodeRecordV4& old, StringTable& strings) {
            NodeRecord rec;
            rec.nameStr   = old.nameStr;
            rec.parent    = old.parent;
            rec.local     = old.local;
            rec.behaviors = std::move(old.behaviors);

            // El orden importa: tiene que ser el mismo que el de registro en
            // RegisterCoreComponents, o abrir y guardar un archivo viejo
            // cambiaria los bytes de lugar sin cambiar la escena.
            if (old.mesh) {
                rec.components.push_back(
                    toComponent(std::move(*old.mesh), kMeshComponentName, strings));
            }
            if (old.light) {
                rec.components.push_back(
                    toComponent(*old.light, kLightComponentName, strings));
            }
            return rec;
        }

        void parseNodes(BinaryReader& r, SceneDocument& out) {
            std::vector<NodeRecordV4> viejos;
            SerializeVector(r, viejos, kMinEncodedNode);
            if (!r.Ok()) return;

            out.nodes.clear();
            out.nodes.reserve(viejos.size());
            for (NodeRecordV4& v : viejos) out.nodes.push_back(upgrade(v, out.strings));
        }

    }

    void SceneDocument::Clear() {
        strings.Clear();
        iblIntensity   = 1.0f;
        primarySunNode = kInvalidIndex;
        materials.clear();
        nodes.clear();
    }

    bool WriteSceneDocument(SceneDocument& doc, std::vector<u8>& out) {
        BinaryWriter w;
        WriteFileHeader(w, ContentType::Scene, kSceneChunkCount);

        usize marker = BeginChunk(w, ChunkId::Strings);
        doc.strings.Serialize(w);
        EndChunk(w, marker);

        marker = BeginChunk(w, ChunkId::Settings);
        w.Field(doc.iblIntensity);
        w.Field(doc.primarySunNode);
        EndChunk(w, marker);

        marker = BeginChunk(w, ChunkId::Materials);
        SerializeVector(w, doc.materials, MinEncodedMaterial(w.Version()));
        EndChunk(w, marker);

        marker = BeginChunk(w, ChunkId::Nodes);
        SerializeVector(w, doc.nodes, kMinEncodedNode);
        EndChunk(w, marker);

        out = w.Take();
        return true;
    }

    bool ParseSceneDocument(const u8* data, usize size, SceneDocument& out) {
        out.Clear();
        if (data == null || size == 0u) {
            EF_LOG_ERROR("SceneDocument: buffer vacio");
            return false;
        }

        BinaryReader r(data, size);
        u32 chunkCount = 0u;
        if (!ReadFileHeader(r, ContentType::Scene, chunkCount)) { out.Clear(); return false; }

        bool vioStrings = false;
        for (u32 i = 0u; i < chunkCount; ++i) {
            ChunkId id  = ChunkId::Strings;
            usize   end = 0u;
            if (!NextChunk(r, id, end)) { out.Clear(); return false; }

            // STRT tiene que venir primero: todo lo demas referencia sus indices.
            if (!vioStrings && id != ChunkId::Strings) {
                EF_LOG_ERROR("SceneDocument: el chunk STRT no es el primero");
                out.Clear();
                return false;
            }

            switch (id) {
                case ChunkId::Strings:
                    out.strings.Serialize(r);
                    vioStrings = true;
                    break;
                case ChunkId::Settings:
                    if (r.Version() >= 2u) {
                        r.Field(out.iblIntensity);
                    } else {
                        // v1: el f32 era ambientFactor. Hay que CONSUMIRLO igual para no
                        // desalinear el cursor, pero su valor no significa lo mismo.
                        f32 legacyAmbient = 0.0f;
                        r.Field(legacyAmbient);
                        out.iblIntensity = 1.0f;
                        EF_LOG_INFO("SceneDocument: escena v1 migrada, ambientFactor %.3f "
                                    "descartado, iblIntensity = 1.0", legacyAmbient);
                    }
                    r.Field(out.primarySunNode);
                    break;
                case ChunkId::Materials:
                    SerializeVector(r, out.materials, MinEncodedMaterial(r.Version()));
                    break;
                case ChunkId::Nodes:
                    // La unica rama por version del layout de nodos. De aca para
                    // abajo nadie mas distingue un archivo v3 de uno v5.
                    if (r.Version() >= 5u) {
                        SerializeVector(r, out.nodes, kMinEncodedNode);
                    } else {
                        legacy::parseNodes(r, out);
                    }
                    break;
                default:
                    // Chunk que el build no conoce se saltea entero
                    break;
            }

            if (!r.Ok()) {
                EF_LOG_ERROR("SceneDocument: error leyendo el chunk %u de %u", i, chunkCount);
                out.Clear();
                return false;
            }
            r.EndSizedBlock(end);
        }

        if (!r.Ok()) { out.Clear(); return false; }

        // Invariante estructural: pre-orden. Sin esto el resolve podria referenciar el
        // handle de un padre que todavia no fue creado.
        if (!out.nodes.empty() && out.nodes[0].parent != kInvalidIndex) {
            EF_LOG_ERROR("SceneDocument: el nodo 0 no es la raiz");
            out.Clear();
            return false;
        }
        for (usize i = 1u; i < out.nodes.size(); ++i) {
            if (out.nodes[i].parent >= static_cast<u32>(i)) {
                EF_LOG_ERROR("SceneDocument: el nodo %zu rompe el pre-orden (parent=%u)",
                             i, out.nodes[i].parent);
                out.Clear();
                return false;
            }
        }
        if (out.primarySunNode != kInvalidIndex
            && out.primarySunNode >= static_cast<u32>(out.nodes.size())) {
            EF_LOG_ERROR("SceneDocument: primarySunNode fuera de rango");
            out.Clear();
            return false;
        }

        return true;
    }

}
}
