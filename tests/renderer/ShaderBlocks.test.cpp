// tests/renderer/ShaderBlocks.test.cpp
// Los mirrors C++ de los bloques std140 son datos puros: se testean sin GPU.
// Esto es lo que el refactor a UBOs vuelve testeable — antes el empaquetado de
// estos valores era una tira de glUniform* que solo se podia verificar a ojo.
#include <doctest/doctest.h>
#include <efengine/renderer/ShaderBlocks.h>
#include <efengine/renderer/Renderer.h>
#include <efengine/renderer/DdgiVolume.h>
#include <efengine/renderer/DdgiSettings.h>

#include <cstddef>
#include <vector>

using namespace efengine;
using namespace efengine::renderer;

TEST_CASE("Layout std140: los tamanos de los bloques son multiplos de 16") {
    CHECK(sizeof(FrameBlock)      % 16u == 0u);
    CHECK(sizeof(LightsBlock)     % 16u == 0u);
    CHECK(sizeof(ObjectBlock)     % 16u == 0u);
    CHECK(sizeof(MaterialBlock)   % 16u == 0u);
    CHECK(sizeof(ShadowPassBlock) % 16u == 0u);
    CHECK(sizeof(PostParamsBlock) % 16u == 0u);
}

TEST_CASE("Layout std140: los offsets de FrameBlock son los calculados a mano") {
    CHECK(offsetof(FrameBlock, view)             ==   0u);
    CHECK(offsetof(FrameBlock, projection)       ==  64u);
    CHECK(offsetof(FrameBlock, lightSpaceMatrix) == 128u);
    CHECK(offsetof(FrameBlock, invViewProjRot)   == 192u);
    CHECK(offsetof(FrameBlock, viewPos)          == 256u);
    CHECK(offsetof(FrameBlock, shadowParams)     == 272u);
    CHECK(offsetof(FrameBlock, iblParams)        == 288u);
    CHECK(sizeof(FrameBlock) == 304u);
}

TEST_CASE("Layout std140: los offsets de LightsBlock son los calculados a mano") {
    CHECK(offsetof(LightsBlock, positions)    ==   0u);
    CHECK(offsetof(LightsBlock, colors)       ==  64u);
    CHECK(offsetof(LightsBlock, dirDirection) == 128u);
    CHECK(offsetof(LightsBlock, dirColor)     == 144u);
    CHECK(offsetof(LightsBlock, counts)       == 160u);
    CHECK(sizeof(LightsBlock) == 176u);
}

TEST_CASE("Layout std140: los offsets de MaterialBlock son los calculados a mano") {
    CHECK(offsetof(MaterialBlock, albedoTint)   ==  0u);
    CHECK(offsetof(MaterialBlock, emissiveTint) == 16u);
    CHECK(offsetof(MaterialBlock, scalars0)     == 32u);
    CHECK(offsetof(MaterialBlock, scalars1)     == 48u);
    CHECK(offsetof(MaterialBlock, mapMask)      == 64u);
    CHECK(sizeof(MaterialBlock) == 80u);
    CHECK(sizeof(ObjectBlock)   == 64u);
}

TEST_CASE("MakeLightsBlock: copia posicion y color de cada luz al slot que le toca") {
    std::vector<PointLight> luces;
    luces.push_back(PointLight{ glm::vec3(1.0f, 2.0f, 3.0f), glm::vec3(0.5f, 0.0f, 0.0f) });
    luces.push_back(PointLight{ glm::vec3(4.0f, 5.0f, 6.0f), glm::vec3(0.0f, 0.5f, 0.0f) });

    DirectionalLight sol { glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(1.0f, 0.9f, 0.8f) };

    const LightsBlock b = MakeLightsBlock(luces, sol);

    CHECK(b.counts.x == 2);
    CHECK(b.positions[0].x == doctest::Approx(1.0f));
    CHECK(b.positions[0].z == doctest::Approx(3.0f));
    CHECK(b.colors[1].y    == doctest::Approx(0.5f));
    CHECK(b.dirDirection.y == doctest::Approx(-1.0f));
    CHECK(b.dirColor.z     == doctest::Approx(0.8f));
}

TEST_CASE("MakeLightsBlock: mas luces que kMaxLights se recortan, no desbordan") {
    std::vector<PointLight> luces;
    for (u32 i = 0u; i < Renderer::kMaxLights + 3u; ++i) {
        luces.push_back(PointLight{ glm::vec3(static_cast<f32>(i)), glm::vec3(1.0f) });
    }

    const LightsBlock b = MakeLightsBlock(luces, DirectionalLight{});

    CHECK(b.counts.x == static_cast<i32>(Renderer::kMaxLights));
    // La ultima luz que entra es la de indice kMaxLights - 1, no la ultima del vector.
    CHECK(b.positions[Renderer::kMaxLights - 1u].x
          == doctest::Approx(static_cast<f32>(Renderer::kMaxLights - 1u)));
}

TEST_CASE("MakeLightsBlock: sin luces puntuales el contador queda en cero") {
    const LightsBlock b = MakeLightsBlock({}, DirectionalLight{});
    CHECK(b.counts.x == 0);
}

TEST_CASE("MakeFrameBlock: empaqueta shadowParams como (enabled, biasMin, biasMax)") {
    ShadowContext shadow;
    shadow.enabled = true;
    shadow.biasMin = 0.001f;
    shadow.biasMax = 0.004f;
    shadow.lightSpaceMatrix = glm::mat4(2.0f);

    const FrameBlock b = MakeFrameBlock(glm::mat4(1.0f), glm::mat4(1.0f),
                                        glm::vec3(7.0f, 8.0f, 9.0f), shadow, IblContext{});

    CHECK(b.shadowParams.x == doctest::Approx(1.0f));
    CHECK(b.shadowParams.y == doctest::Approx(0.001f));
    CHECK(b.shadowParams.z == doctest::Approx(0.004f));
    CHECK(b.lightSpaceMatrix[0][0] == doctest::Approx(2.0f));
    CHECK(b.viewPos.x == doctest::Approx(7.0f));
    CHECK(b.viewPos.z == doctest::Approx(9.0f));
}

TEST_CASE("MakeFrameBlock: shadowParams.x en cero cuando la sombra esta apagada") {
    const FrameBlock b = MakeFrameBlock(glm::mat4(1.0f), glm::mat4(1.0f),
                                        glm::vec3(0.0f), ShadowContext{}, IblContext{});
    CHECK(b.shadowParams.x == doctest::Approx(0.0f));
}

TEST_CASE("MakeFrameBlock: hasIbl es 1 solo con los tres cubemaps presentes") {
    // Sin ninguno.
    CHECK(MakeFrameBlock(glm::mat4(1.0f), glm::mat4(1.0f), glm::vec3(0.0f),
                         ShadowContext{}, IblContext{}).iblParams.x
          == doctest::Approx(0.0f));

    // Con dos de tres: el shader tiene que apagar el ambiente entero, no
    // muestrear una unidad de textura equivocada.
    const Cubemap* falsoCube = reinterpret_cast<const Cubemap*>(0x1);
    IblContext parcial;
    parcial.irradiance  = falsoCube;
    parcial.prefiltered = falsoCube;
    parcial.brdfLut     = null;
    CHECK(MakeFrameBlock(glm::mat4(1.0f), glm::mat4(1.0f), glm::vec3(0.0f),
                         ShadowContext{}, parcial).iblParams.x
          == doctest::Approx(0.0f));
}

TEST_CASE("MakeFrameBlock: iblParams lleva intensidad y maxLod") {
    const Cubemap* falsoCube = reinterpret_cast<const Cubemap*>(0x1);
    const Texture* falsaTex  = reinterpret_cast<const Texture*>(0x1);
    IblContext ibl;
    ibl.irradiance  = falsoCube;
    ibl.prefiltered = falsoCube;
    ibl.brdfLut     = falsaTex;
    ibl.intensity   = 0.75f;
    ibl.maxLod      = 4.0f;

    const FrameBlock b = MakeFrameBlock(glm::mat4(1.0f), glm::mat4(1.0f),
                                        glm::vec3(0.0f), ShadowContext{}, ibl);
    CHECK(b.iblParams.x == doctest::Approx(1.0f));
    CHECK(b.iblParams.y == doctest::Approx(0.75f));
    CHECK(b.iblParams.z == doctest::Approx(4.0f));
}

TEST_CASE("MakeFrameBlock: invViewProjRot ignora la traslacion de la vista") {
    // Dos vistas que solo difieren en la traslacion tienen que dar el MISMO
    // invViewProjRot: el skybox se ve "infinitamente lejos".
    const glm::mat4 proj = glm::mat4(1.0f);
    glm::mat4 viewA = glm::mat4(1.0f);
    glm::mat4 viewB = glm::mat4(1.0f);
    viewB[3] = glm::vec4(10.0f, 20.0f, 30.0f, 1.0f);   // solo traslacion

    const FrameBlock a = MakeFrameBlock(viewA, proj, glm::vec3(0.0f), ShadowContext{}, IblContext{});
    const FrameBlock b = MakeFrameBlock(viewB, proj, glm::vec3(0.0f), ShadowContext{}, IblContext{});

    for (int c = 0; c < 4; ++c)
        for (int r = 0; r < 4; ++r)
            CHECK(a.invViewProjRot[c][r] == doctest::Approx(b.invViewProjRot[c][r]));
}

TEST_CASE("Layout std140: los offsets de DdgiBlock son los calculados a mano") {
    CHECK(offsetof(DdgiBlock, gridOrigin)  ==  0u);
    CHECK(offsetof(DdgiBlock, gridSpacing) == 16u);
    CHECK(offsetof(DdgiBlock, gridCounts)  == 32u);
    CHECK(offsetof(DdgiBlock, atlasLayout) == 48u);
    CHECK(offsetof(DdgiBlock, updateRange) == 64u);
    CHECK(offsetof(DdgiBlock, params0)     == 80u);
    CHECK(offsetof(DdgiBlock, params1)     == 96u);
    CHECK(sizeof(DdgiBlock) == 112u);
    CHECK(sizeof(DdgiBlock) % 16u == 0u);
}

TEST_CASE("MakeDdgiBlock: copia la grilla y calcula el total de probes") {
    DdgiSettings s;
    s.grid.origin  = glm::vec3(-6.0f, 0.2f, -6.0f);
    s.grid.spacing = glm::vec3(1.5f);
    s.grid.counts  = glm::ivec3(8, 4, 8);

    const DdgiBlock b = MakeDdgiBlock(s.grid, s, UpdateRange{}, true);

    CHECK(b.gridOrigin.x  == doctest::Approx(-6.0f));
    CHECK(b.gridOrigin.y  == doctest::Approx( 0.2f));
    CHECK(b.gridSpacing.z == doctest::Approx( 1.5f));
    CHECK(b.gridCounts.x == 8);
    CHECK(b.gridCounts.y == 4);
    CHECK(b.gridCounts.z == 8);
    CHECK(b.gridCounts.w == 256);   // total, para el modulo del round-robin
}

TEST_CASE("MakeDdgiBlock: atlasLayout lleva columnas, filas y los dos tamanos de tile") {
    DdgiSettings s;
    s.grid.counts = glm::ivec3(8, 4, 8);

    const DdgiBlock b = MakeDdgiBlock(s.grid, s, UpdateRange{}, true);

    CHECK(b.atlasLayout.x == 32);   // countX * countY
    CHECK(b.atlasLayout.y ==  8);   // countZ
    CHECK(b.atlasLayout.z == static_cast<i32>(kIrradianceTile));
    CHECK(b.atlasLayout.w == static_cast<i32>(kDistanceTile));
}

TEST_CASE("MakeDdgiBlock: updateRange lleva el rango del frame y el stride de captura") {
    DdgiSettings s;
    s.probesPerFrame = 8u;

    UpdateRange r;
    r.first = 24u;
    r.count = 8u;

    const DdgiBlock b = MakeDdgiBlock(s.grid, s, r, true);

    CHECK(b.updateRange.x == 24);
    CHECK(b.updateRange.y ==  8);
    CHECK(b.updateRange.z == static_cast<i32>(kProbeFaceSize));
    CHECK(b.updateRange.w ==  8);   // probesPerFrame: el blend lo usa como stride
}

TEST_CASE("MakeDdgiBlock: params0 empaqueta hysteresis, intensidad y los dos bias") {
    DdgiSettings s;
    s.hysteresis = 0.9f;
    s.intensity  = 1.5f;
    s.normalBias = 0.3f;
    s.viewBias   = 0.05f;

    const DdgiBlock b = MakeDdgiBlock(s.grid, s, UpdateRange{}, true);

    CHECK(b.params0.x == doctest::Approx(0.9f));
    CHECK(b.params0.y == doctest::Approx(1.5f));
    CHECK(b.params0.z == doctest::Approx(0.3f));
    CHECK(b.params0.w == doctest::Approx(0.05f));
}

TEST_CASE("MakeDdgiBlock: params1.x es 1 solo con enabled y atlas valido") {
    DdgiSettings s;
    s.enabled = true;
    s.chebyshevSharpness = 4.0f;

    CHECK(MakeDdgiBlock(s.grid, s, UpdateRange{}, true).params1.x  == doctest::Approx(1.0f));
    CHECK(MakeDdgiBlock(s.grid, s, UpdateRange{}, true).params1.y  == doctest::Approx(4.0f));

    // Sin atlas: el shader tiene que caer a IBL puro, no samplear una unidad vacia.
    CHECK(MakeDdgiBlock(s.grid, s, UpdateRange{}, false).params1.x == doctest::Approx(0.0f));

    s.enabled = false;
    CHECK(MakeDdgiBlock(s.grid, s, UpdateRange{}, true).params1.x  == doctest::Approx(0.0f));
}

TEST_CASE("MakeDdgiBlock: la grilla se sanea antes de empaquetar") {
    // Un slider en cero no puede llegar al shader como cero.
    DdgiSettings s;
    s.grid.counts  = glm::ivec3(0, 4, 8);
    s.grid.spacing = glm::vec3(0.0f, 1.5f, 1.5f);

    const DdgiBlock b = MakeDdgiBlock(s.grid, s, UpdateRange{}, true);

    CHECK(b.gridCounts.x == 1);
    CHECK(b.gridSpacing.x > 0.0f);
}
