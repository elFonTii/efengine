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

namespace {
    constexpr f32 kPasoFijo = 1.0f / 60.0f;

    // Jolt deja hundir los cuerpos a proposito (penetration slop, 0.02 m por
    // default): una esfera de radio r reposa en r - 0.02, no en r exacto.
    constexpr f32 kTolerancia = 0.03f;

    glm::vec3 posicionDe(const PhysicsWorld& w, BodyHandle h) {
        BodyPose pose;
        REQUIRE(w.GetBodyPose(h, pose));
        return pose.position;
    }

    // Piso de 20x1x20 centrado en y = -0.5: su cara de arriba queda en y = 0.
    BodyHandle crearPiso(PhysicsWorld& w) {
        return w.CreateBody(descCaja(glm::vec3(10.0f, 0.5f, 10.0f)),
                            en(glm::vec3(0.0f, -0.5f, 0.0f)),
                            MotionType::Static);
    }
}

TEST_CASE("PhysicsWorld: una esfera cae y se apoya en el piso") {
    Mundo m;

    const BodyHandle piso = crearPiso(*m.world);
    REQUIRE_FALSE(piso.IsNull());

    const f32 radio = 0.5f;
    const BodyHandle bola = m.world->CreateBody(descEsfera(radio),
                                                en(glm::vec3(0.0f, 5.0f, 0.0f)),
                                                MotionType::Dynamic);
    REQUIRE_FALSE(bola.IsNull());

    m.world->OptimizeBroadPhase();

    for (i32 i = 0; i < 180; ++i) m.world->Step(kPasoFijo);

    CHECK(std::abs(posicionDe(*m.world, bola).y - radio) < kTolerancia);
}

TEST_CASE("PhysicsWorld: una vez apoyada, la esfera se queda quieta") {
    Mundo m;

    crearPiso(*m.world);
    const BodyHandle bola = m.world->CreateBody(descEsfera(0.5f),
                                                en(glm::vec3(0.0f, 5.0f, 0.0f)),
                                                MotionType::Dynamic);
    m.world->OptimizeBroadPhase();

    for (i32 i = 0; i < 180; ++i) m.world->Step(kPasoFijo);

    const glm::vec3 antes = posicionDe(*m.world, bola);
    for (i32 i = 0; i < 30; ++i) m.world->Step(kPasoFijo);
    const glm::vec3 despues = posicionDe(*m.world, bola);

    CHECK(std::abs(despues.y - antes.y) < 1e-3f);
    CHECK(std::abs(despues.x - antes.x) < 1e-3f);
    CHECK(std::abs(despues.z - antes.z) < 1e-3f);
}

TEST_CASE("PhysicsWorld: un cuerpo estatico no se mueve") {
    Mundo m;

    const BodyHandle piso = crearPiso(*m.world);
    const glm::vec3 antes = posicionDe(*m.world, piso);

    for (i32 i = 0; i < 120; ++i) m.world->Step(kPasoFijo);

    const glm::vec3 despues = posicionDe(*m.world, piso);
    CHECK(despues.y == doctest::Approx(antes.y));
}

TEST_CASE("PhysicsWorld: SetBodyPose teletransporta un cuerpo kinematico") {
    Mundo m;

    const BodyHandle plataforma = m.world->CreateBody(descCaja(glm::vec3(1.0f, 0.1f, 1.0f)),
                                                      en(glm::vec3(0.0f, 1.0f, 0.0f)),
                                                      MotionType::Kinematic);
    REQUIRE_FALSE(plataforma.IsNull());

    BodyPose destino;
    destino.position = glm::vec3(3.0f, 2.0f, -1.0f);
    m.world->SetBodyPose(plataforma, destino);

    const glm::vec3 p = posicionDe(*m.world, plataforma);
    CHECK(p.x == doctest::Approx(3.0f));
    CHECK(p.y == doctest::Approx(2.0f));
    CHECK(p.z == doctest::Approx(-1.0f));

    for (i32 i = 0; i < 60; ++i) m.world->Step(kPasoFijo);
    CHECK(posicionDe(*m.world, plataforma).y == doctest::Approx(2.0f));
}

TEST_CASE("PhysicsWorld: GetBodyPose con un handle invalido devuelve false y no toca out") {
    Mundo m;

    BodyPose pose;
    pose.position = glm::vec3(7.0f, 7.0f, 7.0f);

    const BodyHandle nulo;
    CHECK_FALSE(m.world->GetBodyPose(nulo, pose));
    CHECK(pose.position.x == doctest::Approx(7.0f));
}

TEST_CASE("PhysicsWorld: Step con dt cero o negativo es no-op") {
    Mundo m;

    crearPiso(*m.world);
    const BodyHandle bola = m.world->CreateBody(descEsfera(0.5f),
                                                en(glm::vec3(0.0f, 5.0f, 0.0f)),
                                                MotionType::Dynamic);

    const glm::vec3 antes = posicionDe(*m.world, bola);
    m.world->Step(0.0f);
    m.world->Step(-1.0f);
    const glm::vec3 despues = posicionDe(*m.world, bola);

    CHECK(despues.y == doctest::Approx(antes.y));
}
