#pragma once
#include <efengine/renderer/IScenePass.h>

namespace efengine {
namespace renderer {

    // Dibuja la geometria de la escena al framebuffer que dejo bindeado
    // SceneTargetPass. Es el pase que consume todo lo que publicaron los de
    // arriba: sombra, IBL, DDGI, AO e indirecta llegan a pbr.frag por los
    // bloques que subio FrameUploadPass.
    class ForwardPass : public IScenePass {
        public:
            void Execute(FrameContext& ctx) override;

            const char* Name() const override { return "Forward"; }
    };

}
}
