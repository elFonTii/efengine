#include "efengine/renderer/DdgiPass.h"

#include <efecom/RHI.h>

#include <efengine/core/Assert.h>
#include <efengine/core/Log.h>
#include <efengine/renderer/FrameContext.h>
#include <efengine/renderer/Renderer.h>
#include <efengine/renderer/Shader.h>
#include <efengine/renderer/Cubemap.h>
#include <efengine/renderer/ShaderBlocks.h>
#include <efengine/renderer/VertexArray.h>
#include <efengine/renderer/VoxelMath.h>
#include <efengine/scene/SceneGraph.h>

#include <algorithm>
#include <efengine/renderer/GpuProfiler.h>

#include <chrono>
#include <utility>
#include <vector>

namespace efengine {
namespace renderer {

    namespace {
        // GLSL no ve las constantes de C++: estos asserts son la unica atadura.
        static_assert(kMaxRaysPerProbe == 256u,
                      "sincronizar kDdgiMaxRays en assets/shaders/ddgi/update.glsl");
        static_assert(kRayGroupSize == 64u,
                      "sincronizar local_size_x de trace_voxel.comp y kDdgiRayGroup de ddgi/update.glsl");
    }

    std::unique_ptr<DdgiPass> DdgiPass::Create(Renderer& renderer, VertexArray& fullscreenQuad,
                                               const Shaders& shaders) {
        if (shaders.trace == null || shaders.voxelize == null
            || shaders.blendIrradiance == null || shaders.blendDistance == null
            || shaders.probeUpdate == null) {
            EF_LOG_ERROR("DdgiPass::Create: falta algun shader de DDGI");
            return null;
        }

        const DdgiGrid grid = SanitizeGrid(DdgiGrid{});
        const glm::ivec2 irrSize  = IrradianceAtlasSize(grid);
        const glm::ivec2 distSize = DistanceAtlasSize(grid);

        Texture rays = Texture::CreateStorage2D(kMaxRaysPerProbe, kMaxProbesPerFrame,
                                                efecom::TextureFormat::RGBA16F);
        Texture irradiance = Texture::CreateStorage2D(static_cast<u32>(irrSize.x),
                                                      static_cast<u32>(irrSize.y),
                                                      efecom::TextureFormat::RGBA16F);
        Texture distance = Texture::CreateStorage2D(static_cast<u32>(distSize.x),
                                                    static_cast<u32>(distSize.y),
                                                    efecom::TextureFormat::RG16F);

        ClearAtlas(irradiance);
        ClearAtlas(distance);

        StorageBuffer probeData(ProbeDataBytes(grid));
        ClearProbeData(probeData, ProbeCount(grid));

        EF_LOG_INFO("DdgiPass: %u probes, atlas irradiancia %dx%d, distancia %dx%d, buffer de rayos %ux%u",
                    ProbeCount(grid), irrSize.x, irrSize.y, distSize.x, distSize.y,
                    kMaxRaysPerProbe, kMaxProbesPerFrame);

        std::unique_ptr<VoxelizePass> voxelize = VoxelizePass::Create(renderer, shaders.voxelize);
        if (voxelize == null) {
            EF_LOG_ERROR("DdgiPass::Create: no se pudo crear el VoxelizePass");
            return null;
        }

        // El ctor es privado: make_unique no lo alcanza.
        return std::unique_ptr<DdgiPass>(
            new DdgiPass(renderer, fullscreenQuad, shaders, std::move(rays),
                         std::move(irradiance), std::move(distance), std::move(probeData),
                         std::move(voxelize)));
    }

    DdgiPass::DdgiPass(Renderer& renderer, VertexArray& fullscreenQuad, const Shaders& shaders,
                       Texture rays, Texture irradiance, Texture distance,
                       StorageBuffer probeData, std::unique_ptr<VoxelizePass> voxelize)
        : m_renderer(renderer), m_quad(fullscreenQuad), m_shaders(shaders)
        , m_rays(std::move(rays)), m_irradiance(std::move(irradiance))
        , m_distance(std::move(distance)), m_probeData(std::move(probeData))
        , m_voxelize(std::move(voxelize))
        , m_atlasGrid(SanitizeGrid(DdgiGrid{})) {}

    DdgiPass::~DdgiPass() = default;

    void DdgiPass::ClearAtlas(const Texture& atlas) {
        // El storage inmutable arranca con contenido INDEFINIDO, y un mix con
        // hysteresis 0.97 propagaria esa basura para siempre.
        //
        // ClearFramebuffer no bindea, asi que a diferencia de la version vieja
        // esto no pisa el render target ni el viewport del caller. Importa
        // porque el boton "Reset" del panel lo dispara a mitad de frame.
        const u32 fbo = efecom::CreateFramebuffer();
        if (fbo == 0u) return;

        efecom::FramebufferColorTexture(fbo, atlas.id());
        EF_GPU_CHECK(efecom::FramebufferComplete(fbo), "DdgiPass::ClearAtlas: FBO temporal incompleto");
        efecom::ClearFramebuffer(fbo, efecom::ClearMask::Color);
        efecom::DestroyFramebuffer(fbo);
    }

    void DdgiPass::ClearProbeData(const StorageBuffer& buffer, u32 probes) {
        const std::vector<glm::vec4> ceros(probes, glm::vec4(0.0f));
        buffer.Update(ceros.data(), ceros.size() * sizeof(glm::vec4));
    }

    void DdgiPass::Reset() {
        m_cursor      = 0u;
        m_sweepsDone  = 0u;
        m_blendedOnce = false;
        ClearAtlas(m_irradiance);
        ClearAtlas(m_distance);
        ClearProbeData(m_probeData, ProbeCount(m_atlasGrid));
    }

    void DdgiPass::EnsureAtlasSize() {
        const DdgiGrid want = SanitizeGrid(m_settings.grid);
        if (want.counts == m_atlasGrid.counts) {
            // Los offsets se calcularon para la posicion vieja.
            const bool seMovio = want.origin != m_atlasGrid.origin
                              || want.spacing != m_atlasGrid.spacing;
            m_atlasGrid = want;
            if (seMovio) ClearProbeData(m_probeData, ProbeCount(m_atlasGrid));
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
        m_probeData = StorageBuffer(ProbeDataBytes(want));
        ClearProbeData(m_probeData, ProbeCount(want));

        m_atlasGrid   = want;
        m_cursor      = 0u;
        m_sweepsDone  = 0u;
        m_blendedOnce = false;

        EF_LOG_INFO("DdgiPass: grilla a %dx%dx%d, atlas realocados",
                    want.counts.x, want.counts.y, want.counts.z);
    }

    namespace {
        // Mide hasta el fin del scope. Con dos returns tempranos en Update, un
        // par de time_point sueltos dejaria m_lastMs con el valor del ultimo
        // frame que si llego al final.
        struct ScopedMs {
            f32* out;
            std::chrono::steady_clock::time_point t0 = std::chrono::steady_clock::now();
            ~ScopedMs() {
                *out = std::chrono::duration<f32, std::milli>(
                           std::chrono::steady_clock::now() - t0).count();
            }
        };
    }

    void DdgiPass::Execute(FrameContext& ctx) {
        Update(ctx.scene, ctx.lighting.shadow, ctx.lighting.ibl,
               ctx.lighting.ibl.environment);

        // Fuera de Update y no al final de su cuerpo: Update tiene retornos
        // tempranos (sin probes, congelado) y el contexto se publicaba igual.
        // Con el atlas todavia invalido, Context() no entrega los atlas y
        // pbr.frag cae a IBL puro, que es lo correcto.
        ctx.lighting.ddgi = Context();
    }

    void DdgiPass::Update(const scene::SceneGraph& scene, const ShadowContext& shadow,
                          const IblContext& ibl, const Cubemap* env) {
        const ScopedMs medicion { &m_lastMs };

        const bool otraEscena = m_gridGeneracion != scene.Generation();
        if ((!m_gridValido || otraEscena) && scene.WorldBounds().Valid()) Voxelize(scene);

        EnsureAtlasSize();

        const u32 total = ProbeCount(m_atlasGrid);
        if (total == 0u) return;

        const u32 perFrame = m_settings.freeze
                           ? 0u
                           : std::min(m_settings.probeBudget, kMaxProbesPerFrame);

        m_range = NextRange(m_cursor, perFrame, total);
        if (m_range.count == 0u) return;

        if (m_range.nextCursor <= m_cursor && m_cursor != 0u) m_sweepsDone += 1u;
        m_cursor = m_range.nextCursor;

        // Este pase corre antes de BeginScene, que es quien normalmente bindea el shadow
        // map y los atlas: sin esto el trazado samplea unidades vacias y da negro.
        if (shadow.map != null) shadow.map->Bind(8);
        m_irradiance.Bind(kIrradianceAtlasUnit);
        m_distance.Bind(kDistanceAtlasUnit);
        m_probeData.BindTo(kProbeDataBinding);

        // atlasValid = m_blendedOnce: en el primer frame el atlas es el negro del clear y
        // el rebote tiene que valer cero.
        m_renderer.SetDdgiBlock(MakeDdgiBlock(m_atlasGrid, m_settings, m_range, m_blendedOnce));

        const DdgiUpdateBlock update =
            MakeDdgiUpdateBlock(m_settings, m_grid.desc(), static_cast<u32>(m_frame++));
        m_updateUbo.Update(&update, sizeof(update));
        m_updateUbo.BindTo(kDdgiUpdateBinding);

        {
            EF_PROFILE_SCOPE("DDGI captura");
            if (!m_gridValido) return;

            // El trazado sombrea con uLightSpaceMatrix/uShadowParams y lee el cielo con
            // uIblParams: hace falta el bloque del frame, no uno vacio.
            m_renderer.SetFrameBlock(MakeFrameBlock(
                MakeStaticFrameView(glm::mat4(1.0f), glm::mat4(1.0f), glm::vec3(0.0f), 1u, 1u),
                shadow, ibl));

            m_rays.BindImage(0, 0, efecom::ImageAccess::WriteOnly, efecom::TextureFormat::RGBA16F);
            m_grid.BindForSample(2u, 3u, 5u);
            if (env != null) env->Bind(4u);

            m_shaders.trace->Bind();
            efecom::DispatchCompute((update.counts.x + kRayGroupSize - 1u) / kRayGroupSize,
                                    m_range.count, 1u);

            efecom::IssueMemoryBarrier(efecom::Barrier::ShaderImageAccess
                                     | efecom::Barrier::TextureFetch);
        }

        {
            EF_PROFILE_SCOPE("DDGI probes");
            m_shaders.probeUpdate->Bind();
            m_rays.Bind(0);
            efecom::DispatchCompute(m_range.count, 1u, 1u);
        }

        // Hasta completar el primer barrido, hysteresis 0: la primera escritura de cada
        // probe sobreescribe el negro del clear.
        DdgiSettings blendSettings = m_settings;
        if (m_sweepsDone == 0u) blendSettings.hysteresis = 0.0f;
        m_renderer.SetDdgiBlock(MakeDdgiBlock(m_atlasGrid, blendSettings, m_range, true));

        {
            EF_PROFILE_SCOPE("DDGI blend");

            m_shaders.blendIrradiance->Bind();
            m_rays.Bind(0);
            m_irradiance.BindImage(0, 0, efecom::ImageAccess::ReadWrite,
                                   efecom::TextureFormat::RGBA16F);
            efecom::DispatchCompute(m_range.count, 1u, 1u);

            m_shaders.blendDistance->Bind();
            m_distance.BindImage(0, 0, efecom::ImageAccess::ReadWrite,
                                 efecom::TextureFormat::RG16F);
            efecom::DispatchCompute(m_range.count, 1u, 1u);

            efecom::IssueMemoryBarrier(efecom::Barrier::ShaderImageAccess
                                     | efecom::Barrier::TextureFetch
                                     | efecom::Barrier::ShaderStorage);
        }

        m_blendedOnce = true;
    }

    DdgiContext DdgiPass::Context() const {
        DdgiContext ctx;
        // atlasValid depende de que un blend haya corrido: samplear un atlas que
        // nunca se escribio da negro, que apagaria el ambiente adentro del
        // volumen en vez de caer a IBL.
        if (m_blendedOnce) {
            ctx.irradianceAtlas = &m_irradiance;
            ctx.distanceAtlas   = &m_distance;
            ctx.probeData       = &m_probeData;
            ctx.settings        = &m_settings;
        }
        ctx.range = m_range;
        return ctx;
    }

    void DdgiPass::Voxelize(const scene::SceneGraph& scene) {
        if (m_voxelize == null) return;

        const VoxelGridDesc desc = FitVoxelGrid(scene.WorldBounds(), kVoxelResolution);
        if (!m_grid.valid() || m_grid.desc().origin != desc.origin
            || m_grid.desc().voxelSize != desc.voxelSize
            || m_grid.desc().resolution != desc.resolution) {
            m_grid = VoxelGrid::Create(desc);
        }
        if (!m_grid.valid()) return;

        m_voxelize->Execute(scene, m_grid);
        m_gridValido     = true;
        m_gridGeneracion = scene.Generation();

        EF_LOG_INFO("DdgiPass: grid de %u^3 voxeles de %.3f m, %.1f MB, %u draws en %.1f ms",
                    desc.resolution, desc.voxelSize,
                    static_cast<f64>(m_grid.memoryBytes()) / (1024.0 * 1024.0),
                    m_voxelize->lastDraws(), m_voxelize->lastMs());

        // El atlas viejo se integro contra otro proxy: hay que reconstruirlo
        // desde cero, con hysteresis 0.
        Reset();
    }

}
}
