#pragma once
#include <efengine/core/Types.h>
#include <efengine/renderer/IScenePass.h>

namespace efengine {
namespace renderer {

    // Bindea el framebuffer de escena y lo limpia.
    //
    // Es un pase propio y no la primera mitad de ForwardPass porque el skybox
    // corre ENTRE los dos: si el bind+clear viviera adentro del forward, el
    // skybox dibujaria en el target que dejo el AO.
    class SceneTargetPass : public IScenePass {
        public:
            // clearColor son los 4 floats que vive en Application. Puntero y no
            // copia: el color se cambia desde afuera en cualquier momento.
            explicit SceneTargetPass(const f32* clearColor) : m_clear(clearColor) {}

            void Execute(FrameContext& ctx) override;

            const char* Name() const override { return "SceneTarget"; }

        private:
            const f32* m_clear = null;
    };

}
}
