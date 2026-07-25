#pragma once
#include <efengine/scene/Node.h>
#include <efengine/renderer/PointLight.h>
#include <efengine/renderer/DirectionalLight.h>

#include <glm/glm.hpp>
#include <vector>
#include <string>

namespace efengine {
namespace scene {
    
    struct RenderItem {  // combo por frame, queda como vista de salida
        glm::mat4                    world;
        const renderer::Model*       model;
        const renderer::MaterialMap* materials;
    };

    // grafo con la identidad de los nodos.

    class SceneGraph {
        public:
            f32 ambientFactor = 0.08f;

            SceneGraph();

            // Ciclo de vida
            NodeHandle CreateNode(const std::string& name = "");
            void       Destroy(NodeHandle handle);

            // Getters
            bool    IsValid(NodeHandle handle) const;
            Node*   TryGet(NodeHandle handle);
            Node&   Get(NodeHandle handle);

            NodeHandle FindByName(const std::string& name) const;

            // Jerarquía
            NodeHandle Root() const { return m_root; }
            NodeHandle CreateChild(NodeHandle parent, const std::string& name = "");
            void SetLocalTransform(NodeHandle handle, const math::Transform& transform);

        private:
            struct Slot {
                Node node;
                u32 generation  = 0; // > 1 si la instancia está viva
                bool alive      = false;
            };

            NodeHandle allocate(const std::string& name, NodeHandle parent);
            void markSubtreeDirty(NodeHandle handle);
            void updateNode(NodeHandle handle, const glm::mat4& parentWorld, bool parentChanged);
            
            std::vector<Slot> m_slots;
            std::vector<u32>  m_freeList;
            NodeHandle        m_root;
    };
}
}