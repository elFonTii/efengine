#include "Time.h"
#include "Assert.h"
#include <chrono>
#include <algorithm>

namespace {
    f64 nowSeconds() {
        auto point =  std::chrono::steady_clock::now();
        auto duration = point.time_since_epoch();

        return std::chrono::duration<f64>(duration).count(); // constructor de conversión
    }
}

namespace efengine {
namespace core {
    
    void Time::Tick() {
        Advance(nowSeconds());
    }

    f32 Time::DeltaTime() const {
        return m_delta;
    }

    f64 Time::Elapsed() const {
        return m_elapsed;
    }

    void Time::SetMaxDelta(f32 seconds) {
        EF_ASSERT(seconds > 0, "Time::SetMaxDelta: Se intentó guardar maxDelta negativo o cero");
        m_maxDelta = seconds;
    }

    void Time::SetFixedDelta(f32 seconds) {
        EF_ASSERT(seconds > 0.0f, "Time::SetFixedDelta: el paso fijo debe ser positivo");
        m_fixedDelta = seconds;
    }

    void Time::SetMaxFixedSteps(i32 steps) {
        EF_ASSERT(steps >= 1, "Time::SetMaxFixedSteps: hacen falta al menos 1 paso por frame");
        m_maxFixedSteps = steps;
    }

    f32  Time::FixedDelta() const          { return m_fixedDelta; }
    i32  Time::FixedSteps() const          { return m_fixedSteps; }
    f32  Time::Alpha() const               { return m_alpha; }
    bool Time::FixedStepsSaturated() const { return m_saturated; }

    void Time::accumulateFixed() {
        m_accumulator += m_delta;
        m_fixedSteps = 0;
        m_saturated  = false;

        while (m_accumulator >= m_fixedDelta && m_fixedSteps < m_maxFixedSteps) {
            m_accumulator -= m_fixedDelta;
            ++m_fixedSteps;
        }

        // Se agoto el tope y todavia sobra al menos un paso entero. Descartarlo
        // es lo que evita la espiral de la muerte: si se guardara, el frame
        // siguiente pediria todavia mas pasos, tardaria todavia mas, y asi.
        // Ponerlo en cero (en vez de un fmod) hace el resultado deterministico:
        // en un frame que ya reviento el presupuesto, la fase sub-paso no importa.
        if (m_accumulator >= m_fixedDelta) {
            m_saturated   = true;
            m_accumulator = 0.0f;
        }

        m_alpha = m_accumulator / m_fixedDelta;   // en [0,1) por construccion
    }

    void Time::Advance(f64 now) {
        EF_ASSERT(now >= m_last, "Time::Advance: se recibió un timestamp viejo");

        if(!m_started) {
            m_start = now;
            m_last = now;
            m_started = true;
            m_delta = 0.0f;
            m_elapsed = 0.0f;
            return;
        }

        const f32 raw = static_cast<f32>(now - m_last);
        m_delta = std::min(raw, m_maxDelta);
        m_elapsed = now - m_start;
        m_last = now;

        accumulateFixed();
    }
}
}