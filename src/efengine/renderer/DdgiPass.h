#pragma once
#include <efengine/core/Types.h>
#include <efengine/renderer/DdgiVolume.h>
#include <efengine/renderer/DdgiSettings.h>
#include <efengine/renderer/IScenePass.h>
#include <efengine/renderer/DdgiContext.h>
#include <efengine/renderer/Texture.h>
#include <efengine/renderer/ShadowContext.h>
#include <efengine/renderer/IblContext.h>
#include <efengine/renderer/UniformBuffer.h>
#include <efengine/renderer/VoxelGrid.h>
#include <efengine/renderer/VoxelizePass.h>

#include <efengine/renderer/ShaderBlocks.h>

#include <glm/glm.hpp>
#include <memory>
#include <vector>

namespace efengine {
namespace scene { class SceneGraph; }
namespace renderer {

    class Renderer;
    class Shader;
    class Cubemap;
    class VertexArray;

    // Dueno de los dos atlas de probes y del target de captura. Orquesta la
    // captura round-robin y los dos blends.
    //
    // Create devuelve nullopt si falta cualquier shader (calcado de
    // Environment::Create): sin DdgiPass, Application pasa un DdgiContext vacio
    // y pbr.frag cae a IBL puro. Un fallo de shader no rompe el frame.
    class DdgiPass : public IScenePass {
        public:
            struct Shaders {
                Shader* trace           = null;   // ddgi/trace_voxel.comp
                Shader* voxelize        = null;   // voxel/voxelize.vert+frag
                Shader* blendIrradiance = null;
                Shader* blendDistance   = null;
            };

            // fullscreenQuad ya no lo usa NADA adentro del pase: desde que la
            // captura traza contra voxeles en vez de rasterizar, este pase no
            // emite un solo draw. Se conserva igual para no tocar la firma que
            // comparten todos los setups de pase (DdgiPassSetup lo pasa desde
            // PassDeps como el resto), y porque el proximo consumidor probable
            // -- una debug viz del grid -- lo va a necesitar. Si eso no llega,
            // sacarlo es un cambio de una linea aca y otra en DdgiPassSetup.
            static std::unique_ptr<DdgiPass> Create(Renderer& renderer, VertexArray& fullscreenQuad,
                                                    const Shaders& shaders);

            ~DdgiPass();
            DdgiPass(const DdgiPass&)            = delete;
            DdgiPass& operator=(const DdgiPass&) = delete;
            DdgiPass(DdgiPass&& other) noexcept;
            DdgiPass& operator=(DdgiPass&& other) noexcept;

            // Captura los probes del frame y los integra al atlas. Corre DESPUES
            // del ShadowPass (necesita su depth y su matriz) y ANTES del
            // FrameUploadPass (sube su propio FrameBlock para el trazado).
            void Execute(FrameContext& ctx) override;

            const char* Name() const override { return "DDGI"; }

            DdgiContext Context() const;

            DdgiSettings&       settings()       { return m_settings; }
            const DdgiSettings& settings() const { return m_settings; }

            const Texture& captureTarget()   const { return m_capture; }
            const Texture& irradianceAtlas() const { return m_irradiance; }
            const Texture& distanceAtlas()   const { return m_distance; }

            u32  sweepsDone() const { return m_sweepsDone; }
            u32  cursor()     const { return m_cursor; }
            f32  lastMs()     const { return m_lastMs; }

            // Rehornea el grid con la escena. Lo llama la primera carga y el
            // boton del panel.
            void Voxelize(const scene::SceneGraph& scene);

            const VoxelGrid& voxelGrid()  const { return m_grid; }
            bool             gridValido() const { return m_gridValido; }

            // Timing de la ULTIMA voxelizacion, no del frame: la voxelizacion no
            // corre por frame. Se expone desde aca y no con un
            // const VoxelizePass& porque m_voxelize puede ser null.
            f32 voxelizeMs()    const { return m_voxelize != null ? m_voxelize->lastMs()    : 0.0f; }
            u32 voxelizeDraws() const { return m_voxelize != null ? m_voxelize->lastDraws() : 0u; }

            // Si esto es false, Context() no entrega los atlas y pbr.frag esta
            // cayendo a IBL puro: DDGI aporta exactamente cero. Es el primer
            // dato a mirar cuando "no se ve la GI", porque todos los demas
            // controles del panel se ven normales igual.
            bool atlasValid() const { return m_blendedOnce; }

            // Vuelve el atlas a "sin datos": el proximo barrido escribe con
            // hysteresis 0 y lo reconstruye de cero.
            void Reset();

        private:
            DdgiPass(Renderer& renderer, VertexArray& fullscreenQuad, const Shaders& shaders,
                     Texture capture, Texture irradiance, Texture distance,
                     std::unique_ptr<VoxelizePass> voxelize);

            // El trabajo real. Lo llama Execute, que publica el contexto
            // despues -- afuera, porque esto tiene retornos tempranos.
            void Update(const scene::SceneGraph& scene, const ShadowContext& shadow,
                        const IblContext& ibl, const Cubemap* env);

            // Realoca los dos atlas si la grilla cambio de tamano. Los deja en
            // negro y rearma el contador de barridos.
            void EnsureAtlasSize();

            // Deja un atlas en cero. El storage inmutable arranca con contenido
            // INDEFINIDO, y un mix con hysteresis 0.97 propagaria esa basura para
            // siempre. Se hace con un FBO temporal y Clear: cero RHI nuevo.
            static void ClearAtlas(const Texture& atlas);

            Renderer&    m_renderer;
            VertexArray& m_quad;   // sin uso en el pase; ver el comentario de Create
            Shaders      m_shaders;

            Texture m_capture;      // 96x512 RGBA16F: rgb = radiancia, a = distancia
            Texture m_irradiance;   // atlas octaedrico RGBA16F
            Texture m_distance;     // atlas de momentos RG16F

            // El proxy contra el que traza la captura. Lo llena m_voxelize, que
            // NO corre por frame.
            VoxelGrid                     m_grid;
            std::unique_ptr<VoxelizePass> m_voxelize;
            bool                          m_gridValido = false;
            u64                           m_gridGeneracion = 0;   // SceneGraph::Generation() del horneado

            UniformBuffer m_traceUbo { sizeof(TraceVoxelPassBlock) };

            DdgiSettings m_settings;
            DdgiGrid     m_atlasGrid;      // la grilla con la que se alocaron los atlas
            UpdateRange  m_range          {};
            u32          m_cursor         = 0u;
            u32          m_sweepsDone     = 0u;
            bool         m_blendedOnce    = false;   // gate de atlasValid
            f32          m_lastMs         = 0.0f;
    };

}
}
