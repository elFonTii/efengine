#pragma once
#include <efengine/core/Types.h>
#include <efengine/renderer/ShaderBlocks.h>
#include <efengine/renderer/UniformBuffer.h>

namespace efengine {
namespace renderer {

    class Renderer;
    class Shader;
    class Texture;
    class VertexArray;

    // Instrumento de debug de DDGI. En esta tarea solo vuelca el target de
    // captura en una esquina; la tarea 10 le agrega las esferas de probe.
    class DdgiDebugPass {
        public:
            DdgiDebugPass(Renderer& renderer, VertexArray& fullscreenQuad, Shader* blit);

            // Dibuja el target de captura en la esquina inferior izquierda del
            // render target actual. showDistance vuelca el alfa en vez del rgb.
            void DrawCaptureBlit(const Texture& captureTarget, bool showDistance);

        private:
            Renderer&     m_renderer;
            VertexArray&  m_quad;
            Shader*       m_blit;
            UniformBuffer m_paramsUbo { sizeof(PostParamsBlock) };
    };

}
}
