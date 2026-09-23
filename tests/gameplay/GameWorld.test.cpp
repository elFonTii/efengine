#include <doctest/doctest.h>
#include <efengine/core/Time.h>
#include <efengine/gameplay/GameWorld.h>
#include <efengine/scene/Behavior.h>
#include <efengine/scene/SceneGraph.h>

#include <cmath>
#include <memory>

using namespace efengine;

namespace {
    // Timestamps sinteticos, igual que los tests de Time: el reloj no se lee del
    // sistema. Cada caso tiene el suyo, con su propio Time adentro.
    struct Reloj {
        core::Time time;
        f64        ahora = 100.0;

        Reloj() { time.Advance(ahora); }   // el primer Advance no acumula nada

        void Frame(f64 segundos) {
            ahora += segundos;
            time.Advance(ahora);
        }
    };

    constexpr f64 kPasoFijo = 1.0 / 60.0;

    class ContarFixed : public scene::Behavior {
        public:
            i32 fixed = 0;
            i32 vari  = 0;

            void OnUpdate(scene::UpdateContext&) override { ++vari; }
            void OnFixedUpdate(scene::FixedUpdateContext&) override { ++fixed; }
    };

    scene::NodeHandle esferaEnElAire(scene::SceneGraph& g) {
        const scene::NodeHandle h = g.CreateChild(g.Root(), "esfera");

        math::Transform t;
        t.position = glm::vec3(0.0f, 5.0f, 0.0f);
        g.SetLocalTransform(h, t);

        scene::ColliderAttachment col;
        col.kind   = scene::ShapeKind::Sphere;
        col.params = glm::vec3(0.5f, 0.0f, 0.0f);
        col.motion = scene::MotionType::Dynamic;
        g.AttachCollider(h, col);
        return h;
    }
}

TEST_CASE("GameWorld: Create devuelve un mundo que arranca sin simular") {
    scene::SceneGraph g;
    const std::unique_ptr<gameplay::GameWorld> gw = gameplay::GameWorld::Create(g);

    REQUIRE(gw != nullptr);
    CHECK_FALSE(gw->Simulating());
    CHECK(gw->Binding().BoundCount() == 0u);
}

TEST_CASE("GameWorld: BeginSimulation arma los cuerpos y EndSimulation devuelve los locals exactos") {
    scene::SceneGraph g;
    const std::unique_ptr<gameplay::GameWorld> gw = gameplay::GameWorld::Create(g);
    REQUIRE(gw != nullptr);

    const scene::NodeHandle esfera = esferaEnElAire(g);
    const math::Transform antes = g.Get(esfera).local;

    gw->BeginSimulation();
    CHECK(gw->Simulating());
    CHECK(gw->Binding().BoundCount() == 1u);

    Reloj r;
    for (i32 i = 0; i < 30; ++i) { r.Frame(kPasoFijo); gw->Tick(r.time); }
    CHECK(g.Get(esfera).local.position.y < antes.position.y);   // se movio de verdad

    gw->EndSimulation();

    CHECK_FALSE(gw->Simulating());
    CHECK(gw->Binding().BoundCount() == 0u);
    CHECK(g.Get(esfera).local.position.x == doctest::Approx(antes.position.x));
    CHECK(g.Get(esfera).local.position.y == doctest::Approx(antes.position.y));
    CHECK(g.Get(esfera).local.position.z == doctest::Approx(antes.position.z));
}

TEST_CASE("GameWorld: BeginSimulation dos veces es no-op") {
    scene::SceneGraph g;
    const std::unique_ptr<gameplay::GameWorld> gw = gameplay::GameWorld::Create(g);
    REQUIRE(gw != nullptr);
    esferaEnElAire(g);

    gw->BeginSimulation();
    gw->BeginSimulation();

    CHECK(gw->Binding().BoundCount() == 1u);
}

TEST_CASE("GameWorld: EndSimulation sin simular es no-op") {
    scene::SceneGraph g;
    const std::unique_ptr<gameplay::GameWorld> gw = gameplay::GameWorld::Create(g);
    REQUIRE(gw != nullptr);

    gw->EndSimulation();
    CHECK_FALSE(gw->Simulating());
}

TEST_CASE("GameWorld: Tick sin simular corre los behaviors y no toca los transforms") {
    scene::SceneGraph g;
    const std::unique_ptr<gameplay::GameWorld> gw = gameplay::GameWorld::Create(g);
    REQUIRE(gw != nullptr);

    const scene::NodeHandle esfera = esferaEnElAire(g);
    const math::Transform antes = g.Get(esfera).local;

    ContarFixed* contador = static_cast<ContarFixed*>(
        g.AttachBehavior(esfera, std::make_unique<ContarFixed>()));

    Reloj r;
    for (i32 i = 0; i < 10; ++i) { r.Frame(kPasoFijo); gw->Tick(r.time); }

    CHECK(contador->fixed == 10);
    CHECK(contador->vari  == 10);
    CHECK(g.Get(esfera).local.position.y == doctest::Approx(antes.position.y));
}

TEST_CASE("GameWorld: un frame sin pasos fijos no rompe la interpolacion") {
    scene::SceneGraph g;
    const std::unique_ptr<gameplay::GameWorld> gw = gameplay::GameWorld::Create(g);
    REQUIRE(gw != nullptr);

    const scene::NodeHandle esfera = esferaEnElAire(g);
    gw->BeginSimulation();

    // Frames de 1 ms: no llega a juntar un paso fijo de 16.6 ms, asi que
    // FixedSteps() da 0 y lo unico que corre es la interpolacion.
    Reloj r;
    for (i32 i = 0; i < 5; ++i) {
        r.Frame(0.001);
        REQUIRE(r.time.FixedSteps() == 0);
        gw->Tick(r.time);
    }

    CHECK(g.Get(esfera).local.position.y == doctest::Approx(5.0f));
}
