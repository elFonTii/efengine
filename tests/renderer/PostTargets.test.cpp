// Ruteo del ping-pong del post: logica pura, sin GL.
#include <doctest/doctest.h>
#include <efengine/renderer/PostTargets.h>

using efengine::renderer::PingPong;

TEST_CASE("PingPong: arranca leyendo el color de escena") {
    PingPong p;
    p.Begin();
    CHECK(p.current == -1);
}

TEST_CASE("PingPong: alterna A y B y nunca escribe lo que lee") {
    PingPong p;
    p.Begin();
    CHECK(p.Acquire() == 0u);
    p.Publish();
    CHECK(p.current == 0);
    CHECK(p.Acquire() == 1u);
    p.Publish();
    CHECK(p.current == 1);
    CHECK(p.Acquire() == 0u);
    p.Publish();
    CHECK(p.current == 0);
}

TEST_CASE("PingPong: todo el post apagado deja la escena como actual") {
    PingPong p;
    p.Begin();
    p.Acquire(); p.Publish();
    p.Begin();                  // frame siguiente, ningun pase de post corre
    CHECK(p.current == -1);
}

TEST_CASE("PingPong: pedir dos veces sin publicar da el mismo target") {
    PingPong p;
    p.Begin();
    CHECK(p.Acquire() == 0u);
    CHECK(p.Acquire() == 0u);
}
