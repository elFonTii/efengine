#include "efengine/scene/SceneGraph.h"
#include <efengine/core/Assert.h>
#include <efengine/core/Log.h>
#include <utility>

namespace efengine {
namespace scene {
    SceneGraph::SceneGraph() { m_root = allocate("root", NodeHandle{}); } // la raiz existe siempre, un transform pelado, no renderiza

    NodeHandle SceneGraph::allocate(const std::string& name, NodeHandle parent) {
        u32 index;

        if(!m_freeList.empty()) { // tiene que estar root ya poblado
            // hago una copia y elimino el último indice de los no alocados
            index = m_freeList.back();
            m_freeList.pop_back(); 
        } else {
            // asigno indice y lo pusheo al final
            index = static_cast<u32>(m_slots.size());
            m_slots.push_back(Slot{});
            m_slots[index].generation = 0;
        }

        Slot& slot = m_slots[index]; // obtengo el slot por indice
        slot.alive = true;
        if(slot.generation == 0) { slot.generation = 1; }
        slot.node            = Node{};
        slot.node.self       = NodeHandle{ index, slot.generation };
        slot.node.name       = name;
        slot.node.parent     = parent;
        slot.node.worldDirty = true;

        return slot.node.self;
    }

    NodeHandle SceneGraph::CreateNode(const std::string& name) {
        return CreateChild(m_root, name);
    }

    NodeHandle SceneGraph::CreateChild(NodeHandle parent, const std::string& name) {
        EF_ASSERT(IsValid(parent), "SceneGraph::CreateChild: padre invalido");
        NodeHandle handle = allocate(name, parent);
        m_slots[parent.index].node.children.push_back(handle);
        return handle;
    }

    void SceneGraph::SetLocalTransform(NodeHandle handle, const math::Transform& transform) {
        EF_ASSERT(IsValid(handle), "SceneGraph::SetLocalTransform: handle invalido");
        m_slots[handle.index].node.local = transform;
        markSubtreeDirty(handle);
    }

    // El world de un hijo depende del de su padre: si cambia el local de un nodo,
    // todo lo de abajo también queda dirty
    void SceneGraph::markSubtreeDirty(NodeHandle handle) {
        Node& node = m_slots[handle.index].node;
        node.worldDirty = true;
        for (NodeHandle child : node.children) {
            if (IsValid(child)) markSubtreeDirty(child);
        }
    }

    // Camina hacia arriba desde 'of' por la cadena de padres. O(profundidad).
    bool SceneGraph::isAncestorOrSelf(NodeHandle maybeAncestor, NodeHandle of) const {
        NodeHandle cur = of;
        while (IsValid(cur)) {
            if (cur == maybeAncestor) return true;
            cur = m_slots[cur.index].node.parent;
        }
        return false;
    }

    void SceneGraph::SetParent(NodeHandle child, NodeHandle newParent) {
        EF_ASSERT(IsValid(child),     "SceneGraph::SetParent: child invalido");
        EF_ASSERT(IsValid(newParent), "SceneGraph::SetParent: newParent invalido");
        EF_ASSERT(child != m_root,    "SceneGraph::SetParent: la raiz no tiene padre");

        if (isAncestorOrSelf(child, newParent)) {
            EF_LOG_WARNING("SceneGraph::SetParent: reparent rechazado (crearia un ciclo)");
            return;
        }

        // Sacar del padre viejo.
        NodeHandle oldParent = m_slots[child.index].node.parent;
        if (IsValid(oldParent)) {
            std::vector<NodeHandle>& sibs = m_slots[oldParent.index].node.children;
            for (usize i = 0; i < sibs.size(); ++i) {
                if (sibs[i] == child) { sibs.erase(sibs.begin() + i); break; }
            }
        }

        // Enganchar al padre nuevo. El local se preserva; el world cambia.
        m_slots[child.index].node.parent = newParent;
        m_slots[newParent.index].node.children.push_back(child);

        markSubtreeDirty(child);
    }

    void SceneGraph::AttachMesh(NodeHandle handle, MeshAttachment mesh) {
        EF_ASSERT(IsValid(handle), "SceneGraph::AttachMesh: handle invalido");
        m_slots[handle.index].node.mesh = std::move(mesh);
    }

    void SceneGraph::DetachMesh(NodeHandle handle) {
        if (!IsValid(handle)) return;
        m_slots[handle.index].node.mesh.reset();
    }

    void SceneGraph::AttachLight(NodeHandle handle, LightAttachment light) {
        EF_ASSERT(IsValid(handle), "SceneGraph::AttachLight: handle invalido");
        m_slots[handle.index].node.light = light;
    }

    Behavior* SceneGraph::AttachBehavior(NodeHandle handle, std::unique_ptr<Behavior> behavior) {
        EF_ASSERT(IsValid(handle), "SceneGraph::AttachBehavior: handle invalido");
        Node& node = m_slots[handle.index].node;
        node.behaviors.push_back(std::move(behavior));
        return node.behaviors.back().get();
    }

    void SceneGraph::Update(f32 dt) {
        for (Slot& slot : m_slots) {
            if (!slot.alive) continue;
            Node& node = slot.node;
            if (node.behaviors.empty()) continue;

            UpdateContext ctx{ *this, node.self, node, dt };
            for (std::unique_ptr<Behavior>& b : node.behaviors) {
                if (b && b->enabled) b->OnUpdate(ctx);
            }
        }
    }

    void SceneGraph::SetPrimarySun(NodeHandle handle) {
        EF_ASSERT(IsValid(handle), "SceneGraph::SetPrimarySun: handle invalido");
        m_primarySun = handle;
    }

    void SceneGraph::UpdateWorldTransforms() {
        // Sin el clear las listas se duplicarian en cada frame.
        m_renderables.clear();
        m_pointLights.clear();
        m_worldBounds = renderer::AABB::Empty();

        updateNode(m_root, glm::mat4(1.0f), false);

        // La caja de la escena sale de la lista que updateNode acaba de armar, no
        // de recorrer el grafo otra vez.
        for (const RenderItem& item : m_renderables) {
            if (item.model == null) continue;
            m_worldBounds = renderer::AccumulateBounds(m_worldBounds, item.model->bounds(), item.world);
        }

        // Sol: direccion desde el world del nodo primario; color desde su adjunto.
        if (IsValid(m_primarySun)) {
            const Node& sun = m_slots[m_primarySun.index].node;
            if (sun.light && sun.light->kind == LightKind::Directional) {
                // w=0 anula la traslacion: una direccion no tiene posicion.
                glm::vec3 fwd = glm::vec3(sun.worldMatrix * glm::vec4(0.0f, 0.0f, -1.0f, 0.0f));
                if (glm::length(fwd) > 0.0f) m_sun.direction = glm::normalize(fwd);
                m_sun.color = sun.light->color;
            }
        }
    }

    void SceneGraph::updateNode(NodeHandle handle, const glm::mat4& parentWorld, bool parentChanged) {
        Node& node = m_slots[handle.index].node;
        bool recompute = node.worldDirty || parentChanged;
        if (recompute) {
            node.worldMatrix = parentWorld * node.local.Matrix();
            node.worldDirty = false;
        }

        if (node.mesh && node.mesh->model) {
            m_renderables.push_back(RenderItem{ node.worldMatrix, node.mesh->model, &node.mesh->materials });
        }
        if (node.light && node.light->kind == LightKind::Point) {
            // La columna 3 de la matriz world es la traslacion.
            m_pointLights.push_back(renderer::PointLight{ glm::vec3(node.worldMatrix[3]), node.light->color });
        }

        for (NodeHandle child : node.children) {
            if (IsValid(child)) updateNode(child, node.worldMatrix, recompute);
        }
    }

    void SceneGraph::Destroy(NodeHandle handle) {
        EF_ASSERT(handle != m_root, "SceneGraph::Destroy: No se puede destruir la raiz");
        if(!IsValid(handle)) return;

        // 1   lo quito del parent (una sola vez arriba)
        Node& self = m_slots[handle.index].node;
        if(IsValid(self.parent)) {
            std::vector<NodeHandle>& sibs = m_slots[self.parent.index].node.children;
            for(usize i = 0; i < sibs.size(); ++i) {
                if(sibs[i] == handle) { sibs.erase(sibs.begin() + i); break; }
            }
        }

        // 2 libero el subarbol completo
        destroySubtree(handle);
    }

    void SceneGraph::Clear() {

        for (Slot& slot : m_slots) {
            if (slot.alive) {
                slot.alive = false;
                slot.generation++;   // invalida el generation viejo
            }
            slot.node = Node{};     // libera los behaviors
        }

        // Se hace pop_back, asi se comienza desde el indice 0 (root node)
        m_freeList.clear();
        for (usize i = m_slots.size(); i > 0u; --i) {
            m_freeList.push_back(static_cast<u32>(i - 1u));
        }

        m_primarySun = NodeHandle{};
        m_renderables.clear();
        m_pointLights.clear();
        m_sun = renderer::DirectionalLight{ glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(0.0f) };

        m_root = allocate("root", NodeHandle{});
    }
    

    void SceneGraph::destroySubtree(NodeHandle handle) {
        if(!IsValid(handle)) return;

        std::vector<NodeHandle> kids = m_slots[handle.index].node.children;
        for(NodeHandle child : kids) destroySubtree(child);

        Slot& slot = m_slots[handle.index];
        slot.alive = false;
        slot.generation++;
        m_freeList.push_back(handle.index);
        slot.node.behaviors.clear();
    }

    bool SceneGraph::IsValid(NodeHandle handle) const {
         // es valido cuando:
         // generation != 0, h.alive = true y i.gen == h.gen
        return !handle.IsNull()
            && handle.index < m_slots.size()
            && m_slots[handle.index].alive
            && m_slots[handle.index].generation == handle.generation;
    }

    Node* SceneGraph::TryGet(NodeHandle handle) {
        return IsValid(handle) ? &m_slots[handle.index].node : nullptr;
    }

    Node& SceneGraph::Get(NodeHandle handle) {
        EF_ASSERT(IsValid(handle), "SceneGraph::Get: Se intenta obtener un slot invalido");
        return m_slots[handle.index].node;
    }

    const Node* SceneGraph::TryGet(NodeHandle handle) const {
        return IsValid(handle) ? &m_slots[handle.index].node : nullptr;
    }

    const Node& SceneGraph::Get(NodeHandle handle) const {
        EF_ASSERT(IsValid(handle), "SceneGraph::Get(const): Se intenta obtener un slot invalido");
        return m_slots[handle.index].node;
    }

    NodeHandle SceneGraph::FindByName(const std::string& name) const {
        for (const Slot& slot : m_slots) {
            if (slot.alive && slot.node.name == name) return slot.node.self;
        }
        return NodeHandle{};
    }
    
}  
}
