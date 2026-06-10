#include <doctest/doctest.h>
#include <efengine/platform/InputCodes.h>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

using namespace efengine::platform;

TEST_CASE("InputCodes::Key coincide con los códigos ASCII de GLFW") {
    CHECK((i32)Key::Escape == GLFW_KEY_ESCAPE);
    CHECK((i32)Key::Space  == GLFW_KEY_SPACE);
    CHECK((i32)Key::Down   == GLFW_KEY_DOWN);
    CHECK((i32)Key::Up     == GLFW_KEY_UP);
    CHECK((i32)Key::Left   == GLFW_KEY_LEFT);
    CHECK((i32)Key::Right  == GLFW_KEY_RIGHT);
    CHECK((i32)Key::W      == GLFW_KEY_W);
    CHECK((i32)Key::A      == GLFW_KEY_A);
    CHECK((i32)Key::S      == GLFW_KEY_S);
    CHECK((i32)Key::D      == GLFW_KEY_D);
}

TEST_CASE("InputCodes::MouseButton coincide con los códigos de GLFW") {
    CHECK((i32)MouseButton::Left   == GLFW_MOUSE_BUTTON_LEFT);
    CHECK((i32)MouseButton::Right  == GLFW_MOUSE_BUTTON_RIGHT);
    CHECK((i32)MouseButton::Middle == GLFW_MOUSE_BUTTON_MIDDLE);
}

TEST_CASE("InputCodes::InputAction coincide con los códigos de GLFW") {
    CHECK((i32)InputAction::Press    == GLFW_PRESS);
    CHECK((i32)InputAction::Release  == GLFW_RELEASE);
    CHECK((i32)InputAction::Repeat   == GLFW_REPEAT);
}
