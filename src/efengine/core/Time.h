#pragma once
#include <efengine/core/Types.h>

namespace efengine {
namespace core {
    class Time {
        public:
            void Tick();
            void Advance(f64 nowSeconds);
            void SetMaxDelta(f32 seconds);

            // PASO FIJO
            // El frame variable se acumula y se reparte en pasos de largo
            // constante. FixedSteps() dice cuantos correr ESTE frame; Alpha() es
            // el sobrante normalizado, para interpolar al renderizar.
            //
            // Los dos son getters puros: el reparto lo hace Advance(). Si
            // consumieran el acumulador, llamarlos dos veces daria distinto.
            void SetFixedDelta(f32 seconds);
            void SetMaxFixedSteps(i32 steps);

            f32  FixedDelta() const;
            i32  FixedSteps() const;
            f32  Alpha() const;

            // El frame pidio mas pasos que el tope y el sobrante se descarto: el
            // tiempo simulado se atrasa del real a proposito.
            bool FixedStepsSaturated() const;

            f32 DeltaTime() const;
            f64 Elapsed() const;
        private:
            void accumulateFixed();

            f32  m_fixedDelta    = 1.0f / 60.0f;
            i32  m_maxFixedSteps = 5;
            f32  m_accumulator   = 0.0f;
            i32  m_fixedSteps    = 0;
            f32  m_alpha         = 0.0f;
            bool m_saturated     = false;

            f64 m_start =       0.0;
            f64 m_last =        0.0;
            f64 m_elapsed =     0.0;
            f32 m_delta =       0.0f;
            f32 m_maxDelta =    0.1f; // 100ms
            bool m_started =    false;
    };
}
}