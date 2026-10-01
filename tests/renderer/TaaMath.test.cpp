#include <doctest/doctest.h>
#include <efengine/renderer/FrameView.h>
#include <efengine/renderer/TaaMath.h>

#include <glm/gtc/matrix_transform.hpp>

#include <cmath>
#include <limits>

using namespace efengine;
using namespace efengine::renderer;

namespace {
    bool Cerca(const glm::vec3& a, const glm::vec3& b, f32 eps = 1e-5f) {
        return std::abs(a.x - b.x) <= eps && std::abs(a.y - b.y) <= eps && std::abs(a.z - b.z) <= eps;
    }

    glm::mat4 Proyeccion() {
        return glm::perspective(glm::radians(60.0f), 16.0f / 9.0f, 0.1f, 100.0f);
    }
}

TEST_CASE("YCoCg: ida y vuelta") {
    const glm::vec3 colores[] = { {0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.2f, 0.7f, 0.1f}, {12.0f, 3.0f, 0.5f} };
    for (const glm::vec3& c : colores) CHECK(Cerca(YCoCgToRgb(RgbToYCoCg(c)), c));
    CHECK(RgbToYCoCg(glm::vec3(1.0f)).x == doctest::Approx(1.0f));
}

TEST_CASE("ClipToAabb: adentro no se toca") {
    const glm::vec3 h(0.2f, -0.3f, 0.9f);
    CHECK(Cerca(ClipToAabb(h, glm::vec3(-1.0f), glm::vec3(1.0f)), h));
}

TEST_CASE("ClipToAabb: afuera cae en la cara, sobre el segmento al centro") {
    CHECK(Cerca(ClipToAabb(glm::vec3(2.0f, 0.0f, 0.0f), glm::vec3(-1.0f), glm::vec3(1.0f)), glm::vec3(1.0f, 0.0f, 0.0f), 1e-4f));
    CHECK(Cerca(ClipToAabb(glm::vec3(3.0f, 1.5f, 0.0f), glm::vec3(-1.0f), glm::vec3(1.0f)), glm::vec3(1.0f, 0.5f, 0.0f), 1e-4f));
}

TEST_CASE("ClipToAabb: una caja degenerada no produce NaN") {
    const glm::vec3 r = ClipToAabb(glm::vec3(1.0f), glm::vec3(0.5f), glm::vec3(0.5f));
    CHECK(std::isfinite(r.x));
    CHECK(std::isfinite(r.y));
    CHECK(std::isfinite(r.z));
    CHECK(Cerca(r, glm::vec3(0.5f), 1e-3f));
}

TEST_CASE("CatmullRomWeights: suman 1 e interpolan en los extremos") {
    for (f32 t = 0.0f; t <= 1.0f; t += 0.125f) {
        const glm::vec4 w = CatmullRomWeights(t);
        CHECK(w.x + w.y + w.z + w.w == doctest::Approx(1.0f));
    }
    const glm::vec4 w0 = CatmullRomWeights(0.0f);
    CHECK(w0.x == doctest::Approx(0.0f));
    CHECK(w0.y == doctest::Approx(1.0f));
    CHECK(w0.z == doctest::Approx(0.0f));
    CHECK(w0.w == doctest::Approx(0.0f));
    const glm::vec4 w1 = CatmullRomWeights(1.0f);
    CHECK(w1.z == doctest::Approx(1.0f));
}

TEST_CASE("VelocityUv: un objeto que se mueve a la derecha da velocidad positiva y reproyecta") {
    const glm::mat4 vp = Proyeccion();
    const glm::vec4 p(0.0f, 0.0f, 0.0f, 1.0f);
    const glm::mat4 antes = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, -10.0f));
    const glm::mat4 ahora = glm::translate(glm::mat4(1.0f), glm::vec3(1.0f, 0.0f, -10.0f));
    const glm::vec4 clipCur  = vp * ahora * p;
    const glm::vec4 clipPrev = vp * antes * p;
    const glm::vec2 v = VelocityUv(clipCur, clipPrev);
    CHECK(v.x > 0.0f);
    CHECK(v.y == doctest::Approx(0.0f));
    const glm::vec2 uvPrev = ClipToUv(clipCur) - v;
    CHECK(uvPrev.x == doctest::Approx(ClipToUv(clipPrev).x));
}

TEST_CASE("CameraVelocityUv: camara quieta da cero, con o sin jitter") {
    const glm::mat4 vp  = Proyeccion();
    const glm::mat4 inv = glm::inverse(vp);
    const glm::vec2 v0 = CameraVelocityUv(glm::vec2(0.3f, 0.7f), 0.9f, inv, vp, glm::vec2(0.0f));
    CHECK(v0.x == doctest::Approx(0.0f).epsilon(1e-4));
    CHECK(v0.y == doctest::Approx(0.0f).epsilon(1e-4));
    const glm::vec2 v1 = CameraVelocityUv(glm::vec2(0.3f, 0.7f), 1.0f, inv, vp, glm::vec2(0.0004f, -0.0003f));
    CHECK(std::abs(v1.x) < 1e-4f);
    CHECK(std::abs(v1.y) < 1e-4f);
}

TEST_CASE("CameraVelocityUv: un yaw conocido da el corrimiento horizontal esperado") {
    const f32 theta = 0.1f;
    const glm::mat4 vp     = Proyeccion();
    const glm::mat4 prevVp = Proyeccion() * glm::rotate(glm::mat4(1.0f), -theta, glm::vec3(0.0f, 1.0f, 0.0f));
    const glm::vec2 v = CameraVelocityUv(glm::vec2(0.5f), 1.0f, glm::inverse(vp), prevVp, glm::vec2(0.0f));
    const f32 esperado = -0.5f * std::tan(theta) / (std::tan(glm::radians(30.0f)) * (16.0f / 9.0f));
    CHECK(v.x == doctest::Approx(esperado).epsilon(1e-3));
    CHECK(v.y == doctest::Approx(0.0f).epsilon(1e-4));
}

TEST_CASE("Velocidad: el camino del prepass y el del cielo coinciden para un punto estatico") {
    const f32 theta = 0.05f;
    const glm::mat4 vp     = Proyeccion();
    const glm::mat4 prevVp = Proyeccion() * glm::rotate(glm::mat4(1.0f), -theta, glm::vec3(0.0f, 1.0f, 0.0f));
    const glm::vec4 p(2.0f, 1.0f, -20.0f, 1.0f);
    const glm::vec4 clipCur  = vp * p;
    const glm::vec4 clipPrev = prevVp * p;
    const glm::vec2 porObjeto = VelocityUv(clipCur, clipPrev);
    const f32 depth = clipCur.z / clipCur.w * 0.5f + 0.5f;
    const glm::vec2 porCamara = CameraVelocityUv(ClipToUv(clipCur), depth, glm::inverse(vp), prevVp, glm::vec2(0.0f));
    CHECK(porCamara.x == doctest::Approx(porObjeto.x).epsilon(1e-3));
    CHECK(porCamara.y == doctest::Approx(porObjeto.y).epsilon(1e-3));
}

TEST_CASE("TaaBlend: entradas iguales, alfa 1 y un brillo aislado") {
    const glm::vec3 c(0.4f, 0.5f, 0.6f);
    CHECK(Cerca(TaaBlend(c, c, 0.1f), c));

    const f32 nan = std::numeric_limits<f32>::quiet_NaN();
    const glm::vec3 r = TaaBlend(c, glm::vec3(nan), 1.0f);
    CHECK(Cerca(r, c));

    const glm::vec3 brillo = TaaBlend(glm::vec3(0.1f), glm::vec3(100.0f), 0.1f);
    CHECK(brillo.x < 10.0f);
    CHECK(brillo.x > 0.1f);
}

TEST_CASE("MakeTaaBlock: alfa 1 sin historia o con reset, y el del slider si no") {
    FrameView v;
    v.width = 1920u; v.height = 1080u;
    v.historyValid = true;
    TaaSettings s;
    s.alpha = 0.1f;
    CHECK(MakeTaaBlock(v, s, false).params.x == doctest::Approx(0.1f));
    CHECK(MakeTaaBlock(v, s, true).params.x  == doctest::Approx(1.0f));
    v.historyValid = false;
    CHECK(MakeTaaBlock(v, s, false).params.x == doctest::Approx(1.0f));
}

TEST_CASE("MakeTaaBlock: alfa acotado, jitter en UV, pantalla y debug") {
    FrameView v;
    v.width = 1920u; v.height = 1080u;
    v.historyValid = true;
    v.jitterNdc = glm::vec2(0.002f, -0.004f);
    TaaSettings s;
    s.alpha = 0.0f;
    s.debugVelocity = true;
    const TaaBlock b = MakeTaaBlock(v, s, false);
    CHECK(b.params.x == doctest::Approx(kTaaMinAlpha));
    CHECK(b.params.y == doctest::Approx(1.0f));
    CHECK(b.jitterUv.x == doctest::Approx(0.001f));
    CHECK(b.jitterUv.y == doctest::Approx(-0.002f));
    CHECK(b.screen.x == doctest::Approx(1920.0f));
    CHECK(b.screen.w == doctest::Approx(1.0f / 1080.0f));
}

TEST_CASE("MakeTaaBlock: 0x0 no divide por cero") {
    FrameView v;
    const TaaBlock b = MakeTaaBlock(v, TaaSettings(), false);
    CHECK(b.screen.z == doctest::Approx(0.0f));
    CHECK(b.screen.w == doctest::Approx(0.0f));
    CHECK(std::isfinite(b.invViewProjNoJitter[0][0]));
}

TEST_CASE("TaaBlend: una historia clipeada fuera de gamut no deja canales negativos") {
    const glm::vec3 r = TaaBlend(glm::vec3(0.5f), glm::vec3(-0.25f, 0.75f, -0.25f), 0.1f);
    CHECK(r.x >= 0.0f);
    CHECK(r.y >= 0.0f);
    CHECK(r.z >= 0.0f);
}
