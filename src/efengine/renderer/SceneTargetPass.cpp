#include "efengine/renderer/SceneTargetPass.h"

#include <efecom/RHI.h>

#include <efengine/renderer/FrameContext.h>
#include <efengine/renderer/Framebuffer.h>
#include <efengine/renderer/Renderer.h>

namespace efengine {
namespace renderer {

    void SceneTargetPass::Execute(FrameContext& ctx) {
        ctx.sceneFB.Bind();

        // --- El depth prepass del forward es el prepass del AO ---
        // Si corrio, el depth del framebuffer de escena YA tiene la profundidad
        // de la escena con esta camara: se limpia solo el color y el forward
        // dibuja con GL_EQUAL, asi el overdraw se sombrea una sola vez y lo
        // tapado ni entra al fragment shader.
        //
        // Si no corrio (AO apagado, o fallo la carga de sus shaders), ese depth
        // tiene la profundidad del FRAME ANTERIOR y hay que limpiarlo: dibujar
        // con GL_EQUAL contra el dejaria la pantalla vacia.
        if (ctx.depthReady) {
            efecom::SetClearColor(m_clear[0], m_clear[1], m_clear[2], m_clear[3]);
            efecom::Clear(efecom::ClearMask::Color);
        } else {
            ctx.renderer.Clear(m_clear[0], m_clear[1], m_clear[2], m_clear[3]);
        }
    }

}
}
