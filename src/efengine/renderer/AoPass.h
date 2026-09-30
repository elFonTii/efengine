#pragma once
#include <efengine/core/Types.h>
#include <efengine/renderer/AoSettings.h>
#include <efengine/renderer/AoContext.h>
#include <efengine/renderer/Framebuffer.h>
#include <efengine/renderer/IScenePass.h>
#include <efengine/renderer/ReducedRes.h>
#include <efengine/renderer/ShaderBlocks.h>
#include <efengine/renderer/UniformBuffer.h>

#include <glm/glm.hpp>
#include <memory>

namespace efengine {
namespace scene { class SceneGraph; }
namespace renderer {

    class Renderer;
    class Shader;
    class VertexArray;

    // Dueno de los targets del AO. Orquesta gtao -> blur sobre el prepass de DepthPrepass.
    //
    // Corre ANTES de BeginScene, junto a ShadowPass y DdgiPass: si el AO tiene
    // que modular la indirecta DENTRO de pbr.frag, tiene que estar calculado
    // antes de que el forward sombree. Esa es la restriccion que obliga al
    // prepass propio en vez de sacar las normales del forward con MRT.
    //
    // Create devuelve nullopt si falta cualquier shader (calcado de DdgiPass):
    // sin AoPass, Application pasa un AoContext vacio y pbr.frag cae al ao del
    // material. Un fallo de shader no rompe el frame.
    class AoPass : public IScenePass {
        public:
            struct Shaders {
                Shader* gtao        = null;
                Shader* denoise     = null;
            };

            static std::unique_ptr<AoPass> Create(Renderer& renderer, VertexArray& fullscreenQuad,
                                                  const Shaders& shaders, u32 width, u32 height);

            AoPass(const AoPass&)            = delete;
            AoPass& operator=(const AoPass&) = delete;
            AoPass(AoPass&& other) noexcept;
            AoPass& operator=(AoPass&& other) noexcept;

            // Corre el kernel sobre el depthNormal del ctx y publica
            // ctx.lighting.ao. La camara sale del ctx y no del bloque Frame
            // porque este pase corre antes del FrameUploadPass.
            void Execute(FrameContext& ctx) override;

            const char* Name() const override { return "AO"; }

            void Resize(u32 width, u32 height) override;

            AoContext Context(const Texture* depthNormal) const;

            // Cuantos texels de resolucion completa cubre uno del target de AO
            // por eje. 1 con halfRes apagado, kReducedScale con el encendido.
            // Lo consultan IndirectPass (para saber en que grilla leer el bent
            // normal) y el panel.
            i32 scale() const;

            AoSettings&       settings()       { return m_settings; }
            const AoSettings& settings() const { return m_settings; }

            const Texture& aoTexture()    const;

        private:
            AoPass(Renderer& renderer, VertexArray& fullscreenQuad, const Shaders& shaders,
                   u32 width, u32 height);

            // El trabajo real; lo llama Execute, que publica el contexto
            // despues -- afuera, porque esto tiene retornos tempranos.
            void Render(const Texture& depthNormal, const glm::mat4& view, const glm::mat4& projection,
                        f32 noiseFrame);

            Renderer&    m_renderer;
            VertexArray& m_quad;
            Shaders      m_shaders;

            // Realoca m_aoA/m_aoB si la resolucion completa o el flag halfRes
            // cambiaron. Se llama al principio de Render y no desde el setter
            // porque el flag lo mueve un checkbox de ImGui a mitad de frame.
            void EnsureTargetSize();

            Framebuffer m_aoA;        // xyz = bent normal world, w = visibilidad
            Framebuffer m_aoB;        // scratch del blur separable

            // La resolucion completa de la pantalla. m_aoA/m_aoB pueden estar a otra.
            u32 m_fullWidth  = 0u;
            u32 m_fullHeight = 0u;

            AoSettings m_settings;

            UniformBuffer m_gtaoUbo { sizeof(AoPassBlock) };

            // Cual de los dos FBOs tiene el resultado. Con blur apagado es A;
            // con blur, la pasada vertical vuelve a dejarlo en A.
            bool m_resultInA = true;

            // Render corrio este frame; sin eso Context() sale vacio.
            bool m_hasResult = false;
    };

}
}
