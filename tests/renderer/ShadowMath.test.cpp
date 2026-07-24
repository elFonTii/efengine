#include <doctest/doctest.h>
#include <efengine/renderer/ShadowMath.h>
#include <glm/glm.hpp>
#include <cmath>

using efengine::renderer::ComputeDirectionalLightMatrix;

TEST_CASE("ComputeDirectionalLightMatrix: el centro cae dentro del clip") {
    // Sol apuntando hacia abajo; caja de media-anchura 10, ojo a 50 de distancia.
    glm::mat4 m = ComputeDirectionalLightMatrix(
        glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(0.0f),
        10.0f, 50.0f, 0.1f, 100.0f);

    glm::vec4 clip = m * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
    glm::vec3 ndc  = glm::vec3(clip) / clip.w;

    CHECK(ndc.x == doctest::Approx(0.0f).epsilon(0.001));
    CHECK(ndc.y == doctest::Approx(0.0f).epsilon(0.001));
    CHECK(ndc.z > -1.0f);
    CHECK(ndc.z <  1.0f);
}

TEST_CASE("ComputeDirectionalLightMatrix: un punto en el borde de la caja mapea a |ndc|~1") {
    glm::mat4 m = ComputeDirectionalLightMatrix(
        glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(0.0f),
        10.0f, 50.0f, 0.1f, 100.0f);

    // Punto a exactamente orthoHalfSize en X (perpendicular a la dirección de luz).
    glm::vec4 clip = m * glm::vec4(10.0f, 0.0f, 0.0f, 1.0f);
    glm::vec3 ndc  = glm::vec3(clip) / clip.w;

    CHECK(std::abs(ndc.x) == doctest::Approx(1.0f).epsilon(0.001));
    CHECK(ndc.y == doctest::Approx(0.0f).epsilon(0.001));
}

TEST_CASE("ComputeDirectionalLightMatrix: fuera de la caja cae con |ndc|>1") {
    glm::mat4 m = ComputeDirectionalLightMatrix(
        glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(0.0f),
        10.0f, 50.0f, 0.1f, 100.0f);

    glm::vec4 clip = m * glm::vec4(20.0f, 0.0f, 0.0f, 1.0f);
    glm::vec3 ndc  = glm::vec3(clip) / clip.w;

    CHECK(std::abs(ndc.x) > 1.0f);
}
