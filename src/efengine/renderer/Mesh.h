#pragma once
#include <efengine/math/Aabb.h>
#include <efengine/renderer/Vertex.h>
#include <efengine/renderer/VertexArray.h>

#include <string>
#include <vector>

namespace efengine {
namespace renderer {
    class Mesh {
        public:
            Mesh(const std::vector<Vertex>& vertices, const std::vector<u32>& indices, std::string materialName);
            const VertexArray& vertexArray() const { return m_va; }
            const std::string& materialName() const { return m_materialName; }
            // Caja envolvente en espacio local, calculada al construir.
            const math::Aabb& localAabb() const { return m_localAabb; }
        private:
            VertexArray m_va;
            std::string m_materialName;
            math::Aabb  m_localAabb;
    };
}
}