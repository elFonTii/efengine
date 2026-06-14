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
            void SetOpacityMap(const Texture* texture);

            const Shader& shader() const { return *m_shader; }

            // escalares para el muestreo de cada tex
            glm::vec3 albedoTint = glm::vec3(1.0f);
            f32 metallic = 0.5f;
            f32 roughness = 1.0f;
            f32 aoStrength = 0.5f;
            f32 heightScale = 0.05f;
            f32 alphaCutoff = 0.5f;

            void Bind() const;

        private:
            static void bindMap(const Shader& shader, const Texture* texture, u32 unit, const char* mapUniform, const char* hasUniform);

            const Shader* m_shader;
            
            const Texture* m_albedo    = null;
            const Texture* m_normal    = null;
            const Texture* m_ao        = null;
            const Texture* m_roughness = null;
            const Texture* m_metallic  = null;
            const Texture* m_height    = null;
            const Texture* m_opacity   = null;
    };
}
}