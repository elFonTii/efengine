#pragma once
#include <efengine/core/Types.h>
#include <glm/glm.hpp>

// material no posee, solo contiene observadores para shaders y texturas.
namespace efengine {
namespace renderer {
    // forward dec
    class Shader; 
    class Texture;

    class Material {
        public:
            explicit Material(const Shader* shader) : m_shader(shader) {}

            // setters
            void SetAlbedoMap(const Texture* texture);
            void SetNormalMap(const Texture* texture);
            void SetAOMap(const Texture* texture);
            void SetRoughnessMap(const Texture* texture);
            void SetMetallicMap(const Texture* texture);
            void SetHeightMap(const Texture* texture);

            // escalares para el muestreo de cada tex
            glm::vec3 albedoTint = glm::vec3(1.0f);
            f32 metallic = 0.0f;
            f32 roughness = 0.5f;
            f32 aoStrength = 1.0f;
            f32 heightScale = 0.05f;

            void Bind() const;

        private:
            const Shader* m_shader;
            
            const Texture* m_albedo    = null;
            const Texture* m_normal    = null;
            const Texture* m_ao        = null;
            const Texture* m_roughness = null;
            const Texture* m_metallic  = null;
            const Texture* m_height    = null;
    };
}
}