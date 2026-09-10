#pragma once
#include <efengine/renderer/IScenePass.h>

namespace efengine {
namespace renderer {

    // Sube el bloque Frame (binding 0) con la camara del frame y los contextos
    // de iluminacion que los pases de arriba dejaron en el FrameContext.
    //
    // Es LA FRONTERA del pipeline, y por eso es un eslabon visible y no una
    // llamada escondida: los pases que van ANTES suben sus propios bloques
    // Frame (la captura de DDGI lo re-sube una vez por cara), asi que tienen
    // que correr antes de que este fije el del frame; los que van DESPUES leen
    // la camara de aca en vez de recibirla por parametro.
    class FrameUploadPass : public IScenePass {
        public:
            void Execute(FrameContext& ctx) override;

            const char* Name() const override { return "FrameUpload"; }
    };

}
}
