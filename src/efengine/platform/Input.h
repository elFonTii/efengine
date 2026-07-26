#pragma once

#include <efengine/core/Types.h>
#include <efengine/platform/IEventListener.h>
#include <efengine/platform/InputCodes.h>

#include <glm/glm.hpp>

#include <array>

namespace efengine {
namespace platform {


    class Window;

    class Input : public IEventListener {
        public:
            void OnMouseMove(f32 x, f32 y)                        override;
            void OnMouseButton(i32 button, i32 action, i32 mods)  override;
            void OnMouseScroll(f32 xOffset, f32 yOffset)          override;
            void NewFrame(const Window& window);

            bool      IsDown(Key key) const;
            bool      WasPressed(Key key) const;
            bool      IsMouseDown(MouseButton b) const;
            bool      WasMousePressed(MouseButton b) const;
            glm::vec2 MousePosition() const;
            glm::vec2 MouseDelta() const;
            f32       ScrollDelta() const;

            void SetKeyDown(Key key, bool down);

            // cierra frame: prev <- curr <- pending.
            void Commit();

        private:
            static constexpr usize kKeyCount    = 349;   // GLFW_KEY_LAST + 1
            static constexpr usize kButtonCount = 3;

            struct Snapshot {
                std::array<bool, kKeyCount>    keys {};
                std::array<bool, kButtonCount> buttons {};
                glm::vec2 cursor { 0.0f };
                f32       scroll = 0.0f;
            };

            Snapshot m_pending, m_curr, m_prev;

            bool m_hasPrev   = false; // false hasta el segundo commit
            bool m_committed = false;
    };

}
}
