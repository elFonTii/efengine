#pragma once

#include <efengine/core/Types.h>
#include <efengine/renderer/PointLight.h>
#include <efengine/renderer/DirectionalLight.h>
#include <efengine/renderer/ShadowContext.h>
#include <efengine/renderer/IblContext.h>

#include <glm/glm.hpp>
#include <vector>

namespace efengine {
namespace renderer {

    // ── Indices de binding de los uniform blocks ───────────────────────────
    // Agrupados por frecuencia de actualizacion, que es lo que en Vulkan mapea a
    // set=0 (frame) / set=1 (material) / set=2 (object).
    inline constexpr u32 kFrameBinding    = 0u;   // 1x por frame
    inline constexpr u32 kLightsBinding   = 1u;   // 1x por frame
    inline constexpr u32 kObjectBinding   = 2u;   // 1x por render item
    inline constexpr u32 kMaterialBinding = 3u;   // 1x por bind de material
    inline constexpr u32 kPassBinding     = 4u;   // 1x por invocacion de pase

    // ── Mirrors C++ de los bloques std140 ──────────────────────────────────
    // Regla de std140 que gobierna todo esto: un vec3 ocupa igual 16 bytes, y un
    // array de vec3 paddea CADA elemento a 16. Promover todo a vec4 evita pelear
    // con eso y hace que el layout natural de C++ coincida byte a byte.
    // Los static_assert de sizeof/offsetof estan en ShaderBlocks.cpp.

    // Lo que no cambia en todo el frame. Lo sube Renderer::BeginScene.
    struct alignas(16) FrameBlock {
        glm::mat4 view;
        glm::mat4 projection;
        glm::mat4 lightSpaceMatrix;
        glm::mat4 invViewProjRot;   // para el skybox: inverse(proj * mat3(view))
        glm::vec4 viewPos;          // .xyz
        glm::vec4 shadowParams;     // x=enabled, y=biasMin, z=biasMax
        glm::vec4 iblParams;        // x=hasIbl, y=intensity, z=prefilterMaxLod
    };

    struct alignas(16) LightsBlock {
        glm::vec4  positions[4];    // .xyz — 4 == Renderer::kMaxLights
        glm::vec4  colors[4];       // .rgb
        glm::vec4  dirDirection;    // .xyz — direccion en la que VIAJA la luz
        glm::vec4  dirColor;        // .rgb — color * intensidad
        glm::ivec4 counts;          // x = cantidad de puntuales activas
    };

    struct alignas(16) ObjectBlock {
        glm::mat4 model;
    };

    struct alignas(16) MaterialBlock {
        glm::vec4  albedoTint;      // .rgb
        glm::vec4  emissiveTint;    // .rgb
        glm::vec4  scalars0;        // metallic, roughness, aoStrength, heightScale
        glm::vec4  scalars1;        // alphaCutoff, emissiveIntensity, normalStrength, _
        glm::uvec4 mapMask;         // x = bitmask indexado por TextureSlot
    };

    // PassParams (binding 4) del pase de sombra: corre ANTES de BeginScene, asi
    // que no puede leer el bloque Frame.
    struct alignas(16) ShadowPassBlock {
        glm::mat4 lightSpaceMatrix;
    };

    // PassParams (binding 4) de los pases de post y del prefiltrado IBL. Un solo
    // vec4 alcanza para todos; que significa cada componente lo dice el shader.
    struct alignas(16) PostParamsBlock {
        glm::vec4 params;
    };

    // ── Funciones puras que arman los bloques ──────────────────────────────
    // No tocan la GPU: son las que vuelven testeable headless lo que antes era
    // una tira de glUniform*.

    FrameBlock  MakeFrameBlock(const glm::mat4& view, const glm::mat4& projection,
                               const glm::vec3& viewPos,
                               const ShadowContext& shadow, const IblContext& ibl);

    // Recorta a Renderer::kMaxLights sin desbordar. Los slots sobrantes quedan en cero.
    LightsBlock MakeLightsBlock(const std::vector<PointLight>& lights,
                                const DirectionalLight& sun);

}
}
