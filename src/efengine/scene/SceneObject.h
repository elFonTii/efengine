#pragma once
#include <efengine/math/Transform.h>
#include <efengine/renderer/Model.h>
#include <efengine/renderer/Material.h>
#include <unordered_map>


namespace efengine {
namespace scene {

    struct SceneObject {
        SceneObject(const renderer::Model* model,
            renderer::MaterialMap  materials,
            math::Transform transform = {}
        );

        math::Transform transform;
        const renderer::Model* model;
        renderer::MaterialMap  materials;
    };
}
}