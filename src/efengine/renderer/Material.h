#pragma once
#include <efengine/core/Types.h>
#include <efengine/renderer/ShaderBlocks.h>
#include <glm/glm.hpp>
#include <string>
#include <unordered_map>
#include <vector>

// material no posee, solo contiene observadores para shaders y texturas.
namespace efengine {
namespace renderer {
    class Shader;
    class Texture;

    class Material {
        public:
            explicit Material(const Shader* shader) : m_shader(shader) {}

            void SetAlbedoMap(const Texture* texture);
            void SetNormalMap(const Texture* texture);
            void SetAOMap(const Texture* texture);
            void SetRoughnessMap(const Texture* texture);
            void SetMetallicMap(const Texture* texture);
            void SetHeightMap(const Texture* texture);
            void SetOpacityMap(const Texture* texture);
            void SetEmissiveMap(const Texture* texture);

            const Shader& shader() const { return *m_shader; }

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

            // Elige el PipelineState del draw: OpaqueState u OpaqueDoubleSidedState.
            bool      doubleSided       = false;

            // Tiling de la UV, comun a los 8 mapas: uv = vUV * uvTiling + uvOffset.
            // (1,1)/(0,0) es la identidad — la UV de la malla tal cual. Es un solo
            // par para todos los mapas a proposito: un set PBR desalineado entre
            // albedo y normal siempre es un bug, nunca una decision artistica.
            glm::vec2 uvTiling = glm::vec2(1.0f);
            glm::vec2 uvOffset = glm::vec2(0.0f);

            // Bindea los mapas del material a sus unidades (0-7, el valor de su
            // TextureSlot). NO bindea el shader ni sube nada al UBO: los samplers
            // ya saben su unidad por layout(binding=N) en el GLSL.
            void BindTextures() const;

            // Empaqueta los escalares y la presencia de cada mapa en el bloque
            // std140. Funcion pura: no toca la GPU. El UBO lo posee el Renderer,
            // no el Material — meterle un handle aca rompe MaterialMap.test.cpp,
            // que construye un Material headless.
            MaterialBlock ToBlock() const;

        private:
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