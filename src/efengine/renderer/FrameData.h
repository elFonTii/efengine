#pragma once

#include <efengine/core/Types.h>
#include <glm/glm.hpp>

namespace efengine {
namespace renderer {

    // Máximo de point lights del frame. DEBE COINCIDIR con MAX_LIGHTS en los
    // shaders que declaran el bloque FrameData (assets/shaders/pbr.frag).
    inline constexpr u32 kFrameMaxLights = 4;

    // Espejo C++ exacto del uniform block std140 "FrameData" (binding = 0).
    // Se sube entero una vez por frame (Renderer::BeginScene) en lugar de
    // glUniform* por nombre. El contrato de layout lo clavan los static_assert
    // de abajo y tests/renderer/FrameData.test.cpp.
    //
    // Regla std140 aplicada: mat4 = 4 columnas vec4 (64 B); los arrays se
    // empaquetan con stride 16 B porque el elemento ya es vec4. Nada de vec3
    // sueltos: siempre vec4 con el canal w aprovechado como escalar.
    struct alignas(16) FrameData {
        glm::mat4 view;                                // 0
        glm::mat4 proj;                                // 64
        glm::mat4 viewProj;                            // 128
        glm::mat4 lightSpaceMatrix;                    // 192
        glm::vec4 camPos;                              // 256  xyz = posición de cámara
        glm::vec4 sunDirection;                        // 272  xyz = dirección, w = sombra on (0/1)
        glm::vec4 sunColor;                            // 288  xyz = color*intensidad, w = ambientFactor
        glm::vec4 shadowParams;                        // 304  x = biasMin, y = biasMax
        glm::vec4 pointPositions[kFrameMaxLights];     // 320  xyz = posición
        glm::vec4 pointColors[kFrameMaxLights];        // 384  xyz = color*intensidad
        glm::ivec4 counts;                             // 448  x = cantidad de point lights
    };                                                 // 464 total

    static_assert(sizeof(FrameData) == 464, "FrameData: tamaño std140 inesperado");
    static_assert(sizeof(FrameData) % 16 == 0, "FrameData: std140 exige multiplo de 16");

}
}
