#include "efengine/renderer/DdgiConvergence.h"

namespace efengine {
namespace renderer {

    f32 MeanDelta(const DdgiGpuStats& stats) {
        if (stats.deltaCount == 0u) return -1.0f;
        return static_cast<f32>(stats.deltaSum)
             / (kDdgiDeltaScale * static_cast<f32>(stats.deltaCount));
    }

    void DdgiConvergence::Reset(u64 frame, f64 seconds) {
        m_resetFrame       = frame;
        m_resetSeconds     = seconds;
        m_convergedSeconds = -1.0;
    }

    void DdgiConvergence::Observe(u64 frame, f64 seconds, f32 meanDelta, f32 epsilon) {
        if (converged() || frame <= m_resetFrame || meanDelta < 0.0f) return;
        if (meanDelta < epsilon) m_convergedSeconds = seconds - m_resetSeconds;
    }

}
}
