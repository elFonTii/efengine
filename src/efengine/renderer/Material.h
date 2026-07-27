#pragma once
#include <efengine/core/Types.h>
#include <glm/glm.hpp>
#include <string>
#include <unordered_map>
#include <vector>

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
            void SetEmissiveMap(const Texture* texture);

            const Shader& shader() const { return *m_shader; }

            // escalares para el muestreo de cada tex
            glm::vec3 albedoTint = glm::vec3(1.0f);
            f32 metallic = 0.5f;
            f32 roughness = 1.0f;
            f32 aoStrength = 0.5f;
            f32 heightScale = 0.05f;
            f32 alphaCutoff = 0.5f;

            // Emision propia: no la modula el AO, ni la sombra, ni la intensidad de IBL.
            glm::vec3 emissiveTint      = glm::vec3(1.0f);
            f32       emissiveIntensity = 0.0f;
            // Escala la componente tangencial del normal map. 1 = el mapa tal cual.
            f32       normalStrength    = 1.0f;

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
            const Texture* m_emissive  = null;
    };

    using MaterialMap = std::unordered_map<std::string, const Material*>;

    // Un mismo material para todos los submeshes. Toma NOMBRES y no un Model
    // porque construir un Mesh crea un VAO y necesita contexto GL: asi esto se
    // puede testear headless. material == null devuelve mapa vacio, nunca
    // entradas nulas (son las que hacen que el renderer loguee por frame).
    MaterialMap MakeUniformMaterialMap(const std::vector<std::string>& submeshNames,
                                       const Material* material);
}
}