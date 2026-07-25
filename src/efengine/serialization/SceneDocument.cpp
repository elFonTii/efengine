#include "efengine/serialization/SceneDocument.h"
#include <efengine/core/Log.h>
#include <utility>

namespace efengine {
namespace serialization {

    void SceneDocument::Clear() {
        strings.Clear();
        ambientFactor  = 0.08f;
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
        w.Field(doc.ambientFactor);
        w.Field(doc.primarySunNode);
        EndChunk(w, marker);

        marker = BeginChunk(w, ChunkId::Materials);
        SerializeVector(w, doc.materials, kMinEncodedMaterial);
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
                    r.Field(out.ambientFactor);
                    r.Field(out.primarySunNode);
                    break;
                case ChunkId::Materials:
                    SerializeVector(r, out.materials, kMinEncodedMaterial);
                    break;
                case ChunkId::Nodes:
                    SerializeVector(r, out.nodes, kMinEncodedNode);
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
