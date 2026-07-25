#include "VertexArray.h"

#include <efecom/RHI.h>
#include <utility>

#include <efengine/core/Assert.h>

namespace efengine {
namespace renderer {

    // Constructor
    VertexArray::VertexArray() {
        m_id = efecom::CreateVertexArray();
        EF_ASSERT(m_id != 0, "VertexArray: efecom::CreateVertexArray fallo");
    }

    // Destructor
    VertexArray::~VertexArray() {
        if (m_id != 0) {
            efecom::DestroyVertexArray(m_id);
        }
    }

    // Move constructor
    VertexArray::VertexArray(VertexArray&& other) noexcept
        : m_id(std::exchange(other.m_id, 0))
        , m_buffers(std::move(other.m_buffers))
        , m_indexBuffer(std::move(other.m_indexBuffer))
        , m_vertexCount(std::exchange(other.m_vertexCount, 0)) {}

    // Move assignment operator
    VertexArray& VertexArray::operator=(VertexArray&& other) noexcept {
        if (this != &other) {
            if (m_id != 0) {
                efecom::DestroyVertexArray(m_id);
            }
            m_id          = std::exchange(other.m_id, 0);
            m_buffers     = std::move(other.m_buffers);
            m_indexBuffer = std::move(other.m_indexBuffer);
            m_vertexCount = std::exchange(other.m_vertexCount, 0);
        }
        return *this;
    }

    void VertexArray::AddVertexBuffer(Buffer&& buffer, const VertexLayout& layout) {
        EF_ASSERT(layout.stride() > 0, "VertexArray: el layout no tiene atributos");
        EF_ASSERT(buffer.size() % layout.stride() == 0,
                  "VertexArray: el tamano del buffer no es multiplo del stride");

        // concepto de binding point: el vao ya no apunta al vbo bindeado;
        // el buffer se engancha a un binding point del VA y los atributos se
        // asocian a ese binding (modelo DSA que el RHI expone tal cual).
        const u32 bindingIndex = static_cast<u32>(m_buffers.size());

        efecom::VertexArraySetVertexBuffer(m_id, bindingIndex, buffer.id(), layout.stride());

        for (const VertexAttribute& attr : layout.attributes()) {
            efecom::VertexArraySetAttribute(m_id, attr.location, ComponentCount(attr.type),
                                            attr.offset, bindingIndex);
        }

        m_vertexCount = static_cast<u32>(buffer.size() / layout.stride());
        m_buffers.push_back(std::move(buffer));
    }

    void VertexArray::Bind() const {
        EF_ASSERT(m_id != 0, "VertexArray::Bind: VAO vacio (movido o no inicializado)");
        efecom::BindVertexArray(m_id);
    }

    void VertexArray::SetIndexBuffer(IndexBuffer&& indexBuffer) {
        efecom::VertexArraySetIndexBuffer(m_id, indexBuffer.id());
        m_indexBuffer = std::move(indexBuffer);
    }

    u32 VertexArray::indexCount() const {
        EF_ASSERT(hasIndexBuffer(), "VertexArray::indexCount: no hay index buffer");
        return m_indexBuffer->count();
    }

}
}
