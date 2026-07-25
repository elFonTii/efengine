#pragma once

#include <efengine/core/Types.h>

namespace sandbox {

    // Los alias de Types.h (f32, usize, ...) viven en el namespace global.
    class FrameStats {
        public:
            static constexpr usize kHistory = 120;   // ~2 segundos a 60 FPS
            static constexpr f32   kRefresh = 0.25f; // cada cuanto se recalcula el promedio

            void Push(f32 dt) {
                const f32 ms = dt * 1000.0f;

                m_history[m_cursor] = ms;
                m_cursor = (m_cursor + 1) % kHistory;
                if (m_count < kHistory) ++m_count;

                m_accum += ms;
                ++m_frames;
                m_sinceRefresh += dt;
                if (m_sinceRefresh >= kRefresh && m_frames > 0) {
                    m_avgMs        = m_accum / static_cast<f32>(m_frames);
                    m_accum        = 0.0f;
                    m_frames       = 0;
                    m_sinceRefresh = 0.0f;
                }
            }

            f32 AvgMs()  const { return m_avgMs; }
            f32 AvgFps() const { return m_avgMs > 0.0f ? 1000.0f / m_avgMs : 0.0f; }
            
            const f32* History()       const { return m_history; }
            int        HistoryCount()  const { return static_cast<int>(m_count); }
            int        HistoryOffset() const { return m_count < kHistory ? 0 : static_cast<int>(m_cursor); }

        private:
            f32   m_history[kHistory] = {};
            usize m_cursor = 0;
            usize m_count  = 0;

            f32   m_avgMs        = 0.0f;
            f32   m_accum        = 0.0f;
            usize m_frames       = 0;
            f32   m_sinceRefresh = 0.0f;
    };

}
