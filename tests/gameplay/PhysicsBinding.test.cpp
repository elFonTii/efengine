#include <doctest/doctest.h>
#include <efengine/gameplay/PhysicsBinding.h>
#include <efengine/physics/JoltRuntime.h>
#include <efengine/physics/PhysicsWorld.h>
#include <efengine/scene/SceneGraph.h>

#include <cmath>
#include <memory>

using namespace efengine;
using efengine::physics::BodyPose;
using efengine::physics::JoltRuntime;
using efengine::physics::PhysicsWorld;

namespace {
    // Un runtime por TEST_CASE: Jolt admite uno solo por proceso y doctest corre
    // todo junto. El orden de los miembros importa, porque el de destruccion es
    // el inverso: primero el binding, despues el mundo, despues el runtime.
    struct Banco {
        scene::SceneGraph               scene;
        std::unique_ptr<JoltRuntime>    runtime;
        std::unique_ptr<PhysicsWorld>   world;
        std::unique_ptr<gameplay::PhysicsBinding> binding;

        Banco() {
            runtime = JoltRuntime::Create();
            REQUIRE(runtime != nullptr);
            world = PhysicsWorld::Create(*runtime);
            REQUIRE(world != nullptr);
            binding = std::make_unique<gameplay::PhysicsBinding>(scene, *world);
        }
    };

    scene::NodeHandle conCollider(scene::SceneGraph& g, scene::NodeHandle padre,
                                  const char* nombre, const glm::vec3& pos,
                                  scene::ShapeKind kind, const glm::vec3& params,
                                  scene::MotionType motion) {
        const scene::NodeHandle h = g.CreateChild(padre, nombre);

        math::Transform t;
        t.position = pos;
        g.SetLocalTransform(h, t);

        scene::ColliderAttachment col;
        col.kind   = kind;
        col.params = params;
        col.motion = motion;
        g.AttachCollider(h, col);
        return h;
    }

    glm::vec3 poseDe(const PhysicsWorld& w, physics::BodyHandle h) {
        BodyPose pose;
        REQUIRE(w.GetBodyPose(h, pose));
        return pose.position;
    }
}

TEST_CASE("PhysicsBinding: Build crea un cuerpo por nodo con collider y ninguno para el resto") {
    Banco b;
    const scene::NodeHandle piso = conCollider(b.scene, b.scene.Root(), "piso", glm::vec3(0.0f),
                                               scene::ShapeKind::Box, glm::vec3(1.0f),
                                               scene::MotionType::Static);
    b.scene.CreateChild(b.scene.Root(), "sin collider");

    b.binding->Build();

    CHECK(b.binding->BoundCount() == 1u);
    CHECK(b.world->BodyCount() == 1u);
    CHECK_FALSE(b.binding->BodyOf(piso).IsNull());
}

TEST_CASE("PhysicsBinding: BodyOf y NodeOf son inversos") {
    Banco b;
    const scene::NodeHandle n = conCollider(b.scene, b.scene.Root(), "caja", glm::vec3(0.0f),
                                            scene::ShapeKind::Box, glm::vec3(1.0f),
                                            scene::MotionType::Static);
    b.binding->Build();

    const physics::BodyHandle body = b.binding->BodyOf(n);
    REQUIRE_FALSE(body.IsNull());
    CHECK(b.binding->NodeOf(body) == n);
}

TEST_CASE("PhysicsBinding: un nodo sin cuerpo devuelve handle nulo") {
    Banco b;
    const scene::NodeHandle pelado = b.scene.CreateChild(b.scene.Root(), "pelado");
    b.binding->Build();

    CHECK(b.binding->BodyOf(pelado).IsNull());
    CHECK(b.binding->NodeOf(physics::BodyHandle{}).IsNull());
}

TEST_CASE("PhysicsBinding: Clear destruye los cuerpos y vacia el mapeo") {
    Banco b;
    const scene::NodeHandle n = conCollider(b.scene, b.scene.Root(), "caja", glm::vec3(0.0f),
                                            scene::ShapeKind::Box, glm::vec3(1.0f),
                                            scene::MotionType::Static);
    b.binding->Build();
    REQUIRE(b.world->BodyCount() == 1u);

    b.binding->Clear();

    CHECK(b.binding->BoundCount() == 0u);
    CHECK(b.world->BodyCount() == 0u);
    CHECK(b.binding->BodyOf(n).IsNull());
}

TEST_CASE("PhysicsBinding: Build dos veces no duplica cuerpos") {
    Banco b;
    conCollider(b.scene, b.scene.Root(), "caja", glm::vec3(0.0f), scene::ShapeKind::Box,
                glm::vec3(1.0f), scene::MotionType::Static);

    b.binding->Build();
    b.binding->Build();

    CHECK(b.binding->BoundCount() == 1u);
    CHECK(b.world->BodyCount() == 1u);
}

TEST_CASE("PhysicsBinding: el cuerpo nace en la posicion de MUNDO del nodo, no en la local") {
    Banco b;
    const scene::NodeHandle padre = b.scene.CreateChild(b.scene.Root(), "padre");
    math::Transform tp;
    tp.position = glm::vec3(10.0f, 0.0f, 0.0f);
    b.scene.SetLocalTransform(padre, tp);

    const scene::NodeHandle hijo = conCollider(b.scene, padre, "hijo", glm::vec3(0.0f, 5.0f, 0.0f),
                                               scene::ShapeKind::Sphere, glm::vec3(0.5f, 0.0f, 0.0f),
                                               scene::MotionType::Static);
    b.binding->Build();

    const glm::vec3 p = poseDe(*b.world, b.binding->BodyOf(hijo));
    CHECK(p.x == doctest::Approx(10.0f));
    CHECK(p.y == doctest::Approx(5.0f));
}

TEST_CASE("PhysicsBinding: el localOffset corre el cuerpo respecto del nodo") {
    Banco b;
    const scene::NodeHandle n = conCollider(b.scene, b.scene.Root(), "caja", glm::vec3(1.0f, 0.0f, 0.0f),
                                            scene::ShapeKind::Box, glm::vec3(0.5f),
                                            scene::MotionType::Static);
    b.scene.Get(n).collider->localOffset.position = glm::vec3(0.0f, 3.0f, 0.0f);

    b.binding->Build();

    const glm::vec3 p = poseDe(*b.world, b.binding->BodyOf(n));
    CHECK(p.x == doctest::Approx(1.0f));
    CHECK(p.y == doctest::Approx(3.0f));
}

TEST_CASE("PhysicsBinding: un collider Mesh sin malla en el nodo no crea cuerpo") {
    Banco b;
    conCollider(b.scene, b.scene.Root(), "sin malla", glm::vec3(0.0f), scene::ShapeKind::Mesh,
                glm::vec3(1.0f), scene::MotionType::Static);

    b.binding->Build();

    CHECK(b.binding->BoundCount() == 0u);
    CHECK(b.world->BodyCount() == 0u);
}

TEST_CASE("PhysicsBinding: una forma degenerada no entra al mapeo") {
    Banco b;
    const scene::NodeHandle n = conCollider(b.scene, b.scene.Root(), "chata", glm::vec3(0.0f),
                                            scene::ShapeKind::Box, glm::vec3(1.0f),
                                            scene::MotionType::Static);
    math::Transform t = b.scene.Get(n).local;
    t.scale = glm::vec3(1.0f, 0.0f, 1.0f);
    b.scene.SetLocalTransform(n, t);

    b.binding->Build();

    CHECK(b.binding->BoundCount() == 0u);
    CHECK(b.binding->BodyOf(n).IsNull());
}

namespace {
    constexpr f32 kPasoFijo = 1.0f / 60.0f;

    // Jolt hunde los cuerpos a proposito (penetration slop, 0.02 m por default):
    // una esfera de radio r reposa en r - 0.02, no en r exacto. Sin este margen
    // el test parece flaky y termina en un +-0.5 que ya no prueba nada.
    constexpr f32 kTolerancia = 0.03f;

    void correr(Banco& b, i32 pasos) {
        for (i32 i = 0; i < pasos; ++i) {
            b.binding->PushKinematic();
            b.world->Step(kPasoFijo);
            b.binding->WriteBack();
        }
    }

    // Piso de 20x1x20 con la cara de arriba en y = 0.
    scene::NodeHandle piso(scene::SceneGraph& g) {
        return conCollider(g, g.Root(), "piso", glm::vec3(0.0f, -0.5f, 0.0f),
                           scene::ShapeKind::Box, glm::vec3(10.0f, 0.5f, 10.0f),
                           scene::MotionType::Static);
    }
}

TEST_CASE("PhysicsBinding: un dinamico cae y el local del nodo baja") {
    Banco b;
    piso(b.scene);
    const scene::NodeHandle esfera = conCollider(b.scene, b.scene.Root(), "esfera",
                                                 glm::vec3(0.0f, 5.0f, 0.0f),
                                                 scene::ShapeKind::Sphere, glm::vec3(0.5f, 0.0f, 0.0f),
                                                 scene::MotionType::Dynamic);
    b.binding->Build();

    correr(b, 180);
    b.binding->Interpolate(1.0f);

    CHECK(std::fabs(b.scene.Get(esfera).local.position.y - 0.5f) < kTolerancia);
}

TEST_CASE("PhysicsBinding: un estatico no se mueve") {
    Banco b;
    const scene::NodeHandle p = piso(b.scene);
    b.binding->Build();

    correr(b, 60);
    b.binding->Interpolate(1.0f);

    CHECK(b.scene.Get(p).local.position.y == doctest::Approx(-0.5f));
}

TEST_CASE("PhysicsBinding: un kinematico sigue al nodo") {
    Banco b;
    const scene::NodeHandle plataforma = conCollider(b.scene, b.scene.Root(), "plataforma",
                                                     glm::vec3(0.0f), scene::ShapeKind::Box,
                                                     glm::vec3(1.0f), scene::MotionType::Kinematic);
    b.binding->Build();

    math::Transform t = b.scene.Get(plataforma).local;
    t.position = glm::vec3(0.0f, 4.0f, 0.0f);
    b.scene.SetLocalTransform(plataforma, t);

    correr(b, 1);

    // SetBodyPose teletransporta, y un kinematico con velocidad cero no se mueve
    // en el Step: la posicion queda exacta, no aproximada.
    CHECK(poseDe(*b.world, b.binding->BodyOf(plataforma)).y == doctest::Approx(4.0f));
}

TEST_CASE("PhysicsBinding: Interpolate(0) da la pose vieja y Interpolate(1) la nueva") {
    Banco b;
    piso(b.scene);
    const scene::NodeHandle esfera = conCollider(b.scene, b.scene.Root(), "esfera",
                                                 glm::vec3(0.0f, 5.0f, 0.0f),
                                                 scene::ShapeKind::Sphere, glm::vec3(0.5f, 0.0f, 0.0f),
                                                 scene::MotionType::Dynamic);
    b.binding->Build();
    correr(b, 1);   // prev = 5.0, curr = un poquito mas abajo

    b.binding->Interpolate(0.0f);
    const f32 enPrev = b.scene.Get(esfera).local.position.y;

    b.binding->Interpolate(1.0f);
    const f32 enCurr = b.scene.Get(esfera).local.position.y;

    b.binding->Interpolate(0.5f);
    const f32 enMedio = b.scene.Get(esfera).local.position.y;

    CHECK(enPrev == doctest::Approx(5.0f));
    CHECK(enCurr < enPrev);
    CHECK(enMedio < enPrev);
    CHECK(enMedio > enCurr);
}

TEST_CASE("PhysicsBinding: el write-back de un hijo devuelve un local relativo al padre") {
    Banco b;
    piso(b.scene);

    const scene::NodeHandle padre = b.scene.CreateChild(b.scene.Root(), "padre");
    math::Transform tp;
    tp.position = glm::vec3(0.0f, 10.0f, 0.0f);
    b.scene.SetLocalTransform(padre, tp);

    // Mundo: y = 10 + 5 = 15. Al apoyarse en y = 0.5 de MUNDO, su local tiene que
    // quedar en 0.5 - 10 = -9.5.
    const scene::NodeHandle esfera = conCollider(b.scene, padre, "esfera", glm::vec3(0.0f, 5.0f, 0.0f),
                                                 scene::ShapeKind::Sphere, glm::vec3(0.5f, 0.0f, 0.0f),
                                                 scene::MotionType::Dynamic);
    b.binding->Build();

    correr(b, 240);
    b.binding->Interpolate(1.0f);

    CHECK(std::fabs(b.scene.Get(esfera).local.position.y - (-9.5f)) < kTolerancia);
}

TEST_CASE("PhysicsBinding: la escala del padre la absorbe la forma") {
    Banco b;
    piso(b.scene);

    const scene::NodeHandle padre = b.scene.CreateChild(b.scene.Root(), "padre");
    math::Transform tp;
    tp.position = glm::vec3(0.0f, 0.0f, 0.0f);
    tp.scale    = glm::vec3(2.0f);
    b.scene.SetLocalTransform(padre, tp);

    // Radio 0.5 escalado x2 = 1.0: reposa en y = 1.0 de MUNDO, o sea local 0.5.
    const scene::NodeHandle esfera = conCollider(b.scene, padre, "esfera", glm::vec3(0.0f, 3.0f, 0.0f),
                                                 scene::ShapeKind::Sphere, glm::vec3(0.5f, 0.0f, 0.0f),
                                                 scene::MotionType::Dynamic);
    b.binding->Build();

    correr(b, 240);
    b.binding->Interpolate(1.0f);

    const f32 mundoY = b.scene.Get(esfera).local.position.y * 2.0f;
    CHECK(std::fabs(mundoY - 1.0f) < kTolerancia);
}

TEST_CASE("PhysicsBinding: un nodo destruido en pleno vuelo se saca del mapeo") {
    Banco b;
    piso(b.scene);
    const scene::NodeHandle esfera = conCollider(b.scene, b.scene.Root(), "esfera",
                                                 glm::vec3(0.0f, 5.0f, 0.0f),
                                                 scene::ShapeKind::Sphere, glm::vec3(0.5f, 0.0f, 0.0f),
                                                 scene::MotionType::Dynamic);
    b.binding->Build();
    REQUIRE(b.binding->BoundCount() == 2u);

    correr(b, 10);
    b.scene.Destroy(esfera);
    correr(b, 1);

    CHECK(b.binding->BoundCount() == 1u);
    CHECK(b.world->BodyCount() == 1u);
    CHECK(b.binding->BodyOf(esfera).IsNull());
}
