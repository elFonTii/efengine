#include <doctest/doctest.h>
#include <efengine/math/Math.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

using efengine::math::ProjectToScreen;
using efengine::math::ScreenRect;

namespace {
    // Camara en el origen mirando a -Z, que es la convencion de GL y la de
    // scene::Camera.
    glm::mat4 viewProjDePrueba() {
        const glm::mat4 view = glm::lookAt(glm::vec3(0.0f), glm::vec3(0.0f, 0.0f, -1.0f),
                                           glm::vec3(0.0f, 1.0f, 0.0f));
        const glm::mat4 proj = glm::perspective(glm::radians(60.0f), 16.0f / 9.0f, 0.1f, 100.0f);
        return proj * view;
    }
}

TEST_CASE("ProjectToScreen: un punto en el eje de la camara cae en el centro del rect") {
    const ScreenRect rect { 0.0f, 0.0f, 1280.0f, 720.0f };
    const std::optional<glm::vec2> p = ProjectToScreen(viewProjDePrueba(),
                                                       glm::vec3(0.0f, 0.0f, -5.0f), rect);

    REQUIRE(p.has_value());
    CHECK(p->x == doctest::Approx(640.0f).epsilon(0.001));
    CHECK(p->y == doctest::Approx(360.0f).epsilon(0.001));
}

TEST_CASE("ProjectToScreen: el offset y el tamano del rect se respetan") {
    const ScreenRect rect { 100.0f, 50.0f, 400.0f, 200.0f };
    const std::optional<glm::vec2> p = ProjectToScreen(viewProjDePrueba(),
                                                       glm::vec3(0.0f, 0.0f, -5.0f), rect);

    REQUIRE(p.has_value());
    CHECK(p->x == doctest::Approx(300.0f).epsilon(0.001));   // 100 + 400/2
    CHECK(p->y == doctest::Approx(150.0f).epsilon(0.001));   // 50 + 200/2
}

TEST_CASE("ProjectToScreen: la Y de pantalla crece hacia abajo") {
    // Un punto ARRIBA en el mundo tiene que dar una Y de pantalla MENOR que el
    // centro. Si esto falla, la flecha apunta al reves en vertical.
    const ScreenRect rect { 0.0f, 0.0f, 1280.0f, 720.0f };
    const std::optional<glm::vec2> arriba = ProjectToScreen(viewProjDePrueba(),
                                                            glm::vec3(0.0f, 1.0f, -5.0f), rect);
    REQUIRE(arriba.has_value());
    CHECK(arriba->y < 360.0f);
}

TEST_CASE("ProjectToScreen: la X de pantalla crece hacia la derecha") {
    const ScreenRect rect { 0.0f, 0.0f, 1280.0f, 720.0f };
    const std::optional<glm::vec2> derecha = ProjectToScreen(viewProjDePrueba(),
                                                             glm::vec3(1.0f, 0.0f, -5.0f), rect);
    REQUIRE(derecha.has_value());
    CHECK(derecha->x > 640.0f);
}

TEST_CASE("ProjectToScreen: un punto detras de la camara no proyecta") {
    // Con w <= 0 la division da una posicion de pantalla plausible pero
    // espejada: devolver nullopt es lo unico honesto.
    const ScreenRect rect { 0.0f, 0.0f, 1280.0f, 720.0f };
    const bool proyecta = ProjectToScreen(viewProjDePrueba(), glm::vec3(0.0f, 0.0f, 5.0f), rect).has_value();
    CHECK_FALSE(proyecta);
}

TEST_CASE("ProjectToScreen: un punto en el plano de la camara no proyecta") {
    const ScreenRect rect { 0.0f, 0.0f, 1280.0f, 720.0f };
    const bool proyecta = ProjectToScreen(viewProjDePrueba(), glm::vec3(0.0f), rect).has_value();
    CHECK_FALSE(proyecta);
}
