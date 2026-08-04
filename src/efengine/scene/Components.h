#pragma once
#include <efengine/renderer/Model.h>
#include <efengine/renderer/Material.h>

#include <glm/glm.hpp>

namespace efengine {
namespace scene {

    // Los componentes del motor: datos planos y nada mas. No saben guardarse a
    // si mismos -- eso vive en serialization/CoreComponents.h, del mismo modo
    // que Behavior vive aca y BehaviorRegistry vive alla. Sin esa separacion
    // scene/ terminaria dependiendo del formato de archivo.
    //
    // No se guardan dentro del Node: viven en el ComponentStore del SceneGraph,
    // en un array por tipo indexado por NodeHandle::index.

    enum class LightKind { Point, Directional };

    struct MeshAttachment {
        const renderer::Model* model = nullptr;
        renderer::MaterialMap  materials;
    };

    struct LightAttachment {
        LightKind kind = LightKind::Point;
        glm::vec3 color { 1.0f };
    };

}
}
