#pragma once
#include <efengine/core/Types.h>
#include <efengine/renderer/AoSettings.h>
#include <efengine/renderer/AoContext.h>
#include <efengine/renderer/Framebuffer.h>
#include <efengine/renderer/ShaderBlocks.h>
#include <efengine/renderer/UniformBuffer.h>

#include <glm/glm.hpp>
#include <optional>

namespace efengine {
namespace scene { class SceneGraph; }
namespace renderer {

    class Renderer;
    class Shader;
    class VertexArray;

    // Dueno de los tres targets del AO. Orquesta prepass -> gtao -> blur.
    //
    // Corre ANTES de BeginScene, junto a ShadowPass y DdgiPass: si el AO tiene
    // que modular la indirecta DENTRO de pbr.frag, tiene que estar calculado
    // antes de que el forward sombree. Esa es la restriccion que obliga al
    // prepass propio en vez de sacar las normales del forward con MRT.
    //
    // Create devuelve nullopt si falta cualquier shader (calcado de DdgiPass):
    // sin AoPass, Application pasa un AoContext vacio y pbr.frag cae al ao del
    // material. Un fallo de shader no rompe el frame.
    class AoPass {
        public:
            struct Shaders {
                Shader* depthNormal = null;
                Shader* gtao        = null;
                Shader* denoise     = null;
            };

            static std::optional<AoPass> Create(Renderer& renderer, VertexArray& fullscreenQuad,
                                                const Shaders& shaders, u32 width, u32 height);

            AoPass(const AoPass&)            = delete;
            AoPass& operator=(const AoPass&) = delete;
            AoPass(AoPass&& other) noexcept;
            AoPass& operator=(AoPass&& other) noexcept;

            // Dibuja el prepass y corre el kernel. La view y la projection son
            // las de la camara del frame; no se leen del bloque Frame porque
            // este pase corre antes de BeginScene.
            void Render(const scene::SceneGraph& scene,
                        const glm::mat4& view, const glm::mat4& projection);

            void Resize(u32 width, u32 height);

            AoContext Context() const;

            AoSettings&       settings()       { return m_settings; }
            const AoSettings& settings() const { return m_settings; }

            const Texture& normalTarget() const { return m_normalFb.ColorTexture(); }
            const Texture& aoTexture()    const;

        private:
            AoPass(Renderer& renderer, VertexArray& fullscreenQuad, const Shaders& shaders,
                   u32 width, u32 height);

            Renderer&    m_renderer;
            VertexArray& m_quad;
            Shaders      m_shaders;

            Framebuffer m_normalFb;   // xyz = normal view, w = viewZ lineal
            Framebuffer m_aoA;        // xyz = bent normal world, w = visibilidad
            Framebuffer m_aoB;        // scratch del blur separable

            AoSettings m_settings;

            // Dos UBOs y no uno compartido: los dos bloques van al mismo binding
            // 4 pero tienen layouts distintos, y un solo buffer reusado invita al
            // bug de subir uno y leer el otro.
            UniformBuffer m_prepassUbo { sizeof(AoPrepassBlock) };
            UniformBuffer m_gtaoUbo    { sizeof(AoPassBlock) };

            // Cual de los dos FBOs tiene el resultado. Con blur apagado es A;
            // con blur, la pasada vertical vuelve a dejarlo en A.
            bool m_resultInA = true;
    };

}
}
