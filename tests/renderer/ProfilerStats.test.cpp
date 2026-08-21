#include <doctest/doctest.h>
#include <efengine/renderer/ProfilerStats.h>

#include <string>

using efengine::renderer::NanosToMs;
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
