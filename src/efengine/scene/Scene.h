#pragma once

#include <efengine/core/Types.h>
#include <efengine/scene/SceneObject.h>
#include <efengine/renderer/PointLight.h>
#include <efengine/renderer/DirectionalLight.h>

#include <glm/glm.hpp>
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

            // Sol direccional (una sola luz direccional en la escena).
            renderer::DirectionalLight&       sun()       { return m_sun; }
            const renderer::DirectionalLight& sun() const { return m_sun; }
            void SetSun(renderer::DirectionalLight sun)   { m_sun = sun; }

            // Para iterar y dibujar
            const std::vector<SceneObject>& objects() const { return m_objects; }
            const std::vector<Light>& lights() const { return m_lights; }

        private:
            std::vector<SceneObject> m_objects;
            std::vector<Light> m_lights;
            
            renderer::DirectionalLight m_sun {
                glm::normalize(glm::vec3(-0.3f, -1.0f, -0.2f)),
                glm::vec3(3.0f)
            };
    };
}
}
