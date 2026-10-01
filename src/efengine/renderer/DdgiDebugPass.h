#pragma once
#include <efengine/core/Types.h>
#include <efengine/renderer/IScenePass.h>
#include <efengine/renderer/ShaderBlocks.h>
#include <efengine/renderer/UniformBuffer.h>

namespace efengine {
namespace renderer {

    class DdgiPass;
    class Model;
    class Renderer;
    class Shader;
    class Texture;
    class VertexArray;

    struct DdgiGrid;
    struct DdgiSettings;

    // Instrumento de debug de DDGI: el volcado del target de captura en una
    // esquina, y una esfera por probe mostrando su tile del atlas.
    class DdgiDebugPass : public IScenePass {
        public:
            // ddgi y sphere son observadores y pueden ser null: sin el pase de
            // DDGI no hay nada que volcar, y sin la esfera el modo de probes se
            // saltea (el resto del debug sigue funcionando).
            DdgiDebugPass(Renderer& renderer, VertexArray& fullscreenQuad,
                          Shader* blit, Shader* probe,
                          const DdgiPass* ddgi, const Model* sphere);

            // Va sobre la imagen HDR de la escena y antes del post: es un
            // instrumento de debug, no parte de la imagen.
            void Execute(FrameContext& ctx) override;

            const char* Name() const override { return "DdgiDebug"; }

            // Dibuja el buffer de rayos en la esquina inferior izquierda del render target
            // actual, recortado a los rayos y probes en uso. showDistance vuelca el alfa.
            void DrawRayBlit(const Texture& rays, bool showDistance, u32 rayCount, u32 probeCount);

            // Un draw por probe. Solo se llama con debugProbes activo: son
            // ProbeCount() draws, y no hay draw instanciado en el RHI (agregarlo
            // por un pase de debug no se justifica).
            void DrawProbes(const DdgiGrid& grid, const DdgiSettings& settings,
                            const Model& sphere);

        private:
            Renderer&     m_renderer;
            VertexArray&  m_quad;
            Shader*         m_blit;
            Shader*         m_probe;
            const DdgiPass* m_ddgi   = null;
            const Model*    m_sphere = null;
            UniformBuffer m_paramsUbo { sizeof(PostParamsBlock) };
    };

}
}
