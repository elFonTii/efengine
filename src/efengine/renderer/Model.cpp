#include "efengine/renderer/Model.h"

#include <utility>

namespace efengine {
namespace renderer {
    Model::Model(std::vector<Mesh> meshes) : m_meshes(std::move(meshes)) {
        for (const Mesh& mesh : m_meshes) {
            m_localAabb = math::MergeAabb(m_localAabb, mesh.localAabb());
        }
    }
}
}
