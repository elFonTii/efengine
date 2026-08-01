#pragma once

#include <efengine/core/Types.h>
#include <efengine/renderer/PointLight.h>
#include <efengine/renderer/DirectionalLight.h>
#include <efengine/renderer/ShadowContext.h>
#include <efengine/renderer/IblContext.h>
#include <efengine/renderer/DdgiSettings.h>

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
    inline constexpr u32 kDdgiBinding     = 5u;   // 1x por frame — solo lo declaran los shaders de DDGI
    inline constexpr u32 kAoBinding       = 6u;   // 1x por frame — solo lo declara pbr.frag

    // Unidades de sampler de los atlas de DDGI. 0-7 material, 8 sombra, 9/10/11 IBL.
    inline constexpr u32 kIrradianceAtlasUnit = 12u;
    inline constexpr u32 kDistanceAtlasUnit   = 13u;

    // Unidad del AO screen-space, atrás de los dos atlas de DDGI.
    inline constexpr u32 kAoTextureUnit       = 14u;

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
        glm::vec4 shadowParams;     // x=enabled, y=biasMin, z=biasMax, w=normalOffset (m)
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
        // Transformacion de la UV comun a los 8 mapas: uv = vUV * xy + zw.
        // Va al final y no en un hueco porque no hay hueco: lo unico libre era
        // scalars1.w, un solo float, y hacen falta cuatro.
        glm::vec4  uvTransform;     // xy = tiling (repeticiones), zw = offset
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

    // Constantes de DDGI del frame (binding 5). Bloque propio en vez de extender
    // FrameBlock: extender Frame obliga a tocar los cuatro shaders que lo
    // declaran (skybox, tonemap, fxaa, shadow) sin que ninguno use el dato.
    struct alignas(16) DdgiBlock {
        glm::vec4  gridOrigin;    // .xyz
        glm::vec4  gridSpacing;   // .xyz
        glm::ivec4 gridCounts;    // .xyz = probes por eje, .w = total
        glm::ivec4 atlasLayout;   // x=cols, y=rows, z=irrTile(8), w=distTile(16)
        glm::ivec4 updateRange;   // x=firstProbe, y=count, z=faceSize, w=probesPerFrame
        glm::vec4  params0;       // hysteresis, intensity, normalBias, viewBias
        glm::vec4  params1;       // enabled, chebyshevSharpness, _, _
        glm::vec4  params2;       // maxDistance, backfaceFadeStart, backfaceFadeEnd, _
    };

    // PassParams (binding 4) del prepass de profundidad+normales del AO. Corre
    // ANTES de BeginScene, asi que no puede leer el bloque Frame — la misma
    // restriccion y la misma solucion que ShadowPassBlock.
    struct alignas(16) AoPrepassBlock {
        glm::mat4 view;
        glm::mat4 projection;
    };

    // PassParams (binding 4) de gtao.frag y denoise.frag.
    struct alignas(16) AoPassBlock {
        glm::mat4  viewToWorld;   // inverse(view): emite el bent normal en world space
        glm::vec4  projInfo;      // xy = reconstruccion view-space, zw = 1/resolucion
        glm::vec4  params0;       // radius (m), thickness, intensity, maxScreenRadius (px)
        glm::vec4  params1;       // projScale (px por metro a 1 m), _, _, _
        glm::ivec4 counts;        // x=slices, y=steps, z=direccion del blur (0=H,1=V), w=debugView
    };

    // Constantes de AO del frame (binding 6). Bloque propio y no campos nuevos
    // en FrameBlock por el mismo motivo que DdgiBlock: extender Frame obliga a
    // tocar los cuatro shaders que lo declaran sin que ninguno use el dato.
    struct alignas(16) AoBlock {
        glm::vec4 params;   // enabled, bentNormal, multiBounce, debugView
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

    // maxDistance SI viaja en el bloque (params2.x). Ademas del far plane de la
    // captura, es el techo con el que blend_distance.comp recorta las distancias
    // y con el que debug_probe.frag las normaliza: antes cada uno tenia su propia
    // constante 4*spacing y el slider del panel no movia ninguna de las dos.
    //
    // atlasValid apaga params1.x aunque settings.enabled este en true: es el caso
    // "no hay DdgiPass" (fallo de shader), donde pbr.frag tiene que caer a IBL
    // puro en vez de samplear una unidad de textura sin contenido.
    DdgiBlock MakeDdgiBlock(const DdgiGrid& grid, const DdgiSettings& settings,
                            UpdateRange range, bool atlasValid);

}
}
