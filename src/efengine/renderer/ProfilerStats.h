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

    // Percentil q (0..1) de un conjunto de muestras, por el metodo del vecino
    // mas cercano. REORDENA `muestras`: usa nth_element, que es O(n) y no pide
    // ordenar todo -- para un percentil no hace falta.
    //
    // Existe porque el promedio miente exactamente donde importa. Un frame de
    // 4.3 ms de media con dos picos de 15 ms se siente peor que uno de 6 ms
    // parejo, y el promedio dice lo contrario. El p99 es lo que hace visible
    // esa diferencia, y por eso el plan de optimizacion pide medirlo.
    //
    // Con menos de dos muestras devuelve la unica que hay (o 0 si no hay
    // ninguna): un percentil sobre un solo dato no significa nada, pero
    // devolver basura significa menos.
    f32 Percentile(std::vector<f32>& muestras, f32 q);

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

        // El PEOR frame de la ventana, por pase. Es el instrumento para
        // atribuir un pico de frametime: el promedio de un pase que cuesta
        // 0.15 ms casi siempre y 4 ms una vez cada 200 frames sigue diciendo
        // ~0.17 ms, y en el panel no se distingue de uno parejo. El maximo lo
        // delata de inmediato.
        //
        // Sin esto, "hay dos picos en el grafico de frametime" no se puede
        // convertir en "los produce ESTE pase" sin abrir Nsight.
        f32         gpuMaxMs  = 0.0f;
        f32         cpuMaxMs  = 0.0f;
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
                f32 gpuMax = 0.0f;
                f32 cpuMax = 0.0f;
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
