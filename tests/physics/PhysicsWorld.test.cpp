#include <doctest/doctest.h>
#include <efengine/math/Transform.h>
#include <efengine/physics/JoltRuntime.h>
#include <efengine/physics/PhysicsWorld.h>

#include <cmath>

using efengine::math::Transform;
using efengine::physics::BodyHandle;
using efengine::physics::BodyPose;
using efengine::physics::JoltRuntime;
using efengine::physics::MotionType;
using efengine::physics::PhysicsWorld;
using efengine::physics::ShapeDesc;
using efengine::physics::ShapeKind;

TEST_CASE("JoltRuntime: se crea y arranca vivo") {
    const std::unique_ptr<JoltRuntime> rt = JoltRuntime::Create();
    REQUIRE(rt != nullptr);
}

TEST_CASE("JoltRuntime: se puede destruir y volver a crear en el mismo proceso") {
    // El estado de Jolt es GLOBAL de proceso (allocator, Factory, tipos
    // registrados). Si el destructor deja algo sin desarmar, el segundo Create
    // devuelve nullptr o revienta. Este test es la red que atrapa eso, y ademas
    // es lo que permite que cada TEST_CASE de abajo arme su propio runtime.
    {
        const std::unique_ptr<JoltRuntime> primero = JoltRuntime::Create();
        REQUIRE(primero != nullptr);
    }
    {
        const std::unique_ptr<JoltRuntime> segundo = JoltRuntime::Create();
        REQUIRE(segundo != nullptr);
    }
}

TEST_CASE("JoltRuntime: dos runtimes vivos a la vez se rechaza") {
    // Jolt tiene una sola Factory por proceso: dos runtimes simultaneos se
    // pisarian. Falla ruidoso con nullptr, no con un crash a los diez minutos.
    const std::unique_ptr<JoltRuntime> primero = JoltRuntime::Create();
    REQUIRE(primero != nullptr);

    const std::unique_ptr<JoltRuntime> segundo = JoltRuntime::Create();
    CHECK(segundo == nullptr);
}

namespace {
    // Cada TEST_CASE arma su propio runtime: doctest corre todo en un proceso.
    struct Mundo {
        std::unique_ptr<JoltRuntime>  runtime;
        std::unique_ptr<PhysicsWorld> world;

        Mundo() {
            runtime = JoltRuntime::Create();
            REQUIRE(runtime != nullptr);
            world = PhysicsWorld::Create(*runtime);
            REQUIRE(world != nullptr);
        }
    };

    Transform en(const glm::vec3& posicion) {
        Transform t;
        t.position = posicion;
        return t;
    }

    ShapeDesc descEsfera(f32 radio) {
        ShapeDesc d;
        d.kind   = ShapeKind::Sphere;
        d.params = glm::vec3(radio, 0.0f, 0.0f);
        return d;
    }

    ShapeDesc descCaja(const glm::vec3& halfExtents) {
        ShapeDesc d;
        d.kind   = ShapeKind::Box;
        d.params = halfExtents;
        return d;
    }
}

TEST_CASE("PhysicsWorld: arranca vacio") {
    Mundo m;
    CHECK(m.world->BodyCount() == 0u);
}

TEST_CASE("PhysicsWorld: un handle recien creado es valido y cuenta") {
    Mundo m;

    const BodyHandle h = m.world->CreateBody(descEsfera(0.5f), en(glm::vec3(0.0f, 2.0f, 0.0f)),
                                             MotionType::Dynamic);

    CHECK_FALSE(h.IsNull());
    CHECK(m.world->IsValid(h));
    CHECK(m.world->BodyCount() == 1u);
}

TEST_CASE("PhysicsWorld: un handle destruido deja de ser valido") {
    Mundo m;

    const BodyHandle h = m.world->CreateBody(descEsfera(0.5f), en(glm::vec3(0.0f)),
                                             MotionType::Dynamic);
    m.world->DestroyBody(h);

    CHECK_FALSE(m.world->IsValid(h));
    CHECK(m.world->BodyCount() == 0u);
}

TEST_CASE("PhysicsWorld: el indice se reusa pero el handle viejo no revive") {
    Mundo m;

    const BodyHandle viejo = m.world->CreateBody(descEsfera(0.5f), en(glm::vec3(0.0f)),
                                                 MotionType::Dynamic);
    m.world->DestroyBody(viejo);

    const BodyHandle nuevo = m.world->CreateBody(descEsfera(0.5f), en(glm::vec3(0.0f)),
                                                 MotionType::Dynamic);

    CHECK(nuevo.index == viejo.index);
    CHECK(nuevo.generation != viejo.generation);
    CHECK(m.world->IsValid(nuevo));
    CHECK_FALSE(m.world->IsValid(viejo));
}

TEST_CASE("PhysicsWorld: un handle default no es valido") {
    Mundo m;
    const BodyHandle nulo;

    CHECK(nulo.IsNull());
    CHECK_FALSE(m.world->IsValid(nulo));
    m.world->DestroyBody(nulo);
    CHECK(m.world->BodyCount() == 0u);
}

TEST_CASE("PhysicsWorld: una forma degenerada no crea cuerpo") {
    Mundo m;

    Transform t;
    t.scale = glm::vec3(1.0f, 0.0f, 1.0f);

    const BodyHandle h = m.world->CreateBody(descCaja(glm::vec3(1.0f)), t, MotionType::Static);

    CHECK(h.IsNull());
    CHECK(m.world->BodyCount() == 0u);
}

TEST_CASE("PhysicsWorld: pasarse de maxBodies devuelve handle nulo, no revienta") {
    const std::unique_ptr<JoltRuntime> rt = JoltRuntime::Create();
    REQUIRE(rt != nullptr);

    efengine::physics::PhysicsWorldDesc desc;
    desc.maxBodies = 2;

    const std::unique_ptr<PhysicsWorld> w = PhysicsWorld::Create(*rt, desc);
    REQUIRE(w != nullptr);

    CHECK_FALSE(w->CreateBody(descEsfera(0.5f), en(glm::vec3(0.0f)), MotionType::Dynamic).IsNull());
    CHECK_FALSE(w->CreateBody(descEsfera(0.5f), en(glm::vec3(2.0f)), MotionType::Dynamic).IsNull());

    CHECK(w->CreateBody(descEsfera(0.5f), en(glm::vec3(4.0f)), MotionType::Dynamic).IsNull());
    CHECK(w->BodyCount() == 2u);
}

TEST_CASE("PhysicsWorld: destruir dos veces el mismo handle es no-op") {
    Mundo m;

    const BodyHandle h = m.world->CreateBody(descEsfera(0.5f), en(glm::vec3(0.0f)),
                                             MotionType::Dynamic);
    m.world->DestroyBody(h);
    m.world->DestroyBody(h);

    CHECK(m.world->BodyCount() == 0u);
}
