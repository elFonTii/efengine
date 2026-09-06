#include <doctest/doctest.h>
#include <efengine/scene/Camera.h>
#include <efengine/scene/NodeCamera.h>
#include <efengine/scene/SceneGraph.h>
#include <efengine/math/Transform.h>
#include <glm/glm.hpp>

using namespace efengine;

TEST_CASE("ApplyNodeCamera: sin attachment devuelve false y no toca la camara") {
    scene::SceneGraph g;
    scene::NodeHandle pelado = g.CreateChild(g.Root(), "pelado");

    scene::Camera cam;
    cam.SetFov(33.0f);

    CHECK(scene::ApplyNodeCamera(cam, g, pelado, 1.0f) == false);
    CHECK(cam.Fov() == doctest::Approx(33.0f));
}

TEST_CASE("ApplyNodeCamera: con handle invalido devuelve false") {
    scene::SceneGraph g;
    scene::NodeHandle muerto = g.CreateChild(g.Root(), "muerto");
    g.AttachCamera(muerto, scene::CameraAttachment{});
    g.Destroy(muerto);

    scene::Camera cam;
    CHECK(scene::ApplyNodeCamera(cam, g, muerto, 1.0f) == false);
}

TEST_CASE("ApplyNodeCamera: aplica pose y los cuatro campos del attachment") {
    scene::SceneGraph g;
    scene::NodeHandle nodo = g.CreateChild(g.Root(), "camara");

    math::Transform t;
    t.position = glm::vec3(2.0f, 3.0f, 4.0f);
    g.SetLocalTransform(nodo, t);

    scene::CameraAttachment att;
    att.fovDeg    = 70.0f;
    att.nearPlane = 0.25f;
    att.farPlane  = 900.0f;
    att.exposure  = 2.5f;
    g.AttachCamera(nodo, att);

    scene::Camera cam;
    CHECK(scene::ApplyNodeCamera(cam, g, nodo, 16.0f / 9.0f) == true);

    CHECK(cam.Position().x == doctest::Approx(2.0f));
    CHECK(cam.Position().z == doctest::Approx(4.0f));
    CHECK(cam.Target().z   == doctest::Approx(3.0f));   // 4 - 1: mira a -Z
    CHECK(cam.Fov()        == doctest::Approx(70.0f));
    CHECK(cam.NearPlane()  == doctest::Approx(0.25f));
    CHECK(cam.FarPlane()   == doctest::Approx(900.0f));
    CHECK(cam.Exposure()   == doctest::Approx(2.5f));
}

TEST_CASE("ApplyNodeCamera: hereda el transform del padre sin UpdateWorldTransforms") {
    scene::SceneGraph g;
    scene::NodeHandle padre = g.CreateChild(g.Root(), "personaje");
    scene::NodeHandle nodo  = g.CreateChild(padre, "camara");

    math::Transform tp;
    tp.position = glm::vec3(10.0f, 0.0f, 0.0f);
    g.SetLocalTransform(padre, tp);

    math::Transform tc;
    tc.position = glm::vec3(0.0f, 1.8f, 0.0f);
    g.SetLocalTransform(nodo, tc);

    g.AttachCamera(nodo, scene::CameraAttachment{});

    scene::Camera cam;
    CHECK(scene::ApplyNodeCamera(cam, g, nodo, 1.0f) == true);
    CHECK(cam.Position().x == doctest::Approx(10.0f));
    CHECK(cam.Position().y == doctest::Approx(1.8f));
}

TEST_CASE("SceneGraph::DetachCamera saca el attachment y no rompe con handle invalido") {
    scene::SceneGraph g;
    scene::NodeHandle nodo = g.CreateChild(g.Root(), "camara");
    g.AttachCamera(nodo, scene::CameraAttachment{});
    CHECK(g.Get(nodo).camera.has_value() == true);

    g.DetachCamera(nodo);
    CHECK(g.Get(nodo).camera.has_value() == false);

    g.Destroy(nodo);
    g.DetachCamera(nodo);   // no-op silencioso, igual que DetachMesh
}

TEST_CASE("SceneGraph::AttachCollider guarda la descripcion tal cual") {
    scene::SceneGraph g;
    scene::NodeHandle nodo = g.CreateChild(g.Root(), "piso");

    scene::ColliderAttachment col;
    col.kind      = scene::ShapeKind::Capsule;
    col.params    = glm::vec3(0.4f, 0.9f, 0.0f);
    col.motion    = scene::MotionType::Kinematic;
    col.isTrigger = true;
    g.AttachCollider(nodo, col);

    const scene::ColliderAttachment& leido = *g.Get(nodo).collider;
    CHECK(leido.kind      == scene::ShapeKind::Capsule);
    CHECK(leido.params.y  == doctest::Approx(0.9f));
    CHECK(leido.motion    == scene::MotionType::Kinematic);
    CHECK(leido.isTrigger == true);
}
