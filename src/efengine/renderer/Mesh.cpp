#include "efengine/renderer/Mesh.h"

#include "efengine/core/Assert.h"
#include "efengine/renderer/Buffer.h"
#include "efengine/renderer/IndexBuffer.h"

#include <utility>


namespace efengine {
namespace renderer {
    Mesh::Mesh(const std::vector<Vertex>& vertices, const std::vector<u32>& indices) {
        EF_ASSERT(!vertices.empty(), "Mesh::Mesh: Los vértices al construir el mesh están vacíos");
        EF_ASSERT(!indices.empty(), "Mesh::Mesh: Los índices de vértices al construir el mesh están vacíos");

        Buffer vbo(vertices.data(), vertices.size() * sizeof(Vertex));
        IndexBuffer ebo(indices.data(), static_cast<u32>(indices.size()));

        auto layout = StandardVertexLayout();
        m_va.AddVertexBuffer(std::move(vbo), layout);
        m_va.SetIndexBuffer(std::move(ebo));
    };

}
}


