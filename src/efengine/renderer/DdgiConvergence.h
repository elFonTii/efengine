#pragma once
#include <efengine/core/Types.h>
#include <efengine/renderer/ShaderBlocks.h>

namespace efengine {
namespace renderer {

    // Delta media perceptual de la ultima actualizacion; negativa si no hubo texels.
    f32 MeanDelta(const DdgiGpuStats& stats);

    // Cuanto tarda el volumen en converger desde el ultimo reset. Las muestras llegan de
    // la GPU con atraso, asi que cada una trae el frame que la produjo: las de antes del
    // reset se descartan.
    class DdgiConvergence {
        public:
            void Reset(u64 frame, f64 seconds);
            void Observe(u64 frame, f64 seconds, f32 meanDelta, f32 epsilon);

            bool converged() const { return m_convergedSeconds >= 0.0; }
            f64  seconds()   const { return m_convergedSeconds; }

        private:
            u64 m_resetFrame       = 0u;
            f64 m_resetSeconds     = 0.0;
            f64 m_convergedSeconds = -1.0;
    };

}
}
