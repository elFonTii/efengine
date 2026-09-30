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

TEST_CASE("PingPong: una publicacion externa no es ninguno de los dos targets") {
    PingPong p;
    p.Begin();
    p.PublishExternal();
    CHECK(p.current == PingPong::kExternal);
    CHECK(p.Acquire() == 0u);
    p.Publish();
    CHECK(p.current == 0);
    CHECK(p.Acquire() == 1u);
}

TEST_CASE("PingPong: la publicacion externa despues de un target propio") {
    PingPong p;
    p.Begin();
    p.Acquire(); p.Publish();
    CHECK(p.current == 0);
    p.PublishExternal();
    CHECK(p.current == PingPong::kExternal);
    CHECK(p.Acquire() == 0u);
}

TEST_CASE("PingPong: Begin olvida la publicacion externa") {
    PingPong p;
    p.Begin();
    p.PublishExternal();
    p.Begin();
    CHECK(p.current == -1);
}

TEST_CASE("PingPong: PublishExternal descarta un Acquire sin publicar") {
    PingPong p;
    p.Begin();
    p.Acquire();
    p.PublishExternal();
    p.Publish();
    CHECK(p.current == PingPong::kExternal);
}
