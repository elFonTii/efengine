#include "efengine/renderer/StorageBuffer.h"

#include <efecom/RHI.h>
#include <efengine/core/Assert.h>
#include <utility>

namespace efengine {
namespace renderer {

    StorageBuffer::StorageBuffer(usize size) : m_size(size) {
        EF_ASSERT(size > 0u, "StorageBuffer: tamano cero");
        m_id = efecom::CreateStorageBuffer(size);
        EF_ASSERT(m_id != 0, "StorageBuffer: no hay contexto GL");
    }

    StorageBuffer::~StorageBuffer() {
        if (m_id != 0) efecom::DestroyBuffer(m_id);
    }

    StorageBuffer::StorageBuffer(StorageBuffer&& other) noexcept
        : m_id(std::exchange(other.m_id, 0))
        , m_size(std::exchange(other.m_size, 0u)) {}

    StorageBuffer& StorageBuffer::operator=(StorageBuffer&& other) noexcept {
        if (this != &other) {
            if (m_id != 0) efecom::DestroyBuffer(m_id);
            m_id   = std::exchange(other.m_id, 0);
            m_size = std::exchange(other.m_size, 0u);
        }
        return *this;
    }

    void StorageBuffer::Update(const void* data, usize size, usize offset) const {
        EF_GPU_CHECK(offset + size <= m_size, "StorageBuffer::Update: se pasa del tamano reservado");
        efecom::UpdateBuffer(m_id, data, size, offset);
    }

    void StorageBuffer::BindTo(u32 bindingIndex) const {
        efecom::BindStorageBuffer(m_id, bindingIndex);
    }

}
}
