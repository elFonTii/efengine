#include "efengine/renderer/DdgiPass.h"

#include <efecom/RHI.h>

#include <efengine/core/Assert.h>
#include <efengine/core/Log.h>
#include <efengine/renderer/FrameContext.h>
#include <efengine/renderer/Renderer.h>
#include <efengine/renderer/Shader.h>
#include <efengine/renderer/Cubemap.h>
#include <efengine/renderer/CubeFaces.h>
#include <efengine/renderer/ShaderBlocks.h>
#include <efengine/renderer/VertexArray.h>
#include <efengine/renderer/VoxelMath.h>
#include <efengine/scene/SceneGraph.h>

#include <algorithm>
#include <efengine/renderer/GpuProfiler.h>

#include <chrono>
#include <utility>

namespace efengine {
namespace renderer {

    namespace {
        // El target de captura se aloca al maximo una sola vez: 6 caras de
        // kProbeFaceSize en fila por kMaxProbesPerFrame slots apilados. Mover el
        // slider de probes por frame nunca realoca.
        constexpr u32 kCaptureWidth  = 6u * kProbeFaceSize;                 // 96
        constexpr u32 kCaptureHeight = kMaxProbesPerFrame * kProbeFaceSize; // 512

        // Los dos blends recorren la captura cara por cara con un cache de
        // shared memory dimensionado a kDdgiFaceTexels (una constante de
        // ddgi/common.glsl). Si kProbeFaceSize crece por encima de eso, el
        // min() del shader trunca el bucle y la integral se calcula sobre menos
        // direcciones de las que la captura tiene: la irradiancia queda sesgada
        // hacia las primeras caras SIN QUE NADA FALLE. Este assert es la unica
        // defensa, porque el shader no puede assertar.
        constexpr u32 kShaderFaceTexels = 256u;
        static_assert(kProbeFaceSize * kProbeFaceSize <= kShaderFaceTexels,
                      "kProbeFaceSize crecio: subir kDdgiFaceTexels en "
                      "assets/shaders/ddgi/common.glsl y revisar que el cache de "
                      "los blends siga entrando en 32 KB de shared memory");

        // trace_voxel.comp declara local_size 16x16 y usa gl_LocalInvocationID
        // como el texel de la cara. Si kProbeFaceSize crece, el workgroup deja
        // de cubrir la cara y se pierden los texels de mas EN SILENCIO (el
        // shader ni siquiera los descarta: nunca se despachan). GLSL no puede
        // leer esta constante, asi que el assert es la unica atadura.
        constexpr u32 kTraceLocalSize = 16u;
        static_assert(kProbeFaceSize == kTraceLocalSize,
                      "kProbeFaceSize cambio: sincronizar el local_size de "
                      "assets/shaders/ddgi/trace_voxel.comp");
    }

    std::unique_ptr<DdgiPass> DdgiPass::Create(Renderer& renderer, VertexArray& fullscreenQuad,
                                               const Shaders& shaders) {
        if (shaders.trace == null || shaders.voxelize == null
            || shaders.blendIrradiance == null || shaders.blendDistance == null) {
            EF_LOG_ERROR("DdgiPass::Create: falta algun shader de DDGI");
            return null;
        }

        const DdgiGrid grid = SanitizeGrid(DdgiGrid{});
        const glm::ivec2 irrSize  = IrradianceAtlasSize(grid);
        const glm::ivec2 distSize = DistanceAtlasSize(grid);

        // Storage y no color attachment: la captura ya no se rasteriza, la
        // escribe trace_voxel.comp por imageStore.
        Texture capture = Texture::CreateStorage2D(kCaptureWidth, kCaptureHeight,
                                                   efecom::TextureFormat::RGBA16F);
        Texture irradiance = Texture::CreateStorage2D(static_cast<u32>(irrSize.x),
                                                      static_cast<u32>(irrSize.y),
                                                      efecom::TextureFormat::RGBA16F);
        Texture distance = Texture::CreateStorage2D(static_cast<u32>(distSize.x),
                                                    static_cast<u32>(distSize.y),
                                                    efecom::TextureFormat::RG16F);

        ClearAtlas(irradiance);
        ClearAtlas(distance);

        EF_LOG_INFO("DdgiPass: %u probes, atlas irradiancia %dx%d, distancia %dx%d, captura %ux%u",
                    ProbeCount(grid), irrSize.x, irrSize.y, distSize.x, distSize.y,
                    kCaptureWidth, kCaptureHeight);

        std::unique_ptr<VoxelizePass> voxelize = VoxelizePass::Create(renderer, shaders.voxelize);
        if (voxelize == null) {
            EF_LOG_ERROR("DdgiPass::Create: no se pudo crear el VoxelizePass");
            return null;
        }

        // El ctor es privado: make_unique no lo alcanza.
        return std::unique_ptr<DdgiPass>(
            new DdgiPass(renderer, fullscreenQuad, shaders, std::move(capture),
                         std::move(irradiance), std::move(distance), std::move(voxelize)));
    }

    DdgiPass::DdgiPass(Renderer& renderer, VertexArray& fullscreenQuad, const Shaders& shaders,
                       Texture capture, Texture irradiance, Texture distance,
                       std::unique_ptr<VoxelizePass> voxelize)
        : m_renderer(renderer), m_quad(fullscreenQuad), m_shaders(shaders)
        , m_capture(std::move(capture)), m_irradiance(std::move(irradiance))
        , m_distance(std::move(distance)), m_voxelize(std::move(voxelize))
        , m_atlasGrid(SanitizeGrid(DdgiGrid{})) {}

    DdgiPass::~DdgiPass() = default;

    DdgiPass::DdgiPass(DdgiPass&& o) noexcept
        : m_renderer(o.m_renderer), m_quad(o.m_quad), m_shaders(o.m_shaders)
        , m_capture(std::move(o.m_capture)), m_irradiance(std::move(o.m_irradiance))
        , m_distance(std::move(o.m_distance)), m_grid(std::move(o.m_grid))
        , m_voxelize(std::move(o.m_voxelize))
        , m_gridValido(std::exchange(o.m_gridValido, false))
        , m_traceUbo(std::move(o.m_traceUbo))
        , m_settings(o.m_settings), m_atlasGrid(o.m_atlasGrid), m_range(o.m_range)
        , m_cursor(o.m_cursor), m_sweepsDone(o.m_sweepsDone)
        , m_blendedOnce(o.m_blendedOnce), m_lastMs(o.m_lastMs) {}

    // Las dos referencias (renderer, quad) no se reasignan: son las mismas para
    // todo el proceso, y una referencia no se puede rebindear igual.
    DdgiPass& DdgiPass::operator=(DdgiPass&& o) noexcept {
        if (this != &o) {
            m_shaders     = o.m_shaders;
            m_capture     = std::move(o.m_capture);
            m_irradiance  = std::move(o.m_irradiance);
            m_distance    = std::move(o.m_distance);
            m_grid        = std::move(o.m_grid);
            m_voxelize    = std::move(o.m_voxelize);
            m_gridValido  = std::exchange(o.m_gridValido, false);
            m_traceUbo    = std::move(o.m_traceUbo);
            m_settings    = o.m_settings;
            m_atlasGrid   = o.m_atlasGrid;
            m_range       = o.m_range;
            m_cursor      = o.m_cursor;
            m_sweepsDone  = o.m_sweepsDone;
            m_blendedOnce = o.m_blendedOnce;
            m_lastMs      = o.m_lastMs;
        }
        return *this;
    }

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

        // El grid es el insumo de la captura: sin el no hay nada que trazar.
        // Se hornea una vez, cuando la escena ya tiene geometria; despues lo
        // rehornea el boton del panel.
        if (!m_gridValido && scene.WorldBounds().Valid()) Voxelize(scene);

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

        // El shadow map a su unidad fija (la misma 8 que usa BeginScene). Hay que
        // bindearlo aca porque este pase corre ANTES de BeginScene, que es quien
        // normalmente lo hace: sin esto, la primera captura sombrearia contra una
        // unidad sin contenido y saldria todo en sombra.
        if (shadow.map != null) shadow.map->Bind(8);

        // Los dos atlas a sus unidades, por la MISMA razon que el shadow map de
        // arriba: este pase corre antes de BeginScene, que es quien normalmente
        // los bindea. Sin esto, el rebote de capture.frag samplea unidades sin
        // contenido y da negro sin que nada falle ruidosamente.
        m_irradiance.Bind(kIrradianceAtlasUnit);
        m_distance.Bind(kDistanceAtlasUnit);

        // El bloque que va a leer la CAPTURA. No alcanza con el que se sube mas
        // abajo para el blend: ese lleva atlasValid en true siempre, y aca hace
        // falta lo contrario. En el primer frame el atlas todavia es el negro
        // del clear, asi que atlasValid = m_blendedOnce apaga DdgiEnabled() y el
        // rebote vale cero en vez de realimentar basura.
        m_renderer.SetDdgiBlock(MakeDdgiBlock(m_atlasGrid, m_settings, m_range, m_blendedOnce));

        {
            EF_PROFILE_SCOPE("DDGI captura");

            // Sin grid no hay nada que trazar. m_blendedOnce sigue en false, asi
            // que Context() no entrega los atlas y pbr.frag cae a IBL puro.
            if (!m_gridValido) return;

            // El trazado lee uLightSpaceMatrix, uShadowParams y uIblParams de
            // este bloque para sombrear el impacto y para el cielo. Ya no hay
            // proyeccion de captura: las direcciones salen de dirForFace.
            //
            // El ibl que llega aca es el del frame, y tiene que serlo: el rayo
            // que se escapa lee la intensidad de uIblParams.y, y con un bloque
            // vacio el cielo de la captura sale negro.
            m_renderer.SetFrameBlock(MakeFrameBlock(glm::mat4(1.0f), glm::mat4(1.0f),
                                                    glm::vec3(0.0f), shadow, ibl));

            const TraceVoxelPassBlock bloque =
                MakeTraceVoxelPassBlock(m_grid.desc(), m_settings.opacityThreshold);
            m_traceUbo.Update(&bloque, sizeof(bloque));
            m_traceUbo.BindTo(kPassBinding);

            m_capture.BindImage(0, 0, efecom::ImageAccess::WriteOnly,
                                efecom::TextureFormat::RGBA16F);
            m_grid.BindForSample(2u, 3u);
            if (env != null) env->Bind(4u);

            m_shaders.trace->Bind();
            // Seis caras por los probes del frame; el workgroup ES la cara.
            efecom::DispatchCompute(kCubeFaceCount, m_range.count, 1u);

            // La captura ahora se escribe por imageStore y los blends la leen
            // por sampler: sin este barrier leen una captura a medio escribir y
            // dan ruido o negro SIN que nada falle ruidosamente.
            efecom::IssueMemoryBarrier(efecom::Barrier::ShaderImageAccess
                                     | efecom::Barrier::TextureFetch);
        }

        // El blend. El barrier de arriba es el que ordena captura -> blend; el
        // de mas abajo ordena blend -> sampleo de pbr.frag.
        const bool primerBarrido = (m_sweepsDone == 0u);

        // Hasta completar el primer barrido se fuerza hysteresis 0: la primera
        // escritura de cada probe SOBREESCRIBE en vez de mezclar. Sin esto, el
        // negro del clear inicial tardaria cientos de frames en salir con
        // hysteresis 0.97.
        DdgiSettings blendSettings = m_settings;
        if (primerBarrido) blendSettings.hysteresis = 0.0f;

        const DdgiBlock block = MakeDdgiBlock(m_atlasGrid, blendSettings, m_range, true);
        m_renderer.SetDdgiBlock(block);

        {
            EF_PROFILE_SCOPE("DDGI blend");

            m_shaders.blendIrradiance->Bind();
            m_capture.Bind(0);
            m_irradiance.BindImage(0, 0, efecom::ImageAccess::ReadWrite,
                                   efecom::TextureFormat::RGBA16F);
            efecom::DispatchCompute(m_range.count, 1u, 1u);

            // El segundo blend comparte el DdgiBlock que ya se subio: no hay que
            // re-subirlo. Escribe otra imagen, asi que tampoco necesita barrier
            // entre los dos dispatches.
            m_shaders.blendDistance->Bind();
            m_capture.Bind(0);
            m_distance.BindImage(0, 0, efecom::ImageAccess::ReadWrite,
                                 efecom::TextureFormat::RG16F);
            efecom::DispatchCompute(m_range.count, 1u, 1u);

            efecom::IssueMemoryBarrier(efecom::Barrier::ShaderImageAccess
                                     | efecom::Barrier::TextureFetch);
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
        m_gridValido = true;

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
