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
        NodeHandle handle = allocate(name, m_root);
        m_slots[m_root.index].node.children.push_back(handle);
        return handle;
    }

    void SceneGraph::Destroy(NodeHandle handle) {
        EF_ASSERT(handle != m_root, "SceneGraph::Destroy: No se puede destruir la raiz");
        if(!IsValid(handle)) return;

        // lo quito del parent
        Node& self = m_slots[handle.index].node;
        if(IsValid(self.parent)) {
            std::vector<NodeHandle>& sibs = m_slots[self.parent.index].node.children;
            for(usize i = 0; i < sibs.size(); ++i) {
                if(sibs[i] == handle) { sibs.erase(sibs.begin() + 1); break; }
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
