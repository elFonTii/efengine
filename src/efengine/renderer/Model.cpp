#include "efengine/renderer/Model.h"

#include <utility>

namespace efengine {
namespace renderer {
    Model::Model(std::vector<Mesh> meshes)
        : m_meshes(std::move(meshes)), m_bounds(AABB::Empty()) {
        for (const Mesh& mesh : m_meshes) {
            m_bounds = AABB::Merge(m_bounds, mesh.bounds());
        }
    }
}
}
