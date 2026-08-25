#pragma once

#include <efengine/core/Types.h>
#include <efengine/renderer/ProfilerStats.h>

#include <vector>

namespace sandbox {

    // Los alias de Types.h (f32, usize, ...) viven en el namespace global.
    //
    // Ademas del promedio lleva p99, p95, maximo y un conteo de picos.
    //
    // El promedio miente exactamente donde importa. Un frame de 4.3 ms de media
    // con dos picos de 15 ms se siente PEOR que uno de 6 ms parejo, y el promedio
    // dice lo contrario: 4.3 contra 6. Lo que se nota al mover la camara es la
    // cola, no la media, y la cola solo se ve con percentiles.
    //
    // La ventana del historico es de 512 frames y no de 120 por la misma razon:
    // 120 frames a 233 FPS son medio segundo, y un p99 sobre 120 muestras es
    // "el segundo peor de la ventana" -- ruido, no estadistica. Con 512 son ~2 s
    // y el p99 empieza a significar algo.
    class FrameStats {
        public:
            static constexpr usize kHistory = 512;   // ~2 s a 233 FPS, ~8.5 s a 60
            static constexpr f32   kRefresh = 0.25f; // cada cuanto se recalculan las metricas

            // A partir de cuantas veces el promedio un frame cuenta como pico.
            // 2.0 y no 3.0: los picos del grafico rondan 3-4x, y un umbral pegado
            // a lo que se quiere contar no distingue "sigue estando" de "ya casi".
            static constexpr f32   kSpikeFactor = 2.0f;

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
                    Recalcular();
                }
            }

            f32 AvgMs()  const { return m_avgMs; }
            f32 AvgFps() const { return m_avgMs > 0.0f ? 1000.0f / m_avgMs : 0.0f; }

            // Sobre la ventana entera del historico, no sobre el ultimo cuarto de
            // segundo: un pico cada 200 frames no aparece en una ventana de 60.
            f32 P95Ms() const { return m_p95; }
            f32 P99Ms() const { return m_p99; }
            f32 MaxMs() const { return m_max; }

            // Frames de la ventana por encima de kSpikeFactor * promedio. Es el
            // numero a mirar despues de tocar algo: si baja, el cambio ayudo a la
            // estabilidad aunque el promedio no se haya movido.
            u32 Spikes() const { return m_spikes; }

            const f32* History()       const { return m_history; }
            int        HistoryCount()  const { return static_cast<int>(m_count); }
            int        HistoryOffset() const { return m_count < kHistory ? 0 : static_cast<int>(m_cursor); }

        private:
            // Corre cada kRefresh, no por frame: son dos nth_element sobre 512
            // floats cuatro veces por segundo. Medir no puede costar mas que lo
            // que se mide.
            void Recalcular() {
                if (m_count == 0u) return;

                m_scratch.assign(m_history, m_history + m_count);

                // El maximo ANTES de los nth_element: los reordenan, y buscarlo
                // despues seguiria funcionando pero deja el codigo dependiendo de
                // un detalle de implementacion de Percentile.
                m_max = 0.0f;
                f32 suma = 0.0f;
                for (f32 v : m_scratch) {
                    if (v > m_max) m_max = v;
                    suma += v;
                }

                // El umbral de pico usa el promedio de LA VENTANA, no m_avgMs,
                // que es el del ultimo cuarto de segundo: comparar una ventana
                // contra el promedio de otra cuenta picos donde no los hay cada
                // vez que el framerate cambia de nivel.
                const f32 media = suma / static_cast<f32>(m_count);
                const f32 corte = media * kSpikeFactor;

                m_spikes = 0u;
                for (f32 v : m_scratch) if (v > corte) ++m_spikes;

                m_p95 = efengine::renderer::Percentile(m_scratch, 0.95f);
                m_p99 = efengine::renderer::Percentile(m_scratch, 0.99f);
            }

            f32   m_history[kHistory] = {};
            usize m_cursor = 0;
            usize m_count  = 0;

            f32   m_avgMs        = 0.0f;
            f32   m_accum        = 0.0f;
            usize m_frames       = 0;
            f32   m_sinceRefresh = 0.0f;

            f32   m_p95    = 0.0f;
            f32   m_p99    = 0.0f;
            f32   m_max    = 0.0f;
            u32   m_spikes = 0u;

            // Buffer de trabajo de Recalcular. Miembro y no local: Percentile
            // reordena lo que recibe, asi que hace falta una copia, y una copia
            // por refresco es una alocacion cada 0.25 s que no hace falta pagar.
            std::vector<f32> m_scratch;
    };

}
