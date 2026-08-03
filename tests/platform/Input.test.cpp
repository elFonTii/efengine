#include <doctest/doctest.h>
#include <efengine/platform/Input.h>
#include <efengine/platform/InputCodes.h>
#include <glm/glm.hpp>

using namespace efengine;

TEST_CASE("Input: IsDown refleja la tecla recien despues de cerrar el frame") {
    platform::Input in;
    CHECK_FALSE(in.IsDown(platform::Key::W));

    in.SetKeyDown(platform::Key::W, true);
    CHECK_FALSE(in.IsDown(platform::Key::W));   // el frame todavia no se cerro
    in.Commit();
    CHECK(in.IsDown(platform::Key::W));
}

TEST_CASE("Input: WasPressed solo es true en el frame de la transicion") {
    platform::Input in;
    in.SetKeyDown(platform::Key::F, true);
    in.Commit();
    CHECK(in.WasPressed(platform::Key::F));

    in.Commit();                                // se mantiene apretada
    CHECK(in.IsDown(platform::Key::F));
    CHECK_FALSE(in.WasPressed(platform::Key::F));
}

TEST_CASE("Input: WasPressed vuelve a prender tras soltar y reapretar") {
    platform::Input in;
    in.SetKeyDown(platform::Key::F, true);
    in.Commit();
    in.SetKeyDown(platform::Key::F, false);
    in.Commit();
    CHECK_FALSE(in.IsDown(platform::Key::F));
    CHECK_FALSE(in.WasPressed(platform::Key::F));

    in.SetKeyDown(platform::Key::F, true);
    in.Commit();
    CHECK(in.WasPressed(platform::Key::F));
}

TEST_CASE("Input: MouseDelta es cero en el primer frame aunque el cursor este lejos") {
    // El cursor "aparece" en la ventana: sin esta regla la camara pega un salto.
    platform::Input in;
    in.OnMouseMove(900.0f, 640.0f);
    in.Commit();

    CHECK(in.MousePosition().x == doctest::Approx(900.0f));
    CHECK(in.MousePosition().y == doctest::Approx(640.0f));
    CHECK(in.MouseDelta().x == doctest::Approx(0.0f));
    CHECK(in.MouseDelta().y == doctest::Approx(0.0f));
}

TEST_CASE("Input: MouseDelta es la diferencia entre frames consecutivos") {
    platform::Input in;
    in.OnMouseMove(100.0f, 100.0f);
    in.Commit();
    in.OnMouseMove(130.0f, 80.0f);
    in.Commit();

    CHECK(in.MouseDelta().x == doctest::Approx(30.0f));
    CHECK(in.MouseDelta().y == doctest::Approx(-20.0f));
}

TEST_CASE("Input: la posicion del cursor persiste si no llegan eventos") {
    // El cursor es estado, no acumulador: quieto un frame significa delta cero.
    platform::Input in;
    in.OnMouseMove(50.0f, 60.0f);
    in.Commit();
    in.Commit();

    CHECK(in.MousePosition().x == doctest::Approx(50.0f));
    CHECK(in.MouseDelta().x == doctest::Approx(0.0f));
    CHECK(in.MouseDelta().y == doctest::Approx(0.0f));
}

TEST_CASE("Input: ScrollDelta acumula el frame y se resetea en el siguiente") {
    platform::Input in;
    in.OnMouseScroll(0.0f, 1.0f);
    in.OnMouseScroll(0.0f, 2.0f);
    in.Commit();
    CHECK(in.ScrollDelta() == doctest::Approx(3.0f));

    in.Commit();
    CHECK(in.ScrollDelta() == doctest::Approx(0.0f));
}

TEST_CASE("Input: los botones del mouse se comportan igual que las teclas") {
    platform::Input in;
    in.OnMouseButton((i32)platform::MouseButton::Right,
                     (i32)platform::InputAction::Press, 0);
    in.Commit();
    CHECK(in.IsMouseDown(platform::MouseButton::Right));
    CHECK(in.WasMousePressed(platform::MouseButton::Right));

    in.Commit();
    CHECK(in.IsMouseDown(platform::MouseButton::Right));
    CHECK_FALSE(in.WasMousePressed(platform::MouseButton::Right));

    in.OnMouseButton((i32)platform::MouseButton::Right,
                     (i32)platform::InputAction::Release, 0);
    in.Commit();
    CHECK_FALSE(in.IsMouseDown(platform::MouseButton::Right));
}
