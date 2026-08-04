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

        // v3. u32 y no bool: el tamano de bool no esta garantizado y el formato
        // no puede depender de eso. Mismo patron que los enums ya serializados.
        u32       doubleSided       = 0u;

        // v4. Transformacion de la UV comun a los 8 mapas: uv = vUV * tiling + offset.
        // El default es la identidad, que es lo que hacia el shader antes de v4.
        glm::vec2 uvTiling = glm::vec2(1.0f);
        glm::vec2 uvOffset = glm::vec2(0.0f);
    };

    struct BehaviorRecord {
        u32 typeNameStr = 0u;
        u32 enabled     = 1u;
        std::vector<u8> payload;
    };

    // Un adjunto cualquiera, visto por el contenedor: un nombre de tipo y un
    // blob. El documento NO sabe que es una malla ni una luz -- eso lo sabe el
    // ComponentRegistry. Un componente nuevo entra al .efe sin tocar este
    // archivo.
    //
    // El payload va por Array, o sea con largo: un tipo que esta build no
    // conoce se saltea con un warning. Hasta v4 mesh y light iban posicionales
    // y sin tamano, asi que un adjunto desconocido desalineaba el nodo entero.
    struct ComponentRecord {
        u32 typeNameStr = 0u;
        std::vector<u8> payload;
    };

    struct NodeRecord {
        u32 nameStr = 0u;
        u32 parent  = kInvalidIndex;
        math::Transform local;
        std::vector<ComponentRecord> components;
        std::vector<BehaviorRecord>  behaviors;
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
    //   v3: + u32 (doubleSided)                                        = 76
    //   v4: + vec2 (uvTiling) + vec2 (uvOffset)                        = 92
    inline constexpr usize MinEncodedMaterial(u32 version) {
        if (version >= 4u) return 92u;
        if (version >= 3u) return 76u;
        return (version >= 2u) ? 72u : 52u;
    }
    // 52 en las dos versiones, por casualidad util: v<=4 era
    // 3*u32 (name, parent, flags) + transform(36) + count de behaviors, y v5 es
    // 2*u32 (name, parent) + transform(36) + los counts de components y behaviors.
    inline constexpr usize kMinEncodedNode      = 52u;
    inline constexpr usize kMinEncodedBehavior  = 12u;   // 2*u32 + count del payload
    inline constexpr usize kMinEncodedComponent =  8u;   // u32 + count del payload

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

        // v3 al final del record. Leyendo v2 este campo no se toca: queda en 0
        // (material de una sola cara), que es el comportamiento de siempre.
        if (ar.Version() >= 3u) {
            ar.Field(m.doubleSided);
        }

        // v4 al final del record. Leyendo v3 estos campos no se tocan y quedan en
        // (1,1)/(0,0) — la identidad, o sea la UV de la malla tal cual.
        if (ar.Version() >= 4u) {
            ar.Field(m.uvTiling);
            ar.Field(m.uvOffset);
        }
    }

    template <class Ar>
    void Serialize(Ar& ar, BehaviorRecord& b) {
        ar.Field(b.typeNameStr);
        ar.Field(b.enabled);
        ar.Array(b.payload);
    }

    template <class Ar>
    void Serialize(Ar& ar, ComponentRecord& c) {
        ar.Field(c.typeNameStr);
        ar.Array(c.payload);
    }

    // v5 en adelante. Sin un solo 'if' por tipo de adjunto: los flags y los
    // cuerpos de mesh/light que habia hasta v4 los decodifica el shim legacy de
    // SceneDocument.cpp, que los sube a components.
    template <class Ar>
    void Serialize(Ar& ar, NodeRecord& n) {
        ar.Field(n.nameStr);
        ar.Field(n.parent);
        Serialize(ar, n.local);
        SerializeVector(ar, n.components, kMinEncodedComponent);
        SerializeVector(ar, n.behaviors,  kMinEncodedBehavior);
    }

    bool WriteSceneDocument(SceneDocument& doc, std::vector<u8>& out);
    bool ParseSceneDocument(const u8* data, usize size, SceneDocument& out);

}
}
