#pragma once

#include <efengine/core/Types.h>
#include <efengine/renderer/ProfilerStats.h>

#include <chrono>
#include <vector>

namespace efengine {
namespace renderer {

    // Mide cada pase del frame en GPU y en CPU, y cuantos draws emitio.
    //
    // El resultado de una consulta de GPU no esta listo en el frame en que se
    // emite, asi que se lee CON ATRASO: un anillo de kSlots frames, y el slot
    // que se va a pisar se cosecha antes de escribirlo. Con 3 slots eso da 3
    // frames de atraso, invisible para quien mira el panel.
    class GpuProfiler {
        public:
            static constexpr u32 kSlots     = 3u;   // = frames de atraso
            static constexpr u32 kMaxScopes = 32u;  // techo de scopes por frame

            GpuProfiler();
            ~GpuProfiler();

            // No-copiable NI movible: Application le pasa &m_profiler a
            // SetActiveProfiler, y mover el objeto dejaria ese puntero colgando.
            GpuProfiler(const GpuProfiler&)            = delete;
            GpuProfiler& operator=(const GpuProfiler&) = delete;
            GpuProfiler(GpuProfiler&&)                 = delete;
            GpuProfiler& operator=(GpuProfiler&&)      = delete;

            // Cosecha el frame viejo del slot que toca y abre el frame nuevo.
            void BeginFrame(f32 dt);

            void PushScope(const char* name);
            void PopScope();

            bool enabled()   const { return m_enabled; }
            bool supported() const { return m_supported; }
            void SetEnabled(bool v) { m_enabled = v; }

            const ProfilerStats& stats() const { return m_stats; }

        private:
            struct Entrada {
                const char* name = nullptr;
                u32 qInicio = 0u;
                u32 qFin    = 0u;
                f32 cpuMs   = 0.0f;
                u32 drawsInicio = 0u;
                u32 drawsFin    = 0u;
            };

            struct Slot {
                std::vector<Entrada> entradas;
                bool pendiente = false;   // tiene consultas sin cosechar
            };

            // Devuelve true si cosecho (o si no habia nada que cosechar).
            bool Cosechar(Slot& slot);

            Slot m_slots[kSlots];
            u32  m_slotActual = 0u;
            bool m_midiendoEsteFrame = false;

            // Indices dentro de m_slots[m_slotActual].entradas de los scopes
            // abiertos. Permite anidar.
            std::vector<usize> m_pila;

            // Cronometros de CPU de los scopes abiertos, en el mismo orden que
            // m_pila.
            std::vector<std::chrono::steady_clock::time_point> m_inicios;

            // Pool plano de consultas: kSlots * kMaxScopes * 2.
            std::vector<u32> m_pool;
            u32              m_poolCursor = 0u;

            ProfilerStats m_stats;
            bool m_supported = false;
            bool m_enabled   = true;
    };

    // El profiler del proceso. Es null hasta que Application construya el suyo,
    // y los scopes que corran antes de eso simplemente no miden.
    //
    // Estado global, si. Es el mismo trato que efecom ya hace con el cache de
    // pipeline state: la alternativa era un parametro mas en las firmas de los
    // cinco pases y de Renderer::Submit.
    void         SetActiveProfiler(GpuProfiler* profiler);
    GpuProfiler* ActiveProfiler();

    // Abre un scope en el ctor y lo cierra en el dtor. Con RAII el scope se
    // cierra tambien por los RETORNOS TEMPRANOS, que es exactamente lo que
    // AoPass::Render y DdgiPass::Update tienen.
    struct ProfileScope {
        explicit ProfileScope(const char* name);
        ~ProfileScope();

        ProfileScope(const ProfileScope&)            = delete;
        ProfileScope& operator=(const ProfileScope&) = delete;
    };

}
}

// La doble indireccion NO es decorativa: con una sola capa, el ## impide que
// __LINE__ se expanda y todas las variables se llamarian "efScope___LINE__",
// que colisiona apenas haya dos scopes en la misma funcion.
#define EF_PROFILE_CONCAT2(a, b) a##b
#define EF_PROFILE_CONCAT(a, b)  EF_PROFILE_CONCAT2(a, b)

#define EF_PROFILE_SCOPE(nombre) \
    const ::efengine::renderer::ProfileScope EF_PROFILE_CONCAT(efScope_, __LINE__) { nombre }
