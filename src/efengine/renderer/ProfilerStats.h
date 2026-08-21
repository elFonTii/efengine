#pragma once

#include <efengine/core/Types.h>

#include <vector>

namespace efengine {
namespace renderer {

    // Nanosegundos a milisegundos. Pasa por f64 a proposito: un f32 tiene 24
    // bits de mantisa, y una division entera (ns / 1000000) truncaria todo lo
    // que este por debajo del milisegundo -- que es donde van a vivir la
    // mayoria de estos pases.
    f32 NanosToMs(u64 nanos);

    // Lo que UN scope aporta en UN frame. Lo arma GpuProfiler al cosechar.
    struct ScopeSample {
        const char* name     = nullptr;
        u64         gpuNanos = 0u;
        f32         cpuMs    = 0.0f;
        u32         drawCalls = 0u;
    };

    // Una fila del panel, ya promediada sobre la ventana.
    struct PassRow {
        const char* name      = nullptr;
        f32         gpuMs     = 0.0f;
        f32         cpuMs     = 0.0f;
        f32         drawCalls = 0.0f;  // promedio: no tiene por que ser entero
        bool        present   = false; // false = no corrio en toda la ventana
    };

    // Acumula muestras por pase y las promedia cada kWindow segundos.
    //
    // NO llama a GL ni una vez: por eso se prueba headless, igual que AoMath.
    // GpuProfiler es quien la alimenta.
    class ProfilerStats {
        public:
            // La misma ventana que FrameStats::kRefresh. Los valores crudos por
            // frame bailan demasiado para leerlos en pantalla.
            static constexpr f32 kWindow = 0.25f;

            // Arranca la acumulacion de un frame. Las muestras que lleguen
            // despues pertenecen a este frame.
            void BeginFrame();

            // Suma una muestra al frame en curso. Un 'name' que no se vio antes
            // se registra al final del orden.
            void AddSample(const ScopeSample& sample);

            // Cierra el frame. Si la ventana se cumplio, recalcula las filas.
            void EndFrame(f32 dt);

            const std::vector<PassRow>& Rows() const { return m_rows; }

            // Suman solo las filas presentes.
            f32 TotalGpuMs() const;
            f32 TotalCpuMs() const;

        private:
            struct Acumulador {
                const char* name = nullptr;
                f64 gpuMs = 0.0;
                f64 cpuMs = 0.0;
                f64 draws = 0.0;
                // Cuantos frames de la ventana tuvieron este pase. NO es igual
                // al total de frames: un pase apagado no aporta ninguno, y
                // dividir por el total daria un promedio artificialmente bajo.
                u32 frames = 0u;
            };

            // El orden de este vector es el orden de las filas del panel: el de
            // PRIMERA APARICION. Se mantiene estable aunque un pase se saltee
            // frames.
            std::vector<Acumulador> m_acum;
            std::vector<PassRow>    m_rows;

            f32 m_sinceRefresh = 0.0f;

            // Devuelve el indice en m_acum, registrandolo si es nuevo.
            usize IndiceDe(const char* name);
    };

}
}
