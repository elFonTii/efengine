#include "Buffer.h"

#include <glad/gl.h>
#include <utility>

#include <efengine/core/Assert.h>

namespace efengine {
namespace renderer {

    Buffer::Buffer(const void* data, usize size) {
        EF_ASSERT(data != null, "Buffer: data no puede ser null");
        EF_ASSERT(size > 0, "Buffer: size debe ser > 0");

        /* GL 3.3 (binding de VBO y subida de datos)
        glGenBuffers(1, &m_id);
        EF_ASSERT(m_id != 0, "Buffer: glGenBuffers fallo");

        glBindBuffer(GL_ARRAY_BUFFER, m_id);
        glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(size), data, GL_STATIC_DRAW);
        */

        // GL 4.5, DSA direct state access papaa
        glCreateBuffers(1, &m_id);
        EF_ASSERT(m_id != 0, "Buffer: glCreateBuffers fallo");
        glNamedBufferData(m_id, static_cast<GLsizeiptr>(size), data, GL_STATIC_DRAW);

        // m_size solo se asigna tras el éxito: si glGenBuffers falla, el objeto
        // queda con m_id == 0 y m_size == 0 (estado "vacío" coherente).
        m_size = size;
    }

    Buffer::~Buffer() {
        if (m_id != 0) {
            glDeleteBuffers(1, &m_id);
        }
    }

    Buffer::Buffer(Buffer&& other) noexcept
        : m_id(std::exchange(other.m_id, 0))
        , m_size(std::exchange(other.m_size, 0)) {}

    Buffer& Buffer::operator=(Buffer&& other) noexcept {
        if (this != &other) {
            if (m_id != 0) {
                glDeleteBuffers(1, &m_id);
            }
            m_id   = std::exchange(other.m_id, 0);
            m_size = std::exchange(other.m_size, 0);
        }
        return *this;
    }

}
}
