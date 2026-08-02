// Los 6 pares (forward, up) de la captura de probes contra la tabla
// dirForFace de assets/shaders/common/cubeface.glsl.
//
// Que prueba esto de verdad: que los pares son consistentes con el MIRROR en
// C++ de la tabla GLSL. Lo que NO prueba es que el mirror siga igual al GLSL
// -- ese salto lo cierra la debug viz, no un test. Igual atrapa lo que mas
// probablemente se rompa: un typo al transcribir, un up que no es perpendicular,
// o una cara duplicada.
#include <doctest/doctest.h>
#include <efengine/renderer/CubeFaces.h>

#include <cmath>

using namespace efengine;
using namespace efengine::renderer;

namespace {
    void CheckVecEq(const glm::vec3& a, const glm::vec3& b) {
        CHECK(a.x == doctest::Approx(b.x).epsilon(0.0001));
        CHECK(a.y == doctest::Approx(b.y).epsilon(0.0001));
        CHECK(a.z == doctest::Approx(b.z).epsilon(0.0001));
    }
}

TEST_CASE("CubeFaces: los 6 forward son los ejes mayores, uno por cara") {
    // Sumar los 6 forward tiene que dar cero: son +-X, +-Y, +-Z exactamente
    // una vez cada uno. Una cara duplicada o con signo invertido rompe esto.
    glm::vec3 suma(0.0f);
    for (u32 f = 0u; f < kCubeFaceCount; ++f) {
        const glm::vec3 fwd = CubeFace(f).forward;
        CHECK(glm::length(fwd) == doctest::Approx(1.0f));
        // Es un eje: exactamente una componente vale +-1 y las otras dos cero.
        const f32 absSum = std::abs(fwd.x) + std::abs(fwd.y) + std::abs(fwd.z);
        CHECK(absSum == doctest::Approx(1.0f));
        suma += fwd;
    }
    CheckVecEq(suma, glm::vec3(0.0f));
}

TEST_CASE("CubeFaces: cada up es unitario y perpendicular a su forward") {
    for (u32 f = 0u; f < kCubeFaceCount; ++f) {
        const CubeFaceBasis& b = CubeFace(f);
        CHECK(glm::length(b.up) == doctest::Approx(1.0f));
        CHECK(glm::dot(b.forward, b.up) == doctest::Approx(0.0f));
    }
}

TEST_CASE("CubeFaces: en uv (0,0) dirForFace devuelve el forward de la cara") {
    for (u32 f = 0u; f < kCubeFaceCount; ++f) {
        CheckVecEq(DirForFace(f, glm::vec2(0.0f)), CubeFace(f).forward);
    }
}

TEST_CASE("CubeFaces: la base de camara reproduce dirForFace en todo el NDC") {
    // ESTE es el test que importa. Para cada cara y varios puntos del NDC,
    // la direccion que sale de la base de lookAt tiene que coincidir con la
    // tabla. Si no coincide, la GI sale rotada o espejada.
    const glm::vec2 muestras[] = {
        {  0.0f,  0.0f }, {  1.0f,  0.0f }, { -1.0f,  0.0f },
        {  0.0f,  1.0f }, {  0.0f, -1.0f }, {  1.0f,  1.0f },
        { -1.0f,  1.0f }, {  1.0f, -1.0f }, { -1.0f, -1.0f },
        {  0.5f, -0.25f },
    };

    for (u32 f = 0u; f < kCubeFaceCount; ++f) {
        const CubeFaceBasis& b = CubeFace(f);
        const glm::vec3 right = glm::normalize(glm::cross(b.forward, b.up));
        const glm::vec3 trueUp = glm::cross(right, b.forward);

        for (const glm::vec2& s : muestras) {
            const glm::vec3 desdeBase =
                glm::normalize(b.forward + s.x * right + s.y * trueUp);
            CheckVecEq(desdeBase, DirForFace(f, s));
        }
    }
}
