#include "efengine/serialization/BehaviorRegistry.h"
#include <efengine/core/Log.h>

namespace efengine {
namespace serialization {

    void BehaviorRegistry::add(std::type_index type, Entry entry) {
        const std::string name = entry.name;

        auto [it, ok] = m_byType.emplace(type, std::move(entry));
        if (!ok) {
            EF_LOG_WARNING("BehaviorRegistry: el tipo de '%s' ya estaba registrado", name.c_str());
            return;
        }
        // Los valores de un unordered_map viven en nodos: el puntero es estable.
        auto [it2, ok2] = m_byName.emplace(name, &it->second);
        if (!ok2) {
            EF_LOG_WARNING("BehaviorRegistry: el nombre '%s' ya estaba usado", name.c_str());
            m_byType.erase(it);
        }
    }

    const BehaviorRegistry::Entry* BehaviorRegistry::FindByType(const scene::Behavior& behavior) const {
        // typeid sobre una referencia a tipo polimorfico da el tipo DINAMICO real.
        auto it = m_byType.find(std::type_index(typeid(behavior)));
        return (it != m_byType.end()) ? &it->second : null;
    }

    const BehaviorRegistry::Entry* BehaviorRegistry::FindByName(std::string_view name) const {
        auto it = m_byName.find(std::string(name));
        return (it != m_byName.end()) ? it->second : null;
    }

}
}
