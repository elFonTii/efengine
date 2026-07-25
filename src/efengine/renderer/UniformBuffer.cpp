#include "UniformBuffer.h"

#include <glad/gl.h>
#include <utility>

#include <efengine/core/Assert.h>
#include <efengine/core/Log.h>

namespace efengine {
namespace renderer {

    std::optional<UniformBuffer> UniformBuffer::Create(usize sizeBytes) {
        EF_ASSERT(sizeBytes > 0, "UniformBuffer::Create: sizeBytes debe ser mayor a 0");

        u32 id = 0;
        glCreateBuffers(1, &id);
        EF_ASSERT(id != 0, "UniformBuffer::Create: glCreateBuffers devuelve 0 (sin contexto GL)");

        // Storage inmutable: el tamaño no puede cambiar, el contenido sí.
        glNamedBufferStorage(id, (GLsizeiptr)sizeBytes, null, GL_DYNAMIC_STORAGE_BIT);

        return UniformBuffer(id, sizeBytes);
    }

    UniformBuffer::UniformBuffer(u32 id, usize size) : m_id(id), m_size(size) {}

    UniformBuffer::~UniformBuffer() {
        if (m_id != 0) glDeleteBuffers(1, &m_id);
    }

    UniformBuffer::UniformBuffer(UniformBuffer&& other) noexcept
        : m_id(std::exchange(other.m_id, 0))
        , m_size(std::exchange(other.m_size, 0)) {}

    UniformBuffer& UniformBuffer::operator=(UniformBuffer&& other) noexcept {
        if (this != &other) {
            if (m_id != 0) glDeleteBuffers(1, &m_id);
            m_id   = std::exchange(other.m_id, 0);
            m_size = std::exchange(other.m_size, 0);
        }
        return *this;
    }

    void UniformBuffer::BindBase(u32 binding) const {
        EF_ASSERT(m_id != 0, "UniformBuffer::BindBase: buffer vacio (movido o no inicializado)");
        glBindBufferBase(GL_UNIFORM_BUFFER, binding, m_id);
    }

    void UniformBuffer::Update(const void* data, usize sizeBytes, usize offset) const {
        EF_ASSERT(m_id != 0, "UniformBuffer::Update: buffer vacio (movido o no inicializado)");
        EF_ASSERT(data != null, "UniformBuffer::Update: data no puede ser null");
        EF_ASSERT(offset + sizeBytes <= m_size, "UniformBuffer::Update: escritura fuera de rango");

        glNamedBufferSubData(m_id, (GLintptr)offset, (GLsizeiptr)sizeBytes, data);
    }

}
}
