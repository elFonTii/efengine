#pragma once

#include <efengine/core/Types.h>
#include <efengine/renderer/Buffer.h>
#include <efengine/renderer/VertexLayout.h>
#include <vector>

namespace efengine {
namespace renderer {

    // Posee un Vertex Array Object (VAO) y los buffers que referencia.
    // Handle-owner RO5. El VAO referencia los VBOs, así que debe poseerlos
    // para que vivan tanto como él.
    class VertexArray {
        public:
            VertexArray();
            ~VertexArray();

            VertexArray(const VertexArray&)            = delete;
            VertexArray& operator=(const VertexArray&) = delete;
            VertexArray(VertexArray&& other) noexcept;
            VertexArray& operator=(VertexArray&& other) noexcept;

            // Toma propiedad del buffer y configura los atributos según el layout.
            void AddVertexBuffer(Buffer&& buffer, const VertexLayout& layout);

            void Bind() const;
            u32  vertexCount() const { return m_vertexCount; }

        private:
            u32                 m_id          = 0;
            std::vector<Buffer> m_buffers;
            u32                 m_vertexCount = 0;
    };

}
}
