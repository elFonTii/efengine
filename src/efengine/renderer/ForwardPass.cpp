#include "efengine/renderer/ForwardPass.h"

#include <efengine/core/Log.h>
#include <efengine/renderer/FrameContext.h>
#include <efengine/renderer/Renderer.h>
#include <efengine/scene/SceneGraph.h>

namespace efengine {
namespace renderer {

    void ForwardPass::Execute(FrameContext& ctx) {
        DrawOptions opciones;
        // Con el prepass del AO ya resuelto, la visibilidad no se vuelve a
        // calcular: GL_EQUAL sombrea cada pixel una sola vez. Sin el, hay que
        // escribir profundidad como siempre.
        opciones.depth = ctx.depthReady ? DepthMode::Equal : DepthMode::Write;

        for (const scene::RenderItem& item : ctx.scene.Renderables()) {
            if (!item.model) {
                EF_LOG_WARNING("Se intenta renderizar un item sin modelo");
                continue;
            }
            ctx.renderer.Submit(*item.model, *item.materials, item.world, opciones);
        }
    }

}
}
