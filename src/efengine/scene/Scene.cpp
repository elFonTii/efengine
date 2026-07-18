#include "efengine/scene/Scene.h"
#include <efengine/core/Assert.h>
#include <utility>

namespace efengine {
namespace scene {
    // ADDITIONS
    u32 Scene::Add(SceneObject object) {
        const u32 handle = static_cast<u32>(m_objects.size());
        m_objects.push_back(std::move(object));

        return handle;
    }
    u32 Scene::AddLight(Light light) {
        const u32 handle = static_cast<u32>(m_lights.size());
        m_lights.push_back(std::move(light));

        return handle;
    }

    // GETTERS
    SceneObject& Scene::Get(u32 handle) {
        EF_ASSERT(handle < m_objects.size(), "Scene::Get: Se intenta acceder a un índice fuera de rango");

        return m_objects[handle];
    }
    const SceneObject& Scene::Get(u32 handle) const { 
        EF_ASSERT(handle < m_objects.size(), "Scene::Get: Se intenta acceder a un índice fuera de rango (const)");

        return m_objects[handle];
    }
    Light& Scene::GetLight(u32 handle) {
        EF_ASSERT(handle < m_lights.size(), "Scene::GetLight: Se intenta acceder a un índice fuera de rango");

        return m_lights[handle];
    }
    const Light& Scene::GetLight(u32 handle) const {
        EF_ASSERT(handle < m_lights.size(), "Scene::GetLight: Se intenta acceder a un índice fuera de rango");

        return m_lights[handle];
    }   
}
}