#pragma once

#include <efengine/core/Types.h>

namespace efengine {
namespace renderer {

    // Posee un Vertex Buffer Object (VBO) de OpenGL. Handle-owner RO5:
    // adquiere el VBO en el ctor, lo libera en el dtor; no-copiable, movible.
    class Buffer {
        public:
            Buffer(const void* data, usize size);  // crea el VBO y sube los datos
            ~Buffer();

            Buffer(const Buffer&)            = delete;
            Buffer& operator=(const Buffer&) = delete;
            Buffer(Buffer&& other) noexcept;
            Buffer& operator=(Buffer&& other) noexcept;
            
            u32 id() const { return m_id; }
            usize size() const { return m_size; }

        private:
            u32   m_id   = 0;
            usize m_size = 0;
    };

}
}
