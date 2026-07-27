// tests/renderer/ShaderBlocks.test.cpp
// Los mirrors C++ de los bloques std140 son datos puros: se testean sin GPU.
// Esto es lo que el refactor a UBOs vuelve testeable — antes el empaquetado de
// estos valores era una tira de glUniform* que solo se podia verificar a ojo.
#include <doctest/doctest.h>
#include <efengine/renderer/ShaderBlocks.h>
#include <efengine/renderer/Renderer.h>

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
