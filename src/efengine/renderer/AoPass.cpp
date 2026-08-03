#include "efengine/renderer/AoPass.h"

#include <efecom/RHI.h>

#include <efengine/core/Log.h>
#include <efengine/renderer/AoMath.h>
#include <efengine/renderer/Renderer.h>
#include <efengine/renderer/Shader.h>
#include <efengine/renderer/PipelineStates.h>
#include <efengine/renderer/VertexArray.h>
#include <efengine/scene/SceneGraph.h>

#include <utility>

namespace efengine {
namespace renderer {

    std::optional<AoPass> AoPass::Create(Renderer& renderer, VertexArray& fullscreenQuad,
                                         const Shaders& shaders, u32 width, u32 height) {
        if (shaders.depthNormal == null || shaders.gtao == null || shaders.denoise == null) {
            EF_LOG_ERROR("AoPass::Create: falta algun shader de AO");
            return std::nullopt;
        }
        if (width == 0u || height == 0u) {
            EF_LOG_ERROR("AoPass::Create: tamano invalido %ux%u", width, height);
            return std::nullopt;
        }

        EF_LOG_INFO("AoPass: targets %ux%u (normal + 2 de AO)", width, height);
        return AoPass(renderer, fullscreenQuad, shaders, width, height);
    }

    AoPass::AoPass(Renderer& renderer, VertexArray& fullscreenQuad, const Shaders& shaders,
                   u32 width, u32 height)
        : m_renderer(renderer), m_quad(fullscreenQuad), m_shaders(shaders)
        , m_normalFb(width, height), m_aoA(width, height), m_aoB(width, height) {}

    AoPass::AoPass(AoPass&& o) noexcept
        : m_renderer(o.m_renderer), m_quad(o.m_quad), m_shaders(o.m_shaders)
        , m_normalFb(std::move(o.m_normalFb)), m_aoA(std::move(o.m_aoA))
        , m_aoB(std::move(o.m_aoB))
        , m_settings(o.m_settings)
        , m_prepassUbo(std::move(o.m_prepassUbo)), m_gtaoUbo(std::move(o.m_gtaoUbo))
        , m_resultInA(o.m_resultInA) {}

    // Las dos referencias (renderer, quad) no se reasignan: son las mismas para
    // todo el proceso, y una referencia no se puede rebindear igual.
    AoPass& AoPass::operator=(AoPass&& o) noexcept {
        if (this != &o) {
            m_shaders    = o.m_shaders;
            m_normalFb   = std::move(o.m_normalFb);
            m_aoA        = std::move(o.m_aoA);
            m_aoB        = std::move(o.m_aoB);
            m_settings   = o.m_settings;
            m_prepassUbo = std::move(o.m_prepassUbo);
            m_gtaoUbo    = std::move(o.m_gtaoUbo);
            m_resultInA  = o.m_resultInA;
        }
        return *this;
    }

    const Texture& AoPass::aoTexture() const {
        return m_resultInA ? m_aoA.ColorTexture() : m_aoB.ColorTexture();
    }

    void AoPass::Resize(u32 width, u32 height) {
        m_normalFb.Resize(width, height);
        m_aoA.Resize(width, height);
        m_aoB.Resize(width, height);
    }

    void AoPass::Render(const scene::SceneGraph& scene,
                        const glm::mat4& view, const glm::mat4& projection) {
        if (!m_settings.enabled) return;

        // -- 1. Prepass: normal view-space + profundidad lineal ----------------
        m_normalFb.Bind();
        // Negro con alfa 0: viewZ == 0 es el centinela de "aca no hay geometria".
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

        // -- 2. Kernel de GTAO -------------------------------------------------
        const AoPassBlock params = MakeAoPassBlock(view, projection, m_settings,
                                                   m_aoA.width(), m_aoA.height(), 0);
        m_gtaoUbo.Update(&params, sizeof(params));
        m_gtaoUbo.BindTo(kPassBinding);

        m_aoA.Bind();
        efecom::ApplyPipelineState(FullscreenState());
        m_normalFb.ColorTexture().Bind(0);
        m_renderer.Draw(m_quad, *m_shaders.gtao);

        m_resultInA = true;

        // -- 3. Blur bilateral separable: A -> B (horizontal) -> A (vertical) --
        // Los modos de debug 3 y 4 saltean el blur: borronear el volcado del
        // prepass no significa nada, y la tabla de verificacion los lee crudos.
        if (m_settings.blur && m_settings.debugView < 3u) {
            // La guia de profundidad es la misma en las dos pasadas.
            m_normalFb.ColorTexture().Bind(1);

            for (i32 dir = 0; dir < 2; ++dir) {
                const AoPassBlock blurParams = MakeAoPassBlock(view, projection, m_settings,
                                                               m_aoA.width(), m_aoA.height(), dir);
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
        ctx.enabled     = true;
        ctx.bentNormal  = m_settings.bentNormal;
        ctx.multiBounce = m_settings.multiBounce;
        ctx.debugView   = m_settings.debugView;
        return ctx;
    }

}
}
