#include <doctest/doctest.h>
#include <efengine/scene/SceneGraph.h>
#include <efengine/scene/Behavior.h>
#include <efengine/scene/Node.h>
#include <memory>

using namespace efengine;

namespace {
    // Doble de prueba: cuenta llamadas a OnUpdate y guarda el ultimo dt visto.
    struct RecordingBehavior : scene::Behavior {
        int calls = 0;
        f32 lastDt = 0.0f;
        void OnUpdate(scene::UpdateContext& ctx) override {
            ++calls;
            lastDt = ctx.dt;
        }
    };

    // Integra una rotacion en Y usando SetLocal: prueba mutacion + dirty.
    struct SpinY : scene::Behavior {
        f32 degPerSec = 90.0f;
        void OnUpdate(scene::UpdateContext& ctx) override {
            math::Transform t = ctx.node.local;
            t.rotation.y += ctx.dt * degPerSec;
            ctx.SetLocal(t);
        }
    };

    // Mueve el nodo a x=7 una sola vez: prueba dirty -> recompute del world.
    struct MoverX : scene::Behavior {
        void OnUpdate(scene::UpdateContext& ctx) override {
            math::Transform t = ctx.node.local;
            t.position.x = 7.0f;
            ctx.SetLocal(t);
        }
    };

    // Escribe a un contador externo que sobrevive al behavior: sirve para afirmar
    // "no corrio" incluso despues de que el nodo (y su behavior) fue destruido.
    struct FlagBehavior : scene::Behavior {
        int* counter;
        explicit FlagBehavior(int* c) : counter(c) {}
        void OnUpdate(scene::UpdateContext&) override { ++(*counter); }
    };

    // Marca un flag externo en su destructor: prueba que el unique_ptr se libera
    // en el acto al destruir el nodo.
    struct DtorFlagBehavior : scene::Behavior {
        bool* destroyed;
        explicit DtorFlagBehavior(bool* d) : destroyed(d) {}
        ~DtorFlagBehavior() override { *destroyed = true; }
        void OnUpdate(scene::UpdateContext&) override {}
    };
}

TEST_CASE("Behavior: AttachBehavior adjunta al nodo y devuelve el puntero no-dueno") {
    scene::SceneGraph g;
    scene::NodeHandle h = g.CreateNode("actor");

    auto beh = std::make_unique<RecordingBehavior>();
    RecordingBehavior* p = beh.get();
    scene::Behavior* ret = g.AttachBehavior(h, std::move(beh));

    CHECK(ret == p);                          // devuelve el que acaba de insertar
    CHECK(p->enabled == true);                // enabled por defecto
    CHECK(p->calls == 0);                     // adjuntar no lo corre
    CHECK(g.Get(h).behaviors.size() == 1u);   // quedo guardado en el nodo
}

TEST_CASE("Behavior: Update corre OnUpdate una vez con el dt dado") {
    scene::SceneGraph g;
    scene::NodeHandle h = g.CreateNode("actor");
    auto beh = std::make_unique<RecordingBehavior>();
    RecordingBehavior* p = beh.get();
    g.AttachBehavior(h, std::move(beh));

    g.Update(0.016f);

    CHECK(p->calls == 1);
    CHECK(p->lastDt == doctest::Approx(0.016f));
}

TEST_CASE("Behavior: un behavior enabled=false no corre") {
    scene::SceneGraph g;
    scene::NodeHandle h = g.CreateNode("actor");
    auto beh = std::make_unique<RecordingBehavior>();
    RecordingBehavior* p = beh.get();
    p->enabled = false;
    g.AttachBehavior(h, std::move(beh));

    g.Update(0.016f);

    CHECK(p->calls == 0);
}

TEST_CASE("Behavior: varios behaviors en un nodo corren todos") {
    scene::SceneGraph g;
    scene::NodeHandle h = g.CreateNode("actor");
    auto b1 = std::make_unique<RecordingBehavior>(); RecordingBehavior* p1 = b1.get();
    auto b2 = std::make_unique<RecordingBehavior>(); RecordingBehavior* p2 = b2.get();
    g.AttachBehavior(h, std::move(b1));
    g.AttachBehavior(h, std::move(b2));

    g.Update(0.02f);

    CHECK(p1->calls == 1);
    CHECK(p2->calls == 1);
}

TEST_CASE("Behavior: SetLocal integra el transform a lo largo de varios ticks") {
    scene::SceneGraph g;
    scene::NodeHandle h = g.CreateNode("actor");
    g.AttachBehavior(h, std::make_unique<SpinY>());   // 90 grados/seg

    for (int i = 0; i < 10; ++i) g.Update(0.1f);      // 10 * 0.1s = 1s

    CHECK(g.Get(h).local.rotation.y == doctest::Approx(90.0f));
}

TEST_CASE("Behavior: SetLocal desde un behavior marca dirty y recomputa el world") {
    scene::SceneGraph g;
    scene::NodeHandle h = g.CreateNode("actor");
    g.AttachBehavior(h, std::make_unique<MoverX>());

    g.Update(0.016f);
    g.UpdateWorldTransforms();

    CHECK(glm::vec3(g.Get(h).worldMatrix[3]).x == doctest::Approx(7.0f));
}

TEST_CASE("Behavior: tras destruir el nodo, su behavior no vuelve a correr") {
    scene::SceneGraph g;
    scene::NodeHandle h = g.CreateNode("actor");
    int counter = 0;
    g.AttachBehavior(h, std::make_unique<FlagBehavior>(&counter));

    g.Update(0.016f);
    CHECK(counter == 1);          // corrio una vez estando vivo

    g.Destroy(h);
    g.Update(0.016f);             // muerto: no corre, no crashea

    CHECK(counter == 1);          // sigue en 1
    CHECK_FALSE(g.IsValid(h));
}

TEST_CASE("Behavior: destruir el nodo libera sus behaviors en el acto") {
    scene::SceneGraph g;
    scene::NodeHandle h = g.CreateNode("actor");
    bool destroyed = false;
    g.AttachBehavior(h, std::make_unique<DtorFlagBehavior>(&destroyed));

    g.Destroy(h);

    CHECK(destroyed == true);     // el destructor del behavior corrio al destruir el nodo
}
