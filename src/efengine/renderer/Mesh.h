#pragma once
#include <efengine/renderer/Vertex.h>
#include <efengine/renderer/VertexArray.h>

#include <vector>

namespace efengine {
namespace renderer {
    class Mesh {
        public:
            Mesh(const std::vector<Vertex>& vertices, const std::vector<u32>& indices);
            const VertexArray& vertexArray() const { return m_va; }
        private:
            VertexArray m_va;
    };
}
}