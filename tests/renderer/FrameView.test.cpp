#include <doctest/doctest.h>
#include <efengine/renderer/FrameView.h>
#include <efengine/scene/Camera.h>

#include <cmath>

using namespace efengine;
using namespace efengine::renderer;

namespace {
    scene::Camera Camara() {
        scene::Camera c;
        c.SetAspect(16.0f / 9.0f);
        c.LookAt(glm::vec3(1.0f, 2.0f, 5.0f), glm::vec3(0.0f));
        return c;
    }

    bool CasiIdentidad(const glm::mat4& m) {
        for (int c = 0; c < 4; ++c)
            for (int r = 0; r < 4; ++r)
                if (std::abs(m[c][r] - (c == r ? 1.0f : 0.0f)) > 1e-4f) return false;
        return true;
    }

    bool Finita(const glm::mat4& m) {
        for (int c = 0; c < 4; ++c)
            for (int r = 0; r < 4; ++r)
                if (!std::isfinite(m[c][r])) return false;
        return true;
    }
}

TEST_CASE("Halton: primeros valores en base 2 y 3") {
    CHECK(Halton(1, 2) == doctest::Approx(0.5f));
    CHECK(Halton(2, 2) == doctest::Approx(0.25f));
    CHECK(Halton(3, 2) == doctest::Approx(0.75f));
    CHECK(Halton(4, 2) == doctest::Approx(0.125f));
    CHECK(Halton(1, 3) == doctest::Approx(1.0f / 3.0f));
    CHECK(Halton(2, 3) == doctest::Approx(2.0f / 3.0f));
    CHECK(Halton(3, 3) == doctest::Approx(1.0f / 9.0f));
}

TEST_CASE("HaltonJitterPx: centrado en cero, dentro de medio pixel y ciclico") {
    CHECK(HaltonJitterPx(0, 8).x == doctest::Approx(0.0f));
    CHECK(HaltonJitterPx(0, 8).y == doctest::Approx(1.0f / 3.0f - 0.5f));
    CHECK(HaltonJitterPx(1, 8).x == doctest::Approx(-0.25f));
    for (u32 i = 0; i < 16; ++i) {
        const glm::vec2 j = HaltonJitterPx(i, 8);
        CHECK(std::abs(j.x) <= 0.5f);
        CHECK(std::abs(j.y) <= 0.5f);
        CHECK(j == HaltonJitterPx(i + 8, 8));
    }
    // sequenceLength 0 no divide por cero.
    CHECK(HaltonJitterPx(5, 0) == HaltonJitterPx(0, 1));
}

TEST_CASE("MakeFrameView sin jitter: la proyeccion es la de la camara bit a bit") {
    const scene::Camera cam = Camara();
    FrameHistory h;
    const FrameView v = MakeFrameView(cam, 1920, 1080, TemporalSettings{}, h);

    CHECK(v.projection == cam.ProjectionMatrix());
    CHECK(v.projectionNoJitter == cam.ProjectionMatrix());
    CHECK(v.view == cam.ViewMatrix());
    CHECK(v.jitterNdc == glm::vec2(0.0f));
    CHECK_FALSE(v.jitterEnabled);
    CHECK(v.viewProjNoJitter == cam.ProjectionMatrix() * cam.ViewMatrix());
}

TEST_CASE("MakeFrameView con jitter: solo cambian [2][0] y [2][1]") {
    const scene::Camera cam = Camara();
    TemporalSettings s; s.jitter = true;
    FrameHistory h;
    const FrameView v = MakeFrameView(cam, 1920, 1080, s, h);

    for (int c = 0; c < 4; ++c)
        for (int r = 0; r < 4; ++r) {
            if (c == 2 && (r == 0 || r == 1)) continue;
            CHECK(v.projection[c][r] == v.projectionNoJitter[c][r]);
        }
    CHECK(v.jitterEnabled);
    CHECK(v.viewProjNoJitter == cam.ProjectionMatrix() * cam.ViewMatrix());
}

TEST_CASE("MakeFrameView con jitter: el offset en NDC es exactamente jitterNdc") {
    const scene::Camera cam = Camara();
    TemporalSettings s; s.jitter = true;
    FrameHistory h; h.frameIndex = 1;   // Halton(2): (-0.25, +0.1667) px
    const FrameView v = MakeFrameView(cam, 1920, 1080, s, h);

    CHECK(v.jitterNdc.x == doctest::Approx(-0.25f * 2.0f / 1920.0f));
    CHECK(v.jitterNdc.y == doctest::Approx((2.0f / 3.0f - 0.5f) * 2.0f / 1080.0f));

    const glm::vec4 p(0.3f, -0.2f, 0.1f, 1.0f);
    const glm::vec4 conJ = v.projection * v.view * p;
    const glm::vec4 sinJ = v.projectionNoJitter * v.view * p;
    const glm::vec2 d = glm::vec2(conJ) / conJ.w - glm::vec2(sinJ) / sinJ.w;
    CHECK(d.x == doctest::Approx(v.jitterNdc.x).epsilon(1e-3));
    CHECK(d.y == doctest::Approx(v.jitterNdc.y).epsilon(1e-3));
}

TEST_CASE("MakeFrameView: 0x0 con jitter no produce NaN") {
    const scene::Camera cam = Camara();
    TemporalSettings s; s.jitter = true;
    FrameHistory h;
    const FrameView v = MakeFrameView(cam, 0, 0, s, h);
    CHECK(Finita(v.projection));
    CHECK(v.jitterNdc == glm::vec2(0.0f));
}

TEST_CASE("MakeFrameView: la historia da el viewProj del frame anterior") {
    scene::Camera cam = Camara();
    TemporalSettings s; s.jitter = true;
    FrameHistory h;

    const FrameView a = MakeFrameView(cam, 1280, 720, s, h);
    CHECK(a.prevViewProjNoJitter == a.viewProjNoJitter);   // sin historia: prev == actual
    CHECK(a.prevJitterNdc == a.jitterNdc);
    CHECK(a.frameIndex == 0u);
    CHECK(h.valid);
    CHECK(h.frameIndex == 1u);

    cam.LookAt(glm::vec3(4.0f, 2.0f, 5.0f), glm::vec3(0.0f));
    const FrameView b = MakeFrameView(cam, 1280, 720, s, h);
    CHECK(b.prevViewProjNoJitter == a.viewProjNoJitter);
    CHECK(b.prevJitterNdc == a.jitterNdc);
    CHECK(b.frameIndex == 1u);
    CHECK(b.viewProjNoJitter != a.viewProjNoJitter);

    h.valid = false;   // lo que hace ResetTemporalHistory
    const FrameView c = MakeFrameView(cam, 1280, 720, s, h);
    CHECK(c.prevViewProjNoJitter == c.viewProjNoJitter);
}

TEST_CASE("MakeFrameView: las inversas son inversas") {
    const scene::Camera cam = Camara();
    TemporalSettings s; s.jitter = true;
    FrameHistory h;
    const FrameView v = MakeFrameView(cam, 1920, 1080, s, h);
    CHECK(CasiIdentidad(v.invView * v.view));
    CHECK(CasiIdentidad(v.invProjection * v.projection));
    CHECK(v.viewPos == cam.Position());
}

TEST_CASE("MakeStaticFrameView: sin jitter y prev == actual") {
    const glm::mat4 proj = glm::mat4(1.0f);
    const glm::mat4 view = glm::mat4(1.0f);
    const FrameView v = MakeStaticFrameView(view, proj, glm::vec3(1.0f), 1u, 1u);
    CHECK(v.projection == proj);
    CHECK(v.projectionNoJitter == proj);
    CHECK(v.prevViewProjNoJitter == v.viewProjNoJitter);
    CHECK(v.jitterNdc == glm::vec2(0.0f));
    CHECK(v.frameIndex == 0u);
    CHECK_FALSE(v.jitterEnabled);
}

TEST_CASE("MakeFrameView: un frame 0x0 (minimizada) invalida la historia") {
    const scene::Camera cam = Camara();
    FrameHistory h;
    MakeFrameView(cam, 1280, 720, TemporalSettings{}, h);
    REQUIRE(h.valid);

    MakeFrameView(cam, 0, 0, TemporalSettings{}, h);
    CHECK_FALSE(h.valid);

    const FrameView v = MakeFrameView(cam, 1280, 720, TemporalSettings{}, h);
    CHECK(v.prevViewProjNoJitter == v.viewProjNoJitter);
}
