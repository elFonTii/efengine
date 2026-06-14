#include "efengine/renderer/Model.h"

#include <utility>

namespace efengine {
namespace renderer {
    Model::Model(std::vector<Mesh> meshes) : m_meshes(std::move(meshes)) {}
}
}
