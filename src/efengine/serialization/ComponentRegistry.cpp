#include "efengine/serialization/ComponentRegistry.h"
#include <efengine/core/Log.h>

namespace efengine {
namespace serialization {

    void ComponentRegistry::add(Entry entry) {
        const std::string name = entry.name;

        // El indice se reserva antes de mover la entry: si el nombre ya estaba,
        // no se inserta nada y el vector queda igual que antes.
        auto [it, ok] = m_byName.emplace(name, static_cast<u32>(m_entries.size()));
        if (!ok) {
            EF_LOG_WARNING("ComponentRegistry: el nombre '%s' ya estaba usado", name.c_str());
            return;
        }
        m_entries.push_back(std::move(entry));
    }

    const ComponentRegistry::Entry* ComponentRegistry::FindByName(std::string_view name) const {
        auto it = m_byName.find(std::string(name));
        return (it != m_byName.end()) ? &m_entries[it->second] : null;
    }

}
}
