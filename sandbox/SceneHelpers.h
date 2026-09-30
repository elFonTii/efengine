#pragma once
#include <efengine/core/Types.h>
#include <efengine/math/Transform.h>
#include <efengine/renderer/BoxMesh.h>
#include <efengine/renderer/Material.h>
#include <efengine/scene/NodeHandle.h>

#include <glm/glm.hpp>

namespace efengine { namespace renderer { class Model; } }

namespace sandbox {

    struct EditorContext;

    // Material plano sin texturas. Devuelve el indice en SceneAssets, o
    // SceneAssets::kInvalidIndex si no se pudo construir.
    u32 MaterialPlano(EditorContext& ctx, const char* nombre, const glm::vec3& albedo, f32 roughness);

    // Genera la caja, la registra como malla generada de la escena (sobrevive un
    // round-trip por .efe) y devuelve el puntero estable que guarda el nodo.
    const efengine::renderer::Model* AgregarCaja(EditorContext& ctx, const efengine::renderer::BoxParams& p);

    efengine::scene::NodeHandle NodoConMalla(EditorContext& ctx, const char* nombre,
                                             const efengine::renderer::Model* model,
                                             const efengine::math::Transform& t,
                                             efengine::renderer::MaterialMap materiales);

    // Las 6 submallas de una caja de AgregarCaja con el mismo material.
    efengine::renderer::MaterialMap MaterialEnTodasLasCaras(const efengine::renderer::Material* m);

}
