#include <doctest/doctest.h>
#include <efengine/scene/ComponentStore.h>

#include <string>

using namespace efengine;

namespace {

    struct Masa   { f32 kg = 1.0f; };
    struct Etiqueta { std::string texto; };

}

TEST_CASE("ComponentStore: un store vacio no tiene nada de ningun tipo") {
    scene::ComponentStore store;
    CHECK(store.TryGet<Masa>(0u) == null);
    CHECK_FALSE(store.Has<Masa>(0u));
    // Preguntar por un tipo que nunca se seteo no puede crashear ni crear el array.
    CHECK_FALSE(store.Has<Etiqueta>(9999u));
}

TEST_CASE("ComponentStore: Set y TryGet sobre el mismo indice") {
    scene::ComponentStore store;
    store.Set<Masa>(3u, Masa{ 7.5f });

    REQUIRE(store.Has<Masa>(3u));
    const Masa* m = store.TryGet<Masa>(3u);
    REQUIRE(m != null);
    CHECK(m->kg == doctest::Approx(7.5f));

    // Los indices vecinos siguen vacios: el array es disperso, no denso.
    CHECK_FALSE(store.Has<Masa>(2u));
    CHECK_FALSE(store.Has<Masa>(4u));
}

TEST_CASE("ComponentStore: Set dos veces pisa el valor") {
    scene::ComponentStore store;
    store.Set<Masa>(1u, Masa{ 1.0f });
    store.Set<Masa>(1u, Masa{ 2.0f });
    REQUIRE(store.TryGet<Masa>(1u) != null);
    CHECK(store.TryGet<Masa>(1u)->kg == doctest::Approx(2.0f));
}

TEST_CASE("ComponentStore: TryGet no const permite mutar en el lugar") {
    scene::ComponentStore store;
    store.Set<Masa>(0u, Masa{ 1.0f });
    store.TryGet<Masa>(0u)->kg = 42.0f;
    CHECK(store.TryGet<Masa>(0u)->kg == doctest::Approx(42.0f));
}

TEST_CASE("ComponentStore: dos tipos distintos no se pisan en el mismo indice") {
    scene::ComponentStore store;
    store.Set<Masa>(5u, Masa{ 3.0f });
    store.Set<Etiqueta>(5u, Etiqueta{ "caja" });

    REQUIRE(store.TryGet<Masa>(5u) != null);
    REQUIRE(store.TryGet<Etiqueta>(5u) != null);
    CHECK(store.TryGet<Masa>(5u)->kg == doctest::Approx(3.0f));
    CHECK(store.TryGet<Etiqueta>(5u)->texto == "caja");

    store.Remove<Masa>(5u);
    CHECK_FALSE(store.Has<Masa>(5u));
    CHECK(store.Has<Etiqueta>(5u));   // el otro tipo ni se entero
}

TEST_CASE("ComponentStore: Remove de algo que no esta es no-op") {
    scene::ComponentStore store;
    store.Remove<Masa>(0u);            // tipo nunca usado
    store.Set<Masa>(0u, Masa{ 1.0f });
    store.Remove<Masa>(77u);           // indice fuera del array
    CHECK(store.Has<Masa>(0u));
}

TEST_CASE("ComponentStore: ResetNode borra todos los tipos de ese indice") {
    scene::ComponentStore store;
    store.Set<Masa>(2u, Masa{ 1.0f });
    store.Set<Etiqueta>(2u, Etiqueta{ "x" });
    store.Set<Masa>(3u, Masa{ 9.0f });

    store.ResetNode(2u);

    CHECK_FALSE(store.Has<Masa>(2u));
    CHECK_FALSE(store.Has<Etiqueta>(2u));
    // Es la garantia que hace seguro reusar un indice del free list: el nodo
    // nuevo no hereda nada del muerto, pero los otros nodos no se tocan.
    CHECK(store.Has<Masa>(3u));
}

TEST_CASE("ComponentStore: ResetNode de un indice fuera de rango no crashea") {
    scene::ComponentStore store;
    store.Set<Masa>(0u, Masa{ 1.0f });
    store.ResetNode(12345u);
    CHECK(store.Has<Masa>(0u));
}

TEST_CASE("ComponentStore: Clear deja el store como recien construido") {
    scene::ComponentStore store;
    store.Set<Masa>(1u, Masa{ 1.0f });
    store.Set<Etiqueta>(1u, Etiqueta{ "y" });

    store.Clear();

    CHECK_FALSE(store.Has<Masa>(1u));
    CHECK_FALSE(store.Has<Etiqueta>(1u));
    // Y sigue usable despues del Clear.
    store.Set<Masa>(1u, Masa{ 4.0f });
    CHECK(store.TryGet<Masa>(1u)->kg == doctest::Approx(4.0f));
}

TEST_CASE("ComponentStore: Reserve no inventa componentes") {
    scene::ComponentStore store;
    store.Set<Masa>(0u, Masa{ 1.0f });
    store.Reserve(64u);

    CHECK(store.Has<Masa>(0u));
    // Dimensionar el array no es lo mismo que poblarlo.
    CHECK_FALSE(store.Has<Masa>(10u));
    CHECK(store.TryGet<Masa>(63u) == null);
}

TEST_CASE("ComponentStore: un tipo estrenado despues de Reserve arranca dimensionado") {
    scene::ComponentStore store;
    store.Reserve(32u);
    store.Set<Etiqueta>(31u, Etiqueta{ "z" });
    REQUIRE(store.TryGet<Etiqueta>(31u) != null);
    CHECK(store.TryGet<Etiqueta>(31u)->texto == "z");
}

TEST_CASE("ComponentStore: Set en un indice alto hace crecer el array solo") {
    scene::ComponentStore store;
    store.Set<Masa>(1000u, Masa{ 5.0f });
    REQUIRE(store.TryGet<Masa>(1000u) != null);
    CHECK(store.TryGet<Masa>(1000u)->kg == doctest::Approx(5.0f));
    CHECK_FALSE(store.Has<Masa>(999u));
}

TEST_CASE("ComponentStore: el store const solo lee") {
    scene::ComponentStore store;
    store.Set<Masa>(4u, Masa{ 2.5f });

    const scene::ComponentStore& ro = store;
    REQUIRE(ro.TryGet<Masa>(4u) != null);
    CHECK(ro.TryGet<Masa>(4u)->kg == doctest::Approx(2.5f));
    CHECK(ro.TryGet<Etiqueta>(4u) == null);
    CHECK_FALSE(ro.Has<Etiqueta>(4u));
}
