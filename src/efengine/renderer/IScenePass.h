#pragma once
#include <efengine/core/Types.h>

namespace efengine {
namespace renderer {

    struct FrameContext;

    // Un eslabon del frame. El equivalente de IPostPass para los pases de
    // escena, que a diferencia del post NO comparten firma: un pase de post
    // siempre es (textura de entrada) -> (target), mientras que estos publican
    // y consumen contextos distintos. Por eso el parametro es el FrameContext
    // entero y no una textura.
    class IScenePass {
        public:
            // Campo publico y no un virtual, igual que Behavior::enabled. El
            // pipeline lo consulta antes de llamar a Execute.
            bool enabled = true;

            virtual ~IScenePass() = default;

            virtual void Execute(FrameContext& ctx) = 0;

            // Default vacio: un pase sin targets propios no tiene por que
            // implementarlo. Lo recibe incluso estando deshabilitado.
            virtual void Resize(u32 width, u32 height) {}

            // Para el scope del profiler y para el panel. Literal estatico.
            virtual const char* Name() const = 0;
    };

}
}
