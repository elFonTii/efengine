#pragma once
#include <efengine/core/Types.h>
#include <efengine/renderer/DdgiVolume.h>
#include <efengine/renderer/DdgiSettings.h>
#include <efengine/renderer/DdgiContext.h>
#include <efengine/renderer/Texture.h>
#include <efengine/renderer/ShadowContext.h>
#include <efengine/renderer/IblContext.h>

#include <optional>

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
    class DdgiPass {
        public:
            struct Shaders {
                Shader* capture         = null;
                Shader* captureSky      = null;
                Shader* blendIrradiance = null;
                Shader* blendDistance   = null;
            };

            // fullscreenQuad es el mismo VertexArray que usa SkyboxPass: el cielo
            // de la captura se dibuja con skybox.vert, que reconstruye la
            // direccion desde las esquinas del quad, no con un cubo.
            static std::optional<DdgiPass> Create(Renderer& renderer, VertexArray& fullscreenQuad,
                                                  const Shaders& shaders);

            ~DdgiPass();
            DdgiPass(const DdgiPass&)            = delete;
            DdgiPass& operator=(const DdgiPass&) = delete;
            DdgiPass(DdgiPass&& other) noexcept;
            DdgiPass& operator=(DdgiPass&& other) noexcept;

            // Captura los probes del frame y los integra al atlas. Corre DESPUES
            // del ShadowPass (necesita su depth y su matriz) y ANTES de
            // BeginScene (sube su propio FrameBlock por cara).
            void Update(const scene::SceneGraph& scene, const ShadowContext& shadow,
                        const IblContext& ibl, const Cubemap* env);

            DdgiContext Context() const;

            DdgiSettings&       settings()       { return m_settings; }
            const DdgiSettings& settings() const { return m_settings; }

            const Texture& captureTarget()   const { return m_capture; }
            const Texture& irradianceAtlas() const { return m_irradiance; }
            const Texture& distanceAtlas()   const { return m_distance; }

            u32  sweepsDone() const { return m_sweepsDone; }
            u32  cursor()     const { return m_cursor; }
            f32  lastMs()     const { return m_lastMs; }

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
                     u32 captureFbo, u32 captureDepthRbo);

            // Realoca los dos atlas si la grilla cambio de tamano. Los deja en
            // negro y rearma el contador de barridos.
            void EnsureAtlasSize();

            // Deja un atlas en cero. El storage inmutable arranca con contenido
            // INDEFINIDO, y un mix con hysteresis 0.97 propagaria esa basura para
            // siempre. Se hace con un FBO temporal y Clear: cero RHI nuevo.
            static void ClearAtlas(const Texture& atlas);

            void CaptureProbe(const scene::SceneGraph& scene, const ShadowContext& shadow,
                              const IblContext& ibl, const Cubemap* env,
                              u32 probeIndex, u32 slot);

            Renderer&    m_renderer;
            VertexArray& m_quad;
            Shaders      m_shaders;

            Texture m_capture;      // 96x512 RGBA16F: rgb = radiancia, a = distancia
            Texture m_irradiance;   // atlas octaedrico RGBA16F
            Texture m_distance;     // atlas de momentos RG16F

            u32 m_captureFbo      = 0u;
            u32 m_captureDepthRbo = 0u;

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
