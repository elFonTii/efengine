#include <doctest/doctest.h>
#include <efengine/scene/SceneGraph.h>
#include <efengine/scene/Node.h>
#include <efengine/scene/Behavior.h>
#include <memory>

using namespace efengine;

namespace {
    struct ContarBehavior : scene::Behavior {
        int* llamadas;
        explicit ContarBehavior(int* c) : llamadas(c) {}
        void OnUpdate(scene::UpdateContext&) override { ++(*llamadas); }
    };
}

TEST_CASE("SceneGraphClear: despues de Clear solo queda la raiz") {
    scene::SceneGraph g;
    g.CreateNode("a");
    scene::NodeHandle b = g.CreateNode("b");
    g.CreateChild(b, "b_hijo");

    REQUIRE(g.IsValid(g.Root()));
    REQUIRE(g.Get(g.Root()).children.size() == 2u);

    g.Clear();

    CHECK(g.IsValid(g.Root()));
    CHECK(g.Get(g.Root()).children.empty());
    CHECK(g.Get(g.Root()).name == "root");
    CHECK(g.FindByName("a").IsNull());
    CHECK(g.FindByName("b").IsNull());
    CHECK(g.FindByName("b_hijo").IsNull());
}

TEST_CASE("SceneGraphClear: los handles viejos quedan invalidos para siempre") {
    scene::SceneGraph g;
    scene::NodeHandle viejo = g.CreateNode("actor");
    const scene::NodeHandle raizVieja = g.Root();
    REQUIRE(g.IsValid(viejo));

    g.Clear();

    // La generacion NO se reinicia: un handle guardado antes del Clear no puede
    // resucitar apuntando a un nodo nuevo que reuso su indice.
    CHECK_FALSE(g.IsValid(viejo));

    scene::NodeHandle nuevo = g.CreateNode("otro");
    CHECK(g.IsValid(nuevo));
    CHECK(nuevo != viejo);
    CHECK_FALSE(g.IsValid(viejo));

    // Incluso la raiz vieja: es un handle distinto del de la raiz nueva.
    CHECK(g.Root() != raizVieja);
    CHECK_FALSE(g.IsValid(raizVieja));
}

TEST_CASE("SceneGraphClear: Clear libera los behaviors y ya no corren") {
    scene::SceneGraph g;
    scene::NodeHandle h = g.CreateNode("actor");
    int llamadas = 0;
    g.AttachBehavior(h, std::make_unique<ContarBehavior>(&llamadas));

    g.Update(0.016f);
    REQUIRE(llamadas == 1);

    g.Clear();
    g.Update(0.016f);

    CHECK(llamadas == 1);      // no volvio a correr, y no crasheo
}

TEST_CASE("SceneGraphClear: Clear resetea el sol primario y las listas juntadas") {
    scene::SceneGraph g;
    scene::NodeHandle sol = g.CreateNode("sol");
    g.AttachLight(sol, { scene::LightKind::Directional, glm::vec3(3.0f) });
    g.SetPrimarySun(sol);

    scene::NodeHandle luz = g.CreateNode("luz");
    g.AttachLight(luz, { scene::LightKind::Point, glm::vec3(100.0f) });
    g.UpdateWorldTransforms();
    REQUIRE(g.PointLights().size() == 1u);

    g.Clear();
    g.UpdateWorldTransforms();

    CHECK(g.PrimarySun().IsNull());
    CHECK(g.PointLights().empty());
    CHECK(g.Renderables().empty());
}

TEST_CASE("SceneGraphClear: el grafo queda plenamente usable despues de Clear") {
    scene::SceneGraph g;
    g.CreateNode("viejo");
    g.Clear();

    scene::NodeHandle a = g.CreateNode("a");
    scene::NodeHandle b = g.CreateChild(a, "b");
    math::Transform t;
    t.position = glm::vec3(0.0f, 4.0f, 0.0f);
    g.SetLocalTransform(a, t);
    g.UpdateWorldTransforms();

    CHECK(g.IsValid(a));
    CHECK(g.IsValid(b));
    CHECK(g.Get(a).children.size() == 1u);
    CHECK(glm::vec3(g.Get(b).worldMatrix[3]).y == doctest::Approx(4.0f));
    CHECK(g.FindByName("viejo").IsNull());
}

TEST_CASE("SceneGraphClear: Clear dos veces seguidas es inocuo") {
    scene::SceneGraph g;
    g.CreateNode("a");
    g.Clear();
    g.Clear();

    CHECK(g.IsValid(g.Root()));
    CHECK(g.Get(g.Root()).children.empty());
}

TEST_CASE("SceneGraphClear: Get const devuelve el mismo nodo que la version no-const") {
    scene::SceneGraph g;
    scene::NodeHandle h = g.CreateNode("actor");

    const scene::SceneGraph& cg = g;
    const scene::Node& porConst = cg.Get(h);
    CHECK(porConst.name == "actor");
    CHECK(cg.TryGet(h) == &porConst);

    scene::NodeHandle invalido;
    CHECK(cg.TryGet(invalido) == nullptr);
}
