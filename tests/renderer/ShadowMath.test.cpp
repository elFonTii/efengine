#include <doctest/doctest.h>
#include <efengine/renderer/ShadowMath.h>
#include <glm/glm.hpp>
#include <cmath>

using efengine::renderer::AABB;
using efengine::renderer::ComputeDirectionalLightMatrix;
using efengine::renderer::DirectionalLightFit;
using efengine::renderer::FitDirectionalLight;

namespace {

    // La caja de Cornell de TestScene: interior 8 x 4 x 8 con el piso en y=0.
    // Radius() = |(4,2,4)| = 6.
    AABB salaCornell() {
        AABB b;
        b.min = glm::vec3(-4.0f, 0.0f, -4.0f);
        b.max = glm::vec3( 4.0f, 4.0f,  4.0f);
        return b;
    }

    // El |ndc| mas grande de las 8 esquinas de la AABB: si da <= 1, la caja
    // ortografica contiene la escena entera y ningun caster se pierde.
    f32 MaxNdcDeLasEsquinas(const glm::mat4& m, const AABB& b) {
        f32 peor = 0.0f;
        for (int i = 0; i < 8; ++i) {
            const glm::vec3 esquina {
                (i & 1) ? b.max.x : b.min.x,
                (i & 2) ? b.max.y : b.min.y,
                (i & 4) ? b.max.z : b.min.z,
            };
            const glm::vec4 clip = m * glm::vec4(esquina, 1.0f);
            const glm::vec3 ndc  = glm::vec3(clip) / clip.w;
            peor = glm::max(peor, glm::max(std::abs(ndc.x),
                            glm::max(std::abs(ndc.y), std::abs(ndc.z))));
        }
        return peor;
    }

}

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

// El bias de sombra se compara contra profundidad NDC [0,1], asi que un bias de
// B vale B * depthRange metros de holgura a lo largo del rayo de luz. Con el
// encuadre fijo viejo (near 0.5, far 450) eso eran 449.5 m para una sala de 8 m:
// el bias minimo dialable en el panel ya valia 4.5 cm y abria una banda de luz
// en cada arista entre paredes. El rango tiene que salir de la escena.
TEST_CASE("FitDirectionalLight: el rango de profundidad lo acota la escena") {
    const AABB sala = salaCornell();                 // Radius() == 6
    const DirectionalLightFit fit = FitDirectionalLight(
        glm::normalize(glm::vec3(0.30f, -0.62f, 0.72f)), sala, 1.0f);

    // Ojo a radius+padding del centro, con padding de aire de cada lado.
    CHECK(fit.depthRange   == doctest::Approx(14.0f).epsilon(0.001));
    CHECK(fit.orthoHalfSize == doctest::Approx(7.0f).epsilon(0.001));

    // Lo que importa como invariante: el rango sigue al tamano de la escena.
    CHECK(fit.depthRange <= 4.0f * sala.Radius());
}

TEST_CASE("FitDirectionalLight: la escena entera entra en la caja, gire donde gire el sol") {
    const AABB sala = salaCornell();

    const glm::vec3 direcciones[] = {
        glm::normalize(glm::vec3( 0.30f, -0.62f,  0.72f)),
        glm::normalize(glm::vec3(-0.80f, -0.20f, -0.55f)),
        glm::normalize(glm::vec3( 0.00f, -1.00f,  0.00f)),
        glm::normalize(glm::vec3( 0.95f, -0.05f,  0.00f)),   // casi rasante
    };

    for (const glm::vec3& dir : direcciones) {
        const DirectionalLightFit fit = FitDirectionalLight(dir, sala, 1.0f);
        CHECK(MaxNdcDeLasEsquinas(fit.matrix, sala) <= 1.0f);
    }
}

// El encuadre sale de la esfera que contiene la AABB, no de la AABB: por eso es
// invariante a la direccion. Si dependiera de la direccion, el tamano del texel
// (y con el, el bias necesario) cambiaria al girar el sol.
TEST_CASE("FitDirectionalLight: girar el sol no cambia el encuadre") {
    const AABB sala = salaCornell();

    const DirectionalLightFit a = FitDirectionalLight(
        glm::normalize(glm::vec3(0.30f, -0.62f, 0.72f)), sala, 1.0f);
    const DirectionalLightFit b = FitDirectionalLight(
        glm::normalize(glm::vec3(-0.9f, -0.3f, 0.1f)), sala, 1.0f);

    CHECK(a.orthoHalfSize == doctest::Approx(b.orthoHalfSize));
    CHECK(a.depthRange    == doctest::Approx(b.depthRange));
}

// SceneGraph::WorldBounds() devuelve AABB::Empty() (min=+inf, max=-inf) si la
// escena no tiene mallas. Center()/Radius() sobre eso dan NaN, y un NaN en la
// matriz light-space apaga las sombras de toda la escena sin decir por que.
TEST_CASE("FitDirectionalLight: una AABB invalida no propaga infinitos") {
    const DirectionalLightFit fit = FitDirectionalLight(
        glm::vec3(0.0f, -1.0f, 0.0f), AABB::Empty(), 1.0f);

    CHECK(fit.depthRange > 0.0f);
    CHECK(fit.orthoHalfSize > 0.0f);
    for (int col = 0; col < 4; ++col)
        for (int row = 0; row < 4; ++row)
            CHECK(std::isfinite(fit.matrix[col][row]));
}
