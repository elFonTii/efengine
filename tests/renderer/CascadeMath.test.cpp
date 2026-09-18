// Los cortes son la unica parte del CSM que decide como se reparte la resolucion
// entre cerca y lejos. Un reparto mal hecho no rompe nada visible de golpe: deja
// la cascada 0 desperdiciada y la ultima pastosa, que es dificil de diagnosticar
// mirando la pantalla.
#include <doctest/doctest.h>
#include <efengine/renderer/ShadowMath.h>
#include <glm/gtc/matrix_transform.hpp>
#include <cmath>

using efengine::renderer::ComputeCascadeSplits;
using efengine::renderer::kMaxCascades;

TEST_CASE("ComputeCascadeSplits: el ultimo corte es exactamente shadowDistance") {
    f32 fars[kMaxCascades] = {};
    ComputeCascadeSplits(0.1f, 200.0f, 4u, 0.75f, fars);
    CHECK(fars[3] == doctest::Approx(200.0f));
}

TEST_CASE("ComputeCascadeSplits: los cortes son estrictamente crecientes") {
    f32 fars[kMaxCascades] = {};
    ComputeCascadeSplits(0.1f, 200.0f, 4u, 0.75f, fars);
    CHECK(fars[0] > 0.1f);
    CHECK(fars[1] > fars[0]);
    CHECK(fars[2] > fars[1]);
    CHECK(fars[3] > fars[2]);
}

TEST_CASE("ComputeCascadeSplits: lambda 0 da el reparto uniforme") {
    f32 fars[kMaxCascades] = {};
    ComputeCascadeSplits(0.0f, 200.0f, 4u, 0.0f, fars);
    CHECK(fars[0] == doctest::Approx(50.0f));
    CHECK(fars[1] == doctest::Approx(100.0f));
    CHECK(fars[2] == doctest::Approx(150.0f));
    CHECK(fars[3] == doctest::Approx(200.0f));
}

TEST_CASE("ComputeCascadeSplits: lambda 1 da el reparto logaritmico") {
    // log: near * (far/near)^(i/N). Con near=1, far=16, N=4 -> 2, 4, 8, 16.
    f32 fars[kMaxCascades] = {};
    ComputeCascadeSplits(1.0f, 16.0f, 4u, 1.0f, fars);
    CHECK(fars[0] == doctest::Approx(2.0f));
    CHECK(fars[1] == doctest::Approx(4.0f));
    CHECK(fars[2] == doctest::Approx(8.0f));
    CHECK(fars[3] == doctest::Approx(16.0f));
}

TEST_CASE("ComputeCascadeSplits: una sola cascada cubre todo el rango") {
    f32 fars[kMaxCascades] = {};
    ComputeCascadeSplits(0.1f, 200.0f, 1u, 0.75f, fars);
    CHECK(fars[0] == doctest::Approx(200.0f));
}

TEST_CASE("ComputeCascadeSplits: lambda intermedio queda entre los dos extremos") {
    f32 uniforme[kMaxCascades] = {};
    f32 logaritmico[kMaxCascades] = {};
    f32 mezcla[kMaxCascades] = {};
    ComputeCascadeSplits(1.0f, 16.0f, 4u, 0.0f, uniforme);
    ComputeCascadeSplits(1.0f, 16.0f, 4u, 1.0f, logaritmico);
    ComputeCascadeSplits(1.0f, 16.0f, 4u, 0.5f, mezcla);
    for (u32 i = 0; i < 3u; ++i) {
        CHECK(mezcla[i] <= uniforme[i]);
        CHECK(mezcla[i] >= logaritmico[i]);
    }
}

TEST_CASE("ComputeCascadeSplits: un near mayor que shadowDistance no produce NaN") {
    f32 fars[kMaxCascades] = {};
    ComputeCascadeSplits(300.0f, 200.0f, 4u, 0.75f, fars);
    for (u32 i = 0; i < 4u; ++i) {
        CHECK(std::isfinite(fars[i]));
    }
}

using efengine::renderer::CascadeFit;
using efengine::renderer::FitCascade;

namespace {
    // invView de una camara parada en 'pos' mirando a 'target'.
    glm::mat4 InvViewDe(const glm::vec3& pos, const glm::vec3& target) {
        return glm::inverse(glm::lookAt(pos, target, glm::vec3(0.0f, 1.0f, 0.0f)));
    }
}

TEST_CASE("FitCascade: el radio NO cambia al rotar la camara") {
    // Es la razon de ser de encuadrar por esfera y no por AABB del frustum. Si
    // este test se rompe, el tamano del texel pasa a depender de hacia donde
    // mira la camara y las sombras respiran al girar la cabeza.
    const glm::vec3 dir { -0.5f, -1.0f, -0.3f };
    const glm::vec3 pos { 10.0f, 2.0f, 10.0f };

    const CascadeFit a = FitCascade(dir, InvViewDe(pos, pos + glm::vec3(1, 0, 0)),
                                    60.0f, 1.777f, 1.0f, 50.0f, 2048u, 20.0f);
    const CascadeFit b = FitCascade(dir, InvViewDe(pos, pos + glm::vec3(0, 0, 1)),
                                    60.0f, 1.777f, 1.0f, 50.0f, 2048u, 20.0f);
    const CascadeFit c = FitCascade(dir, InvViewDe(pos, pos + glm::vec3(1, 1, 1)),
                                    60.0f, 1.777f, 1.0f, 50.0f, 2048u, 20.0f);

    CHECK(a.radius == doctest::Approx(b.radius));
    CHECK(a.radius == doctest::Approx(c.radius));
}

TEST_CASE("FitCascade: el radio NO cambia al trasladar la camara") {
    const glm::vec3 dir { -0.5f, -1.0f, -0.3f };
    const CascadeFit a = FitCascade(dir, InvViewDe(glm::vec3(0.0f), glm::vec3(0, 0, -1)),
                                    60.0f, 1.777f, 1.0f, 50.0f, 2048u, 20.0f);
    const CascadeFit b = FitCascade(dir, InvViewDe(glm::vec3(500.0f, 0.0f, 300.0f),
                                                  glm::vec3(500.0f, 0.0f, 299.0f)),
                                    60.0f, 1.777f, 1.0f, 50.0f, 2048u, 20.0f);
    CHECK(a.radius == doctest::Approx(b.radius));
}

TEST_CASE("FitCascade: una rebanada mas lejana tiene radio mayor") {
    const glm::vec3 dir { -0.5f, -1.0f, -0.3f };
    const glm::mat4 iv = InvViewDe(glm::vec3(0.0f), glm::vec3(0, 0, -1));
    const CascadeFit cerca = FitCascade(dir, iv, 60.0f, 1.777f, 1.0f, 10.0f, 2048u, 20.0f);
    const CascadeFit lejos = FitCascade(dir, iv, 60.0f, 1.777f, 10.0f, 100.0f, 2048u, 20.0f);
    CHECK(lejos.radius > cerca.radius);
}

TEST_CASE("FitCascade: el texel en metros es el diametro sobre la resolucion") {
    const glm::vec3 dir { 0.0f, -1.0f, 0.0f };
    const glm::mat4 iv = InvViewDe(glm::vec3(0.0f), glm::vec3(0, 0, -1));
    const CascadeFit f = FitCascade(dir, iv, 60.0f, 1.777f, 1.0f, 50.0f, 2048u, 20.0f);
    CHECK(f.texelWorldSize == doctest::Approx(2.0f * f.radius / 2048.0f));
}

TEST_CASE("FitCascade: un desplazamiento sub-texel NO mueve el centro") {
    // La estabilizacion. Sin esto el centro se corre una fraccion de texel por
    // frame y los bordes de sombra hierven al caminar.
    const glm::vec3 dir { -0.5f, -1.0f, -0.3f };
    const CascadeFit base = FitCascade(dir, InvViewDe(glm::vec3(0.0f), glm::vec3(0, 0, -1)),
                                       60.0f, 1.777f, 1.0f, 50.0f, 2048u, 20.0f);
    const f32 epsilon = base.texelWorldSize * 0.001f;
    const CascadeFit movida = FitCascade(dir, InvViewDe(glm::vec3(epsilon, 0.0f, 0.0f),
                                                       glm::vec3(epsilon, 0.0f, -1.0f)),
                                         60.0f, 1.777f, 1.0f, 50.0f, 2048u, 20.0f);
    CHECK(base.center.x == doctest::Approx(movida.center.x));
    CHECK(base.center.y == doctest::Approx(movida.center.y));
    CHECK(base.center.z == doctest::Approx(movida.center.z));
}

TEST_CASE("FitCascade: un desplazamiento de varios texels SI mueve el centro") {
    // El complemento del test anterior: si el centro no siguiera nunca a la
    // camara, la cascada se quedaria atras y la sombra desapareceria al caminar.
    const glm::vec3 dir { -0.5f, -1.0f, -0.3f };
    const CascadeFit base = FitCascade(dir, InvViewDe(glm::vec3(0.0f), glm::vec3(0, 0, -1)),
                                       60.0f, 1.777f, 1.0f, 50.0f, 2048u, 20.0f);
    const f32 lejos = base.texelWorldSize * 100.0f;
    const CascadeFit movida = FitCascade(dir, InvViewDe(glm::vec3(lejos, 0.0f, 0.0f),
                                                       glm::vec3(lejos, 0.0f, -1.0f)),
                                         60.0f, 1.777f, 1.0f, 50.0f, 2048u, 20.0f);
    CHECK(glm::length(movida.center - base.center) > base.texelWorldSize);
}

TEST_CASE("FitCascade: la extension hacia la luz agranda el rango de profundidad") {
    const glm::vec3 dir { 0.0f, -1.0f, 0.0f };
    const glm::mat4 iv = InvViewDe(glm::vec3(0.0f), glm::vec3(0, 0, -1));
    const CascadeFit sin_ext = FitCascade(dir, iv, 60.0f, 1.777f, 1.0f, 50.0f, 2048u, 0.0f);
    const CascadeFit con_ext = FitCascade(dir, iv, 60.0f, 1.777f, 1.0f, 50.0f, 2048u, 100.0f);
    CHECK(con_ext.depthRange == doctest::Approx(sin_ext.depthRange + 100.0f));
    // La extension NO puede cambiar el encuadre lateral: si lo cambiara, subirla
    // engordaria el texel y arruinaria la nitidez para tapar un bug de culling.
    CHECK(con_ext.radius == doctest::Approx(sin_ext.radius));
}

TEST_CASE("FitCascade: las 8 esquinas de la rebanada caen dentro de la caja") {
    const glm::vec3 dir { -0.5f, -1.0f, -0.3f };
    const glm::mat4 iv = InvViewDe(glm::vec3(3.0f, 2.0f, 1.0f), glm::vec3(0.0f));
    const CascadeFit f = FitCascade(dir, iv, 60.0f, 1.777f, 1.0f, 50.0f, 2048u, 20.0f);

    const f32 mitadV = std::tan(glm::radians(60.0f * 0.5f));
    for (int i = 0; i < 8; ++i) {
        const f32 z = (i & 4) ? 50.0f : 1.0f;
        const f32 y = ((i & 2) ? 1.0f : -1.0f) * mitadV * z;
        const f32 x = ((i & 1) ? 1.0f : -1.0f) * mitadV * 1.777f * z;
        const glm::vec3 mundo = glm::vec3(iv * glm::vec4(x, y, -z, 1.0f));
        const glm::vec4 ndc   = f.matrix * glm::vec4(mundo, 1.0f);
        CHECK(std::abs(ndc.x) <= doctest::Approx(1.0f).epsilon(0.01));
        CHECK(std::abs(ndc.y) <= doctest::Approx(1.0f).epsilon(0.01));
        CHECK(ndc.z >= doctest::Approx(-1.0f).epsilon(0.01));
        CHECK(ndc.z <= doctest::Approx( 1.0f).epsilon(0.01));
    }
}
