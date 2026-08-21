#include "efengine/renderer/IndirectPass.h"

#include <efecom/RHI.h>

#include <efengine/core/Log.h>
#include <efengine/renderer/AoMath.h>
#include <efengine/renderer/GpuProfiler.h>
#include <efengine/renderer/PipelineStates.h>
#include <efengine/renderer/Renderer.h>
#include <efengine/renderer/Shader.h>
#include <efengine/renderer/Texture.h>
#include <efengine/renderer/VertexArray.h>

#include <utility>

namespace efengine {
namespace renderer {

    std::optional<IndirectPass> IndirectPass::Create(Renderer& renderer, VertexArray& fullscreenQuad,
                                                     Shader* shader, u32 width, u32 height) {
        if (shader == null) {
            EF_LOG_ERROR("IndirectPass::Create: falta el shader de indirecta");
            return std::nullopt;
        }
        if (width == 0u || height == 0u) {
            EF_LOG_ERROR("IndirectPass::Create: tamano invalido %ux%u", width, height);
            return std::nullopt;
        }

        EF_LOG_INFO("IndirectPass: target %ux%u (1/%d de %ux%u)",
                    ReducedExtent(width), ReducedExtent(height), kScale, width, height);
        return IndirectPass(renderer, fullscreenQuad, shader, width, height);
    }

    IndirectPass::IndirectPass(Renderer& renderer, VertexArray& fullscreenQuad, Shader* shader,
                               u32 width, u32 height)
        : m_renderer(renderer), m_quad(fullscreenQuad), m_shader(shader)
        , m_fb(ReducedExtent(width), ReducedExtent(height))
        , m_fullSize(static_cast<i32>(width), static_cast<i32>(height)) {}

    IndirectPass::IndirectPass(IndirectPass&& o) noexcept
        : m_renderer(o.m_renderer), m_quad(o.m_quad), m_shader(o.m_shader)
        , m_fb(std::move(o.m_fb)), m_fullSize(o.m_fullSize)
        , m_ubo(std::move(o.m_ubo))
        , m_enabled(o.m_enabled), m_ranEste(o.m_ranEste)
        , m_aoReduced(o.m_aoReduced) {}

    // Las dos referencias (renderer, quad) no se reasignan: son las mismas para
    // todo el proceso, y una referencia no se puede rebindear igual. Mismo
    // patron que AoPass y DdgiPass.
    IndirectPass& IndirectPass::operator=(IndirectPass&& o) noexcept {
        if (this != &o) {
            m_shader      = o.m_shader;
            m_fb          = std::move(o.m_fb);
            m_fullSize    = o.m_fullSize;
            m_ubo         = std::move(o.m_ubo);
            m_enabled     = o.m_enabled;
            m_ranEste     = o.m_ranEste;
            m_aoReduced   = o.m_aoReduced;
        }
        return *this;
    }

    void IndirectPass::Resize(u32 fullWidth, u32 fullHeight) {
        if (fullWidth == 0u || fullHeight == 0u) return;

        m_fullSize = glm::ivec2(static_cast<i32>(fullWidth), static_cast<i32>(fullHeight));
        m_fb.Resize(ReducedExtent(fullWidth), ReducedExtent(fullHeight));
    }

    void IndirectPass::Render(const AoContext& ao,
                              const glm::mat4& view, const glm::mat4& projection) {
        const Texture* depthNormal = ao.depthNormal;
        const Texture* aoTexture   = ao.texture;

        // Se apaga PRIMERO y se enciende solo al final: cualquier salida
        // temprana tiene que dejar el contexto invalido, o pbr.frag se quedaria
        // con el target del frame anterior y una camara que ya se movio.
        m_ranEste = false;

        if (!m_enabled || m_shader == null) return;

        // Sin el prepass del AO no hay de donde sacar posicion ni normal. Ver la
        // nota de dependencia en el header: no es una degradacion silenciosa,
        // pbr.frag vuelve al camino inline y la imagen es la misma.
        if (depthNormal == null || !ao.enabled) return;

        EF_PROFILE_SCOPE("Indirecta (1/2)");

        // En que grilla vive el target del AO. Sale de comparar los tamanos y no
        // del ao.scale que viene en el contexto: lo que este shader necesita es
        // que el texel que lee EXISTA, y eso lo decide el tamano real de la
        // textura, no un numero que viaja aparte y puede ir un frame atrasado.
        m_aoReduced = (aoTexture != null
                    && aoTexture->width()  == m_fb.width()
                    && aoTexture->height() == m_fb.height()
                    && m_fb.width() != static_cast<u32>(m_fullSize.x));


        IndirectPassBlock block {};
        block.viewToWorld = glm::inverse(view);
        // La reconstruccion view-space es la misma que la del AO: (1/P00, 1/P11).
        // Se reusa AoProjInfo en vez de duplicar la formula -- es el mismo
        // despeje sobre la misma matriz, y tenerlo dos veces es el bug que se
        // arregla en una copia y no en la otra.
        const glm::vec2 info = AoProjInfo(projection);
        // zw es 1/resolucion COMPLETA y no la del target: el shader reconstruye
        // la posicion desde el texel de GUIA, que vive en la grilla full.
        block.projInfo = glm::vec4(info.x, info.y,
                                   (m_fullSize.x > 0) ? 1.0f / static_cast<f32>(m_fullSize.x) : 0.0f,
                                   (m_fullSize.y > 0) ? 1.0f / static_cast<f32>(m_fullSize.y) : 0.0f);

        // De donde sale el bent normal, en la codificacion kBent* de
        // indirect.frag. En que GRILLA se lee el target del AO no es un detalle:
        // leerlo en la equivocada devuelve el bent normal de otro pixel y tuerce
        // la direccion del color bleeding sin romper nada ruidosamente.
        const i32 bent = (!ao.bentNormal || aoTexture == null)
                       ? kBentNinguno
                       : (m_aoReduced ? kBentReducido : kBentFullRes);
        block.counts = glm::ivec4(kScale, bent, m_fullSize.x, m_fullSize.y);

        m_ubo.Update(&block, sizeof(block));
        m_ubo.BindTo(kPassBinding);

        m_fb.Bind();
        efecom::ApplyPipelineState(FullscreenState());

        depthNormal->Bind(0);
        // Si el AO no produce bent normal, la unidad 1 igual tiene que tener algo
        // valido bindeado: el shader no la samplea (counts.y == 0), pero dejar
        // una unidad vacia con un sampler declarado es comportamiento indefinido,
        // no negro. Se bindea el prepass, que siempre existe llegado aca.
        (aoTexture != null ? aoTexture : depthNormal)->Bind(1);

        m_renderer.Draw(m_quad, *m_shader);

        m_ranEste = true;
    }

    IndirectContext IndirectPass::Context() const {
        IndirectContext ctx;
        if (!m_ranEste) return ctx;   // invalido -> pbr.frag samplea inline

        ctx.texture = &m_fb.ColorTexture();
        ctx.scale   = kScale;
        return ctx;
    }

}
}
