#include "SceneHelpers.h"

#include "EditorUI.h"

#include <efengine/core/Log.h>
#include <efengine/renderer/MaterialDef.h>
#include <efengine/renderer/Model.h>
#include <efengine/resources/MaterialBuilder.h>
#include <efengine/resources/SceneAssets.h>
#include <efengine/scene/Node.h>
#include <efengine/scene/SceneGraph.h>
#include <efengine/serialization/MeshGeneratorRegistry.h>
#include <efengine/serialization/SceneRegistry.h>

#include <memory>
#include <optional>
#include <utility>
#include <vector>

namespace sandbox {

using namespace efengine;

// Material plano: sin texturas. uMapMask de pbr.frag ya soporta slots vacios,
// y un checker solo agrega ruido visual cuando lo que se quiere juzgar es la GI.
u32 MaterialPlano(EditorContext& ctx, const char* nombre,
                  const glm::vec3& albedo, f32 roughness) {
    renderer::MaterialDef def;
    def.name       = nombre;
    def.shaderName = "pbr";
    def.vertPath   = "assets/shaders/pbr.vert";
    def.fragPath   = "assets/shaders/pbr.frag";
    def.albedoTint = albedo;
    def.metallic   = 0.0f;
    def.roughness  = roughness;
    def.aoStrength = 0.0f;   // sin mapa de AO, que no oscurezca de gratis

    std::optional<renderer::Material> mat = resources::BuildMaterial(def, ctx.rm);
    if (!mat) {
        EF_LOG_ERROR("SceneHelpers: no se pudo construir el material '%s'", nombre);
        return resources::SceneAssets::kInvalidIndex;
    }
    return ctx.assets.AddMaterial(std::move(def), std::move(*mat));
}

// Genera la caja, la registra como malla generada de la escena (asi sobrevive
// un round-trip por .efe) y devuelve el puntero estable que guarda el nodo.
const renderer::Model* AgregarCaja(EditorContext& ctx, const renderer::BoxParams& p) {
    std::vector<u8> payload;
    std::unique_ptr<renderer::Model> model =
        serialization::CreateGenerated(ctx.registry.meshes, "sandbox.box", p, payload);
    if (model == nullptr) {
        EF_LOG_ERROR("SceneHelpers: el generador 'sandbox.box' no esta registrado o fallo");
        return null;
    }
    const u32 idx = ctx.assets.AddGenerated("sandbox.box", std::move(payload), std::move(model));
    return ctx.assets.GeneratedAt(idx);
}

renderer::MaterialMap MaterialEnTodasLasCaras(const renderer::Material* m) {
    renderer::MaterialMap mm;
    for (const char* n : { "pared_xneg", "pared_xpos", "piso", "techo", "pared_zneg", "pared_zpos" }) mm[n] = m;
    return mm;
}

scene::NodeHandle NodoConMalla(EditorContext& ctx, const char* nombre,
                               const renderer::Model* model,
                               const math::Transform& t,
                               renderer::MaterialMap materiales) {
    const scene::NodeHandle h = ctx.scene.CreateChild(ctx.scene.Root(), nombre);
    ctx.scene.SetLocalTransform(h, t);
    ctx.scene.AttachMesh(h, scene::MeshAttachment{ model, std::move(materiales) });
    return h;
}

} // namespace sandbox
