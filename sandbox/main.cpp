#include "EditorUI.h"
#include "TestScene.h"

#include <efengine/application/Application.h>
#include <efengine/renderer/Model.h>
#include <efengine/renderer/Mesh.h>
#include <efengine/renderer/BoxMesh.h>
#include <efengine/renderer/Vertex.h>
#include <efengine/resources/SceneAssets.h>
#include <efengine/serialization/SceneSerializer.h>
#include <efengine/serialization/SceneRegistry.h>
#include <efengine/serialization/BinaryReader.h>
#include <efengine/scene/Camera.h>
#include <efengine/scene/CameraController.h>
#include <efengine/scene/SceneGraph.h>
#include <efengine/scene/Behavior.h>
#include <efengine/math/Transform.h>
#include <efengine/core/Types.h>
#include <efengine/core/Log.h>

#include <glm/glm.hpp>

#include <cmath>
#include <memory>
#include <vector>


namespace {
    using namespace efengine;
    
    // Params del generador de plano. Es lo que viaja en el .efe como genPayload.
    struct PlaneParams {
        f32 halfSize = 300.0f;
        f32 tiles    = 24.0f;

        template <class Ar>
        void Serialize(Ar& ar) {
            ar.Field(halfSize);
            ar.Field(tiles);
        }
    };

    renderer::Model makePlane(f32 halfSize, f32 tiles) {
        const std::vector<renderer::Vertex> vertices = {
            // position                            normal         uv                tangent
            { {-halfSize, 0.0f, -halfSize}, {0.0f, 1.0f, 0.0f}, {0.0f,  0.0f},  {1.0f, 0.0f, 0.0f} },
            { { halfSize, 0.0f, -halfSize}, {0.0f, 1.0f, 0.0f}, {tiles, 0.0f},  {1.0f, 0.0f, 0.0f} },
            { { halfSize, 0.0f,  halfSize}, {0.0f, 1.0f, 0.0f}, {tiles, tiles}, {1.0f, 0.0f, 0.0f} },
            { {-halfSize, 0.0f,  halfSize}, {0.0f, 1.0f, 0.0f}, {0.0f,  tiles}, {1.0f, 0.0f, 0.0f} },
        };
        const std::vector<u32> indices = { 0, 3, 2, 2, 1, 0 }; // CCW visto desde +Y: winding concuerda con la normal (0,1,0)

        std::vector<renderer::Mesh> meshes;
        meshes.emplace_back(vertices, indices, "ground");
        return renderer::Model(std::move(meshes));
    }

    std::unique_ptr<renderer::Model> generarPlano(serialization::BinaryReader& r) {
        PlaneParams p;
        p.Serialize(r);
        if (!r.Ok()) return nullptr;
        return std::make_unique<renderer::Model>(makePlane(p.halfSize, p.tiles));
    }

    // La sala de Cornell y sus bloques son cajas generadas: TestScene las pide
    // por este nombre. Los params ya viven en renderer::BoxParams, asi que aca
    // no hay struct propio como en el plano.
    std::unique_ptr<renderer::Model> generarCaja(serialization::BinaryReader& r) {
        renderer::BoxParams p;
        p.Serialize(r);
        if (!r.Ok()) return nullptr;
        return std::make_unique<renderer::Model>(renderer::MakeBoxModel(p));
    }

    // BEHAVIORS
    class RotarY : public scene::Behavior {
        public:
            RotarY() = default;
            explicit RotarY(f32 degPerSec) : m_degPerSec(degPerSec) {}

            void OnUpdate(scene::UpdateContext& ctx) override {
                math::Transform t = ctx.node.local;
                t.rotation.y += ctx.dt * m_degPerSec;
                ctx.SetLocal(t);
            }

            template <class Ar>
            void Serialize(Ar& ar) { ar.Field(m_degPerSec); }

        private:
            f32 m_degPerSec = 0.0f;
    };

    class OrbitarXZ : public scene::Behavior {
        public:
            OrbitarXZ() = default;
            OrbitarXZ(glm::vec3 center, f32 radius, f32 speed)
                : m_center(center), m_radius(radius), m_speed(speed) {}

            void OnUpdate(scene::UpdateContext& ctx) override {
                m_angle += ctx.dt * m_speed;
                math::Transform t = ctx.node.local;
                t.position = m_center + m_radius * glm::vec3(std::cos(m_angle), 0.0f, std::sin(m_angle));
                ctx.SetLocal(t);
            }

            template <class Ar>
            void Serialize(Ar& ar) {
                ar.Field(m_center);
                ar.Field(m_radius);
                ar.Field(m_speed);
                ar.Field(m_angle);   // se guarda: la orbita reanuda donde estaba
            }

        private:
            glm::vec3 m_center { 0.0f };
            f32 m_radius = 0.0f;
            f32 m_speed  = 0.0f;
            f32 m_angle  = 0.0f;
    };

    class RotarSolY : public scene::Behavior {
        public:
            RotarSolY() = default;
            explicit RotarSolY(f32 degPerSec) : m_degPerSec(degPerSec) {}

            void OnUpdate(scene::UpdateContext& ctx) override {
                m_angle += ctx.dt * m_degPerSec;
                math::Transform t = ctx.node.local;
                t.rotation.y = m_angle;
                ctx.SetLocal(t);
            }

            template <class Ar>
            void Serialize(Ar& ar) {
                ar.Field(m_degPerSec);
                ar.Field(m_angle);
            }

        private:
            f32 m_degPerSec = 0.0f;
            f32 m_angle     = 0.0f;
    };
}

int main() {
    using namespace efengine;

    EF_LOG_INFO("=== efengine: sandbox street rat ===");

    application::Application app;
    app.SetClearColor(0.18f, 0.18f, 0.18f);
    resources::ResourceManager& rm = app.GetResources();

    serialization::SceneRegistry registry;
    registry.behaviors.Register<RotarY>("RotarY");
    registry.behaviors.Register<OrbitarXZ>("OrbitarXZ");
    registry.behaviors.Register<RotarSolY>("RotarSolY");
    registry.meshes.Register("sandbox.plane", &generarPlano);
    registry.meshes.Register("sandbox.box",   &generarCaja);

    resources::SceneAssets assets; // Dueño de los materiales y de las mallas generadas de la escena.
    scene::SceneGraph scene;

    scene::Camera cam;
    cam.SetAspect(app.GetWindow().GetAspectRatio());
    scene::CameraController controller(&cam);
    // El listener de la ventana ahora es el Input de Application; el
    // controller solo lo consume.

    // El estado de la UI vive aca (el loop es el dueno); el editor solo lo usa.
    sandbox::EditorState editorState;
    sandbox::EditorContext editor { app, scene, cam, controller, assets, rm, registry, editorState };
    // La escena de arranque es la sala de Cornell, no el .efe: la sala de 200 m
    // de sandbox.efe no sirve para validar iluminacion indirecta. Cargar el .efe
    // sigue disponible desde el menu "Escena". Volver al arranque viejo es
    // cambiar esta linea por el SceneSerializer::Load de siempre.
    sandbox::BuildCornellScene(editor);   // ya llama a RefreshHandles}
    //sandbox::RefreshHandles(editor);

    while (app.Running()) {
        app.BeginFrame();
        // Esc sale primero del mouselook: cerrar el sandbox de un tecleo
        // mientras el cursor esta capturado es demasiado facil.
        if (app.IsKeyPressed(platform::Key::Escape)) {
            if (controller.LookToggled()) controller.SetLookToggled(false);
            else                          app.Close();
        }

        const platform::Input& in = app.GetInput();

        controller.SetMouseEnabled(!app.GetDebugUI().WantsMouse());
        controller.SetKeyboardEnabled(!app.GetDebugUI().WantsKeyboard());

        sandbox::DrawEditor(editor);

        // Despues de DrawEditor para que use la seleccion de ESTE frame.
        if (in.WasPressed(platform::Key::F)) sandbox::FocusSelection(editor);

        controller.Update(in, app.DeltaTime());
        app.GetWindow().SetCursorCaptured(controller.WantsCursorCaptured());
        cam.SetAspect(app.GetWindow().GetAspectRatio());

        scene.Update(app.DeltaTime());

        app.RenderScene(scene, cam);
        app.EndFrame();
    }

    EF_LOG_INFO("=== efengine: sandbox shutdown ===");
    return 0;
}
