#pragma once
#include <efengine/core/Types.h>

namespace efengine {
namespace renderer {

    struct FrameContext;

    // Un eslabon del frame. Los pases publican y consumen contextos distintos,
    // por eso el parametro es el FrameContext entero y no una textura; el post
    // se pasa el color por FrameContext::post.
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
