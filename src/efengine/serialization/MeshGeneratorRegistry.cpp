#include "efengine/serialization/MeshGeneratorRegistry.h"
#include <efengine/core/Assert.h>
#include <efengine/core/Log.h>

namespace efengine {
namespace serialization {

    void MeshGeneratorRegistry::Register(const char* name, CreateFn fn) {
        EF_ASSERT(name != null && fn != null, "MeshGeneratorRegistry::Register: nombre o fn nulos");
        auto [it, ok] = m_byName.emplace(name, fn);
        if (!ok) EF_LOG_WARNING("MeshGeneratorRegistry: '%s' ya estaba registrado", name);
    }

    MeshGeneratorRegistry::CreateFn MeshGeneratorRegistry::Find(std::string_view name) const {
        auto it = m_byName.find(std::string(name));
        return (it != m_byName.end()) ? it->second : null;
    }

}
}
