#include "VertexArray.h"

#include <glad/gl.h>
#include <utility>

#include <efengine/core/Assert.h>

namespace efengine {
namespace renderer {

    // Constructor
    VertexArray::VertexArray() {
        glCreateVertexArrays(1, &m_id);
        EF_ASSERT(m_id != 0, "VertexArray: glCreateVertexArrays fallo");
    }

    // Destructor
    VertexArray::~VertexArray() {
        if (m_id != 0) {
            glDeleteVertexArrays(1, &m_id);
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
                glDeleteVertexArrays(1, &m_id);
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

    /* GL 3.3 con glBindVertexArray
        glBindVertexArray(m_id);
        buffer.Bind();

        for (const VertexAttribute& attr : layout.attributes()) {
            glEnableVertexAttribArray(attr.location);
            // glVertexAttribPointer toma el byte-offset como const void*. Pasar un
            // u32 directo es impl-defined en 64-bit; ensancharlo a uintptr_t antes
            // del reinterpret_cast es la forma portable y correcta.
            glVertexAttribPointer(
                attr.location,
                static_cast<GLint>(ComponentCount(attr.type)),
                GL_FLOAT,
                GL_FALSE,
                static_cast<GLsizei>(layout.stride()),
                reinterpret_cast<const void*>(static_cast<std::uintptr_t>(attr.offset)));
        }

        // stride() > 0 garantizado por el EF_ASSERT de arriba (sería UB en release si se viola).
        m_vertexCount = static_cast<u32>(buffer.size() / layout.stride());
        m_buffers.push_back(std::move(buffer));

        // Postcondición: dejamos el VAO desenlazado para no filtrar estado GL a
        // llamadas posteriores (configurar otro VAO no corrompe este).
        glBindVertexArray(0);
    */

    /* GL 4.5 con glVertexArrayVertexBuffer */
    // concepto de binding point: el vao ya no apunta al vao bindeado
    // ahora se puede acceder por nombre, lo guardamos en un binding point y luego se puede usar en varios vao
    const u32 bindingIndex = static_cast<u32>(m_buffers.size());

    glVertexArrayVertexBuffer(m_id, bindingIndex, buffer.id(), 0,
                            static_cast<GLsizei>(layout.stride()));

    for (const VertexAttribute& attr : layout.attributes()) {
        glEnableVertexArrayAttrib(m_id, attr.location);
        glVertexArrayAttribFormat(m_id, attr.location,
                                static_cast<GLint>(ComponentCount(attr.type)),
                                GL_FLOAT, GL_FALSE,
                                static_cast<GLuint>(attr.offset));
        glVertexArrayAttribBinding(m_id, attr.location, bindingIndex);
    }

    m_vertexCount = static_cast<u32>(buffer.size() / layout.stride());
    m_buffers.push_back(std::move(buffer));
    }

    void VertexArray::Bind() const {
        EF_ASSERT(m_id != 0, "VertexArray::Bind: VAO vacio (movido o no inicializado)");
        glBindVertexArray(m_id);
    }

    void VertexArray::SetIndexBuffer(IndexBuffer&& indexBuffer) {
    glVertexArrayElementBuffer(m_id, indexBuffer.id());
    m_indexBuffer = std::move(indexBuffer);
    }
    
    u32 VertexArray::indexCount() const {
        EF_ASSERT(hasIndexBuffer(), "VertexArray::indexCount: no hay index buffer");
        return m_indexBuffer->count();
    }

}
}
