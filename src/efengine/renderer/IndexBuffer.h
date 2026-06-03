#pragma once

#include <efengine/core/Types.h>

namespace efengine {
namespace renderer {

    // Posee un EBO de OpenGL. Handle-owner RO5:
    // adquiere el EBO en el ctor, lo libera en el dtor; no-copiable, movible.
    // Los índices son u32 (GL_UNSIGNED_INT).
    class IndexBuffer {
        public:
            IndexBuffer(const u32* indices, u32 count);  // crea el EBO y sube los datos
            ~IndexBuffer();

            IndexBuffer(const IndexBuffer&)            = delete;
            IndexBuffer& operator=(const IndexBuffer&) = delete;
            IndexBuffer(IndexBuffer&& other) noexcept;
            IndexBuffer& operator=(IndexBuffer&& other) noexcept;

            void Bind() const;                 // glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_id)
            u32  count() const { return m_count; }

        private:
            u32 m_id    = 0;
            u32 m_count = 0;
    };

}
}
