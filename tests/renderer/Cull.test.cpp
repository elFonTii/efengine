// El culling es geometría de índices: un signo cambiado no se ve como un error,
// se ve como geometría que desaparece de la captura de un probe y como una
// sombra de GI que nadie sabe de dónde salió.
#include <doctest/doctest.h>
#include <efengine/renderer/Cull.h>

#include <limits>
#include <vector>

using namespace efengine;
using namespace efengine::renderer;

namespace {
    AABB Caja(glm::vec3 min, glm::vec3 max) {
        AABB a;
        a.min = min;
        a.max = max;
        return a;
    }

    MeshSpan Span(u32 item, u32 mesh, AABB bounds) {
        MeshSpan s;
        s.item   = item;
        s.mesh   = mesh;
        s.bounds = bounds;
        return s;
    }
}

TEST_CASE("Overlaps: cajas separadas en un solo eje no solapan") {
    const AABB a = Caja(glm::vec3(0.0f), glm::vec3(1.0f));
    CHECK_FALSE(Overlaps(a, Caja(glm::vec3(2.0f, 0.0f, 0.0f), glm::vec3(3.0f, 1.0f, 1.0f))));
    CHECK_FALSE(Overlaps(a, Caja(glm::vec3(0.0f, 2.0f, 0.0f), glm::vec3(1.0f, 3.0f, 1.0f))));
    CHECK_FALSE(Overlaps(a, Caja(glm::vec3(0.0f, 0.0f, 2.0f), glm::vec3(1.0f, 1.0f, 3.0f))));
}

TEST_CASE("Overlaps: el contacto exacto en una cara cuenta como solape") {
    // Inclusivo a propósito. Con un test exclusivo, la geometría que apoya
    // justo sobre el plano del volumen desaparece de la captura, y eso es una
    // pared que deja de bloquear luz.
    const AABB a = Caja(glm::vec3(0.0f), glm::vec3(1.0f));
    const AABB b = Caja(glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(2.0f, 1.0f, 1.0f));
    CHECK(Overlaps(a, b));
}

TEST_CASE("Overlaps: una caja contenida solapa, y es simétrico") {
    const AABB grande  = Caja(glm::vec3(-10.0f), glm::vec3(10.0f));
    const AABB chica   = Caja(glm::vec3(-1.0f),  glm::vec3(1.0f));
    CHECK(Overlaps(grande, chica));
    CHECK(Overlaps(chica, grande));
}

TEST_CASE("Overlaps: una AABB invalida nunca solapa") {
    // AABB::Empty() es min=+inf, max=-inf. Sin el chequeo explícito, el test de
    // slabs daría inf <= -inf == false por casualidad, pero una caja a medio
    // construir (min > max en un solo eje) podría colarse.
    const AABB valida = Caja(glm::vec3(0.0f), glm::vec3(1.0f));
    CHECK_FALSE(Overlaps(valida, AABB::Empty()));
    CHECK_FALSE(Overlaps(AABB::Empty(), valida));

    AABB rota = Caja(glm::vec3(0.0f), glm::vec3(1.0f));
    rota.min.y = 5.0f;
    CHECK_FALSE(Overlaps(valida, rota));
}

TEST_CASE("CullAabb: devuelve los indices de los spans que solapan") {
    std::vector<MeshSpan> spans {
        Span(0u, 0u, Caja(glm::vec3(0.0f),  glm::vec3(1.0f))),    // adentro
        Span(0u, 1u, Caja(glm::vec3(50.0f), glm::vec3(51.0f))),   // lejos
        Span(1u, 0u, Caja(glm::vec3(2.0f),  glm::vec3(3.0f))),    // adentro
    };

    std::vector<u32> out;
    CullAabb(spans, Caja(glm::vec3(-5.0f), glm::vec3(5.0f)), out);

    REQUIRE(out.size() == 2u);
    CHECK(out[0] == 0u);
    CHECK(out[1] == 2u);
}

TEST_CASE("CullAabb: un volumen que contiene todo devuelve todos, en orden") {
    // Esta es la invariante de no-regresión del ciclo: con el volumen en
    // infinito la captura tiene que emitir las mismas submallas en el mismo
    // orden que antes del culling.
    std::vector<MeshSpan> spans;
    for (u32 i = 0u; i < 10u; ++i) {
        spans.push_back(Span(0u, i, Caja(glm::vec3(f32(i)), glm::vec3(f32(i) + 1.0f))));
    }

    const f32 inf = std::numeric_limits<f32>::infinity();
    std::vector<u32> out;
    CullAabb(spans, Caja(glm::vec3(-inf), glm::vec3(inf)), out);

    REQUIRE(out.size() == 10u);
    for (u32 i = 0u; i < 10u; ++i) CHECK(out[i] == i);
}

TEST_CASE("CullAabb: limpia la salida antes de escribir") {
    std::vector<MeshSpan> spans { Span(0u, 0u, Caja(glm::vec3(0.0f), glm::vec3(1.0f))) };

    std::vector<u32> out { 99u, 98u, 97u };
    CullAabb(spans, Caja(glm::vec3(100.0f), glm::vec3(101.0f)), out);

    CHECK(out.empty());
}

TEST_CASE("CullAabb: lista vacia no revienta") {
    std::vector<MeshSpan> spans;
    std::vector<u32> out;
    CullAabb(spans, AABB::Empty(), out);
    CHECK(out.empty());
}
