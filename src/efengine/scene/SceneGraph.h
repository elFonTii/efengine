#pragma once
#include <efengine/scene/Node.h>
#include <efengine/renderer/PointLight.h>
#include <efengine/renderer/DirectionalLight.h>

#include <glm/glm.hpp>
#include <vector>
#include <string>
#include <memory>

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
            void       Clear(); 


            // Getters (const vs no const ->)
            bool    IsValid(NodeHandle handle) const;
            Node*   TryGet(NodeHandle handle);
            Node&   Get(NodeHandle handle);
            // con const SceneGraph& el comp elige estas versiones
            const Node*  TryGet(NodeHandle handle) const; 
            const Node&  Get(NodeHandle handle) const;

            NodeHandle FindByName(const std::string& name) const;

            // Jerarquía
            NodeHandle Root() const { return m_root; }
            NodeHandle CreateChild(NodeHandle parent, const std::string& name = "");
            void SetLocalTransform(NodeHandle handle, const math::Transform& transform);
            void SetParent(NodeHandle child, NodeHandle newParent);

            // Transforms
            void UpdateWorldTransforms();

            // Adjuntos
            void AttachMesh(NodeHandle handle, MeshAttachment mesh);

            // Saca la malla del nodo. No toca hijos, luz ni behaviors. Handle
            // invalido o nodo sin malla: no-op silencioso (la UI puede pedirlo
            // sobre un nodo que ya se destruyo).
            void DetachMesh(NodeHandle handle);

            void AttachLight(NodeHandle handle, LightAttachment light);

            // Behaviors
            Behavior* AttachBehavior(NodeHandle handle, std::unique_ptr<Behavior> behavior);
            void      Update(f32 dt);

            void       SetPrimarySun(NodeHandle handle);
            NodeHandle PrimarySun() const { return m_primarySun; }

            const std::vector<RenderItem>&           Renderables() const { return m_renderables; }
            const std::vector<renderer::PointLight>& PointLights() const { return m_pointLights; }
            const renderer::DirectionalLight&        Sun()         const { return m_sun; }

        private:
            struct Slot {
                Node node;
                u32 generation  = 0; // > 1 si la instancia está viva
                bool alive      = false;
            };

            NodeHandle allocate(const std::string& name, NodeHandle parent);
            void destroySubtree(NodeHandle handle);
            bool isAncestorOrSelf(NodeHandle maybeAncestor, NodeHandle of) const;
            void markSubtreeDirty(NodeHandle handle);
            void updateNode(NodeHandle handle, const glm::mat4& parentWorld, bool parentChanged);
            
            std::vector<Slot> m_slots;
            std::vector<u32>  m_freeList;
            NodeHandle        m_root;

            // Estado juntado por UpdateWorldTransforms
            NodeHandle                        m_primarySun;
            std::vector<RenderItem>           m_renderables;
            std::vector<renderer::PointLight> m_pointLights;
            renderer::DirectionalLight        m_sun { glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(0.0f) };
    };
}
}