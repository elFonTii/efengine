#include <doctest/doctest.h>
#include <efengine/scene/NodeHandle.h>

using namespace efengine;

TEST_CASE("SceneGraph: NodeHandle por defecto es nulo") {
    scene::NodeHandle h;
    CHECK(h.IsNull());
    CHECK(h.generation == 0u);
}

TEST_CASE("SceneGraph: un handle con generation >= 1 no es nulo") {
    scene::NodeHandle h { 0u, 1u };
    CHECK_FALSE(h.IsNull());
}

TEST_CASE("SceneGraph: igualdad de handles compara index y generation") {
    scene::NodeHandle a { 3u, 2u };
    scene::NodeHandle b { 3u, 2u };
    scene::NodeHandle c { 3u, 5u };   // misma ranura, otra generación
    CHECK(a == b);
    CHECK(a != c);
}
