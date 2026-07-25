#include <doctest/doctest.h>
#include <efengine/math/Aabb.h>
#include <glm/gtc/matrix_transform.hpp>
#include <cmath>

using namespace efengine::math;

TEST_CASE("Aabb: el default es vacío y de puntos no lo es") {
    CHECK(AabbIsEmpty(Aabb{}));

    const glm::vec3 pts[2] = { { -1.0f, 0.0f, 2.0f }, { 3.0f, -2.0f, 5.0f } };
    Aabb box = AabbFromPoints(pts, 2);
    CHECK(!AabbIsEmpty(box));
    CHECK(box.min.x == doctest::Approx(-1.0f));
    CHECK(box.min.y == doctest::Approx(-2.0f));
    CHECK(box.max.z == doctest::Approx(5.0f));
}

TEST_CASE("Aabb: AabbFromPoints con entrada nula o vacía devuelve vacío") {
    CHECK(AabbIsEmpty(AabbFromPoints(null, 0)));
    const glm::vec3 p { 1.0f };
    CHECK(AabbIsEmpty(AabbFromPoints(&p, 0)));
}

TEST_CASE("Aabb: merge ignora cajas vacías y une las válidas") {
    Aabb a { { 0.0f, 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f } };
    Aabb b { { -2.0f, 0.5f, 0.0f }, { 0.5f, 3.0f, 1.0f } };

    Aabb ab = MergeAabb(a, b);
    CHECK(ab.min.x == doctest::Approx(-2.0f));
    CHECK(ab.max.y == doctest::Approx(3.0f));

    // El neutro del merge es la caja vacía, por ambos lados.
    Aabb conVacia = MergeAabb(a, Aabb{});
    CHECK(conVacia.min.x == doctest::Approx(a.min.x));
    CHECK(conVacia.max.x == doctest::Approx(a.max.x));
    conVacia = MergeAabb(Aabb{}, a);
    CHECK(conVacia.max.z == doctest::Approx(a.max.z));
}

TEST_CASE("Aabb: transformar con la identidad no cambia la caja") {
    Aabb box { { -1.0f, -2.0f, -3.0f }, { 1.0f, 2.0f, 3.0f } };
    Aabb t = TransformAabb(glm::mat4(1.0f), box);
    CHECK(t.min.x == doctest::Approx(box.min.x));
    CHECK(t.max.z == doctest::Approx(box.max.z));
}

TEST_CASE("Aabb: rotar 45° un cubo unitario lo agranda a sqrt(2) en el plano") {
    Aabb cubo { glm::vec3(-0.5f), glm::vec3(0.5f) };
    glm::mat4 rot = glm::rotate(glm::mat4(1.0f), glm::radians(45.0f), glm::vec3(0.0f, 1.0f, 0.0f));

    Aabb t = TransformAabb(rot, cubo);
    const f32 half = std::sqrt(2.0f) * 0.5f;
    CHECK(t.max.x == doctest::Approx(half).epsilon(0.001));
    CHECK(t.max.z == doctest::Approx(half).epsilon(0.001));
    CHECK(t.max.y == doctest::Approx(0.5f).epsilon(0.001)); // el eje de rotación no cambia
}

TEST_CASE("Aabb: la traslación mueve centro y conserva extents") {
    Aabb box { glm::vec3(-1.0f), glm::vec3(1.0f) };
    glm::mat4 tr = glm::translate(glm::mat4(1.0f), glm::vec3(10.0f, 0.0f, -5.0f));

    Aabb t = TransformAabb(tr, box);
    glm::vec3 centro = AabbCenter(t);
    glm::vec3 ext    = AabbExtents(t);
    CHECK(centro.x == doctest::Approx(10.0f));
    CHECK(centro.z == doctest::Approx(-5.0f));
    CHECK(ext.x == doctest::Approx(1.0f));
    CHECK(ext.y == doctest::Approx(1.0f));
}
