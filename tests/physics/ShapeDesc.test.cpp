#include <doctest/doctest.h>
#include <efengine/physics/ShapeDesc.h>

using efengine::physics::ScaleShape;
using efengine::physics::ScaledShape;
using efengine::physics::ShapeDesc;
using efengine::physics::ShapeKind;

namespace {
    ShapeDesc caja(const glm::vec3& halfExtents) {
        ShapeDesc d;
        d.kind   = ShapeKind::Box;
        d.params = halfExtents;
        return d;
    }

    ShapeDesc esfera(f32 radio) {
        ShapeDesc d;
        d.kind   = ShapeKind::Sphere;
        d.params = glm::vec3(radio, 0.0f, 0.0f);
        return d;
    }

    ShapeDesc capsula(f32 radio, f32 medioAlto) {
        ShapeDesc d;
        d.kind   = ShapeKind::Capsule;
        d.params = glm::vec3(radio, medioAlto, 0.0f);
        return d;
    }
}

TEST_CASE("ScaleShape: la caja absorbe la escala eje a eje") {
    const ScaledShape s = ScaleShape(caja(glm::vec3(0.5f, 1.0f, 2.0f)),
                                     glm::vec3(2.0f, 1.0f, 3.0f));

    CHECK(s.desc.params.x == doctest::Approx(1.0f));
    CHECK(s.desc.params.y == doctest::Approx(1.0f));
    CHECK(s.desc.params.z == doctest::Approx(6.0f));
    CHECK(s.residualScale == glm::vec3(1.0f));
    CHECK_FALSE(s.approximated);
    CHECK_FALSE(s.degenerate);
}

TEST_CASE("ScaleShape: esfera con escala uniforme no aproxima nada") {
    const ScaledShape s = ScaleShape(esfera(0.5f), glm::vec3(3.0f));

    CHECK(s.desc.params.x == doctest::Approx(1.5f));
    CHECK_FALSE(s.approximated);
    CHECK_FALSE(s.degenerate);
}

TEST_CASE("ScaleShape: esfera con escala no uniforme toma el eje dominante y avisa") {
    // Una esfera escalada 2x1x3 no existe en Jolt: se envuelve la mas grande
    // que contiene a la malla, y approximated es lo que dispara el warning.
    const ScaledShape s = ScaleShape(esfera(0.5f), glm::vec3(2.0f, 1.0f, 3.0f));

    CHECK(s.desc.params.x == doctest::Approx(1.5f));
    CHECK(s.approximated);
    CHECK_FALSE(s.degenerate);
}

TEST_CASE("ScaleShape: capsula con x != z colapsa el radio al mayor y avisa") {
    const ScaledShape s = ScaleShape(capsula(0.3f, 0.9f), glm::vec3(2.0f, 4.0f, 1.0f));

    CHECK(s.desc.params.x == doctest::Approx(0.6f));   // radio: max(2, 1) = 2
    CHECK(s.desc.params.y == doctest::Approx(3.6f));   // medio alto: 4
    CHECK(s.approximated);
}

TEST_CASE("ScaleShape: capsula con x == z no aproxima aunque y sea distinto") {
    const ScaledShape s = ScaleShape(capsula(0.3f, 0.9f), glm::vec3(2.0f, 5.0f, 2.0f));

    CHECK(s.desc.params.x == doctest::Approx(0.6f));
    CHECK(s.desc.params.y == doctest::Approx(4.5f));
    CHECK_FALSE(s.approximated);
}

TEST_CASE("ScaleShape: la escala negativa se toma en valor absoluto") {
    const ScaledShape s = ScaleShape(caja(glm::vec3(1.0f)), glm::vec3(-2.0f, 3.0f, -4.0f));

    CHECK(s.desc.params.x == doctest::Approx(2.0f));
    CHECK(s.desc.params.y == doctest::Approx(3.0f));
    CHECK(s.desc.params.z == doctest::Approx(4.0f));
    CHECK_FALSE(s.degenerate);
}

TEST_CASE("ScaleShape: un eje de escala en cero deja la forma degenerada") {
    const ScaledShape s = ScaleShape(caja(glm::vec3(1.0f)), glm::vec3(1.0f, 0.0f, 1.0f));

    CHECK(s.degenerate);
}

TEST_CASE("ScaleShape: un parametro en cero tambien deja la forma degenerada") {
    // Una caja de espesor cero o una capsula de medio alto cero no existen,
    // aunque la escala sea perfectamente valida.
    CHECK(ScaleShape(caja(glm::vec3(1.0f, 0.0f, 1.0f)), glm::vec3(1.0f)).degenerate);
    CHECK(ScaleShape(esfera(0.0f), glm::vec3(1.0f)).degenerate);
    CHECK(ScaleShape(capsula(0.3f, 0.0f), glm::vec3(1.0f)).degenerate);
}

TEST_CASE("ScaleShape: la malla no absorbe la escala, la deja en residualScale") {
    ShapeDesc d;
    d.kind   = ShapeKind::Mesh;
    d.params = glm::vec3(0.5f);

    const ScaledShape s = ScaleShape(d, glm::vec3(2.0f, 1.0f, 3.0f));

    CHECK(s.desc.params == glm::vec3(0.5f));           // intacto
    CHECK(s.residualScale.x == doctest::Approx(2.0f));
    CHECK(s.residualScale.y == doctest::Approx(1.0f));
    CHECK(s.residualScale.z == doctest::Approx(3.0f));
    CHECK_FALSE(s.approximated);                        // la malla si escala libre
}
