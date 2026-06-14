#pragma once
#include <efengine/renderer/Mesh.h>

#include <vector>

namespace efengine {
namespace renderer {
    class Model {
    public:
        explicit Model(std::vector<Mesh> meshes);
        const std::vector<Mesh>& meshes() const { return m_meshes; }
    private:
        std::vector<Mesh> m_meshes;
    };
}
}