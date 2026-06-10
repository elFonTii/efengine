// Escrito por Claude Opus 4.8
#include <cmath>
#include <doctest/doctest.h>
#include <efengine/scene/Camera.h>
#include <efengine/scene/CameraController.h>
#include <efengine/platform/InputCodes.h>
#include <glm/glm.hpp>

using namespace efengine;
TEST_CASE("CameraController: orbitar preserva la distancia al target") {
    scene::Camera cam;
    scene::CameraController controller(&cam);

    f32 d0 = glm::length(cam.Position());

    controller.OnMouseButton((i32)platform::MouseButton::Left, (i32)platform::InputAction::Press, 0);
    controller.OnMouseMove(150.0f, 130.0f);   // arrastre

    CHECK(glm::length(cam.Position()) == doctest::Approx(d0));
}

TEST_CASE("CameraController: el scroll acerca la cámara") {
    scene::Camera cam;
    scene::CameraController controller(&cam);

    f32 d0 = glm::length(cam.Position());
    controller.OnMouseScroll(0.0f, 1.0f);   // scroll hacia arriba = zoom in

    CHECK(glm::length(cam.Position()) < d0);
}

TEST_CASE("CameraController: el pitch se limita a +-89 grados") {
    scene::Camera cam;
    scene::CameraController controller(&cam);

    controller.OnMouseButton((i32)platform::MouseButton::Left, (i32)platform::InputAction::Press, 0);
    controller.OnMouseMove(0.0f, 100000.0f);   // arrastre vertical brutal -> satura el pitch

    // Con el clamp, la altura nunca alcanza la distancia completa (no pasa "por el polo").
    // std::abs hace el test independiente de hacia qué lado clampee el signo.
    f32 maxY = 2.5f * std::sin(glm::radians(89.0f));
    CHECK(std::abs(cam.Position().y) == doctest::Approx(maxY));
}

TEST_CASE("CameraController: sin arrastre, mover el mouse no orbita") {
    scene::Camera cam;
    scene::CameraController controller(&cam);

    glm::vec3 before = cam.Position();
    controller.OnMouseMove(500.0f, 500.0f);   // sin botón presionado
    glm::vec3 after = cam.Position();

    CHECK(after.x == doctest::Approx(before.x));
    CHECK(after.y == doctest::Approx(before.y));
    CHECK(after.z == doctest::Approx(before.z));
}
