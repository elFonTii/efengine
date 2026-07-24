#include <doctest/doctest.h>
#include <efengine/scene/Scene.h>
#include <efengine/scene/SceneObject.h>
#include <efengine/math/Transform.h>
#include <efengine/renderer/Mesh.h>
#include <efengine/renderer/Model.h>

#include <glm/glm.hpp>
#include <vector>

using namespace efengine;

/*
    Sin contexto GL, y no es un accidente: un renderer::Model con un vector de
    meshes VACÍO no construye ningún Mesh, así que no construye ningún
    VertexArray, así que no hace una sola llamada a OpenGL (Model::Model solo
    mueve el vector).

    El puntero al Model nunca se deferencia acá: SceneObject lo observa
    (README A.4) y Scene solo lo guarda. Quien dibuja es el sandbox.

    Model no es copiable (VertexArray es RO5 con copia borrada), pero
    MakeEmptyModel devuelve un prvalue y C++17 garantiza la elisión, así que
    "Model model = MakeEmptyModel();" no necesita constructor de movimiento.
*/

namespace {
    renderer::Model MakeEmptyModel() {
        return renderer::Model(std::vector<renderer::Mesh>{});
    }
}

TEST_CASE("Scene arranca vacia") {
    scene::Scene s;

    CHECK(s.objects().empty());
}

TEST_CASE("Scene::Add devuelve handles consecutivos desde 0") {
    scene::Scene s;
    renderer::Model model = MakeEmptyModel();

    const u32 first  = s.Add({ &model, {} });
    const u32 second = s.Add({ &model, {} });

    CHECK(first  == 0u);
    CHECK(second == 1u);
    CHECK(s.objects().size() == 2u);
}

TEST_CASE("Scene::Get devuelve el objeto que se agrego, con su transform intacto") {
    scene::Scene s;
    renderer::Model model = MakeEmptyModel();

    math::Transform t;
    t.position = glm::vec3(1.0f, 2.0f, 3.0f);
    t.scale    = glm::vec3(10.0f);

    const u32 handle = s.Add({ &model, {}, t });

    CHECK(s.Get(handle).model == &model);
    CHECK(s.Get(handle).transform.position.y == doctest::Approx(2.0f));
    CHECK(s.Get(handle).transform.scale.x    == doctest::Approx(10.0f));
}

TEST_CASE("Scene::Get da acceso mutable: mover el transform se ve en objects()") {
    scene::Scene s;
    renderer::Model model = MakeEmptyModel();
    const u32 handle = s.Add({ &model, {} });

    // Esto es exactamente lo que hace el sandbox en su loop para girar la rata.
    s.Get(handle).transform.rotation.y = 45.0f;

    CHECK(s.objects()[handle].transform.rotation.y == doctest::Approx(45.0f));
}

TEST_CASE("Scene::Add preserva los objetos previos al crecer") {
    scene::Scene s;
    renderer::Model model = MakeEmptyModel();

    math::Transform first;
    first.position = glm::vec3(7.0f, 0.0f, 0.0f);
    const u32 firstHandle = s.Add({ &model, {}, first });

    // Varios Add fuerzan al menos una realocación del vector. Los HANDLES
    // sobreviven (son índices); las referencias de un Get previo no. Este caso
    // fija la mitad que sí vale.
    for (int i = 0; i < 8; i++) {
        s.Add({ &model, {} });
    }

    CHECK(s.objects().size() == 9u);
    CHECK(s.Get(firstHandle).transform.position.x == doctest::Approx(7.0f));
}

TEST_CASE("Scene arranca sin luces") {
    scene::Scene s;

    CHECK(s.lights().empty());
}

TEST_CASE("Scene::AddLight devuelve handles consecutivos desde 0") {
    scene::Scene s;

    const u32 first  = s.AddLight({ glm::vec3(1.0f), glm::vec3(5000.0f) });
    const u32 second = s.AddLight({ glm::vec3(2.0f), glm::vec3(5000.0f) });

    CHECK(first  == 0u);
    CHECK(second == 1u);
    CHECK(s.lights().size() == 2u);
}

TEST_CASE("Scene::GetLight devuelve la luz agregada") {
    scene::Scene s;
    const u32 handle = s.AddLight({ glm::vec3(10.0f, 20.0f, 30.0f), glm::vec3(5000.0f) });

    CHECK(s.GetLight(handle).position.y == doctest::Approx(20.0f));
    CHECK(s.GetLight(handle).color.r    == doctest::Approx(5000.0f));
}

TEST_CASE("Scene::GetLight da acceso mutable: mover la luz se ve en lights()") {
    scene::Scene s;
    const u32 handle = s.AddLight({ glm::vec3(0.0f), glm::vec3(5000.0f) });

    // Esto es exactamente lo que hará el sandbox para orbitar la luz.
    s.GetLight(handle).position = glm::vec3(50.0f, 80.0f, 0.0f);

    CHECK(s.lights()[handle].position.x == doctest::Approx(50.0f));
}

TEST_CASE("Scene::ambientFactor default es 0.08") {
    scene::Scene s;

    CHECK(s.ambientFactor == doctest::Approx(0.08f));
}

TEST_CASE("Scene::sun arranca con una direccion no-nula") {
    scene::Scene s;
    CHECK(glm::length(s.sun().direction) > 0.0f);
}

TEST_CASE("Scene::SetSun / sun round-trip") {
    scene::Scene s;
    s.SetSun({ glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(2.0f, 2.0f, 2.0f) });
    CHECK(s.sun().direction.y == doctest::Approx(-1.0f));
    CHECK(s.sun().color.r     == doctest::Approx(2.0f));
}

TEST_CASE("Scene::sun da acceso mutable: editar se ve reflejado") {
    scene::Scene s;
    s.sun().color = glm::vec3(5.0f);
    CHECK(s.sun().color.g == doctest::Approx(5.0f));
}
