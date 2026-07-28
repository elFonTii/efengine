#include "efengine/renderer/UniformBuffer.h"

#include <efecom/RHI.h>
#include <efengine/core/Assert.h>
#include <utility>

namespace efengine {
namespace renderer {

    UniformBuffer::UniformBuffer(usize size) : m_size(size) {
        EF_ASSERT(size > 0u, "UniformBuffer: tamano cero");
        m_id = efecom::CreateUniformBuffer(size);
        EF_ASSERT(m_id != 0, "UniformBuffer: no hay contexto GL");
    }

    UniformBuffer::~UniformBuffer() {
        if (m_id != 0) efecom::DestroyBuffer(m_id);
    }

    UniformBuffer::UniformBuffer(UniformBuffer&& other) noexcept
        : m_id(std::exchange(other.m_id, 0))
        , m_size(std::exchange(other.m_size, 0u)) {}

    UniformBuffer& UniformBuffer::operator=(UniformBuffer&& other) noexcept {
        if (this != &other) {
            if (m_id != 0) efecom::DestroyBuffer(m_id);
            m_id   = std::exchange(other.m_id, 0);
            m_size = std::exchange(other.m_size, 0u);
        }
        return *this;
    }

    void UniformBuffer::Update(const void* data, usize size, usize offset) const {
        EF_ASSERT(offset + size <= m_size, "UniformBuffer::Update: se pasa del tamano reservado");
        efecom::UpdateBuffer(m_id, data, size, offset);
    }

    void UniformBuffer::BindTo(u32 bindingIndex) const {
        efecom::BindUniformBuffer(m_id, bindingIndex);
    }

}
}
