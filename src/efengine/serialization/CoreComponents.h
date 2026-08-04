#pragma once
#include <efengine/core/Types.h>
#include <efengine/scene/Components.h>
#include <efengine/serialization/ComponentArchive.h>
#include <efengine/serialization/ComponentPayloads.h>

namespace efengine {
namespace serialization {

    class ComponentRegistry;   // fwd: solo hace falta para RegisterCoreComponents

    // Los dos componentes que trae el motor. Se registran solos desde el
    // constructor de SceneRegistry: si dependieran de que el cliente se acuerde
    // de registrarlos, olvidarse significaria cargar todas las escenas sin
    // mallas ni luces. Los behaviors si son del cliente y se siguen registrando
    // a mano. Sus nombres en el archivo estan en ComponentPayloads.h.

    // --- traduccion entre el adjunto vivo y el payload ----------------------
    // La malla es el unico componente con las dos direcciones genuinamente
    // asimetricas: guardar es buscar de que asset vino, cargar es resolverlo
    // contra el ResourceManager. Las dos pueden fallar, y en los dos casos la
    // respuesta es "este nodo se queda sin malla", no "el archivo esta roto".
    bool DescribeMesh(ComponentWriter& ar, const scene::MeshAttachment& att, MeshPayload& out);
    bool RebuildMesh(ComponentReader& ar, const MeshPayload& payload, scene::MeshAttachment& out);

    template <class Ar>
    bool SerializeComponent(Ar& ar, scene::MeshAttachment& m) {
        MeshPayload payload;

        if constexpr (!Ar::kIsReading) {
            if (!DescribeMesh(ar, m, payload)) return false;   // no se emite el record
        }

        SerializePayload(ar, payload);

        if constexpr (Ar::kIsReading) {
            if (!ar.Ok()) return false;
            return RebuildMesh(ar, payload, m);
        }
        return true;
    }

    template <class Ar>
    bool SerializeComponent(Ar& ar, scene::LightAttachment& l) {
        LightPayload payload;

        if constexpr (!Ar::kIsReading) {
            payload.kind  = (l.kind == scene::LightKind::Directional)
                                ? LightKindId::Directional : LightKindId::Point;
            payload.color = l.color;
        }

        SerializePayload(ar, payload);

        if constexpr (Ar::kIsReading) {
            if (!ar.Ok()) return false;
            l.kind  = (payload.kind == LightKindId::Directional)
                          ? scene::LightKind::Directional : scene::LightKind::Point;
            l.color = payload.color;
        }
        return true;
    }

    void RegisterCoreComponents(ComponentRegistry& registry);

}
}
