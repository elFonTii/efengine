#include <doctest/doctest.h>
#include <efengine/scene/SceneGraph.h>
#include <efengine/scene/Behavior.h>
#include <efengine/scene/Node.h>
#include <memory>

using namespace efengine;

namespace {
    // Cuenta las llamadas de cada canal por separado: asi se ve que Update y
    // FixedUpdate no se pisan.
    struct ContadorDobleCanal : scene::Behavior {
        int updates      = 0;
        int fixedUpdates = 0;
        f32 lastFixedDt  = -1.0f;

        void OnUpdate(scene::UpdateContext&) override { ++updates; }

        void OnFixedUpdate(scene::FixedUpdateContext& ctx) override {
            ++fixedUpdates;
            lastFixedDt = ctx.dt;
        }
    };

    // Solo implementa el canal variable: el default vacio de OnFixedUpdate tiene
    // que dejarlo pasar sin romper nada.
    struct SoloVariable : scene::Behavior {
        int updates = 0;
        void OnUpdate(scene::UpdateContext&) override { ++updates; }
    };

    // Mueve el nodo desde el canal fijo: prueba que SetLocal tambien funciona ahi.
    struct AvanzarEnX : scene::Behavior {
        f32 unitsPerSec = 0.0f;

        AvanzarEnX() = default;
        explicit AvanzarEnX(f32 speed) : unitsPerSec(speed) {}

        void OnUpdate(scene::UpdateContext&) override {}

        void OnFixedUpdate(scene::FixedUpdateContext& ctx) override {
            math::Transform t = ctx.node.local;
            t.position.x += ctx.dt * unitsPerSec;
            ctx.SetLocal(t);
        }
    };
}

TEST_CASE("SceneGraph::FixedUpdate llama OnFixedUpdate con el dt fijo") {
    scene::SceneGraph scene;
    scene::NodeHandle n = scene.CreateChild(scene.Root(), "n");
    auto* b = static_cast<ContadorDobleCanal*>(
        scene.AttachBehavior(n, std::make_unique<ContadorDobleCanal>()));

    scene.FixedUpdate(1.0f / 60.0f);

    CHECK(b->fixedUpdates == 1);
    CHECK(b->lastFixedDt == doctest::Approx(1.0f / 60.0f));
}

TEST_CASE("Update y FixedUpdate son canales independientes") {
    scene::SceneGraph scene;
    scene::NodeHandle n = scene.CreateChild(scene.Root(), "n");
    auto* b = static_cast<ContadorDobleCanal*>(
        scene.AttachBehavior(n, std::make_unique<ContadorDobleCanal>()));

    scene.FixedUpdate(0.016f);
    scene.FixedUpdate(0.016f);
    scene.Update(0.033f);

    CHECK(b->fixedUpdates == 2);
    CHECK(b->updates      == 1);
}

TEST_CASE("Un behavior que no implementa OnFixedUpdate sobrevive al canal fijo") {
    scene::SceneGraph scene;
    scene::NodeHandle n = scene.CreateChild(scene.Root(), "n");
    auto* b = static_cast<SoloVariable*>(
        scene.AttachBehavior(n, std::make_unique<SoloVariable>()));

    scene.FixedUpdate(0.016f);   // no debe romper nada
    scene.Update(0.016f);

    CHECK(b->updates == 1);
}

TEST_CASE("FixedUpdate respeta el flag enabled") {
    scene::SceneGraph scene;
    scene::NodeHandle n = scene.CreateChild(scene.Root(), "n");
    auto* b = static_cast<ContadorDobleCanal*>(
        scene.AttachBehavior(n, std::make_unique<ContadorDobleCanal>()));
    b->enabled = false;

    scene.FixedUpdate(0.016f);

    CHECK(b->fixedUpdates == 0);
}

TEST_CASE("SetLocal desde OnFixedUpdate ensucia el world del nodo") {
    scene::SceneGraph scene;
    scene::NodeHandle n = scene.CreateChild(scene.Root(), "n");
    scene.AttachBehavior(n, std::make_unique<AvanzarEnX>(60.0f));  // 60 u/s

    scene.FixedUpdate(1.0f / 60.0f);   // un paso => 1 unidad
    scene.UpdateWorldTransforms();

    CHECK(scene.Get(n).local.position.x == doctest::Approx(1.0f));
    CHECK(scene.Get(n).worldMatrix[3][0] == doctest::Approx(1.0f));
}
