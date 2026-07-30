// Escrito por Claude Opus 5
#include <doctest/doctest.h>
#include <efengine/renderer/Bounds.h>
#include <efengine/renderer/Vertex.h>
#include <glm/gtc/matrix_transform.hpp>
#include <cmath>
#include <vector>

using namespace efengine;
using efengine::renderer::AABB;
using efengine::renderer::ComputeBounds;

namespace {
    // ComputeBounds solo mira position; el resto del vertice queda en cero.
    renderer::Vertex vtx(f32 x, f32 y, f32 z) {
        renderer::Vertex v {};
        v.position = glm::vec3(x, y, z);
        return v;
    }

    AABB caja(glm::vec3 mn, glm::vec3 mx) {
        AABB b;
        b.min = mn;
        b.max = mx;
        return b;
    }
}

TEST_CASE("ComputeBounds: los 8 vertices de un cubo dan el cubo") {
    const std::vector<renderer::Vertex> verts = {
        vtx(-1.0f, -1.0f, -1.0f), vtx(1.0f, -1.0f, -1.0f),
        vtx( 1.0f,  1.0f, -1.0f), vtx(-1.0f, 1.0f, -1.0f),
        vtx(-1.0f, -1.0f,  1.0f), vtx(1.0f, -1.0f,  1.0f),
        vtx( 1.0f,  1.0f,  1.0f), vtx(-1.0f, 1.0f,  1.0f),
    };
    const AABB b = ComputeBounds(verts);

    CHECK(b.Valid());
    CHECK(b.min.x == doctest::Approx(-1.0f));
    CHECK(b.min.y == doctest::Approx(-1.0f));
    CHECK(b.min.z == doctest::Approx(-1.0f));
    CHECK(b.max.x == doctest::Approx(1.0f));
    CHECK(b.max.y == doctest::Approx(1.0f));
    CHECK(b.max.z == doctest::Approx(1.0f));
}

TEST_CASE("ComputeBounds: sin vertices devuelve una AABB invalida") {
    const AABB b = ComputeBounds({});
    CHECK_FALSE(b.Valid());
}

TEST_CASE("AABB::Merge: Empty() es el elemento neutro") {
    const AABB box = caja(glm::vec3(-2.0f, 0.0f, 1.0f), glm::vec3(3.0f, 4.0f, 5.0f));
    const AABB m   = AABB::Merge(AABB::Empty(), box);

    CHECK(m.Valid());
    CHECK(m.min.x == doctest::Approx(-2.0f));
    CHECK(m.min.y == doctest::Approx(0.0f));
    CHECK(m.min.z == doctest::Approx(1.0f));
    CHECK(m.max.x == doctest::Approx(3.0f));
    CHECK(m.max.y == doctest::Approx(4.0f));
    CHECK(m.max.z == doctest::Approx(5.0f));
}

TEST_CASE("AABB::Merge: une dos cajas disjuntas") {
    const AABB a = caja(glm::vec3(0.0f), glm::vec3(1.0f));
    const AABB b = caja(glm::vec3(5.0f), glm::vec3(6.0f));
    const AABB m = AABB::Merge(a, b);

    CHECK(m.min.x == doctest::Approx(0.0f));
    CHECK(m.max.x == doctest::Approx(6.0f));
}

TEST_CASE("AABB: Center y Radius de una caja asimetrica") {
    const AABB b = caja(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(4.0f, 2.0f, 6.0f));

    CHECK(b.Center().x == doctest::Approx(2.0f));
    CHECK(b.Center().y == doctest::Approx(1.0f));
    CHECK(b.Center().z == doctest::Approx(3.0f));
    CHECK(b.Extents().x == doctest::Approx(2.0f));
    CHECK(b.Extents().y == doctest::Approx(1.0f));
    CHECK(b.Extents().z == doctest::Approx(3.0f));
    // media diagonal = |(2,1,3)| = sqrt(14)
    CHECK(b.Radius() == doctest::Approx(std::sqrt(14.0f)).epsilon(0.001));
}

TEST_CASE("AABB::Transformed: la traslacion mueve la caja y no la agranda") {
    const AABB b = caja(glm::vec3(-1.0f), glm::vec3(1.0f));
    const AABB t = b.Transformed(glm::translate(glm::mat4(1.0f), glm::vec3(10.0f, 0.0f, 0.0f)));

    CHECK(t.min.x == doctest::Approx(9.0f));
    CHECK(t.max.x == doctest::Approx(11.0f));
    CHECK(t.Extents().x == doctest::Approx(1.0f));
    CHECK(t.Extents().y == doctest::Approx(1.0f));
    CHECK(t.Extents().z == doctest::Approx(1.0f));
}

TEST_CASE("AABB::Transformed: rotar 45 grados en Y agranda la caja") {
    // Propiedad clave y contraintuitiva: la AABB de una caja rotada es mas
    // grande que la original. Un cubo de extent 1 rotado 45 en Y pasa a
    // extent |cos45| + |sin45| = sqrt(2) en X y Z, y queda igual en Y.
    const AABB b = caja(glm::vec3(-1.0f), glm::vec3(1.0f));
    const AABB t = b.Transformed(
        glm::rotate(glm::mat4(1.0f), glm::radians(45.0f), glm::vec3(0.0f, 1.0f, 0.0f)));

    CHECK(t.Extents().x == doctest::Approx(std::sqrt(2.0f)).epsilon(0.001));
    CHECK(t.Extents().y == doctest::Approx(1.0f).epsilon(0.001));
    CHECK(t.Extents().z == doctest::Approx(std::sqrt(2.0f)).epsilon(0.001));
}

TEST_CASE("AABB::Transformed: la escala escala los extents") {
    const AABB b = caja(glm::vec3(-1.0f), glm::vec3(1.0f));
    const AABB t = b.Transformed(glm::scale(glm::mat4(1.0f), glm::vec3(2.0f, 3.0f, 4.0f)));

    CHECK(t.Extents().x == doctest::Approx(2.0f));
    CHECK(t.Extents().y == doctest::Approx(3.0f));
    CHECK(t.Extents().z == doctest::Approx(4.0f));
}

TEST_CASE("AccumulateBounds: sobre un acumulador vacio da el local transformado") {
    AABB local;
    local.min = glm::vec3(-1.0f);
    local.max = glm::vec3( 1.0f);

    const glm::mat4 world = glm::translate(glm::mat4(1.0f), glm::vec3(5.0f, 0.0f, 0.0f));
    const AABB out = renderer::AccumulateBounds(AABB::Empty(), local, world);

    CHECK(out.min.x == doctest::Approx(4.0f));
    CHECK(out.max.x == doctest::Approx(6.0f));
    CHECK(out.min.y == doctest::Approx(-1.0f));
    CHECK(out.max.y == doctest::Approx( 1.0f));
}

TEST_CASE("AccumulateBounds: dos items dan la union") {
    AABB unidad;
    unidad.min = glm::vec3(-1.0f);
    unidad.max = glm::vec3( 1.0f);

    AABB acc = AABB::Empty();
    acc = renderer::AccumulateBounds(acc, unidad, glm::translate(glm::mat4(1.0f), glm::vec3(-5.0f, 0.0f, 0.0f)));
    acc = renderer::AccumulateBounds(acc, unidad, glm::translate(glm::mat4(1.0f), glm::vec3( 5.0f, 0.0f, 0.0f)));

    CHECK(acc.min.x == doctest::Approx(-6.0f));
    CHECK(acc.max.x == doctest::Approx( 6.0f));
}

TEST_CASE("AccumulateBounds: un local invalido se ignora") {
    // Transformed() sobre una AABB vacia da NaN (sus esquinas son +inf y -inf).
    // Si esto no se filtra, un solo Model sin vertices envenena la caja entera y
    // el gizmo del sol y el encaje de la grilla dejan de funcionar.
    AABB unidad;
    unidad.min = glm::vec3(-1.0f);
    unidad.max = glm::vec3( 1.0f);

    AABB acc = renderer::AccumulateBounds(AABB::Empty(), unidad, glm::mat4(1.0f));
    acc = renderer::AccumulateBounds(acc, AABB::Empty(), glm::mat4(1.0f));

    CHECK(acc.min.x == doctest::Approx(-1.0f));
    CHECK(acc.max.x == doctest::Approx( 1.0f));
    CHECK(std::isnan(acc.min.x) == false);
    CHECK(std::isnan(acc.max.x) == false);
}

TEST_CASE("AccumulateBounds: acumulador vacio y local vacio sigue vacio") {
    const AABB out = renderer::AccumulateBounds(AABB::Empty(), AABB::Empty(), glm::mat4(1.0f));
    CHECK_FALSE(out.Valid());
}

TEST_CASE("AccumulateBounds: rotar 45 grados agranda la caja") {
    AABB unidad;
    unidad.min = glm::vec3(-1.0f);
    unidad.max = glm::vec3( 1.0f);

    const glm::mat4 rot = glm::rotate(glm::mat4(1.0f), glm::radians(45.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    const AABB out = renderer::AccumulateBounds(AABB::Empty(), unidad, rot);

    // Una caja de lado 2 rotada 45 grados en Y tiene medio-ancho sqrt(2) en X y Z.
    CHECK(out.max.x == doctest::Approx(std::sqrt(2.0f)).epsilon(0.001));
    CHECK(out.max.y == doctest::Approx(1.0f).epsilon(0.001));
}
