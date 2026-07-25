#pragma once
#include <efengine/scene/NodeHandle.h>
#include <efengine/scene/Behavior.h>
#include <efengine/math/Transform.h>
#include <efengine/renderer/Model.h>
#include <efengine/renderer/Material.h>

#include <glm/glm.hpp>
#include <string>
#include <vector>
#include <optional>
#include <memory>

namespace efengine {
namespace scene {
    enum class LightKind { Point, Directional }; // TODO: extraer / separar

    struct MeshAttachment { 
        const renderer::Model* model = nullptr;
        renderer::MaterialMap  materials;
    };

    struct LightAttachment {
        LightKind kind = LightKind::Point;
        glm::vec3 color { 1.0f };
    };

    class Node {
        public: 
            NodeHandle self; // yo mismo
            std::string name;

            math::Transform         local; // transform relativo al padre (parent.transform * local.transform)
            glm::mat4               worldMatrix {1.0f};
            bool                    worldDirty = true; // ¿recalcular?

            NodeHandle              parent;
            std::vector<NodeHandle> children;

            // con esta estructura podemos expandir a X attachments (scripts, colliders, etc...)
            std::optional<MeshAttachment> mesh;
            std::optional<LightAttachment> light;
            std::vector<std::unique_ptr<Behavior>> behaviors; // runtime scripts

            Node()                       = default;
            Node(const Node&)            = delete;
            Node& operator=(const Node&) = delete;
            Node(Node&&)                 = default;
            Node& operator=(Node&&)      = default;
    };
}
}
