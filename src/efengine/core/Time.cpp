#include "Time.h"
#include "Assert.h"
#include <chrono>
#include <algorithm>

namespace {
    f64 nowSeconds() {
        auto point =  std::chrono::steady_clock::now(); // time point es un instante
        auto duration = point.time_since_epoch(); // duration es un intervalo

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
    }
}
}