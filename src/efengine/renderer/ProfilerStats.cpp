#include "efengine/renderer/ProfilerStats.h"

#include <algorithm>
#include <cstddef>
#include <cstring>

namespace efengine {
namespace renderer {

    f32 NanosToMs(u64 nanos) {
        return static_cast<f32>(static_cast<f64>(nanos) / 1.0e6);
    }

    f32 Percentile(std::vector<f32>& muestras, f32 q) {
        if (muestras.empty()) return 0.0f;
        if (muestras.size() == 1u) return muestras[0];

        const f32 qc = std::clamp(q, 0.0f, 1.0f);

        // Vecino mas cercano y no interpolacion lineal: lo que se quiere leer es
        // "hubo un frame que costo esto", no un numero promediado entre dos
        // frames que nunca ocurrio. Para un pico eso importa.
        const usize n = muestras.size();
        usize k = static_cast<usize>(qc * static_cast<f32>(n - 1u) + 0.5f);
        if (k >= n) k = n - 1u;

        std::nth_element(muestras.begin(),
                         muestras.begin() + static_cast<std::ptrdiff_t>(k),
                         muestras.end());
        return muestras[k];
    }

    usize ProfilerStats::IndiceDe(const char* name) {
        // strcmp y no ==. Comparar dos const char* con == compara DIRECCIONES,
        // no contenido: dos literales iguales en dos .cpp distintos pueden vivir
        // en direcciones distintas, y el mismo pase apareceria dos veces en el
        // panel. Con doce pases el costo de strcmp es irrelevante.
        for (usize i = 0; i < m_acum.size(); ++i) {
            if (std::strcmp(m_acum[i].name, name) == 0) return i;
        }
        Acumulador nuevo;
        nuevo.name = name;
        m_acum.push_back(nuevo);
        return m_acum.size() - 1u;
    }

    void ProfilerStats::BeginFrame() {
        // No hay nada que reiniciar por frame: los acumuladores viven toda la
        // ventana, y 'frames' de cada pase cuenta cuantos frames lo tuvieron.
        // El metodo existe igual para que el ciclo del caller sea legible y para
        // tener donde colgar el reinicio si la ventana cambia de forma.
    }

    void ProfilerStats::AddSample(const ScopeSample& sample) {
        if (sample.name == nullptr) return;

        Acumulador& a = m_acum[IndiceDe(sample.name)];
        a.gpuMs += static_cast<f64>(NanosToMs(sample.gpuNanos));
        a.cpuMs += static_cast<f64>(sample.cpuMs);
        a.draws += static_cast<f64>(sample.drawCalls);
        a.frames += 1u;

        const f32 gpuMs = NanosToMs(sample.gpuNanos);
        if (gpuMs        > a.gpuMax) a.gpuMax = gpuMs;
        if (sample.cpuMs > a.cpuMax) a.cpuMax = sample.cpuMs;
    }

    void ProfilerStats::EndFrame(f32 dt) {
        m_sinceRefresh += dt;
        if (m_sinceRefresh < kWindow) return;

        m_rows.clear();
        m_rows.reserve(m_acum.size());

        for (Acumulador& a : m_acum) {
            PassRow fila;
            fila.name    = a.name;
            fila.present = (a.frames > 0u);

            if (fila.present) {
                // Se divide por los frames en que el pase CORRIO, no por el
                // total de la ventana: un pase que se prende la mitad del tiempo
                // no cuesta la mitad cuando corre.
                const f64 n = static_cast<f64>(a.frames);
                fila.gpuMs     = static_cast<f32>(a.gpuMs / n);
                fila.cpuMs     = static_cast<f32>(a.cpuMs / n);
                fila.drawCalls = static_cast<f32>(a.draws / n);
                fila.gpuMaxMs  = a.gpuMax;
                fila.cpuMaxMs  = a.cpuMax;
            }
            // Si no corrio, la fila queda en ceros con present=false. La fila NO
            // se borra: sacarla haria saltar el orden de las demas en el panel.

            m_rows.push_back(fila);

            a.gpuMs  = 0.0;
            a.cpuMs  = 0.0;
            a.draws  = 0.0;
            a.gpuMax = 0.0f;
            a.cpuMax = 0.0f;
            a.frames = 0u;
        }

        m_sinceRefresh = 0.0f;
    }

    f32 ProfilerStats::TotalGpuMs() const {
        f32 total = 0.0f;
        for (const PassRow& r : m_rows) if (r.present) total += r.gpuMs;
        return total;
    }

    f32 ProfilerStats::TotalCpuMs() const {
        f32 total = 0.0f;
        for (const PassRow& r : m_rows) if (r.present) total += r.cpuMs;
        return total;
    }

}
}
