#pragma once
#include <efengine/core/Types.h>

namespace efengine {
namespace renderer {

    class Framebuffer;
    class ShadowMap;
    class RenderTarget {
        public:
            static RenderTarget Present();

            void Bind() const;
            u32  width()  const { return m_width; }
            u32  height() const { return m_height; }

        private:
            RenderTarget(u32 handle, u32 width, u32 height)
                : m_handle(handle), m_width(width), m_height(height) {}

            // Los duenos de FBO construyen su propio target
            friend class Framebuffer;
            friend class ShadowMap;

            u32 m_handle = 0;
            u32 m_width  = 0;
            u32 m_height = 0;
    };

}
}
