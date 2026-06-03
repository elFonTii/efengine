#include "IndexBuffer.h"

#include <glad/gl.h>
#include <utility>

#include <efengine/core/Assert.h>

namespace efengine {
namespace renderer {

    IndexBuffer::IndexBuffer(const u32* indices, u32 count) {
        EF_ASSERT(indices != null, "IndexBuffer: indices no puede ser null");
        EF_ASSERT(count > 0, "IndexBuffer: count debe ser > 0");

        glGenBuffers(1, &m_id);
        EF_ASSERT(m_id != 0, "IndexBuffer: glGenBuffers fallo");

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_id);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                     static_cast<GLsizeiptr>(count) * static_cast<GLsizeiptr>(sizeof(u32)),
                     indices, GL_STATIC_DRAW);

        m_count = count;
    }

    IndexBuffer::~IndexBuffer() {
        if (m_id != 0) {
            glDeleteBuffers(1, &m_id);
        }
    }

    IndexBuffer::IndexBuffer(IndexBuffer&& other) noexcept
        : m_id(std::exchange(other.m_id, 0))
        , m_count(std::exchange(other.m_count, 0)) {}

    IndexBuffer& IndexBuffer::operator=(IndexBuffer&& other) noexcept {
        if (this != &other) {
            if (m_id != 0) {
                glDeleteBuffers(1, &m_id);
            }
            m_id    = std::exchange(other.m_id, 0);
            m_count = std::exchange(other.m_count, 0);
        }
        return *this;
    }

    void IndexBuffer::Bind() const {
        EF_ASSERT(m_id != 0, "IndexBuffer::Bind: EBO vacio (movido o no inicializado)");
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_id);
    }

}
}
