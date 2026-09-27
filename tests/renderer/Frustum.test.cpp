#include <doctest/doctest.h>
#include <efengine/renderer/Frustum.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

using namespace efengine;
using namespace efengine::renderer;

namespace {
    // Camara en el origen mirando a -Z, 90 grados, aspecto 1: a z = -10 el
    // volumen mide 10 m hacia cada lado.
    Frustum FrustumDePrueba() {
        const glm::mat4 proj = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 100.0f);
        return ExtractFrustum(proj);
    }
}

TEST_CASE("Frustum: los planos salen normalizados") {
    const Frustum f = FrustumDePrueba();
    for (const glm::vec4& p : f.planes) CHECK(glm::length(glm::vec3(p)) == doctest::Approx(1.0f));
}

TEST_CASE("Frustum: una esfera delante de la camara es visible") {
    CHECK(SphereInFrustum(FrustumDePrueba(), { glm::vec3(0.0f, 0.0f, -10.0f), 1.0f }));
}

TEST_CASE("Frustum: detras de la camara y mas alla del far no es visible") {
    const Frustum f = FrustumDePrueba();
    CHECK_FALSE(SphereInFrustum(f, { glm::vec3(0.0f, 0.0f, 10.0f), 1.0f }));
    CHECK_FALSE(SphereInFrustum(f, { glm::vec3(0.0f, 0.0f, -200.0f), 1.0f }));
    CHECK(SphereInFrustum(f, { glm::vec3(0.0f, 0.0f, -200.0f), 150.0f }));
}

TEST_CASE("Frustum: al costado cuenta la distancia al plano en metros") {
    const Frustum f = FrustumDePrueba();
    CHECK_FALSE(SphereInFrustum(f, { glm::vec3(12.0f, 0.0f, -10.0f), 1.0f }));
    CHECK(SphereInFrustum(f, { glm::vec3(12.0f, 0.0f, -10.0f), 2.0f }));
}

TEST_CASE("Frustum: entre el ojo y el near plane no es visible") {
    CHECK_FALSE(SphereInFrustum(FrustumDePrueba(), { glm::vec3(0.0f, 0.0f, -0.05f), 0.01f }));
}

TEST_CASE("Frustum: la view mueve el volumen con la camara") {
    const glm::mat4 proj = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 100.0f);
    const glm::mat4 view = glm::lookAt(glm::vec3(100.0f, 0.0f, 0.0f),
                                       glm::vec3(100.0f, 0.0f, -1.0f),
                                       glm::vec3(0.0f, 1.0f, 0.0f));
    const Frustum f = ExtractFrustum(proj * view);
    CHECK(SphereInFrustum(f, { glm::vec3(100.0f, 0.0f, -10.0f), 1.0f }));
    CHECK_FALSE(SphereInFrustum(f, { glm::vec3(0.0f, 0.0f, -10.0f), 1.0f }));
}
