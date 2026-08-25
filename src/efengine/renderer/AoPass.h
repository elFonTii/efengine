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
    class AoPass : public IScenePass {
        public:
            struct Shaders {
                Shader* depthNormal = null;
                Shader* gtao        = null;
                Shader* denoise     = null;
            };

            // sceneFB es el framebuffer de escena. El prepass escribe en SU
            // depth y no en uno propio, y por eso el forward puede despues
            // dibujar con GL_EQUAL sin volver a resolver la visibilidad.
            //
            // Se guarda el framebuffer y no el handle del renderbuffer porque su
            // Resize crea uno nuevo y destruye el viejo: leerlo en el momento en
            // que hace falta es lo unico que evita quedar enganchado a un
            // attachment muerto.
            static std::unique_ptr<AoPass> Create(Renderer& renderer, VertexArray& fullscreenQuad,
                                                  const Shaders& shaders, u32 width, u32 height,
                                                  Framebuffer& sceneFB);

            AoPass(const AoPass&)            = delete;
            AoPass& operator=(const AoPass&) = delete;
            AoPass(AoPass&& other) noexcept;
            AoPass& operator=(AoPass&& other) noexcept;

            // Dibuja el prepass y corre el kernel, y publica ctx.lighting.ao y
            // ctx.depthReady. La camara sale del ctx y no del bloque Frame
            // porque este pase corre antes del FrameUploadPass.
            void Execute(FrameContext& ctx) override;

            const char* Name() const override { return "AO"; }

            // El renderbuffer nuevo lo crea el framebuffer de escena en SU
            // Resize, que el pipeline corre antes que este: se lee de ahi.
            void Resize(u32 width, u32 height) override;

            AoContext Context() const;

            // El prepass corrio en ESTE frame, o sea que el depth compartido
            // tiene la profundidad de la escena y el forward puede dibujar con
            // GL_EQUAL. False obliga al forward a limpiar depth y resolver la
            // visibilidad el mismo, como siempre.
            bool depthReady() const { return m_depthReady; }

            // Cuantos texels de resolucion completa cubre uno del target de AO
            // por eje. 1 con halfRes apagado, kReducedScale con el encendido.
            // Lo consultan IndirectPass (para saber en que grilla leer el bent
            // normal) y el panel.
            i32 scale() const;

            AoSettings&       settings()       { return m_settings; }
            const AoSettings& settings() const { return m_settings; }

            const Texture& normalTarget() const { return m_normalFb.ColorTexture(); }
            const Texture& aoTexture()    const;

        private:
            AoPass(Renderer& renderer, VertexArray& fullscreenQuad, const Shaders& shaders,
                   u32 width, u32 height, Framebuffer& sceneFB);

            // El trabajo real; lo llama Execute, que publica el contexto
            // despues -- afuera, porque esto tiene retornos tempranos.
            void Render(const scene::SceneGraph& scene,
                        const glm::mat4& view, const glm::mat4& projection);

            Renderer&    m_renderer;
            VertexArray& m_quad;
            Shaders      m_shaders;

            // El framebuffer de escena, del que este pase toma prestado el
            // depth. Puntero y no referencia solo para que el move siga siendo
            // trivial de escribir.
            Framebuffer* m_sceneFB = null;

            // Realoca m_aoA/m_aoB si la resolucion completa o el flag halfRes
            // cambiaron. Se llama al principio de Render y no desde el setter
            // porque el flag lo mueve un checkbox de ImGui a mitad de frame.
            void EnsureTargetSize();

            // El prepass se queda a resolucion COMPLETA aunque el AO baje: es
            // barato, la marcha del kernel quiere la profundidad fina, y ES la
            // guia del upsample bilateral de pbr.frag.
            Framebuffer m_normalFb;   // xyz = normal view, w = viewZ lineal
            Framebuffer m_aoA;        // xyz = bent normal world, w = visibilidad
            Framebuffer m_aoB;        // scratch del blur separable

            // La resolucion completa de la pantalla. m_aoA/m_aoB pueden estar a
            // otra; m_normalFb siempre esta a esta.
            u32 m_fullWidth  = 0u;
            u32 m_fullHeight = 0u;

            AoSettings m_settings;

            // Dos UBOs y no uno compartido: los dos bloques van al mismo binding
            // 4 pero tienen layouts distintos, y un solo buffer reusado invita al
            // bug de subir uno y leer el otro.
            UniformBuffer m_prepassUbo { sizeof(AoPrepassBlock) };
            UniformBuffer m_gtaoUbo    { sizeof(AoPassBlock) };

            // Cual de los dos FBOs tiene el resultado. Con blur apagado es A;
            // con blur, la pasada vertical vuelve a dejarlo en A.
            bool m_resultInA = true;

            // Lo pone Render y lo lee el forward. Arranca en false y se apaga en
            // cada Render que salga temprano: si el prepass no corrio, el depth
            // compartido tiene la profundidad del FRAME ANTERIOR, y dibujar con
            // GL_EQUAL contra eso deja la pantalla vacia.
            bool m_depthReady = false;
    };

}
}
