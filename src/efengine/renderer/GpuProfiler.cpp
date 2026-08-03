#include "efengine/renderer/GpuProfiler.h"

#include <efecom/RHI.h>
#include <efengine/core/Log.h>

namespace efengine {
namespace renderer {

    namespace {
        GpuProfiler* g_active = nullptr;
    }

    void         SetActiveProfiler(GpuProfiler* profiler) { g_active = profiler; }
    GpuProfiler* ActiveProfiler() { return g_active; }

    GpuProfiler::GpuProfiler() {
        m_supported = efecom::TimestampQueriesSupported();
        if (!m_supported) {
            EF_LOG_WARNING("GpuProfiler: el driver no mide marcas de tiempo; el panel va a mostrar ceros");
            return;
        }

        // Todas las consultas de una: kSlots frames x kMaxScopes scopes x 2
        // marcas. Crearlas por demanda dejaria un glCreateQueries en medio del
        // frame la primera vez que aparece un scope nuevo.
        m_pool.resize(kSlots * kMaxScopes * 2u);
        for (u32& q : m_pool) q = efecom::CreateTimestampQuery();

        for (Slot& s : m_slots) s.entradas.reserve(kMaxScopes);
        m_pila.reserve(8u);
        m_inicios.reserve(8u);
    }

    GpuProfiler::~GpuProfiler() {
        if (g_active == this) g_active = nullptr;
        for (u32 q : m_pool) efecom::DestroyTimestampQuery(q);
    }

    bool GpuProfiler::Cosechar(Slot& slot) {
        if (!slot.pendiente) return true;

        // La ULTIMA entrada es la que termina mas tarde: si su marca de fin ya
        // esta, todas las anteriores tambien. Un solo chequeo alcanza.
        if (!slot.entradas.empty()) {
            const Entrada& ultima = slot.entradas.back();
            // Preguntar SIEMPRE antes de leer. TimestampNanos sobre una consulta
            // que la GPU no termino detiene la CPU hasta que termine.
            if (!efecom::TimestampAvailable(ultima.qFin)) return false;
        }

        for (const Entrada& e : slot.entradas) {
            const u64 t0 = efecom::TimestampNanos(e.qInicio);
            const u64 t1 = efecom::TimestampNanos(e.qFin);

            ScopeSample muestra;
            muestra.name      = e.name;
            muestra.gpuNanos  = (t1 > t0) ? (t1 - t0) : 0ull;
            muestra.cpuMs     = e.cpuMs;
            muestra.drawCalls = e.drawsFin - e.drawsInicio;
            m_stats.AddSample(muestra);
        }

        slot.entradas.clear();
        slot.pendiente = false;
        return true;
    }

    void GpuProfiler::BeginFrame(f32 dt) {
        // El frame anterior cierra ACA, despues de que sus muestras entraron.
        // Cosechar primero y cerrar despues es lo que hace que EndFrame vea el
        // frame completo.
        m_slotActual = (m_slotActual + 1u) % kSlots;

        m_stats.BeginFrame();

        // Se limpian ANTES de la rama de abajo: si el profiler se apago a mitad
        // del frame anterior, quedaron scopes abiertos que nadie va a cerrar.
        m_pila.clear();
        m_inicios.clear();

        if (!m_supported || !m_enabled) {
            m_midiendoEsteFrame = false;
            m_stats.EndFrame(dt);
            return;
        }

        Slot& slot = m_slots[m_slotActual];

        // Si el slot que toca pisar todavia tiene consultas en vuelo, este frame
        // NO se mide. Pisarlas perderia las marcas y daria tiempos basura.
        m_midiendoEsteFrame = Cosechar(slot);

        m_stats.EndFrame(dt);

        m_poolCursor = m_slotActual * kMaxScopes * 2u;
    }

    void GpuProfiler::PushScope(const char* name) {
        if (!m_midiendoEsteFrame) return;

        Slot& slot = m_slots[m_slotActual];
        if (slot.entradas.size() >= kMaxScopes) return;   // techo: se ignora, no se rompe

        Entrada e;
        e.name        = name;
        e.qInicio     = m_pool[m_poolCursor++];
        e.qFin        = m_pool[m_poolCursor++];
        e.drawsInicio = efecom::GetDrawCallCount();

        efecom::WriteTimestamp(e.qInicio);

        slot.entradas.push_back(e);
        m_pila.push_back(slot.entradas.size() - 1u);
        m_inicios.push_back(std::chrono::steady_clock::now());
    }

    void GpuProfiler::PopScope() {
        if (!m_midiendoEsteFrame || m_pila.empty()) return;

        Slot&    slot = m_slots[m_slotActual];
        Entrada& e    = slot.entradas[m_pila.back()];

        efecom::WriteTimestamp(e.qFin);
        e.drawsFin = efecom::GetDrawCallCount();
        e.cpuMs    = std::chrono::duration<f32, std::milli>(
                         std::chrono::steady_clock::now() - m_inicios.back()).count();

        m_pila.pop_back();
        m_inicios.pop_back();
        slot.pendiente = true;
    }

    ProfileScope::ProfileScope(const char* name) {
        if (GpuProfiler* p = ActiveProfiler()) p->PushScope(name);
    }

    ProfileScope::~ProfileScope() {
        if (GpuProfiler* p = ActiveProfiler()) p->PopScope();
    }

}
}
