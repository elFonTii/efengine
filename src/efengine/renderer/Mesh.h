#pragma once
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
        private:
            VertexArray m_va;
            std::string m_materialName;
    };
}
}