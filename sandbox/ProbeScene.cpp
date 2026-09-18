#include "ProbeScene.h"

#include "EditorUI.h"

#include <efengine/math/Transform.h>
#include <efengine/renderer/Bounds.h>
#include <efengine/renderer/Material.h>
#include <efengine/renderer/MaterialDef.h>
#include <efengine/renderer/Mesh.h>
#include <efengine/renderer/Model.h>
#include <efengine/resources/MaterialBuilder.h>
#include <efengine/resources/ResourceManager.h>
#include <efengine/resources/SceneAssets.h>
#include <efengine/scene/Node.h>
#include <efengine/scene/SceneGraph.h>
#include <efengine/core/Log.h>

#include <glm/glm.hpp>

#include <algorithm>
#include <chrono>
#include <optional>
#include <utility>
#include <vector>

namespace sandbox {

using namespace efengine;

namespace {

    u32 materialGris(EditorContext& ctx) {
        renderer::MaterialDef def;
        def.name       = "probe_gris";
        def.shaderName = "pbr";
        def.vertPath   = "assets/shaders/pbr.vert";
        def.fragPath   = "assets/shaders/pbr.frag";
        def.albedoTint = glm::vec3(0.6f);
        def.metallic   = 0.0f;
        def.roughness  = 0.7f;
        def.aoStrength = 0.0f;
        // Bistro esta lleno de mallas de una sola cara (cortinas, hojas, telas)
        // cuyo winding no sobrevive al culling. Sin esto, media escena parpadea.
        def.doubleSided = true;

        std::optional<renderer::Material> mat = resources::BuildMaterial(def, ctx.rm);
        if (!mat) {
            EF_LOG_ERROR("ProbeScene: no se pudo construir el material gris");
            return resources::SceneAssets::kInvalidIndex;
        }
        return ctx.assets.AddMaterial(std::move(def), std::move(*mat));
    }

    // Todo lo que se quiere saber del modelo, recorrido una sola vez.
    struct Reporte {
        u64 submallas  = 0;
        u64 triangulos = 0;
        u64 vertices   = 0;
        std::vector<std::string> materiales;
    };

    Reporte medir(const renderer::Model& model) {
        Reporte r;
        r.submallas = model.meshes().size();
        for (const renderer::Mesh& m : model.meshes()) {
            r.triangulos += m.indices().size() / 3u;
            r.vertices   += m.positions().size();
            if (std::find(r.materiales.begin(), r.materiales.end(), m.materialName())
                == r.materiales.end()) {
                r.materiales.push_back(m.materialName());
            }
        }
        return r;
    }

} // namespace

void BuildProbeScene(EditorContext& ctx, const char* fbxPath) {
    ctx.scene.Clear();
    ctx.assets.Clear();

    const u32 gris = materialGris(ctx);
    if (gris == resources::SceneAssets::kInvalidIndex) return;

    EF_LOG_INFO("ProbeScene: cargando '%s'...", fbxPath);

    const auto t0 = std::chrono::steady_clock::now();
    renderer::Model* modelo = ctx.rm.GetModel(fbxPath);
    const auto t1 = std::chrono::steady_clock::now();
    const f64 ms = std::chrono::duration<f64, std::milli>(t1 - t0).count();

    if (modelo == null) {
        // El caso que la sonda existe para detectar sin adivinar: assimp no
        // pudo, o no habia memoria. El motivo ya lo logueo ModelLoader.
        EF_LOG_ERROR("ProbeScene: '%s' NO se pudo cargar (%.0f ms hasta el fallo)", fbxPath, ms);
        return;
    }

    const Reporte r = medir(*modelo);
    const renderer::AABB& box = modelo->bounds();

    EF_LOG_INFO("ProbeScene: cargado en %.0f ms", ms);
    EF_LOG_INFO("ProbeScene: %llu submallas | %llu triangulos | %llu vertices",
                r.submallas, r.triangulos, r.vertices);
    EF_LOG_INFO("ProbeScene: %llu materiales distintos", (u64)r.materiales.size());
    if (box.Valid()) {
        const glm::vec3 e = box.Extents();
        EF_LOG_INFO("ProbeScene: AABB min(%.2f %.2f %.2f) max(%.2f %.2f %.2f) tamano(%.1f %.1f %.1f) m",
                    box.min.x, box.min.y, box.min.z,
                    box.max.x, box.max.y, box.max.z,
                    e.x * 2.0f, e.y * 2.0f, e.z * 2.0f);
    } else {
        EF_LOG_WARNING("ProbeScene: AABB invalida (el modelo no tiene vertices?)");
    }

    // La lista completa de materiales: es el trabajo que despues habria que
    // hacer para texturizar, y conviene verlo antes de prometerlo.
    for (const std::string& n : r.materiales) {
        EF_LOG_INFO("ProbeScene:   material '%s'", n.c_str());
    }

    const scene::NodeHandle h = ctx.scene.CreateChild(ctx.scene.Root(), "probe");
    ctx.scene.SetLocalTransform(h, math::Transform{});
    ctx.scene.AttachMesh(h, scene::MeshAttachment{
        modelo,
        renderer::MakeUniformMaterialMap(r.materiales, ctx.assets.MaterialAt(gris))
    });

    // El nombre importa: RefreshHandles busca "directional_light".
    const scene::NodeHandle sol = ctx.scene.CreateChild(ctx.scene.Root(), "directional_light");
    math::Transform tSol;
    tSol.rotation = math::EulerFromForward(glm::normalize(glm::vec3(0.30f, -0.62f, 0.72f)));
    ctx.scene.SetLocalTransform(sol, tSol);
    ctx.scene.AttachLight(sol, scene::LightAttachment{ scene::LightKind::Directional,
                                                       glm::vec3(3.0f, 3.0f, 3.0f) });
    ctx.scene.SetPrimarySun(sol);

    ctx.state.selected = h;   // F encuadra el modelo sin tener que buscarlo
    RefreshHandles(ctx);
}

} // namespace sandbox
