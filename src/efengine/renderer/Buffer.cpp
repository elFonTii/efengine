#include "Buffer.h"

#include <efecom/RHI.h>
#include <utility>

#include <efengine/core/Assert.h>

namespace efengine {
namespace renderer {

    Buffer::Buffer(const void* data, usize size) {
        EF_ASSERT(data != null, "Buffer: data no puede ser null");
        EF_ASSERT(size > 0, "Buffer: size debe ser > 0");

        m_id = efecom::CreateBuffer(data, size);
        EF_ASSERT(m_id != 0, "Buffer: efecom::CreateBuffer fallo");

        // m_size solo se asigna tras el éxito: si CreateBuffer falla, el objeto
        // queda con m_id == 0 y m_size == 0 (estado "vacío" coherente).
        m_size = size;
    }

    Buffer::~Buffer() {
        if (m_id != 0) {
            efecom::DestroyBuffer(m_id);
        }
    }

    Buffer::Buffer(Buffer&& other) noexcept
        : m_id(std::exchange(other.m_id, 0))
        , m_size(std::exchange(other.m_size, 0)) {}

    Buffer& Buffer::operator=(Buffer&& other) noexcept {
        if (this != &other) {
            if (m_id != 0) {
                efecom::DestroyBuffer(m_id);
            }
            m_id   = std::exchange(other.m_id, 0);
            m_size = std::exchange(other.m_size, 0);
        }
        return *this;
    }

}
}
