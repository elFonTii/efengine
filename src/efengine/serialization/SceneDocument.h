#pragma once
#include <efengine/core/Types.h>
#include <efengine/math/Transform.h>
#include <efengine/serialization/BinaryReader.h>
#include <efengine/serialization/BinaryWriter.h>
#include <efengine/serialization/EfeFile.h>
#include <efengine/serialization/StringTable.h>

#include <glm/glm.hpp>
#include <optional>
#include <vector>

namespace efengine {
namespace serialization {

    struct TextureRef {                 // va por Array
        u32 slot       = 0u;            // renderer::TextureSlot
        u32 pathStr    = 0u;
        u32 colorSpace = 0u;            // renderer::ColorSpace
    };

    struct MaterialRecord {
        u32 nameStr       = 0u;
        u32 shaderNameStr = 0u;
        u32 vertPathStr   = 0u;
        u32 fragPathStr   = 0u;
        std::vector<TextureRef> textures;

        glm::vec3 albedoTint  = glm::vec3(1.0f);
        f32       metallic    = 0.5f;
        f32       roughness   = 1.0f;
        f32       aoStrength  = 0.5f;
        f32       heightScale = 0.05f;
        f32       alphaCutoff = 0.5f;

        glm::vec3 emissiveTint      = glm::vec3(1.0f);
        f32       emissiveIntensity = 0.0f;
        f32       normalStrength    = 1.0f;
    };

    struct MaterialBinding {            // por Array
        u32 submeshNameStr = 0u;        // Mesh::materialName()
        u32 materialIndex  = kInvalidIndex;
    };

    enum class MeshKind : u32 { Path = 0, Generator = 1 };

    struct MeshRecord {
        MeshKind kind = MeshKind::Path;
        u32      str  = 0u;             // path del modelo, o nombre del generador
        std::vector<u8>              genPayload;   // vacio si kind == Path
        std::vector<MaterialBinding> bindings;
    };

    enum class LightKindId : u32 { Point = 0, Directional = 1 };

    struct LightRecord {
        LightKindId kind  = LightKindId::Point;
        glm::vec3   color = glm::vec3(1.0f);
    };

    struct BehaviorRecord {
        u32 typeNameStr = 0u;
        u32 enabled     = 1u;
        std::vector<u8> payload;
    };

    namespace NodeFlags {
        inline constexpr u32 HasMesh  = 1u << 0;
        inline constexpr u32 HasLight = 1u << 1;
    }

    struct NodeRecord {
        u32 nameStr = 0u;
        u32 parent  = kInvalidIndex;
        math::Transform local;
        std::optional<MeshRecord>   mesh;
        std::optional<LightRecord>  light;
        std::vector<BehaviorRecord> behaviors;
    };

    struct SceneDocument {
        StringTable strings;
        // Multiplica el ambiente difuso Y el especular del IBL. 1.0 = el HDR tal cual.
        // En v1 este f32 era ambientFactor (un ambiente constante que nunca se conecto).
        f32 iblIntensity   = 1.0f;
        u32 primarySunNode = kInvalidIndex;
        std::vector<MaterialRecord> materials;
        std::vector<NodeRecord>     nodes; 

        void Clear();
    };

    // Tamano minimo de un MaterialRecord codificado, por version:
    //   v1: 4*u32 + count del array + vec3 + 5*f32                    = 52
    //   v2: + vec3 (emissiveTint) + f32 (intensity) + f32 (strength)   = 72
    inline constexpr usize MinEncodedMaterial(u32 version) {
        return (version >= 2u) ? 72u : 52u;
    }
    inline constexpr usize kMinEncodedNode     = 52u;   // 3*u32 + transform(36) + count
    inline constexpr usize kMinEncodedBehavior = 12u;   // 2*u32 + count del payload

    template <class Ar>
    void Serialize(Ar& ar, math::Transform& t) {
        ar.Field(t.position);
        ar.Field(t.rotation);
        ar.Field(t.scale);
    }

    template <class Ar, class T>
    void SerializeVector(Ar& ar, std::vector<T>& values, usize minElementSize) {
        u32 count = static_cast<u32>(values.size());
        ar.Count(count, minElementSize);
        if (!ar.Ok()) { values.clear(); return; }

        values.resize(count);
        for (T& v : values) Serialize(ar, v);
    }

    template <class Ar>
    void Serialize(Ar& ar, MaterialRecord& m) {
        ar.Field(m.nameStr);
        ar.Field(m.shaderNameStr);
        ar.Field(m.vertPathStr);
        ar.Field(m.fragPathStr);
        ar.Array(m.textures);
        ar.Field(m.albedoTint);
        ar.Field(m.metallic);
        ar.Field(m.roughness);
        ar.Field(m.aoStrength);
        ar.Field(m.heightScale);
        ar.Field(m.alphaCutoff);

        // v2 al final del record. Leyendo v1 estos campos no se tocan
        if (ar.Version() >= 2u) {
            ar.Field(m.emissiveTint);
            ar.Field(m.emissiveIntensity);
            ar.Field(m.normalStrength);
        }
    }

    template <class Ar>
    void Serialize(Ar& ar, MeshRecord& m) {
        u32 kind = static_cast<u32>(m.kind);
        ar.Field(kind);
        m.kind = (kind == static_cast<u32>(MeshKind::Generator)) ? MeshKind::Generator
                                                                : MeshKind::Path;
        ar.Field(m.str);
        ar.Array(m.genPayload);
        ar.Array(m.bindings);
    }

    template <class Ar>
    void Serialize(Ar& ar, LightRecord& l) {
        u32 kind = static_cast<u32>(l.kind);
        ar.Field(kind);
        l.kind = (kind == static_cast<u32>(LightKindId::Directional))
                     ? LightKindId::Directional : LightKindId::Point;
        ar.Field(l.color);
    }

    template <class Ar>
    void Serialize(Ar& ar, BehaviorRecord& b) {
        ar.Field(b.typeNameStr);
        ar.Field(b.enabled);
        ar.Array(b.payload);
    }

    template <class Ar>
    void Serialize(Ar& ar, NodeRecord& n) {
        ar.Field(n.nameStr);
        ar.Field(n.parent);

        u32 flags = 0u;
        if (n.mesh)  flags |= NodeFlags::HasMesh;
        if (n.light) flags |= NodeFlags::HasLight;
        ar.Field(flags);

        Serialize(ar, n.local);

        if (flags & NodeFlags::HasMesh) {
            if (!n.mesh) n.mesh.emplace();
            Serialize(ar, *n.mesh);
        } else {
            n.mesh.reset();
        }

        if (flags & NodeFlags::HasLight) {
            if (!n.light) n.light.emplace();
            Serialize(ar, *n.light);
        } else {
            n.light.reset();
        }

        SerializeVector(ar, n.behaviors, kMinEncodedBehavior);
    }

    bool WriteSceneDocument(SceneDocument& doc, std::vector<u8>& out);
    bool ParseSceneDocument(const u8* data, usize size, SceneDocument& out);

}
}
