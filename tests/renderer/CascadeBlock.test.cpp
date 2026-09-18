// El layout std140 no perdona: un campo corrido y el shader lee basura SIN error
// de GL. Los static_assert cubren el tamano; estos tests cubren que los valores
// del encuadre lleguen a los slots correctos.
#include <doctest/doctest.h>
#include <efengine/renderer/ShaderBlocks.h>
#include <efengine/renderer/CascadeContext.h>

using efengine::renderer::CascadeBlock;
using efengine::renderer::CascadeContext;
using efengine::renderer::MakeCascadeBlock;
using efengine::renderer::kMaxCascades;

TEST_CASE("MakeCascadeBlock: los cortes van a los 4 slots en orden") {
    CascadeContext ctx;
    ctx.enabled = true;
    ctx.count   = 4u;
    for (u32 i = 0; i < 4u; ++i) {
        ctx.fits[i].splitFar       = 10.0f * static_cast<f32>(i + 1u);
        ctx.fits[i].texelWorldSize = 0.01f;
    }
    const CascadeBlock b = MakeCascadeBlock(ctx);
    CHECK(b.splitFar.x == doctest::Approx(10.0f));
    CHECK(b.splitFar.y == doctest::Approx(20.0f));
    CHECK(b.splitFar.z == doctest::Approx(30.0f));
    CHECK(b.splitFar.w == doctest::Approx(40.0f));
}

TEST_CASE("MakeCascadeBlock: el normal offset se convierte a metros POR cascada") {
    // El punto del bloque entero: el texel de la cascada 0 y el de la 3 no miden
    // lo mismo, asi que un unico slider en texels tiene que dar cuatro metros
    // distintos. Si esto se rompe, la cascada 3 se llena de acne o la 0 de
    // peter-panning, segun para que lado se equivoque.
    CascadeContext ctx;
    ctx.enabled            = true;
    ctx.count              = 4u;
    ctx.normalOffsetTexels = 2.0f;
    ctx.fits[0].texelWorldSize = 0.01f;
    ctx.fits[1].texelWorldSize = 0.04f;
    ctx.fits[2].texelWorldSize = 0.16f;
    ctx.fits[3].texelWorldSize = 0.64f;

    const CascadeBlock b = MakeCascadeBlock(ctx);
    CHECK(b.normalOffsets.x == doctest::Approx(0.02f));
    CHECK(b.normalOffsets.y == doctest::Approx(0.08f));
    CHECK(b.normalOffsets.z == doctest::Approx(0.32f));
    CHECK(b.normalOffsets.w == doctest::Approx(1.28f));
}

TEST_CASE("MakeCascadeBlock: apagado publica count 0") {
    CascadeContext ctx;
    ctx.enabled = false;
    ctx.count   = 4u;
    const CascadeBlock b = MakeCascadeBlock(ctx);
    CHECK(b.params.x == doctest::Approx(0.0f));
}

TEST_CASE("MakeCascadeBlock: count, banda y debug viajan en params") {
    CascadeContext ctx;
    ctx.enabled    = true;
    ctx.count      = 3u;
    ctx.blendRatio = 0.1f;
    ctx.debugView  = true;
    const CascadeBlock b = MakeCascadeBlock(ctx);
    CHECK(b.params.x == doctest::Approx(3.0f));
    CHECK(b.params.y == doctest::Approx(0.1f));
    CHECK(b.params.z == doctest::Approx(1.0f));
}

TEST_CASE("MakeCascadeBlock: las cascadas no usadas no dejan basura en los cortes") {
    // Con count 2, los slots 2 y 3 tienen que quedar en un corte que ningun
    // fragmento pueda elegir antes que los reales. Si quedaran en 0, la seleccion
    // por profundidad mandaria TODO a la primera cascada muerta.
    CascadeContext ctx;
    ctx.enabled = true;
    ctx.count   = 2u;
    ctx.fits[0].splitFar = 10.0f;
    ctx.fits[1].splitFar = 50.0f;
    const CascadeBlock b = MakeCascadeBlock(ctx);
    CHECK(b.splitFar.z >= 50.0f);
    CHECK(b.splitFar.w >= 50.0f);
}
