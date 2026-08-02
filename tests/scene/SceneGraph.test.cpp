#include <doctest/doctest.h>
#include <efengine/scene/SceneGraph.h>

using namespace efengine;

TEST_CASE("SceneGraph: arranca con una raiz valida y sin hijos") {
    scene::SceneGraph g;
    CHECK(g.IsValid(g.Root()));
    CHECK(g.Get(g.Root()).children.empty());
}

TEST_CASE("SceneGraph: CreateNode devuelve un handle valido que cuelga de la raiz") {
    scene::SceneGraph g;
    scene::NodeHandle h = g.CreateNode("rata");

    CHECK(g.IsValid(h));
    CHECK(g.Get(h).name == "rata");
    CHECK(g.Get(h).parent == g.Root());
    CHECK(g.Get(g.Root()).children.size() == 1u);
}

TEST_CASE("SceneGraph: destruir invalida el handle (IsValid/TryGet)") {
    scene::SceneGraph g;
    scene::NodeHandle h = g.CreateNode();

    g.Destroy(h);

    CHECK_FALSE(g.IsValid(h));
    CHECK(g.TryGet(h) == nullptr);
    CHECK(g.Get(g.Root()).children.empty());   // se quita del padre
}

TEST_CASE("SceneGraph: un handle viejo NO revive aunque se reuse la ranura (dangling)") {
    scene::SceneGraph g;
    scene::NodeHandle viejo = g.CreateNode("A");
    g.Destroy(viejo);

    scene::NodeHandle nuevo = g.CreateNode("B");   // reusa la ranura liberada

    CHECK(viejo.index == nuevo.index);             // misma ranura...
    CHECK(viejo.generation != nuevo.generation);   // ...distinta generación
    CHECK_FALSE(g.IsValid(viejo));                 // el handle viejo sigue muerto
    CHECK(g.IsValid(nuevo));
    CHECK(g.Get(nuevo).name == "B");
}

TEST_CASE("SceneGraph: destruir+crear N veces reusa ranuras (el pool no crece sin limite)") {
    scene::SceneGraph g;
    scene::NodeHandle h = g.CreateNode();
    u32 idx = h.index;
    for (int i = 0; i < 50; ++i) {
        g.Destroy(h);
        h = g.CreateNode();
        CHECK(h.index == idx);   // siempre reusa la misma ranura libre
    }
    CHECK(g.IsValid(h));
}

TEST_CASE("SceneGraph: FindByName devuelve el primer match, o nulo si no hay") {
    scene::SceneGraph g;
    g.CreateNode("suelo");
    scene::NodeHandle lampara = g.CreateNode("lampara");

    CHECK(g.FindByName("lampara") == lampara);
    CHECK(g.FindByName("noexiste").IsNull());
}

// ---------------------------------------------------------------------------
// Jerarquia y herencia de transform
// ---------------------------------------------------------------------------

TEST_CASE("SceneGraph: CreateChild cuelga del padre indicado, no de la raiz") {
    scene::SceneGraph g;
    scene::NodeHandle padre = g.CreateNode("mano");
    scene::NodeHandle hijo  = g.CreateChild(padre, "antorcha");

    CHECK(g.Get(hijo).parent == padre);
    CHECK(g.Get(padre).children.size() == 1u);
    CHECK(g.Get(padre).children[0] == hijo);
}

TEST_CASE("SceneGraph: el hijo hereda la traslacion del padre") {
    scene::SceneGraph g;
    scene::NodeHandle padre = g.CreateNode("padre");
    scene::NodeHandle hijo  = g.CreateChild(padre, "hijo");

    math::Transform tp; tp.position = glm::vec3(10.0f, 0.0f, 0.0f);
    g.SetLocalTransform(padre, tp);

    math::Transform th; th.position = glm::vec3(5.0f, 0.0f, 0.0f);
    g.SetLocalTransform(hijo, th);

    g.UpdateWorldTransforms();

    // world del hijo = T(10) * T(5) = T(15). La traslacion esta en la columna 3.
    glm::vec3 mundoHijo = glm::vec3(g.Get(hijo).worldMatrix[3]);
    CHECK(mundoHijo.x == doctest::Approx(15.0f));
    CHECK(mundoHijo.y == doctest::Approx(0.0f));
}

TEST_CASE("SceneGraph: rotar el padre reubica al hijo (herencia de rotacion)") {
    scene::SceneGraph g;
    scene::NodeHandle padre = g.CreateNode("padre");
    scene::NodeHandle hijo  = g.CreateChild(padre, "hijo");

    math::Transform tp; tp.rotation = glm::vec3(0.0f, 90.0f, 0.0f);  // 90 grados en Y
    g.SetLocalTransform(padre, tp);
    math::Transform th; th.position = glm::vec3(1.0f, 0.0f, 0.0f);
    g.SetLocalTransform(hijo, th);

    g.UpdateWorldTransforms();

    // (1,0,0) rotado +90 grados en Y -> (0,0,-1)
    glm::vec3 mundoHijo = glm::vec3(g.Get(hijo).worldMatrix[3]);
    CHECK(mundoHijo.x == doctest::Approx(0.0f).epsilon(0.001));
    CHECK(mundoHijo.z == doctest::Approx(-1.0f).epsilon(0.001));
}

// ---------------------------------------------------------------------------
// Destruccion en cascada
// ---------------------------------------------------------------------------

TEST_CASE("SceneGraph: destruir un padre invalida a TODOS sus descendientes (cascada)") {
    scene::SceneGraph g;
    scene::NodeHandle padre  = g.CreateNode("nave");
    scene::NodeHandle hijo   = g.CreateChild(padre, "ala");
    scene::NodeHandle nieto  = g.CreateChild(hijo,  "turbina");

    g.Destroy(padre);

    CHECK_FALSE(g.IsValid(padre));
    CHECK_FALSE(g.IsValid(hijo));
    CHECK_FALSE(g.IsValid(nieto));
    CHECK(g.Get(g.Root()).children.empty());   // la raiz ya no lo referencia
}

TEST_CASE("SceneGraph: la cascada no toca a los hermanos") {
    scene::SceneGraph g;
    scene::NodeHandle a = g.CreateNode("a");
    scene::NodeHandle b = g.CreateNode("b");
    scene::NodeHandle hijoDeA = g.CreateChild(a, "a.hijo");

    g.Destroy(a);

    CHECK_FALSE(g.IsValid(a));
    CHECK_FALSE(g.IsValid(hijoDeA));
    CHECK(g.IsValid(b));   // el hermano sobrevive
}

// ---------------------------------------------------------------------------
// Reparentar con guard anti-ciclo
// ---------------------------------------------------------------------------

TEST_CASE("SceneGraph: SetParent mueve el nodo al nuevo padre") {
    scene::SceneGraph g;
    scene::NodeHandle a = g.CreateNode("a");
    scene::NodeHandle b = g.CreateNode("b");
    scene::NodeHandle x = g.CreateChild(a, "x");

    g.SetParent(x, b);

    CHECK(g.Get(x).parent == b);
    CHECK(g.Get(a).children.empty());
    CHECK(g.Get(b).children.size() == 1u);
    CHECK(g.Get(b).children[0] == x);
}

TEST_CASE("SceneGraph: reparentar mantiene el transform local (no preserva mundo)") {
    scene::SceneGraph g;
    scene::NodeHandle a = g.CreateNode("a");
    scene::NodeHandle b = g.CreateNode("b");
    scene::NodeHandle x = g.CreateChild(a, "x");

    math::Transform tx; tx.position = glm::vec3(2.0f, 0.0f, 0.0f);
    g.SetLocalTransform(x, tx);
    g.SetParent(x, b);

    CHECK(g.Get(x).local.position.x == doctest::Approx(2.0f));   // el local no cambio
}

TEST_CASE("SceneGraph: SetParent rechaza crear un ciclo (reparentar a un descendiente)") {
    scene::SceneGraph g;
    scene::NodeHandle a = g.CreateNode("a");
    scene::NodeHandle b = g.CreateChild(a, "b");   // b es hijo de a

    // Intentar hacer a hijo de b crearia un ciclo -> se rechaza (no-op).
    g.SetParent(a, b);

    CHECK(g.Get(a).parent == g.Root());   // a NO se movio
    CHECK(g.Get(b).parent == a);          // b sigue bajo a
}

// ---------------------------------------------------------------------------
// Adjuntos, gather de render y sol-nodo
// ---------------------------------------------------------------------------

#include <efengine/renderer/Mesh.h>
#include <efengine/renderer/Model.h>

namespace {
    // Un Model con meshes vacio no toca GL: sirve como puntero identificable.
    renderer::Model MakeEmptyModelSG() {
        return renderer::Model(std::vector<renderer::Mesh>{});
    }
}

TEST_CASE("SceneGraph: un nodo con malla aparece en Renderables con su world") {
    scene::SceneGraph g;
    renderer::Model model = MakeEmptyModelSG();

    scene::NodeHandle n = g.CreateNode("caja");
    math::Transform t; t.position = glm::vec3(4.0f, 0.0f, 0.0f);
    g.SetLocalTransform(n, t);
    g.AttachMesh(n, { &model, {} });

    g.UpdateWorldTransforms();

    REQUIRE(g.Renderables().size() == 1u);
    CHECK(g.Renderables()[0].model == &model);
    CHECK(glm::vec3(g.Renderables()[0].world[3]).x == doctest::Approx(4.0f));
}

TEST_CASE("SceneGraph: la posicion de una luz punto sale del world del nodo") {
    scene::SceneGraph g;
    scene::NodeHandle padre = g.CreateNode("mano");
    scene::NodeHandle luz   = g.CreateChild(padre, "antorcha");

    math::Transform tp; tp.position = glm::vec3(0.0f, 10.0f, 0.0f);
    g.SetLocalTransform(padre, tp);
    g.AttachLight(luz, { scene::LightKind::Point, glm::vec3(5000.0f) });

    g.UpdateWorldTransforms();

    REQUIRE(g.PointLights().size() == 1u);
    CHECK(g.PointLights()[0].position.y == doctest::Approx(10.0f));   // sigue a la mano
    CHECK(g.PointLights()[0].color.r    == doctest::Approx(5000.0f));
}

TEST_CASE("SceneGraph: el sol primario reporta su direccion desde la rotacion del nodo") {
    scene::SceneGraph g;
    scene::NodeHandle sol = g.CreateNode("sol");
    // Sin rotacion, forward local (0,0,-1) -> direccion (0,0,-1).
    g.AttachLight(sol, { scene::LightKind::Directional, glm::vec3(3.0f) });
    g.SetPrimarySun(sol);

    g.UpdateWorldTransforms();

    CHECK(g.Sun().direction.z == doctest::Approx(-1.0f).epsilon(0.001));
    CHECK(g.Sun().color.r     == doctest::Approx(3.0f));
}

TEST_CASE("SceneGraph: Renderables/PointLights se limpian entre updates") {
    scene::SceneGraph g;
    renderer::Model model = MakeEmptyModelSG();
    scene::NodeHandle n = g.CreateNode("caja");
    g.AttachMesh(n, { &model, {} });

    g.UpdateWorldTransforms();
    g.UpdateWorldTransforms();   // dos veces seguidas

    CHECK(g.Renderables().size() == 1u);   // no se duplica
}

TEST_CASE("SceneGraph::DetachMesh saca la malla y deja hijos y luz intactos") {
    scene::SceneGraph g;
    renderer::Model model = MakeEmptyModelSG();

    scene::NodeHandle h    = g.CreateChild(g.Root(), "conMalla");
    scene::NodeHandle hijo = g.CreateChild(h, "hijo");
    g.AttachMesh(h, { &model, {} });
    g.AttachLight(h, { scene::LightKind::Point, glm::vec3(1.0f) });
    REQUIRE(g.Get(h).mesh.has_value());

    g.DetachMesh(h);

    CHECK_FALSE(g.Get(h).mesh.has_value());
    CHECK(g.Get(h).light.has_value());          // la luz no se toca
    REQUIRE(g.Get(h).children.size() == 1u);    // los hijos tampoco
    CHECK(g.Get(h).children[0] == hijo);
}

TEST_CASE("SceneGraph::DetachMesh saca el nodo de Renderables") {
    scene::SceneGraph g;
    renderer::Model model = MakeEmptyModelSG();

    scene::NodeHandle h = g.CreateNode("conMalla");
    g.AttachMesh(h, { &model, {} });

    g.UpdateWorldTransforms();
    REQUIRE(g.Renderables().size() == 1u);

    g.DetachMesh(h);
    g.UpdateWorldTransforms();
    CHECK(g.Renderables().empty());
}

TEST_CASE("SceneGraph::DetachMesh es no-op con handle invalido o nodo sin malla") {
    scene::SceneGraph g;

    g.DetachMesh(scene::NodeHandle{});          // handle nulo: no debe abortar
    scene::NodeHandle h = g.CreateNode("pelado");
    g.DetachMesh(h);
    CHECK_FALSE(g.Get(h).mesh.has_value());
}
