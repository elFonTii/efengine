#pragma once
#include <efengine/core/Types.h>

namespace efengine {
namespace core {
    class Time {
        public:
            void Tick();
            void Advance(f64 nowSeconds); // avanza reloj a un timestamp dado
            void SetMaxDelta(f32 seconds);

            f32 DeltaTime() const;
            f64 Elapsed() const;
        private:
            f64 m_start =       0.0;
            f64 m_last =        0.0;
            f64 m_elapsed =     0.0;
            f32 m_delta =       0.0f;
            f32 m_maxDelta =    0.1f; // 100ms
            bool m_started =    false;
    };
}
}