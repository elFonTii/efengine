#pragma once

namespace efengine {
namespace renderer { class ScenePipeline; }
namespace application {

    struct PassDeps;

    // Una funcion de registro por pase. Cada una vive en su propio .cpp dentro
    // de passes/, junto a la carga de los shaders de ese pase: agregar un pase
    // es escribir uno mas y sumarlo a la lista de abajo.
    //
    // Todas tienen la misma firma a proposito. Ninguna devuelve nada: si el
    // pase no se pudo crear, no se registra y el frame sigue sin el.
    void RegisterShadowPass     (renderer::ScenePipeline&, const PassDeps&);
    void RegisterIblPass        (renderer::ScenePipeline&, const PassDeps&);
    void RegisterDdgiPass       (renderer::ScenePipeline&, const PassDeps&);
    void RegisterAoPass         (renderer::ScenePipeline&, const PassDeps&);
    void RegisterFrameUploadPass(renderer::ScenePipeline&, const PassDeps&);
    void RegisterIndirectPass   (renderer::ScenePipeline&, const PassDeps&);
    void RegisterSceneTargetPass(renderer::ScenePipeline&, const PassDeps&);
    void RegisterSkyboxPass     (renderer::ScenePipeline&, const PassDeps&);
    void RegisterForwardPass    (renderer::ScenePipeline&, const PassDeps&);
    void RegisterDdgiDebugPass  (renderer::ScenePipeline&, const PassDeps&);

    // Arma el frame. Ver el comentario de la definicion: ES el orden.
    void BuildFramePipeline(renderer::ScenePipeline& pipeline, const PassDeps& deps);

}
}
