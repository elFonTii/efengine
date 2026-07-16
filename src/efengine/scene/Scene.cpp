#include "efengine/scene/Scene.h"
#include <efengine/core/Assert.h>
#include <utility>

namespace efengine {
namespace scene {

    u32 Scene::Add(SceneObject object) {
        const u32 handle = static_cast<u32>(m_objects.size());
        m_objects.push_back(std::move(object));

        return handle;
    }

    SceneObject& Scene::Get(u32 handle) {
        EF_ASSERT(handle < m_objects.size(), "Scene::Get: Se intenta acceder a un índice fuera de rango");

        return m_objects[handle];
    }

    const SceneObject& Scene::Get(u32 handle) const { 
        EF_ASSERT(handle < m_objects.size(), "Scene::Get: Se intenta acceder a un índice fuera de rango (const)");

        return m_objects[handle];
    }

}
}