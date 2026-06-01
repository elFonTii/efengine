#include "Buffer.h"

#include <glad/gl.h>
#include <utility>

#include <efengine/core/Assert.h>

namespace efengine {
namespace renderer {

    Buffer::Buffer(const void* data, usize size) {
        EF_ASSERT(data != null, "Buffer: data no puede ser null");
        EF_ASSERT(size > 0, "Buffer: size debe ser > 0");

        glGenBuffers(1, &m_id);
        EF_ASSERT(m_id != 0, "Buffer: glGenBuffers fallo");

        glBindBuffer(GL_ARRAY_BUFFER, m_id);
        glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(size), data, GL_STATIC_DRAW);

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

    void Buffer::Bind() const {
        EF_ASSERT(m_id != 0, "Buffer::Bind: buffer vacio (movido o no inicializado)");
        glBindBuffer(GL_ARRAY_BUFFER, m_id);
    }

}
}
