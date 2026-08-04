#pragma once
#include <efengine/scene/Node.h>
#include <efengine/scene/Components.h>
#include <efengine/scene/ComponentStore.h>
#include <efengine/renderer/PointLight.h>
#include <efengine/renderer/DirectionalLight.h>
#include <efengine/renderer/Bounds.h>
#include <efengine/core/Assert.h>

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

    class SceneGraph {
        public:
            f32 iblIntensity = 1.0f;

            SceneGraph();

            // Ciclo de vida
            NodeHandle CreateNode(const std::string& name = "");
            void       Destroy(NodeHandle handle);
            void       Clear(); 


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

            // Reparenta preservando posicion, rotacion y escala EN EL MUNDO: el
            // nodo no se mueve, se le recalcula el local contra el nuevo padre.
            // Mismos rechazos que SetParent (raiz, handles invalidos, ciclos), y
            // en esos casos NO toca el local.
            //
            // Si el nuevo padre tiene un eje en escala 0 su matriz es singular y
            // preservar el mundo es imposible: loguea un warning y cae a
            // preservar el local, como SetParent.
            void SetParentKeepWorld(NodeHandle child, NodeHandle newParent);

            // Camina hacia arriba desde 'of' por la cadena de padres. Publico
            // porque la UI lo necesita para no ofrecer un drop que crearia un
            // ciclo: sin esto el editor tendria que reimplementar el mismo
            // recorrido que el motor ya sabe hacer.
            bool IsAncestorOrSelf(NodeHandle maybeAncestor, NodeHandle of) const;

            // Transforms
            void UpdateWorldTransforms();

            // Adjuntos (componentes)
            //
            // La forma generica: un tipo nuevo de adjunto no agrega metodos aca.
            // Attach assertea con handle invalido (adjuntar a un nodo muerto es
            // un bug del caller); Detach y las consultas son no-op silencioso,
            // porque la UI puede preguntar por un nodo que ya se destruyo.
            template <class T>
            void Attach(NodeHandle handle, T value) {
                EF_ASSERT(IsValid(handle), "SceneGraph::Attach: handle invalido");
                m_components.Set<T>(handle.index, std::move(value));
            }

            template <class T>
            void Detach(NodeHandle handle) {
                if (!IsValid(handle)) return;
                m_components.Remove<T>(handle.index);
            }

            // El puntero apunta adentro del array del store: no sobrevive a un
            // Attach/Detach de ESE tipo, ni a Clear. Ver ComponentStore.h.
            template <class T>
            T* TryGet(NodeHandle handle) {
                if (!IsValid(handle)) return null;
                return m_components.TryGet<T>(handle.index);
            }

            template <class T>
            const T* TryGet(NodeHandle handle) const {
                if (!IsValid(handle)) return null;
                return m_components.TryGet<T>(handle.index);
            }

            template <class T>
            bool Has(NodeHandle handle) const { return TryGet<T>(handle) != null; }

            // El serializador recorre el store por tipo registrado, sin conocerlos.
            ComponentStore&       Components()       { return m_components; }
            const ComponentStore& Components() const { return m_components; }

            // Envoltorios por comodidad sobre los dos componentes del motor. No
            // hacen nada que Attach<T>/Detach<T> no haga.
            void AttachMesh(NodeHandle handle, MeshAttachment mesh);
            void DetachMesh(NodeHandle handle);
            void AttachLight(NodeHandle handle, LightAttachment light);
            void DetachLight(NodeHandle handle);

            // Behaviors
            Behavior* AttachBehavior(NodeHandle handle, std::unique_ptr<Behavior> behavior);
            void      Update(f32 dt);

            void       SetPrimarySun(NodeHandle handle);
            NodeHandle PrimarySun() const { return m_primarySun; }

            const std::vector<RenderItem>&           Renderables() const { return m_renderables; }
            const std::vector<renderer::PointLight>& PointLights() const { return m_pointLights; }
            const renderer::DirectionalLight&        Sun()         const { return m_sun; }

            // AABB de mundo de todo lo renderizable, recalculada por
            // UpdateWorldTransforms. Invalida (Valid() == false) si la escena no
            // tiene mallas: el caller tiene que chequearlo antes de usar
            // Center() o Radius().
            const renderer::AABB& WorldBounds() const { return m_worldBounds; }

        private:
            struct Slot {
                Node node;
                u32 generation  = 0;
                bool alive      = false;
            };

            NodeHandle allocate(const std::string& name, NodeHandle parent);
            void destroySubtree(NodeHandle handle);
            // World de un nodo compuesto desde los 'local' de su cadena de
            // padres. NO lee Node::worldMatrix: ese cache lo refresca
            // UpdateWorldTransforms, que corre despues de la UI en el frame, asi
            // que al momento de un reparent puede estar sucio.
            glm::mat4 computeWorld(NodeHandle handle) const;
            void markSubtreeDirty(NodeHandle handle);
            void updateNode(NodeHandle handle, const glm::mat4& parentWorld, bool parentChanged);
            
            std::vector<Slot> m_slots;
            std::vector<u32>  m_freeList;
            ComponentStore    m_components;   // paralelo a m_slots, por indice de nodo
            NodeHandle        m_root;

            // Estado juntado por UpdateWorldTransforms
            NodeHandle                        m_primarySun;
            std::vector<RenderItem>           m_renderables;
            std::vector<renderer::PointLight> m_pointLights;
            renderer::DirectionalLight        m_sun { glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(0.0f) };
            renderer::AABB                    m_worldBounds = renderer::AABB::Empty();
    };
}
}