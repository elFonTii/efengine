#pragma once
#include <efengine/renderer/Mesh.h>

#include <vector>

namespace efengine {
namespace renderer {
    class Model {
    public:
        explicit Model(std::vector<Mesh> meshes);
        const std::vector<Mesh>& meshes() const { return m_meshes; }
        // Caja envolvente local: merge de las de todas las mallas.
        const math::Aabb& localAabb() const { return m_localAabb; }
    private:
        std::vector<Mesh> m_meshes;
        math::Aabb m_localAabb;
    };
}
}