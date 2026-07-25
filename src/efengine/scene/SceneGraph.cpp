#include "efengine/scene/SceneGraph.h"
#include <efengine/core/Assert.h>

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

    void SceneGraph::UpdateWorldTransforms() {
        updateNode(m_root, glm::mat4(1.0f), false);
    }

    void SceneGraph::updateNode(NodeHandle handle, const glm::mat4& parentWorld, bool parentChanged) {
        Node& node = m_slots[handle.index].node;
        bool recompute = node.worldDirty || parentChanged;
        if (recompute) {
            node.worldMatrix = parentWorld * node.local.Matrix();
            node.worldDirty = false;
        }
        for (NodeHandle child : node.children) {
            if (IsValid(child)) updateNode(child, node.worldMatrix, recompute);
        }
    }

    void SceneGraph::Destroy(NodeHandle handle) {
        EF_ASSERT(handle != m_root, "SceneGraph::Destroy: No se puede destruir la raiz");
        if(!IsValid(handle)) return;

        // lo quito del parent
        Node& self = m_slots[handle.index].node;
        if(IsValid(self.parent)) {
            std::vector<NodeHandle>& sibs = m_slots[self.parent.index].node.children;
            for(usize i = 0; i < sibs.size(); ++i) {
                if(sibs[i] == handle) { sibs.erase(sibs.begin() + i); break; }
            }
        }

        Slot& slot = m_slots[handle.index];
        slot.alive = false;
        slot.generation++;
        m_freeList.push_back(handle.index);
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

    NodeHandle SceneGraph::FindByName(const std::string& name) const {
        for (const Slot& slot : m_slots) {
            if (slot.alive && slot.node.name == name) return slot.node.self;
        }
        return NodeHandle{};
    }
    
}  
}
