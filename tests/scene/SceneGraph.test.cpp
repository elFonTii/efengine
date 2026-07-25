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
// Task 3: jerarquia y herencia de transform
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
