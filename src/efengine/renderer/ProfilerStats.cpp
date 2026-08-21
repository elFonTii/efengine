#include "efengine/renderer/ProfilerStats.h"

#include <cstring>

namespace efengine {
namespace renderer {

    f32 NanosToMs(u64 nanos) {
        return static_cast<f32>(static_cast<f64>(nanos) / 1.0e6);
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
            }
            // Si no corrio, la fila queda en ceros con present=false. La fila NO
            // se borra: sacarla haria saltar el orden de las demas en el panel.

            m_rows.push_back(fila);

            a.gpuMs  = 0.0;
            a.cpuMs  = 0.0;
            a.draws  = 0.0;
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
