#pragma once

#include <efengine/core/Types.h>
#include <efengine/scene/SceneObject.h>

#include <vector>

namespace efengine{
namespace scene{
    class Scene{
        public:
            u32 Add(SceneObject object);

            SceneObject& Get(u32 handle);
            const SceneObject& Get(u32 handle) const; 

            // Para iterar y dibujar
            const std::vector<SceneObject>& objects() const { return m_objects; }

        private:
            std::vector<SceneObject> m_objects;
    };
}
}