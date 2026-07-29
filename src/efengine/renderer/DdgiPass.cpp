#include "efengine/renderer/DdgiPass.h"

#include <efecom/RHI.h>

#include <efengine/core/Assert.h>
#include <efengine/core/Log.h>
#include <efengine/renderer/Renderer.h>
#include <efengine/renderer/Shader.h>
#include <efengine/renderer/Cubemap.h>
#include <efengine/renderer/CubeFaces.h>
#include <efengine/renderer/PipelineStates.h>
#include <efengine/renderer/ShaderBlocks.h>
#include <efengine/renderer/VertexArray.h>
#include <efengine/scene/SceneGraph.h>

#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <utility>

namespace efengine {
namespace renderer {

    namespace {
        // El target de captura se aloca al maximo una sola vez: 6 caras de
        // kProbeFaceSize en fila por kMaxProbesPerFrame slots apilados. Mover el
        // slider de probes por frame nunca realoca.
        constexpr u32 kCaptureWidth  = 6u * kProbeFaceSize;                 // 96
        constexpr u32 kCaptureHeight = kMaxProbesPerFrame * kProbeFaceSize; // 512

        // "Muy lejos": el cielo y lo que no pega nada. Chebyshev nunca lo cuenta
        // como oclusion.
        constexpr f32 kFarDistance = 1.0e4f;
    }

    std::optional<DdgiPass> DdgiPass::Create(Renderer& renderer, VertexArray& fullscreenQuad,
                                             const Shaders& shaders) {
        if (shaders.capture == null || shaders.captureSky == null
            || shaders.blendIrradiance == null || shaders.blendDistance == null) {
            EF_LOG_ERROR("DdgiPass::Create: falta algun shader de DDGI");
            return std::nullopt;
        }

        const DdgiGrid grid = SanitizeGrid(DdgiGrid{});
        const glm::ivec2 irrSize  = IrradianceAtlasSize(grid);
        const glm::ivec2 distSize = DistanceAtlasSize(grid);

        Texture capture = Texture::CreateColorAttachment(kCaptureWidth, kCaptureHeight);
        Texture irradiance = Texture::CreateStorage2D(static_cast<u32>(irrSize.x),
                                                      static_cast<u32>(irrSize.y),
                                                      efecom::TextureFormat::RGBA16F);
        Texture distance = Texture::CreateStorage2D(static_cast<u32>(distSize.x),
                                                    static_cast<u32>(distSize.y),
                                                    efecom::TextureFormat::RG16F);

        ClearAtlas(irradiance);
        ClearAtlas(distance);

        const u32 fbo = efecom::CreateFramebuffer();
        if (fbo == 0u) {
            EF_LOG_ERROR("DdgiPass::Create: no hay contexto GL");
            return std::nullopt;
        }
        efecom::FramebufferColorTexture(fbo, capture.id());

        const u32 rbo = efecom::CreateDepthRenderbuffer(kCaptureWidth, kCaptureHeight);
        efecom::FramebufferDepthRenderbuffer(fbo, rbo);

        EF_ASSERT(efecom::FramebufferComplete(fbo), "DdgiPass: FBO de captura incompleto");

        EF_LOG_INFO("DdgiPass: %u probes, atlas irradiancia %dx%d, distancia %dx%d, captura %ux%u",
                    ProbeCount(grid), irrSize.x, irrSize.y, distSize.x, distSize.y,
                    kCaptureWidth, kCaptureHeight);

        return DdgiPass(renderer, fullscreenQuad, shaders, std::move(capture),
                        std::move(irradiance), std::move(distance), fbo, rbo);
    }

    DdgiPass::DdgiPass(Renderer& renderer, VertexArray& fullscreenQuad, const Shaders& shaders,
                       Texture capture, Texture irradiance, Texture distance,
                       u32 captureFbo, u32 captureDepthRbo)
        : m_renderer(renderer), m_quad(fullscreenQuad), m_shaders(shaders)
        , m_capture(std::move(capture)), m_irradiance(std::move(irradiance))
        , m_distance(std::move(distance))
        , m_captureFbo(captureFbo), m_captureDepthRbo(captureDepthRbo)
        , m_atlasGrid(SanitizeGrid(DdgiGrid{})) {}

    DdgiPass::~DdgiPass() {
        if (m_captureDepthRbo != 0u) efecom::DestroyRenderbuffer(m_captureDepthRbo);
        if (m_captureFbo      != 0u) efecom::DestroyFramebuffer(m_captureFbo);
    }

    DdgiPass::DdgiPass(DdgiPass&& o) noexcept
        : m_renderer(o.m_renderer), m_quad(o.m_quad), m_shaders(o.m_shaders)
        , m_capture(std::move(o.m_capture)), m_irradiance(std::move(o.m_irradiance))
        , m_distance(std::move(o.m_distance))
        , m_captureFbo(std::exchange(o.m_captureFbo, 0u))
        , m_captureDepthRbo(std::exchange(o.m_captureDepthRbo, 0u))
        , m_settings(o.m_settings), m_atlasGrid(o.m_atlasGrid), m_range(o.m_range)
        , m_cursor(o.m_cursor), m_sweepsDone(o.m_sweepsDone)
        , m_blendedOnce(o.m_blendedOnce), m_lastMs(o.m_lastMs) {}

    // Las dos referencias (renderer, quad) no se reasignan: son las mismas para
    // todo el proceso, y una referencia no se puede rebindear igual.
    DdgiPass& DdgiPass::operator=(DdgiPass&& o) noexcept {
        if (this != &o) {
            if (m_captureDepthRbo != 0u) efecom::DestroyRenderbuffer(m_captureDepthRbo);
            if (m_captureFbo      != 0u) efecom::DestroyFramebuffer(m_captureFbo);

            m_shaders         = o.m_shaders;
            m_capture         = std::move(o.m_capture);
            m_irradiance      = std::move(o.m_irradiance);
            m_distance        = std::move(o.m_distance);
            m_captureFbo      = std::exchange(o.m_captureFbo, 0u);
            m_captureDepthRbo = std::exchange(o.m_captureDepthRbo, 0u);
            m_settings        = o.m_settings;
            m_atlasGrid       = o.m_atlasGrid;
            m_range           = o.m_range;
            m_cursor          = o.m_cursor;
            m_sweepsDone      = o.m_sweepsDone;
            m_blendedOnce     = o.m_blendedOnce;
            m_lastMs          = o.m_lastMs;
        }
        return *this;
    }

    void DdgiPass::ClearAtlas(const Texture& atlas) {
        // El storage inmutable arranca con contenido INDEFINIDO. Se limpia con un
        // FBO temporal, que es la unica forma de "clear image" que da el RHI
        // actual sin agregarle nada.
        const u32 fbo = efecom::CreateFramebuffer();
        if (fbo == 0u) return;

        efecom::FramebufferColorTexture(fbo, atlas.id());
        efecom::BindRenderTarget(fbo, atlas.width(), atlas.height());
        efecom::SetClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        efecom::Clear(efecom::ClearMask::Color);
        efecom::DestroyFramebuffer(fbo);
    }

    void DdgiPass::Reset() {
        m_cursor      = 0u;
        m_sweepsDone  = 0u;
        m_blendedOnce = false;
        ClearAtlas(m_irradiance);
        ClearAtlas(m_distance);
    }

    void DdgiPass::EnsureAtlasSize() {
        const DdgiGrid want = SanitizeGrid(m_settings.grid);
        if (want.counts == m_atlasGrid.counts) {
            // El origen y el spacing no cambian el tamano del atlas, pero si la
            // posicion de cada probe: hay que copiarlos igual.
            m_atlasGrid = want;
            return;
        }

        const glm::ivec2 irrSize  = IrradianceAtlasSize(want);
        const glm::ivec2 distSize = DistanceAtlasSize(want);

        m_irradiance = Texture::CreateStorage2D(static_cast<u32>(irrSize.x),
                                               static_cast<u32>(irrSize.y),
                                               efecom::TextureFormat::RGBA16F);
        m_distance   = Texture::CreateStorage2D(static_cast<u32>(distSize.x),
                                               static_cast<u32>(distSize.y),
                                               efecom::TextureFormat::RG16F);
        ClearAtlas(m_irradiance);
        ClearAtlas(m_distance);

        m_atlasGrid   = want;
        m_cursor      = 0u;
        m_sweepsDone  = 0u;
        m_blendedOnce = false;

        EF_LOG_INFO("DdgiPass: grilla a %dx%dx%d, atlas realocados",
                    want.counts.x, want.counts.y, want.counts.z);
    }

    void DdgiPass::CaptureProbe(const scene::SceneGraph& scene, const ShadowContext& shadow,
                                const IblContext& ibl, const Cubemap* env,
                                u32 probeIndex, u32 slot) {
        // La escena todavia no se dibuja en la captura: eso es la tarea 9. Por
        // ahora solo el cielo, que es lo que hace verificable el layout de tiles.
        (void)scene;

        const glm::vec3 center = ProbeWorldPosition(m_atlasGrid, probeIndex);

        // 90 grados y aspect 1: las 6 caras cubren la esfera exacta y sin solape.
        const glm::mat4 proj = glm::perspective(glm::radians(90.0f), 1.0f,
                                                0.05f, m_settings.maxDistance);

        for (u32 face = 0u; face < kCubeFaceCount; ++face) {
            const CubeFaceBasis& b = CubeFace(face);
            const glm::mat4 view = glm::lookAt(center, center + b.forward, b.up);

            // El FrameBlock del probe. MakeFrameBlock ya calcula invViewProjRot,
            // que es lo que el cielo necesita.
            const FrameBlock fb = MakeFrameBlock(view, proj, center, shadow, ibl);
            m_renderer.SetFrameBlock(fb);

            efecom::SetViewport(face * kProbeFaceSize, slot * kProbeFaceSize,
                                kProbeFaceSize, kProbeFaceSize);

            // El cielo primero: llena el fondo con radiancia real y distancia
            // "lejos", asi los probes de exterior reciben luz de cielo sin que
            // nadie la sume aparte. Mismo quad y mismo estado que SkyboxPass.
            if (env != null) {
                efecom::ApplyPipelineState(SkyboxState());
                m_shaders.captureSky->Bind();
                env->Bind(0);
                m_renderer.Draw(m_quad, *m_shaders.captureSky);
            }
        }
    }

    void DdgiPass::Update(const scene::SceneGraph& scene, const ShadowContext& shadow,
                          const IblContext& ibl, const Cubemap* env) {
        EnsureAtlasSize();

        const u32 total = ProbeCount(m_atlasGrid);
        if (total == 0u) return;

        const u32 perFrame = m_settings.freeze
                           ? 0u
                           : std::min(m_settings.probesPerFrame, kMaxProbesPerFrame);

        m_range = NextRange(m_cursor, perFrame, total);
        if (m_range.count == 0u) return;

        // Un barrido completo cuando el cursor envuelve.
        if (m_range.nextCursor <= m_cursor && m_cursor != 0u) m_sweepsDone += 1u;
        m_cursor = m_range.nextCursor;

        efecom::BindRenderTarget(m_captureFbo, kCaptureWidth, kCaptureHeight);
        efecom::SetClearColor(0.0f, 0.0f, 0.0f, kFarDistance);
        efecom::Clear(efecom::ClearMask::ColorDepth);

        for (u32 slot = 0u; slot < m_range.count; ++slot) {
            const u32 probe = (m_range.first + slot) % total;
            CaptureProbe(scene, shadow, ibl, env, probe, slot);
        }
    }

    DdgiContext DdgiPass::Context() const {
        DdgiContext ctx;
        // atlasValid depende de que un blend haya corrido: samplear un atlas que
        // nunca se escribio da negro, que apagaria el ambiente adentro del
        // volumen en vez de caer a IBL.
        if (m_blendedOnce) {
            ctx.irradianceAtlas = &m_irradiance;
            ctx.distanceAtlas   = &m_distance;
            ctx.settings        = &m_settings;
        }
        ctx.range = m_range;
        return ctx;
    }

}
}
