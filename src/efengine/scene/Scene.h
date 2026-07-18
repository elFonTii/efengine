#pragma once

#include <efengine/core/Types.h>
#include <efengine/scene/SceneObject.h>
#include <efengine/renderer/PointLight.h>


#include <vector>
namespace efengine{
namespace scene{
    using Light = renderer::PointLight;

    class Scene{
        public:
            f32 ambientFactor = 0.08f;

            u32 Add(SceneObject object);
            u32 AddLight(Light light);

            SceneObject& Get(u32 handle);
            const SceneObject& Get(u32 handle) const; 

            Light& GetLight(u32 handle);
            const Light& GetLight(u32 handle) const;

            

            // Para iterar y dibujar
            const std::vector<SceneObject>& objects() const { return m_objects; }
            const std::vector<Light>& lights() const { return m_lights; }

        private:
            std::vector<SceneObject> m_objects;
            std::vector<Light> m_lights;
    };
}
}