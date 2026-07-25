#include <doctest/doctest.h>
#include <efengine/renderer/FrameData.h>
#include <cstddef>

using efengine::renderer::FrameData;
using efengine::renderer::kFrameMaxLights;

// Estos offsets son el contrato con layout(std140, binding = 0) del bloque
// FrameData en GLSL. Si un miembro se mueve o cambia de tipo, este test tiene
// que fallar antes de que el shader lea basura.
TEST_CASE("FrameData: offsets std140 exactos") {
    CHECK(offsetof(FrameData, view)             == 0);
    CHECK(offsetof(FrameData, proj)             == 64);
    CHECK(offsetof(FrameData, viewProj)         == 128);
    CHECK(offsetof(FrameData, lightSpaceMatrix) == 192);
    CHECK(offsetof(FrameData, camPos)           == 256);
    CHECK(offsetof(FrameData, sunDirection)     == 272);
    CHECK(offsetof(FrameData, sunColor)         == 288);
    CHECK(offsetof(FrameData, shadowParams)     == 304);
    CHECK(offsetof(FrameData, pointPositions)   == 320);
    CHECK(offsetof(FrameData, pointColors)      == 384);
    CHECK(offsetof(FrameData, counts)           == 448);
    CHECK(sizeof(FrameData)                     == 464);
}

TEST_CASE("FrameData: los arrays de luces tienen stride vec4 (16 bytes)") {
    // std140 empaqueta arrays con stride del elemento redondeado a 16; al usar
    // vec4 directamente, el layout C++ y el GLSL coinciden 1:1.
    CHECK(sizeof(glm::vec4) == 16);
    CHECK(offsetof(FrameData, pointColors) - offsetof(FrameData, pointPositions)
          == kFrameMaxLights * 16);
}
