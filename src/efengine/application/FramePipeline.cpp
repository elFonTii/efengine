#include "efengine/application/FramePipeline.h"

namespace efengine {
namespace application {

    // EL ORDEN DEL FRAME. Es una lista literal y se lee de arriba abajo.
    //
    // Este archivo NO verifica dependencias, igual que PostChain::Add: las
    // restricciones reales estan documentadas en cada pase, y son estas.
    //
    //   - ShadowPass antes que DdgiPass: la captura de probes sombrea con la
    //     matriz y el depth del sol.
    //   - IblPass antes que DdgiPass y SkyboxPass: los dos leen el cubemap
    //     crudo del entorno, que publica en ctx.lighting.ibl.environment.
    //   - Shadow, Ibl, Ddgi y Ao antes que FrameUploadPass: suben sus propios
    //     bloques Frame (la captura de DDGI, uno por cara), asi que tienen que
    //     correr antes de que se fije el del frame.
    //   - IndirectPass despues de FrameUploadPass, porque lee la camara del
    //     bloque ya subido, y despues de AoPass, porque lee su prepass.
    //   - SceneTargetPass antes que SkyboxPass, y el skybox antes del forward.
    //   - DdgiDebugPass ultimo: dibuja sobre la imagen HDR, antes del post.
    void BuildFramePipeline(renderer::ScenePipeline& p, const PassDeps& d) {
        RegisterShadowPass(p, d);
        RegisterIblPass(p, d);
        RegisterDdgiPass(p, d);
        RegisterAoPass(p, d);
        RegisterFrameUploadPass(p, d);   // la frontera: aca se sube el bloque Frame
        RegisterIndirectPass(p, d);
        RegisterSceneTargetPass(p, d);
        RegisterSkyboxPass(p, d);
        RegisterForwardPass(p, d);
        RegisterDdgiDebugPass(p, d);     // necesita el DdgiPass ya registrado
    }

}
}
