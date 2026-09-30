#include "efengine/application/FramePipeline.h"

namespace efengine {
namespace application {

    // EL ORDEN DEL FRAME. Es una lista literal y se lee de arriba abajo.
    //
    // Este archivo NO verifica dependencias: las
    // restricciones reales estan documentadas en cada pase, y son estas.
    //
    //   - LightUploadPass primero: DdgiPass traza con las luces antes de BeginScene.
    //   - ShadowPass antes que DdgiPass: la captura de probes sombrea con la
    //     matriz y el depth del sol.
    //   - IblPass antes que DdgiPass y SkyboxPass: los dos leen el cubemap
    //     crudo del entorno, que publica en ctx.lighting.ibl.environment.
    //   - Shadow, Ibl, Ddgi, DepthPrepass y Ao antes que FrameUploadPass: suben sus propios
    //     bloques Frame (la captura de DDGI, uno por cara), asi que tienen que
    //     correr antes de que se fije el del frame.
    //   - ClusterLightPass despues de FrameUploadPass (lee uView) y antes del forward.
    //   - IndirectPass despues de FrameUploadPass, porque lee la camara del
    //     bloque ya subido, y despues de DepthPrepass, porque lee su prepass.
    //   - DepthPrepass antes que Ao: el AO lee su depthNormal.
    //   - SceneTargetPass antes que SkyboxPass, y el skybox antes del forward.
    //   - DdgiDebugPass antes del post: dibuja sobre la imagen HDR.
    //   - TaaPass despues de DdgiDebug: lo que se dibuja sobre el HDR tambien se acumula.
    //   - Taa -> Bloom -> Tonemap -> Present: TAA resuelve en HDR antes de que el
    //     bloom esparza la energia; Present siempre ultimo.
    void BuildFramePipeline(renderer::ScenePipeline& p, const PassDeps& d) {
        RegisterLightUploadPass(p, d);      // antes que todo: DDGI lee las luces de este frame
        RegisterCascadedShadowPass(p, d);   // las cascadas que usa pbr.frag
        RegisterShadowPass(p, d);           // el mapa unico de escena que usa la captura de DDGI
        RegisterIblPass(p, d);
        RegisterDdgiPass(p, d);
        RegisterDepthPrepass(p, d);
        RegisterAoPass(p, d);
        RegisterFrameUploadPass(p, d);   // la frontera: aca se sube el bloque Frame
        RegisterClusterLightPass(p, d);  // lee uView del bloque Frame ya subido
        RegisterIndirectPass(p, d);
        RegisterSceneTargetPass(p, d);
        RegisterSkyboxPass(p, d);
        RegisterForwardPass(p, d);
        RegisterDdgiDebugPass(p, d);     // necesita el DdgiPass ya registrado
        RegisterTaaPass(p, d);
        RegisterBloomPass(p, d);
        RegisterTonemapPass(p, d);
        RegisterFxaaPass(p, d);
        RegisterPresentPass(p, d);       // ultimo: deja bindeado el backbuffer para ImGui
    }

}
}
