#include "LightStressScene.h"

#include "EditorUI.h"
#include "SceneHelpers.h"

#include <efengine/core/Log.h>
#include <efengine/math/Transform.h>
#include <efengine/renderer/BoxMesh.h>
#include <efengine/resources/SceneAssets.h>
#include <efengine/scene/Behavior.h>
#include <efengine/scene/Node.h>
#include <efengine/scene/SceneGraph.h>

#include <glm/glm.hpp>

#include <cmath>
#include <memory>
#include <random>
#include <string>

namespace sandbox {

using namespace efengine;

namespace {

    // Gira el pivote: todas las luces se mueven sin un behavior por luz.
    class GirarPivote : public scene::Behavior {
        public:
            void OnUpdate(scene::UpdateContext& ctx) override {
                math::Transform t = ctx.node.local;
                t.rotation.y += ctx.dt * 6.0f;
                ctx.SetLocal(t);
            }
    };

    // Tono HSV saturado a RGB.
    glm::vec3 tono(f32 h) {
        const glm::vec3 k = glm::abs(glm::fract(glm::vec3(h) + glm::vec3(1.0f, 2.0f / 3.0f, 1.0f / 3.0f)) * 6.0f - 3.0f);
        return glm::clamp(k - 1.0f, 0.0f, 1.0f);
    }

    void direccional(EditorContext& ctx, const char* nombre, const glm::vec3& hacia,
                     const glm::vec3& color, f32 intensidad, bool primario) {
        const scene::NodeHandle h = ctx.scene.CreateChild(ctx.scene.Root(), nombre);
        math::Transform t;
        t.rotation = math::EulerFromForward(glm::normalize(hacia));
        ctx.scene.SetLocalTransform(h, t);
        scene::LightAttachment a;
        a.kind      = scene::LightKind::Directional;
        a.color     = color;
        a.intensity = intensidad;
        ctx.scene.AttachLight(h, a);
        if (primario) ctx.scene.SetPrimarySun(h);
    }

} // namespace

void BuildLightStressScene(EditorContext& ctx, const LightStressDesc& desc) {
    EndSimulationBeforeClear(ctx);
    ctx.scene.Clear();
    ctx.assets.Clear();
    ctx.scene.iblIntensity = 0.05f;   // de noche: que manden las luces

    const u32 iPiso    = MaterialPlano(ctx, "estres_piso",    glm::vec3(0.6f), 0.6f);
    const u32 iColumna = MaterialPlano(ctx, "estres_columna", glm::vec3(0.8f), 0.35f);
    if (iPiso == resources::SceneAssets::kInvalidIndex || iColumna == resources::SceneAssets::kInvalidIndex) return;

    const f32 lado = desc.size;

    renderer::BoxParams piso;
    piso.half       = glm::vec3(lado * 0.5f, 0.1f, lado * 0.5f);
    piso.inward     = 0u;
    piso.uvPerMeter = 0.5f;
    renderer::BoxParams columna;
    columna.half   = glm::vec3(0.5f, 3.0f, 0.5f);
    columna.inward = 0u;

    const renderer::Model* modeloPiso    = AgregarCaja(ctx, piso);
    const renderer::Model* modeloColumna = AgregarCaja(ctx, columna);
    if (modeloPiso == null || modeloColumna == null) return;

    math::Transform tPiso;
    tPiso.position = glm::vec3(0.0f, -0.1f, 0.0f);
    NodoConMalla(ctx, "piso", modeloPiso, tPiso, MaterialEnTodasLasCaras(ctx.assets.MaterialAt(iPiso)));

    // Columnas cada 20 m en las dos variantes: la densidad de geometria no cambia
    // entre la escena de 100 m y la de 200 m.
    const i32 mitad = static_cast<i32>(lado / 40.0f);
    for (i32 z = -mitad; z <= mitad; ++z) {
        for (i32 x = -mitad; x <= mitad; ++x) {
            math::Transform t;
            t.position = glm::vec3(static_cast<f32>(x) * 20.0f, 3.0f, static_cast<f32>(z) * 20.0f);
            const std::string nombre = "columna_" + std::to_string(x) + "_" + std::to_string(z);
            NodoConMalla(ctx, nombre.c_str(), modeloColumna, t, MaterialEnTodasLasCaras(ctx.assets.MaterialAt(iColumna)));
        }
    }

    // "luz_animada" es el nombre que RefreshHandles busca para el toggle "Animate".
    const scene::NodeHandle pivote = ctx.scene.CreateChild(ctx.scene.Root(), "luz_animada");
    std::unique_ptr<scene::Behavior> giro = std::make_unique<GirarPivote>();
    giro->enabled = false;
    ctx.scene.AttachBehavior(pivote, std::move(giro));

    std::mt19937 rng(1234u);
    std::uniform_real_distribution<f32> u01(0.0f, 1.0f);
    for (u32 i = 0u; i < desc.lights; ++i) {
        const bool esSpot = u01(rng) < 0.3f;
        scene::LightAttachment a;
        a.kind         = esSpot ? scene::LightKind::Spot : scene::LightKind::Point;
        a.color        = tono(u01(rng));
        a.range        = 3.0f + 7.0f * u01(rng);
        a.intensity    = 0.5f * a.range * a.range;   // a mitad de rango todas aportan parecido
        a.innerConeDeg = 20.0f;
        a.outerConeDeg = 35.0f;

        math::Transform t;
        t.position = glm::vec3((u01(rng) - 0.5f) * lado, 1.0f + 3.0f * u01(rng), (u01(rng) - 0.5f) * lado);
        if (esSpot) t.rotation = glm::vec3(-90.0f, 0.0f, 0.0f);   // el -Z del nodo mira abajo

        const scene::NodeHandle h = ctx.scene.CreateChild(pivote, "luz_" + std::to_string(i));
        ctx.scene.SetLocalTransform(h, t);
        ctx.scene.AttachLight(h, a);
    }

    direccional(ctx, "directional_light", glm::vec3( 0.3f, -0.8f,  0.5f), glm::vec3(0.6f, 0.7f, 1.0f), 0.2f,  true);
    direccional(ctx, "relleno_calido",    glm::vec3(-0.6f, -0.4f,  0.2f), glm::vec3(1.0f, 0.6f, 0.3f), 0.15f, false);
    direccional(ctx, "relleno_frio",      glm::vec3( 0.5f, -0.3f, -0.6f), glm::vec3(0.3f, 0.5f, 1.0f), 0.15f, false);

    RefreshHandles(ctx);
    EF_LOG_INFO("LightStressScene: %u luces sobre %.0f x %.0f m", desc.lights, lado, lado);
}

} // namespace sandbox
