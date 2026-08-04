#pragma once
#include <efengine/core/Types.h>
#include <efengine/serialization/EfeFile.h>   // kInvalidIndex

#include <glm/glm.hpp>
#include <vector>

namespace efengine {
namespace serialization {

    // El formato en disco del payload de los componentes del motor. Vive aparte
    // de SceneDocument porque es el formato de ESTOS componentes, no del
    // contenedor: SceneDocument solo ve un blob con largo y un nombre de tipo.
    //
    // Esta en su propio header, y no adentro de CoreComponents, porque lo usan
    // dos lugares: el componente vivo (CoreComponents) y el decodificador de
    // archivos v<=4 (SceneDocument.cpp), que necesita el layout viejo sin saber
    // nada de MeshAttachment ni de ResourceManager. Compartirlo es lo que hace
    // que "el payload nuevo es identico al record viejo" sea un hecho de
    // compilacion en vez de un comentario que se puede desincronizar.

    // Los nombres con los que estos dos componentes viajan en el archivo. Van
    // aca junto al layout porque el shim legacy tambien los necesita, y no debe
    // arrastrar el resto de CoreComponents (ResourceManager, SceneAssets) solo
    // para escribir dos strings.
    inline constexpr const char* kMeshComponentName  = "efengine.mesh";
    inline constexpr const char* kLightComponentName = "efengine.light";

    enum class MeshKind : u32 { Path = 0, Generator = 1 };

    struct MaterialBinding {            // por Array
        u32 submeshNameStr = 0u;        // Mesh::materialName()
        u32 materialIndex  = kInvalidIndex;
    };

    enum class LightKindId : u32 { Point = 0, Directional = 1 };

    struct MeshPayload {
        MeshKind kind = MeshKind::Path;
        u32      str  = 0u;             // path del modelo, o nombre del generador
        std::vector<u8>              genPayload;   // vacio si kind == Path
        std::vector<MaterialBinding> bindings;
    };

    struct LightPayload {
        LightKindId kind  = LightKindId::Point;
        glm::vec3   color = glm::vec3(1.0f);
    };

    template <class Ar>
    void SerializePayload(Ar& ar, MeshPayload& m) {
        u32 kind = static_cast<u32>(m.kind);
        ar.Field(kind);
        m.kind = (kind == static_cast<u32>(MeshKind::Generator)) ? MeshKind::Generator
                                                                : MeshKind::Path;
        ar.Field(m.str);
        ar.Array(m.genPayload);
        ar.Array(m.bindings);
    }

    template <class Ar>
    void SerializePayload(Ar& ar, LightPayload& l) {
        u32 kind = static_cast<u32>(l.kind);
        ar.Field(kind);
        l.kind = (kind == static_cast<u32>(LightKindId::Directional))
                     ? LightKindId::Directional : LightKindId::Point;
        ar.Field(l.color);
    }

}
}
