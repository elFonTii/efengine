#include <doctest/doctest.h>
#include <efengine/renderer/ProfilerStats.h>

#include <string>
#include <vector>

using efengine::renderer::NanosToMs;
using efengine::renderer::Percentile;
using efengine::renderer::PassRow;
using efengine::renderer::ProfilerStats;
using efengine::renderer::ScopeSample;

namespace {
    // Un frame con un solo pase. dt = 0 para no gatillar el refresco: los tests
    // que quieren el promedio lo gatillan a proposito con un dt grande.
    void frameCon(ProfilerStats& st, const char* nombre, u64 gpuNs, f32 cpuMs,
                  u32 draws, f32 dt) {
        st.BeginFrame();
        st.AddSample(ScopeSample{ nombre, gpuNs, cpuMs, draws });
        st.EndFrame(dt);
    }

    const PassRow* filaLlamada(const ProfilerStats& st, const char* nombre) {
        for (const PassRow& r : st.Rows()) {
            if (std::string(r.name) == nombre) return &r;
        }
        return nullptr;
    }
}

TEST_CASE("NanosToMs: no trunca por debajo del milisegundo") {
    // 250 microsegundos. Una division entera daria 0.
    CHECK(NanosToMs(250'000ull) == doctest::Approx(0.25f).epsilon(0.0001f));
    CHECK(NanosToMs(16'666'667ull) == doctest::Approx(16.666667f).epsilon(0.0001f));
    CHECK(NanosToMs(0ull) == doctest::Approx(0.0f));
}

TEST_CASE("NanosToMs: un segundo entero no pierde precision") {
    // 1e9 ns no entra exacto en la mantisa de un f32 si se divide en f32.
    CHECK(NanosToMs(1'000'000'000ull) == doctest::Approx(1000.0f).epsilon(0.0001f));
}

TEST_CASE("Las filas salen en orden de PRIMERA aparicion, no alfabetico") {
    ProfilerStats st;

    st.BeginFrame();
    st.AddSample(ScopeSample{ "Sombras",  1'000'000ull, 0.1f, 5u });
    st.AddSample(ScopeSample{ "AO blur",  2'000'000ull, 0.2f, 1u });
    st.AddSample(ScopeSample{ "Forward",  3'000'000ull, 0.3f, 9u });
    st.EndFrame(1.0f);   // dt grande: fuerza el refresco

    REQUIRE(st.Rows().size() == 3);
    CHECK(std::string(st.Rows()[0].name) == "Sombras");
    CHECK(std::string(st.Rows()[1].name) == "AO blur");
    CHECK(std::string(st.Rows()[2].name) == "Forward");
}

TEST_CASE("El orden no cambia cuando un pase se saltea un frame") {
    ProfilerStats st;

    st.BeginFrame();
    st.AddSample(ScopeSample{ "Sombras", 1'000'000ull, 0.1f, 5u });
    st.AddSample(ScopeSample{ "Forward", 3'000'000ull, 0.3f, 9u });
    st.EndFrame(0.0f);

    // Este frame Sombras no corre. Forward NO tiene que subir al primer lugar.
    st.BeginFrame();
    st.AddSample(ScopeSample{ "Forward", 3'000'000ull, 0.3f, 9u });
    st.EndFrame(1.0f);

    REQUIRE(st.Rows().size() == 2);
    CHECK(std::string(st.Rows()[0].name) == "Sombras");
    CHECK(std::string(st.Rows()[1].name) == "Forward");
}

TEST_CASE("Un pase ausente en toda la ventana se reporta ausente") {
    ProfilerStats st;

    // Ventana 1: los dos corren.
    st.BeginFrame();
    st.AddSample(ScopeSample{ "Sombras", 4'000'000ull, 0.4f, 5u });
    st.AddSample(ScopeSample{ "Forward", 3'000'000ull, 0.3f, 9u });
    st.EndFrame(1.0f);

    REQUIRE(filaLlamada(st, "Sombras") != nullptr);
    CHECK(filaLlamada(st, "Sombras")->present);
    CHECK(filaLlamada(st, "Sombras")->gpuMs == doctest::Approx(4.0f));

    // Ventana 2: Sombras se apago. NO puede seguir diciendo 4 ms.
    st.BeginFrame();
    st.AddSample(ScopeSample{ "Forward", 3'000'000ull, 0.3f, 9u });
    st.EndFrame(1.0f);

    const PassRow* sombras = filaLlamada(st, "Sombras");
    REQUIRE(sombras != nullptr);        // la fila sigue, para que no salte el orden
    CHECK_FALSE(sombras->present);      // pero marcada como ausente
    CHECK(sombras->gpuMs == doctest::Approx(0.0f));
}

TEST_CASE("El promedio divide por los frames en que el pase CORRIO") {
    ProfilerStats st;

    // Cuatro frames; Sombras corre en dos, con 4 ms cada vez.
    frameCon(st, "Forward", 1'000'000ull, 0.1f, 9u, 0.0f);

    st.BeginFrame();
    st.AddSample(ScopeSample{ "Forward", 1'000'000ull, 0.1f, 9u });
    st.AddSample(ScopeSample{ "Sombras", 4'000'000ull, 0.4f, 5u });
    st.EndFrame(0.0f);

    frameCon(st, "Forward", 1'000'000ull, 0.1f, 9u, 0.0f);

    st.BeginFrame();
    st.AddSample(ScopeSample{ "Forward", 1'000'000ull, 0.1f, 9u });
    st.AddSample(ScopeSample{ "Sombras", 4'000'000ull, 0.4f, 5u });
    st.EndFrame(1.0f);   // cierra la ventana

    // 4 ms, no 2 ms: se divide por los 2 frames en que corrio, no por los 4.
    CHECK(filaLlamada(st, "Sombras")->gpuMs == doctest::Approx(4.0f).epsilon(0.001f));
    CHECK(filaLlamada(st, "Forward")->gpuMs == doctest::Approx(1.0f).epsilon(0.001f));
}

TEST_CASE("Los totales suman solo las filas presentes") {
    ProfilerStats st;

    st.BeginFrame();
    st.AddSample(ScopeSample{ "Sombras", 4'000'000ull, 0.4f, 5u });
    st.AddSample(ScopeSample{ "Forward", 3'000'000ull, 1.5f, 9u });
    st.EndFrame(1.0f);

    CHECK(st.TotalGpuMs() == doctest::Approx(7.0f).epsilon(0.001f));
    CHECK(st.TotalCpuMs() == doctest::Approx(1.9f).epsilon(0.001f));

    // Sombras se apaga: el total baja.
    st.BeginFrame();
    st.AddSample(ScopeSample{ "Forward", 3'000'000ull, 1.5f, 9u });
    st.EndFrame(1.0f);

    CHECK(st.TotalGpuMs() == doctest::Approx(3.0f).epsilon(0.001f));
    CHECK(st.TotalCpuMs() == doctest::Approx(1.5f).epsilon(0.001f));
}

TEST_CASE("Sin ningun frame cerrado, no hay filas ni totales") {
    ProfilerStats st;
    CHECK(st.Rows().empty());
    CHECK(st.TotalGpuMs() == doctest::Approx(0.0f));
    CHECK(st.TotalCpuMs() == doctest::Approx(0.0f));
}

// -- Percentiles --------------------------------------------------------------
// El promedio miente exactamente donde importa: una ventana de 4.3 ms de media
// con dos picos de 15 se siente peor que una de 6 pareja. Estos tests fijan el
// contrato del p99 que el overlay muestra.

TEST_CASE("Percentile: casos degenerados no inventan numeros") {
    std::vector<f32> vacio;
    CHECK(Percentile(vacio, 0.99f) == doctest::Approx(0.0f));

    std::vector<f32> uno { 4.2f };
    CHECK(Percentile(uno, 0.0f)  == doctest::Approx(4.2f));
    CHECK(Percentile(uno, 0.99f) == doctest::Approx(4.2f));
    CHECK(Percentile(uno, 1.0f)  == doctest::Approx(4.2f));
}

TEST_CASE("Percentile: q fuera de [0,1] se recorta en vez de indexar fuera") {
    std::vector<f32> v { 1.0f, 2.0f, 3.0f, 4.0f };
    CHECK(Percentile(v, -5.0f) == doctest::Approx(1.0f));
    CHECK(Percentile(v,  5.0f) == doctest::Approx(4.0f));
}

TEST_CASE("Percentile: devuelve una muestra REAL, no una interpolada") {
    // 0.5 sobre cuatro muestras cae entre la 2da y la 3ra. Interpolando daria
    // 2.5, que es un frame que nunca ocurrio; el metodo del vecino mas cercano
    // devuelve uno de los dos que si ocurrieron.
    std::vector<f32> v { 1.0f, 2.0f, 3.0f, 4.0f };
    const f32 p50 = Percentile(v, 0.5f);
    CHECK((p50 == doctest::Approx(2.0f) || p50 == doctest::Approx(3.0f)));

    std::vector<f32> w { 1.0f, 2.0f, 3.0f, 4.0f };
    CHECK(Percentile(w, 0.0f) == doctest::Approx(1.0f));
    std::vector<f32> x { 1.0f, 2.0f, 3.0f, 4.0f };
    CHECK(Percentile(x, 1.0f) == doctest::Approx(4.0f));
}

TEST_CASE("Percentile: el p99 ve el pico que el promedio esconde") {
    // 100 frames: 98 parejos a 4 ms y dos picos de 16. El promedio queda en
    // 4.24 -- indistinguible de una ventana sin picos -- y el p99 los muestra.
    std::vector<f32> v(98u, 4.0f);
    v.push_back(16.0f);
    v.push_back(16.0f);

    f32 suma = 0.0f;
    for (f32 x : v) suma += x;
    const f32 media = suma / static_cast<f32>(v.size());
    CHECK(media < 4.5f);                       // el promedio no acusa nada

    CHECK(Percentile(v, 0.99f) == doctest::Approx(16.0f));   // el p99 si
    std::vector<f32> w(98u, 4.0f);
    w.push_back(16.0f);
    w.push_back(16.0f);
    CHECK(Percentile(w, 0.50f) == doctest::Approx(4.0f));    // la mediana sigue limpia
}

TEST_CASE("Percentile: el orden de entrada no cambia el resultado") {
    std::vector<f32> asc  { 1.0f, 2.0f, 3.0f, 4.0f, 5.0f };
    std::vector<f32> desc { 5.0f, 4.0f, 3.0f, 2.0f, 1.0f };
    std::vector<f32> mezcla { 3.0f, 1.0f, 5.0f, 2.0f, 4.0f };

    CHECK(Percentile(asc,    0.75f) == doctest::Approx(Percentile(desc, 0.75f)));
    CHECK(Percentile(mezcla, 0.75f) == doctest::Approx(4.0f));
}

// -- Maximo por pase ----------------------------------------------------------

TEST_CASE("PassRow: el maximo por pase delata el frame que el promedio esconde") {
    ProfilerStats stats;

    // Nueve frames a 0.15 ms y uno a 4 ms. El promedio queda en 0.535: en la
    // columna de promedio no se distingue de un pase parejo de medio ms.
    for (int i = 0; i < 9; ++i) {
        stats.BeginFrame();
        ScopeSample m;
        m.name     = "DDGI captura";
        m.gpuNanos = 150000ull;      // 0.15 ms
        m.cpuMs    = 0.15f;
        stats.AddSample(m);
        stats.EndFrame(0.0f);
    }
    stats.BeginFrame();
    {
        ScopeSample pico;
        pico.name     = "DDGI captura";
        pico.gpuNanos = 4000000ull;  // 4 ms
        pico.cpuMs    = 4.0f;
        stats.AddSample(pico);
    }
    // dt que cierra la ventana y fuerza el recalculo de filas.
    stats.EndFrame(ProfilerStats::kWindow);

    REQUIRE(stats.Rows().size() == 1u);
    const PassRow& r = stats.Rows()[0];

    CHECK(r.present == true);
    CHECK(r.gpuMs   == doctest::Approx(0.535f).epsilon(0.01));
    CHECK(r.gpuMaxMs == doctest::Approx(4.0f));
    CHECK(r.cpuMaxMs == doctest::Approx(4.0f));
    // Esa es la senal: el peor frame es ~7x el promedio.
    CHECK(r.gpuMaxMs > r.gpuMs * 2.0f);
}

TEST_CASE("PassRow: el maximo se reinicia con cada ventana") {
    ProfilerStats stats;

    const auto frame = [&stats](f32 ms, f32 dt) {
        stats.BeginFrame();
        ScopeSample m;
        m.name     = "Forward";
        m.gpuNanos = static_cast<u64>(ms * 1.0e6f);
        m.cpuMs    = ms;
        stats.AddSample(m);
        stats.EndFrame(dt);
    };

    frame(9.0f, ProfilerStats::kWindow);          // ventana 1: un pico
    REQUIRE(stats.Rows().size() == 1u);
    CHECK(stats.Rows()[0].gpuMaxMs == doctest::Approx(9.0f));

    frame(1.0f, ProfilerStats::kWindow);          // ventana 2: tranquila
    // Si el maximo no se reiniciara, el panel seguiria acusando un pico que ya
    // paso y nadie podria verificar que un arreglo funciono.
    CHECK(stats.Rows()[0].gpuMaxMs == doctest::Approx(1.0f));
}
