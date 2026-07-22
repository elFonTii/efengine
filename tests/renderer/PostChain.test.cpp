// tests/renderer/PostChain.test.cpp
// Valida el ruteo de targets de la PostChain: qué pase escribe a un scratch
// (y cuál) y cuál va al backbuffer. Lógica pura, sin contexto OpenGL.
#include <doctest/doctest.h>
#include <efengine/renderer/PostChain.h>

using efengine::renderer::PassRoute;
using efengine::renderer::postChainTarget;

TEST_CASE("postChainTarget: un solo pase va directo al backbuffer") {
    PassRoute r = postChainTarget(0, 1);
    CHECK(r.toBackbuffer == true);
}

TEST_CASE("postChainTarget: dos pases -> el 1ro a un scratch, el ultimo al backbuffer") {
    PassRoute a = postChainTarget(0, 2);
    CHECK(a.toBackbuffer == false);
    CHECK(a.scratch == 0u);

    PassRoute b = postChainTarget(1, 2);
    CHECK(b.toBackbuffer == true);
}

TEST_CASE("postChainTarget: tres pases -> los scratch alternan 0/1 (ping-pong)") {
    CHECK(postChainTarget(0, 3).toBackbuffer == false);
    CHECK(postChainTarget(0, 3).scratch == 0u);
    CHECK(postChainTarget(1, 3).toBackbuffer == false);
    CHECK(postChainTarget(1, 3).scratch == 1u);
    CHECK(postChainTarget(2, 3).toBackbuffer == true);
}
