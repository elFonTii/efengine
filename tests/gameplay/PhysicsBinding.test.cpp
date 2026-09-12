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
