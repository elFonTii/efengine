#pragma once
#include <efengine/scene/NodeHandle.h>
#include <efengine/scene/Behavior.h>
#include <efengine/math/Transform.h>

#include <glm/glm.hpp>
#include <string>
#include <vector>
#include <memory>

namespace efengine {
namespace scene {

    class Node {
        public:
            NodeHandle self;
            std::string name;

            math::Transform         local; // transform relativo al padre (parent.transform * local.transform)
            glm::mat4               worldMatrix {1.0f};
            bool                    worldDirty = true;

            NodeHandle              parent;
            std::vector<NodeHandle> children;

            // Los adjuntos (mesh, light, y los que vengan) NO estan aca: viven en
            // el ComponentStore del SceneGraph, indexados por self.index. Un tipo
            // nuevo de adjunto no toca este archivo. Los behaviors si se quedan:
            // son polimorficos y con dueno unico, no datos planos por nodo.
            std::vector<std::unique_ptr<Behavior>> behaviors;

            Node()                       = default;
            Node(const Node&)            = delete;
            Node& operator=(const Node&) = delete;
            Node(Node&&)                 = default;
            Node& operator=(Node&&)      = default;
    };
}
}
