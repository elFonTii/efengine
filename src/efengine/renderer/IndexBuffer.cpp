#include "IndexBuffer.h"

#include <efecom/RHI.h>
#include <utility>

#include <efengine/core/Assert.h>

namespace efengine {
namespace renderer {

    IndexBuffer::IndexBuffer(const u32* indices, u32 count) {
        EF_ASSERT(indices != null, "IndexBuffer: indices no puede ser null");
        EF_ASSERT(count > 0, "IndexBuffer: count debe ser > 0");

        m_id = efecom::CreateBuffer(indices, static_cast<usize>(count) * sizeof(u32));
        EF_ASSERT(m_id != 0, "IndexBuffer: efecom::CreateBuffer fallo");

        m_count = count;
    }

    IndexBuffer::~IndexBuffer() {
        if (m_id != 0) {
            efecom::DestroyBuffer(m_id);
        }
    }

    IndexBuffer::IndexBuffer(IndexBuffer&& other) noexcept
        : m_id(std::exchange(other.m_id, 0))
        , m_count(std::exchange(other.m_count, 0)) {}

    IndexBuffer& IndexBuffer::operator=(IndexBuffer&& other) noexcept {
        if (this != &other) {
            if (m_id != 0) {
                efecom::DestroyBuffer(m_id);
            }
            m_id    = std::exchange(other.m_id, 0);
            m_count = std::exchange(other.m_count, 0);
        }
        return *this;
    }

}
}
