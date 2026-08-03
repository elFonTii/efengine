#include <doctest/doctest.h>
#include <efengine/renderer/AoMath.h>
#include <efengine/renderer/AoSettings.h>
#include <efengine/renderer/AoContext.h>
#include <glm/gtc/matrix_transform.hpp>
#include <cmath>

using efengine::renderer::AoBlock;
using efengine::renderer::AoContext;
using efengine::renderer::AoPassBlock;
using efengine::renderer::AoProjInfo;
using efengine::renderer::AoProjScale;
using efengine::renderer::AoSettings;
using efengine::renderer::MakeAoBlock;
using efengine::renderer::MakeAoPassBlock;
using efengine::renderer::SanitizeAoSettings;

namespace {
    // La camara del sandbox: 45 grados de FOV vertical, 16:9.
    glm::mat4 proyeccion() {
        return glm::perspective(glm::radians(45.0f), 16.0f / 9.0f, 0.1f, 100.0f);
    }
}

TEST_CASE("AoProjInfo: sale de la matriz, no de fov/aspect") {
    const glm::vec2 info = AoProjInfo(proyeccion());

    // tan(22.5 grados) = 0.414214, y el eje x lleva ademas el aspect.
    CHECK(info.y == doctest::Approx(0.414214f).epsilon(0.001f));
    CHECK(info.x == doctest::Approx(0.414214f * 16.0f / 9.0f).epsilon(0.001f));
}

TEST_CASE("AoProjScale: escala linealmente con el alto en pixeles") {
    const glm::mat4 p = proyeccion();

    // 0.5 * alto / tan(fovY/2)
    CHECK(AoProjScale(p, 1080u) == doctest::Approx(540.0f / 0.414214f).epsilon(0.001f));
    CHECK(AoProjScale(p, 2160u) == doctest::Approx(2.0f * AoProjScale(p, 1080u)).epsilon(0.001f));
}

TEST_CASE("SanitizeAoSettings: los valores del panel se corrigen, no se assertean") {
    AoSettings s;
    s.radius          = -1.0f;
    s.intensity       = -3.0f;
    s.thickness       =  0.0f;   // divisor en el shader: cero seria NaN silencioso
    s.slices          =  0;
    s.steps           = -5;
    s.maxScreenRadius =  0.0f;

    const AoSettings c = SanitizeAoSettings(s);

    CHECK(c.radius          == doctest::Approx(0.0f));   // radio 0 es LEGAL: es el control nulo
    CHECK(c.intensity       >= 0.0f);
    CHECK(c.thickness        > 0.0f);
    CHECK(c.slices          >= 1);
    CHECK(c.steps           >= 1);
    CHECK(c.maxScreenRadius  > 0.0f);
}

TEST_CASE("SanitizeAoSettings: los valores validos no se tocan") {
    AoSettings s;   // los defaults
    const AoSettings c = SanitizeAoSettings(s);

    CHECK(c.radius    == doctest::Approx(s.radius));
    CHECK(c.intensity == doctest::Approx(s.intensity));
    CHECK(c.thickness == doctest::Approx(s.thickness));
    CHECK(c.slices    == s.slices);
    CHECK(c.steps     == s.steps);
}

TEST_CASE("MakeAoBlock: sin textura, el AO queda apagado aunque enabled sea true") {
    AoContext ctx;
    ctx.enabled = true;
    ctx.texture = nullptr;   // el caso "fallo la carga de shaders, no hay AoPass"

    const AoBlock b = MakeAoBlock(ctx);

    CHECK(b.params.x == doctest::Approx(0.0f));
}

TEST_CASE("MakeAoBlock: los flags viajan como 0/1") {
    // Sin contexto GL no hay Texture real, pero MakeAoBlock solo compara el
    // puntero contra null: cualquier direccion no nula sirve de centinela.
    const efengine::renderer::Texture* fake =
        reinterpret_cast<const efengine::renderer::Texture*>(0x1);

    AoContext ctx;
    ctx.texture     = fake;
    ctx.enabled     = true;
    ctx.bentNormal  = true;
    ctx.multiBounce = false;
    ctx.debugView   = 2u;

    const AoBlock b = MakeAoBlock(ctx);

    CHECK(b.params.x == doctest::Approx(1.0f));
    CHECK(b.params.y == doctest::Approx(1.0f));
    CHECK(b.params.z == doctest::Approx(0.0f));
    CHECK(b.params.w == doctest::Approx(2.0f));
}

TEST_CASE("MakeAoPassBlock: viewToWorld es la inversa de la view") {
    const glm::mat4 view = glm::lookAt(glm::vec3(3.0f, 2.0f, 5.0f),
                                       glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    const AoSettings s;

    const AoPassBlock b = MakeAoPassBlock(view, proyeccion(), s, 1920u, 1080u, 0);

    const glm::mat4 id = b.viewToWorld * view;
    for (int c = 0; c < 4; ++c) {
        for (int r = 0; r < 4; ++r) {
            CHECK(id[c][r] == doctest::Approx((c == r) ? 1.0f : 0.0f).epsilon(0.001f));
        }
    }
}

TEST_CASE("MakeAoPassBlock: la direccion del blur y el debug llegan a counts") {
    AoSettings s;
    s.slices    = 3;
    s.steps     = 12;
    s.debugView = 4u;

    const AoPassBlock b = MakeAoPassBlock(glm::mat4(1.0f), proyeccion(), s, 1920u, 1080u, 1);

    CHECK(b.counts.x == 3);
    CHECK(b.counts.y == 12);
    CHECK(b.counts.z == 1);
    CHECK(b.counts.w == 4);

    // 1/resolucion, que es lo que el shader usa para dar un paso de un texel.
    CHECK(b.projInfo.z == doctest::Approx(1.0f / 1920.0f).epsilon(0.001f));
    CHECK(b.projInfo.w == doctest::Approx(1.0f / 1080.0f).epsilon(0.001f));
}

TEST_CASE("MakeAoPassBlock: sanea antes de subir") {
    AoSettings s;
    s.slices = 0;
    s.radius = -2.0f;

    const AoPassBlock b = MakeAoPassBlock(glm::mat4(1.0f), proyeccion(), s, 800u, 600u, 0);

    CHECK(b.counts.x  >= 1);
    CHECK(b.params0.x == doctest::Approx(0.0f));
}
