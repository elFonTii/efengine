#pragma once
#include <efengine/core/Types.h>
#include <efengine/renderer/Framebuffer.h>
#include <efengine/renderer/RenderTarget.h>

namespace efengine {
namespace renderer {

    class Texture;

    // current: -1 = el color de sceneFB, 0/1 = uno de los dos targets.
    struct PingPong {
        i32  current  = -1;
        u32  pending  = 0u;
        bool acquired = false;

        void Begin()   { current = -1; acquired = false; }
        u32  Acquire() { pending = (current == 0) ? 1u : 0u; acquired = true; return pending; }
        void Publish() { if (acquired) { current = static_cast<i32>(pending); acquired = false; } }
    };

    // El color que se pasan los pases de post. Cada pase lee Current(), dibuja
    // en AcquireNext() y termina con Publish(); uno apagado no toca nada.
    class PostTargets {
        public:
            PostTargets(u32 width, u32 height);

            void               BeginFrame(const Texture& sceneColor);
            const Texture&     Current() const;
            // null mientras el actual siga siendo el color de escena.
            const Framebuffer* CurrentFramebuffer() const;
            RenderTarget       AcquireNext();
            void               Publish();
            void               Resize(u32 width, u32 height);

        private:
            Framebuffer&       At(u32 i)       { return i == 0u ? m_a : m_b; }
            const Framebuffer& At(u32 i) const { return i == 0u ? m_a : m_b; }

            Framebuffer    m_a;
            Framebuffer    m_b;
            const Texture* m_scene = null;
            PingPong       m_state;
    };

}
}
