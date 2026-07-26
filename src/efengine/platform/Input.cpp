#include <efengine/platform/Input.h>

#include <efengine/core/Assert.h>
#include <efengine/platform/Window.h>

namespace efengine {
namespace platform {

    namespace {
        // Rango de códigos de tecla que GLFW puede reportar.
        // 32 es Space,
        // 348 es GLFW_KEY_LAST.
        constexpr i32 kFirstKeyCode = 32;
        constexpr i32 kLastKeyCode  = 348;

        usize KeyIndex(Key key) {
            const i32 code = static_cast<i32>(key);
            EF_ASSERT(code >= 0 && code < 349, "Input: codigo de tecla fuera de rango");
            return static_cast<usize>(code);
        }

        usize ButtonIndex(MouseButton b) {
            const i32 code = static_cast<i32>(b);
            EF_ASSERT(code >= 0 && code < 3, "Input: boton de mouse fuera de rango");
            return static_cast<usize>(code);
        }
    }

    void Input::OnMouseMove(f32 x, f32 y) {
        m_pending.cursor = glm::vec2(x, y);
    }

    void Input::OnMouseButton(i32 button, i32 action, i32 mods) {
        (void)mods;
        if (button < 0 || button >= static_cast<i32>(kButtonCount)) {
            return;
        }
        m_pending.buttons[static_cast<usize>(button)] =
            (action == static_cast<i32>(InputAction::Press));
    }

    void Input::OnMouseScroll(f32 xOffset, f32 yOffset) {
        (void)xOffset;
        m_pending.scroll += yOffset;
    }

    void Input::NewFrame(const Window& window) {
        for (i32 code = kFirstKeyCode; code <= kLastKeyCode; ++code) {
            m_pending.keys[static_cast<usize>(code)] =
                window.IsKeyPressed(static_cast<Key>(code));
        }
        Commit();
    }

    bool Input::IsDown(Key key) const {
        return m_curr.keys[KeyIndex(key)];
    }

    bool Input::WasPressed(Key key) const {
        const usize i = KeyIndex(key);
        return m_curr.keys[i] && !m_prev.keys[i];
    }

    bool Input::IsMouseDown(MouseButton b) const {
        return m_curr.buttons[ButtonIndex(b)];
    }

    bool Input::WasMousePressed(MouseButton b) const {
        const usize i = ButtonIndex(b);
        return m_curr.buttons[i] && !m_prev.buttons[i];
    }

    glm::vec2 Input::MousePosition() const {
        return m_curr.cursor;
    }

    glm::vec2 Input::MouseDelta() const {
        if (!m_hasPrev) {
            return glm::vec2(0.0f);
        }
        return m_curr.cursor - m_prev.cursor;
    }

    f32 Input::ScrollDelta() const {
        return m_curr.scroll;
    }

    void Input::SetKeyDown(Key key, bool down) {
        m_pending.keys[KeyIndex(key)] = down;
    }

    void Input::Commit() {
        m_hasPrev   = m_committed;   // recién a partir del segundo cierre
        m_committed = true;

        m_prev = m_curr;
        m_curr = m_pending;

        // Resetear el acumulador de scroll
        m_pending.scroll = 0.0f;
    }

}
}
