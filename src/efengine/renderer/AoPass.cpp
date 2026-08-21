#include "efengine/renderer/AoPass.h"

#include <efecom/RHI.h>

#include <efengine/core/Log.h>
#include <efengine/renderer/AoMath.h>
#include <efengine/renderer/ReducedRes.h>
#include <efengine/renderer/Renderer.h>
#include <efengine/renderer/Shader.h>
#include <efengine/renderer/PipelineStates.h>
#include <efengine/renderer/VertexArray.h>
#include <efengine/scene/SceneGraph.h>
#include <efengine/renderer/GpuProfiler.h>

#include <utility>

namespace efengine {
namespace renderer {

    std::optional<AoPass> AoPass::Create(Renderer& renderer, VertexArray& fullscreenQuad,
                                         const Shaders& shaders, u32 width, u32 height,
                                         u32 sharedDepthRbo) {
        if (shaders.depthNormal == null || shaders.gtao == null || shaders.denoise == null) {
            EF_LOG_ERROR("AoPass::Create: falta algun shader de AO");
            return std::nullopt;
        }
        if (width == 0u || height == 0u) {
            EF_LOG_ERROR("AoPass::Create: tamano invalido %ux%u", width, height);
            return std::nullopt;
        }

        if (sharedDepthRbo == 0u) {
            EF_LOG_ERROR("AoPass::Create: sin depth compartido; el prepass no puede "
                         "alimentar el forward");
            return std::nullopt;
        }

        EF_LOG_INFO("AoPass: prepass %ux%u (depth compartido), AO %ux%u",
                    width, height, ReducedExtent(width), ReducedExtent(height));
        return AoPass(renderer, fullscreenQuad, shaders, width, height, sharedDepthRbo);
    }

    AoPass::AoPass(Renderer& renderer, VertexArray& fullscreenQuad, const Shaders& shaders,
                   u32 width, u32 height, u32 sharedDepthRbo)
        : m_renderer(renderer), m_quad(fullscreenQuad), m_shaders(shaders)
        // El prepass escribe en el depth DEL FRAMEBUFFER DE ESCENA, no en uno
        // propio: es lo que despues deja al forward dibujar con GL_EQUAL.
        , m_normalFb(width, height, sharedDepthRbo)
        , m_aoA(ReducedExtent(width), ReducedExtent(height))
        , m_aoB(ReducedExtent(width), ReducedExtent(height))
        , m_fullWidth(width), m_fullHeight(height) {}

    AoPass::AoPass(AoPass&& o) noexcept
        : m_renderer(o.m_renderer), m_quad(o.m_quad), m_shaders(o.m_shaders)
        , m_normalFb(std::move(o.m_normalFb)), m_aoA(std::move(o.m_aoA))
        , m_aoB(std::move(o.m_aoB))
        , m_fullWidth(o.m_fullWidth), m_fullHeight(o.m_fullHeight)
        , m_settings(o.m_settings)
        , m_prepassUbo(std::move(o.m_prepassUbo)), m_gtaoUbo(std::move(o.m_gtaoUbo))
        , m_resultInA(o.m_resultInA), m_depthReady(o.m_depthReady) {}

    // Las dos referencias (renderer, quad) no se reasignan: son las mismas para
    // todo el proceso, y una referencia no se puede rebindear igual.
    AoPass& AoPass::operator=(AoPass&& o) noexcept {
        if (this != &o) {
            m_shaders    = o.m_shaders;
            m_normalFb   = std::move(o.m_normalFb);
            m_aoA        = std::move(o.m_aoA);
            m_aoB        = std::move(o.m_aoB);
            m_fullWidth  = o.m_fullWidth;
            m_fullHeight = o.m_fullHeight;
            m_settings   = o.m_settings;
            m_prepassUbo = std::move(o.m_prepassUbo);
            m_gtaoUbo    = std::move(o.m_gtaoUbo);
            m_resultInA  = o.m_resultInA;
            m_depthReady = o.m_depthReady;
        }
        return *this;
    }

    const Texture& AoPass::aoTexture() const {
        return m_resultInA ? m_aoA.ColorTexture() : m_aoB.ColorTexture();
    }

    i32 AoPass::scale() const {
        // Se deduce de los tamanos y no del flag: el flag puede haber cambiado
        // hace un microsegundo y los targets todavia no. Lo que el shader tiene
        // que saber es donde estan los texels HOY.
        return (m_aoA.width() == m_fullWidth) ? 1 : kReducedScale;
    }

    void AoPass::Resize(u32 width, u32 height, u32 sharedDepthRbo) {
        if (width == 0u || height == 0u || sharedDepthRbo == 0u) return;

        m_fullWidth  = width;
        m_fullHeight = height;
        // El handle tambien cuenta: el dueno pudo realocarse. Framebuffer::Resize
        // lo compara y no hace nada si no cambio nada.
        m_normalFb.Resize(width, height, sharedDepthRbo);
        EnsureTargetSize();
    }

    void AoPass::EnsureTargetSize() {
        const u32 w = m_settings.halfRes ? ReducedExtent(m_fullWidth)  : m_fullWidth;
        const u32 h = m_settings.halfRes ? ReducedExtent(m_fullHeight) : m_fullHeight;
        if (w == 0u || h == 0u) return;

        // Framebuffer::Resize ya sale temprano si el tamano coincide, asi que
        // esto es un par de comparaciones en el caso normal.
        m_aoA.Resize(w, h);
        m_aoB.Resize(w, h);
    }

    void AoPass::Render(const scene::SceneGraph& scene,
                        const glm::mat4& view, const glm::mat4& projection) {
        // Se apaga PRIMERO: si este Render sale temprano, el depth compartido
        // tiene la profundidad del frame anterior, y dibujar el forward con
        // GL_EQUAL contra eso deja la pantalla vacia.
        m_depthReady = false;

        if (!m_settings.enabled) return;

        // El checkbox de media resolucion se mueve a mitad de frame desde ImGui:
        // los targets se ajustan aca y no en el setter.
        EnsureTargetSize();
        const i32 esc = scale();

        // -- 1. Prepass: normal view-space + profundidad lineal ----------------
        {
            EF_PROFILE_SCOPE("AO prepass");

            m_normalFb.Bind();
            // Negro con alfa 0: viewZ == 0 es el centinela de "aca no hay
            // geometria". El Clear tambien limpia la PROFUNDIDAD, que es la del
            // framebuffer de escena: este es el unico clear de depth del frame, y
            // por eso el forward despues limpia solo color.
            m_renderer.Clear(0.0f, 0.0f, 0.0f, 0.0f);

            const AoPrepassBlock prepass { view, projection };
            m_prepassUbo.Update(&prepass, sizeof(prepass));
            m_prepassUbo.BindTo(kPassBinding);

            m_shaders.depthNormal->Bind();
            for (const scene::RenderItem& item : scene.Renderables()) {
                if (!item.model) continue;
                // overrideShader SIN overrideState: asi cada material conserva su
                // doubleSided. Forzar el estado dejaria las salas inward=1 (cascaras
                // de espesor cero, que necesitan CullMode::None) fuera del target.
                m_renderer.Submit(*item.model, *item.materials, item.world, m_shaders.depthNormal);
            }

            // A partir de aca el depth compartido tiene la profundidad de la
            // escena con la camara de este frame.
            m_depthReady = true;
        }

        // -- 2. Kernel de GTAO -------------------------------------------------
        {
            EF_PROFILE_SCOPE("AO kernel");

            // La resolucion que va al bloque es la COMPLETA aunque el target sea
            // mas chico: projScale y projInfo.zw describen la camara, no el
            // buffer. Con la del target, el radio en metros del panel se
            // duplicaria solo por bajar de resolucion.
            const AoPassBlock params = MakeAoPassBlock(view, projection, m_settings,
                                                       m_fullWidth, m_fullHeight, 0, esc);
            m_gtaoUbo.Update(&params, sizeof(params));
            m_gtaoUbo.BindTo(kPassBinding);

            m_aoA.Bind();
            efecom::ApplyPipelineState(FullscreenState());
            m_normalFb.ColorTexture().Bind(0);
            m_renderer.Draw(m_quad, *m_shaders.gtao);

            m_resultInA = true;
        }

        // -- 3. Blur bilateral separable: A -> B (horizontal) -> A (vertical) --
        // Los modos de debug 3 y 4 saltean el blur: borronear el volcado del
        // prepass no significa nada, y la tabla de verificacion los lee crudos.
        if (m_settings.blur && m_settings.debugView < 3u) {
            EF_PROFILE_SCOPE("AO blur");

            // La guia de profundidad es la misma en las dos pasadas.
            m_normalFb.ColorTexture().Bind(1);

            for (i32 dir = 0; dir < 2; ++dir) {
                const AoPassBlock blurParams = MakeAoPassBlock(view, projection, m_settings,
                                                               m_fullWidth, m_fullHeight, dir, esc);
                m_gtaoUbo.Update(&blurParams, sizeof(blurParams));
                m_gtaoUbo.BindTo(kPassBinding);

                Framebuffer& origen  = (dir == 0) ? m_aoA : m_aoB;
                Framebuffer& destino = (dir == 0) ? m_aoB : m_aoA;

                destino.Bind();
                origen.ColorTexture().Bind(0);
                m_renderer.Draw(m_quad, *m_shaders.denoise);
            }

            // La pasada vertical dejo el resultado de vuelta en A.
            m_resultInA = true;
        }
    }

    AoContext AoPass::Context() const {
        AoContext ctx;
        if (!m_settings.enabled) return ctx;   // texture queda en null -> AO apagado

        ctx.texture     = &aoTexture();
        // El prepass viaja en el contexto porque es la guia de los dos upsamples
        // de pbr.frag, no solo del propio AO. Ver AoContext::depthNormal.
        ctx.depthNormal = &m_normalFb.ColorTexture();
        ctx.scale       = scale();
        ctx.enabled     = true;
        ctx.bentNormal  = m_settings.bentNormal;
        ctx.multiBounce = m_settings.multiBounce;
        ctx.debugView   = m_settings.debugView;
        return ctx;
    }

}
}
