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
    // Spot al final: mismo orden que LightKindId del .efe.
    enum class LightKind { Point, Directional, Spot };

    struct MeshAttachment { 
        const renderer::Model* model = nullptr;
        renderer::MaterialMap  materials;
    };

    struct LightAttachment {
        LightKind kind = LightKind::Point;
        glm::vec3 color { 1.0f };            // tinte 0-1
        f32  intensity      = 1.0f;
        bool useTemperature = false;
        f32  temperatureK   = 6500.0f;
        f32  range          = 10.0f;         // m, Point y Spot
        f32  innerConeDeg   = 30.0f;         // semiangulo, Spot
        f32  outerConeDeg   = 45.0f;         // semiangulo, Spot
        f32  sourceRadius   = 0.0f;          // m, Point y Spot
        bool castShadows    = false;         // reservado para el ciclo 2
    };

    struct CameraAttachment {
        f32 fovDeg    = 45.0f;    // vertical, en GRADOS: igual que Camera::Fov
        f32 nearPlane = 0.1f;
        f32 farPlane  = 5000.0f;
        f32 exposure  = 1.025f;
    };

    // Descripcion de un cuerpo, NO un handle ni un tipo del motor de fisica.
    // Vive en scene para que el .efe no quede atado al backend: el puente que
    // llega en el ciclo 3 traduce esta descripcion a una forma de Jolt. En este
    // ciclo lo unico que la toca es el serializador.
    enum class ShapeKind  { Box, Sphere, Capsule, Mesh };
    enum class MotionType { Static, Kinematic, Dynamic };

    struct ColliderAttachment {
        ShapeKind       kind = ShapeKind::Box;
        // Box: halfExtents - Sphere: (r, ., .) - Capsule: (r, halfHeight, .)
        glm::vec3       params { 0.5f };
        math::Transform localOffset;   // respecto del nodo
        MotionType      motion = MotionType::Static;
        bool            isTrigger = false;
    };

    class Node {
        public: 
            NodeHandle self;
            std::string name;

            math::Transform         local; // transform relativo al padre (parent.transform * local.transform)
            glm::mat4               worldMatrix {1.0f};
            bool                    worldDirty = true;
            // El world del frame renderizado anterior. Lo mantiene
            // SceneGraph::UpdateWorldTransforms; prevValid en false hace que el
            // proximo frame no tenga estela (nodo nuevo, teleport, carga).
            glm::mat4               prevWorldMatrix {1.0f};
            bool                    prevValid = false;

            NodeHandle              parent;
            std::vector<NodeHandle> children;

            // con esta estructura podemos expandir a X attachments (scripts, colliders, etc...)
            std::optional<MeshAttachment> mesh;
            std::optional<LightAttachment> light;
            std::optional<CameraAttachment>   camera;
            std::optional<ColliderAttachment> collider;
            std::vector<std::unique_ptr<Behavior>> behaviors;

            Node()                       = default;
            Node(const Node&)            = delete;
            Node& operator=(const Node&) = delete;
            Node(Node&&)                 = default;
            Node& operator=(Node&&)      = default;
    };
}
}
