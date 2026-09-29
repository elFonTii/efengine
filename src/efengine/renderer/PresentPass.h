#pragma once
#include <efengine/renderer/IScenePass.h>

namespace efengine {
namespace renderer {

    // Ultimo pase: copia el color actual al backbuffer. Con todo el post
    // apagado copia sceneFB. Deja bindeado el backbuffer: ImGui dibuja encima
    // de lo que quede bindeado.
    class PresentPass : public IScenePass {
        public:
            void Execute(FrameContext& ctx) override;
            const char* Name() const override { return "Present"; }
    };

}
}
