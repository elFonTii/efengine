#include "ShaderStorageBuffer.h"

#include <glad/gl.h>
#include <utility>

#include <efengine/core/Assert.h>
#include <efengine/core/Log.h>

namespace efengine {
namespace renderer {

    std::optional<ShaderStorageBuffer> ShaderStorageBuffer::Create(usize sizeBytes, u32 flags) {
        EF_ASSERT(sizeBytes > 0, "ShaderStorageBuffer::Create: sizeBytes debe ser mayor a 0");

        u32 id = 0;
        glCreateBuffers(1, &id);
        EF_ASSERT(id != 0, "ShaderStorageBuffer::Create: glCreateBuffers devuelve 0 (sin contexto GL)");

        glNamedBufferStorage(id, (GLsizeiptr)sizeBytes, null, (GLbitfield)flags);

        return ShaderStorageBuffer(id, sizeBytes);
    }

    ShaderStorageBuffer::ShaderStorageBuffer(u32 id, usize size) : m_id(id), m_size(size) {}

    ShaderStorageBuffer::~ShaderStorageBuffer() {
        if (m_id != 0) glDeleteBuffers(1, &m_id);
    }

    ShaderStorageBuffer::ShaderStorageBuffer(ShaderStorageBuffer&& other) noexcept
        : m_id(std::exchange(other.m_id, 0))
        , m_size(std::exchange(other.m_size, 0)) {}

    ShaderStorageBuffer& ShaderStorageBuffer::operator=(ShaderStorageBuffer&& other) noexcept {
        if (this != &other) {
            if (m_id != 0) glDeleteBuffers(1, &m_id);
            m_id   = std::exchange(other.m_id, 0);
            m_size = std::exchange(other.m_size, 0);
        }
        return *this;
    }

    void ShaderStorageBuffer::BindBase(u32 binding) const {
        EF_ASSERT(m_id != 0, "ShaderStorageBuffer::BindBase: buffer vacio (movido o no inicializado)");
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, binding, m_id);
    }

    void ShaderStorageBuffer::Update(const void* data, usize sizeBytes, usize offset) const {
        EF_ASSERT(m_id != 0, "ShaderStorageBuffer::Update: buffer vacio (movido o no inicializado)");
        EF_ASSERT(data != null, "ShaderStorageBuffer::Update: data no puede ser null");
        EF_ASSERT(offset + sizeBytes <= m_size, "ShaderStorageBuffer::Update: escritura fuera de rango");

        glNamedBufferSubData(m_id, (GLintptr)offset, (GLsizeiptr)sizeBytes, data);
    }

}
}
