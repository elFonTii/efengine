#include <doctest/doctest.h>
#include <efenet/World.h>

using namespace efenet;

TEST_CASE("efenet::World arranca vacio") {
    World w;
    CHECK(w.Count() == 0u);
    CHECK(w.Players().empty());
    CHECK_FALSE(w.Full());
}

TEST_CASE("efenet::World Add asigna ids que arrancan en 1") {
    // El 0 es kInvalidPlayer: ningun jugador real lo usa.
    World w;
    CHECK(w.Add(glm::vec3(0.0f)) == 1u);
    CHECK(w.Add(glm::vec3(0.0f)) == 2u);
    CHECK(w.Count() == 2u);
}

TEST_CASE("efenet::World Add guarda la posicion de spawn") {
    World w;
    const PlayerId id = w.Add(glm::vec3(3.0f, 0.0f, -4.0f));

    const PlayerState* p = w.Find(id);
    REQUIRE(p != null);
    CHECK(p->position.x == doctest::Approx(3.0f));
    CHECK(p->position.z == doctest::Approx(-4.0f));
    CHECK(p->id == id);
}

TEST_CASE("efenet::World se llena en kMaxPlayers") {
    World w;
    for (u32 i = 0u; i < kMaxPlayers; ++i) {
        CHECK(w.Add(glm::vec3(0.0f)) != kInvalidPlayer);
    }
    CHECK(w.Full());
    CHECK(w.Add(glm::vec3(0.0f)) == kInvalidPlayer);
    CHECK(w.Count() == kMaxPlayers);
}

TEST_CASE("efenet::World Find de un id inexistente devuelve null") {
    World w;
    w.Add(glm::vec3(0.0f));
    CHECK(w.Find(999u) == null);
    CHECK(w.Find(kInvalidPlayer) == null);
}

TEST_CASE("efenet::World Remove saca al jugador") {
    World w;
    const PlayerId a = w.Add(glm::vec3(1.0f, 0.0f, 0.0f));
    const PlayerId b = w.Add(glm::vec3(2.0f, 0.0f, 0.0f));

    CHECK(w.Remove(a));
    CHECK(w.Count() == 1u);
    CHECK(w.Find(a) == null);

    // El que queda no se corrompio al compactar el array.
    const PlayerState* pb = w.Find(b);
    REQUIRE(pb != null);
    CHECK(pb->position.x == doctest::Approx(2.0f));
}

TEST_CASE("efenet::World Remove de un id inexistente devuelve false") {
    World w;
    CHECK_FALSE(w.Remove(42u));
}

TEST_CASE("efenet::World los ids no se reciclan al liberarse un lugar") {
    // Si se reciclaran, un cliente con snapshots viejos en vuelo podria
    // atribuirle a un jugador nuevo la posicion del que se fue.
    World w;
    const PlayerId a = w.Add(glm::vec3(0.0f));
    w.Remove(a);
    const PlayerId b = w.Add(glm::vec3(0.0f));

    CHECK(b != a);
}

TEST_CASE("efenet::World se puede volver a llenar despues de vaciarlo") {
    World w;
    for (u32 i = 0u; i < kMaxPlayers; ++i) w.Add(glm::vec3(0.0f));
    CHECK(w.Full());

    // Sacar a todos, uno por uno, tomando siempre el primero que quede.
    while (w.Count() > 0u) {
        const PlayerId primero = w.Players()[0].id;
        CHECK(w.Remove(primero));
    }
    CHECK(w.Count() == 0u);
    CHECK_FALSE(w.Full());

    CHECK(w.Add(glm::vec3(0.0f)) != kInvalidPlayer);
}
