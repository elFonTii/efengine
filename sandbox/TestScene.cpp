#include "TestScene.h"

#include "EditorUI.h"

#include <efengine/math/Transform.h>
#include <efengine/renderer/BoxMesh.h>
#include <efengine/renderer/Material.h>
#include <efengine/renderer/MaterialDef.h>
#include <efengine/renderer/Model.h>
#include <efengine/resources/MaterialBuilder.h>
#include <efengine/resources/SceneAssets.h>
#include <efengine/scene/Node.h>
#include <efengine/scene/SceneGraph.h>
#include <efengine/serialization/MeshGeneratorRegistry.h>
#include <efengine/serialization/SceneRegistry.h>
#include <efengine/core/Log.h>

#include <glm/glm.hpp>

#include <memory>
#include <optional>
#include <utility>
#include <vector>

namespace sandbox {

using namespace efengine;

namespace {

    // Material plano: sin texturas. uMapMask de pbr.frag ya soporta slots vacios,
    // y un checker solo agrega ruido visual cuando lo que se quiere juzgar es la GI.
    u32 materialPlano(EditorContext& ctx, const char* nombre,
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
            EF_LOG_ERROR("TestScene: no se pudo construir el material '%s'", nombre);
            return resources::SceneAssets::kInvalidIndex;
        }
        return ctx.assets.AddMaterial(std::move(def), std::move(*mat));
    }

    // Genera la caja, la registra como malla generada de la escena (asi sobrevive
    // un round-trip por .efe) y devuelve el puntero estable que guarda el nodo.
    const renderer::Model* agregarCaja(EditorContext& ctx, const renderer::BoxParams& p) {
        std::vector<u8> payload;
        std::unique_ptr<renderer::Model> model =
            serialization::CreateGenerated(ctx.registry.meshes, "sandbox.box", p, payload);
        if (model == nullptr) {
            EF_LOG_ERROR("TestScene: el generador 'sandbox.box' no esta registrado o fallo");
            return null;
        }
        const u32 idx = ctx.assets.AddGenerated("sandbox.box", std::move(payload), std::move(model));
        return ctx.assets.GeneratedAt(idx);
    }

    scene::NodeHandle nodoConMalla(EditorContext& ctx, const char* nombre,
                                   const renderer::Model* model,
                                   const math::Transform& t,
                                   renderer::MaterialMap materiales) {
        const scene::NodeHandle h = ctx.scene.CreateChild(ctx.scene.Root(), nombre);
        ctx.scene.SetLocalTransform(h, t);
        ctx.scene.AttachMesh(h, scene::MeshAttachment{ model, std::move(materiales) });
        return h;
    }

} // namespace

void BuildCornellScene(EditorContext& ctx) {
    ctx.scene.Clear();
    ctx.assets.Clear();

    // El IBL casi apagado, y va ACA y no en el default de SceneGraph a proposito.
    //
    // El entorno que carga Application es un cielo abierto (citrus_orchard). En
    // una caja cerrada eso es un termino ambiente que no sabe que las paredes
    // existen: llena la sala con luz que no atravesó nada y tapa exactamente el
    // hueco que DDGI tiene que llenar. Con esto en 1.0 no se puede juzgar la GI,
    // porque el cielo la enmascara.
    //
    // No se baja el default global de SceneGraph: 1.0 es el valor neutro y
    // correcto para una escena de exterior. Esto es una decision DE ESTA ESCENA.
    ctx.scene.iblIntensity = 0.014f;

    // -- Materiales ------------------------------------------------------------
    const u32 blanco = materialPlano(ctx, "caja_blanca", glm::vec3(0.85f, 0.85f, 0.85f), 0.55f);
    const u32 rojo   = materialPlano(ctx, "caja_roja",   glm::vec3(0.70f, 0.06f, 0.06f), 0.55f);
    const u32 verde  = materialPlano(ctx, "caja_verde",  glm::vec3(0.10f, 0.55f, 0.14f), 0.55f);
    const u32 gris   = materialPlano(ctx, "caja_gris",   glm::vec3(0.45f, 0.45f, 0.45f), 0.70f);
    if (blanco == resources::SceneAssets::kInvalidIndex) return;

    const renderer::Material* mBlanco = ctx.assets.MaterialAt(blanco);
    const renderer::Material* mRojo   = ctx.assets.MaterialAt(rojo);
    const renderer::Material* mVerde  = ctx.assets.MaterialAt(verde);
    const renderer::Material* mGris   = ctx.assets.MaterialAt(gris);

    // -- La sala ---------------------------------------------------------------
    renderer::BoxParams sala;
    sala.half       = glm::vec3(4.0f, 2.0f, 4.0f);
    sala.inward     = 1u;
    sala.holeFace   = static_cast<u32>(renderer::BoxFaceIndex::ZNeg);
    // El rect de la abertura va en coords de la cara CENTRADAS en ella. La sala
    // esta en y=2, asi que una ventana de y=1.2 a y=3.2 en mundo es v de -0.8 a 1.2.
    sala.hole       = glm::vec4(-1.5f, -0.8f, 1.5f, 1.2f);
    sala.uvPerMeter = 1.0f;

    const renderer::Model* modeloSala = agregarCaja(ctx, sala);
    if (modeloSala == null) return;

    renderer::MaterialMap matSala;
    matSala["piso"]       = mBlanco;
    matSala["techo"]      = mBlanco;
    matSala["pared_xneg"] = mRojo;    // izquierda mirando desde -Z
    matSala["pared_xpos"] = mVerde;
    matSala["pared_zneg"] = mBlanco;  // la de la ventana
    matSala["pared_zpos"] = mBlanco;

    math::Transform tSala;
    tSala.position = glm::vec3(0.0f, 2.0f, 0.0f);   // piso en y=0, techo en y=4
    nodoConMalla(ctx, "sala", modeloSala, tSala, std::move(matSala));

    // -- Los dos bloques -------------------------------------------------------
    // Dan rincones para la oclusion de contacto y una sombra proyectada donde
    // mirar los bias.
    renderer::BoxParams alto;
    alto.half   = glm::vec3(0.6f, 1.2f, 0.6f);
    alto.inward = 0u;
    const renderer::Model* modeloAlto = agregarCaja(ctx, alto);

    renderer::BoxParams bajo;
    bajo.half   = glm::vec3(0.7f, 0.5f, 0.7f);
    bajo.inward = 0u;
    const renderer::Model* modeloBajo = agregarCaja(ctx, bajo);

    if (modeloAlto != null) {
        renderer::MaterialMap m;
        for (const char* n : { "pared_xneg", "pared_xpos", "piso", "techo", "pared_zneg", "pared_zpos" })
            m[n] = mGris;
        math::Transform t;
        t.position = glm::vec3(1.6f, 1.2f, 0.6f);
        t.rotation = glm::vec3(0.0f, 20.0f, 0.0f);
        nodoConMalla(ctx, "bloque_alto", modeloAlto, t, std::move(m));
    }

    if (modeloBajo != null) {
        renderer::MaterialMap m;
        for (const char* n : { "pared_xneg", "pared_xpos", "piso", "techo", "pared_zneg", "pared_zpos" })
            m[n] = mGris;
        math::Transform t;
        t.position = glm::vec3(-1.5f, 0.5f, -1.2f);
        t.rotation = glm::vec3(0.0f, -15.0f, 0.0f);
        nodoConMalla(ctx, "bloque_bajo", modeloBajo, t, std::move(m));
    }

    // -- El sol ----------------------------------------------------------------
    // El nombre importa: RefreshHandles busca "directional_light" para el toggle
    // de animacion y para el gizmo.
    //
    // Sin behavior de rotacion a proposito: RotarSolY vive en el namespace
    // anonimo de main.cpp y, para tunear, un sol quieto es mejor que uno que se
    // mueve. Se gira desde "Rotacion local" en el inspector.
    const scene::NodeHandle sol = ctx.scene.CreateChild(ctx.scene.Root(), "directional_light");
    math::Transform tSol;
    tSol.rotation = math::EulerFromForward(glm::normalize(glm::vec3(0.30f, -0.62f, 0.72f)));
    ctx.scene.SetLocalTransform(sol, tSol);
    ctx.scene.AttachLight(sol, scene::LightAttachment{ scene::LightKind::Directional,
                                                       glm::vec3(3.0f, 3.0f, 3.0f) });
    ctx.scene.SetPrimarySun(sol);

    RefreshHandles(ctx);
    EF_LOG_INFO("TestScene: caja de Cornell armada (interior 8 x 4 x 8 m, piso en y=0)");
}

} // namespace sandbox
