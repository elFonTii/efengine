#pragma once

#include <efengine/core/Types.h>
#include <efengine/renderer/ShadowContext.h>
#include <efengine/renderer/IblContext.h>
#include <efengine/renderer/DdgiSettings.h>
#include <efengine/renderer/CascadeContext.h>
#include <efengine/renderer/VoxelMath.h>
#include <efengine/renderer/ClusterMath.h>
#include <efengine/renderer/FrameView.h>

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
    inline constexpr u32 kCascadeBinding  = 7u;   // 1x por frame — solo lo declara pbr.frag
    inline constexpr u32 kClusterBinding  = 8u;   // 1x por frame — solo lo declaran pbr.frag y el cull
    inline constexpr u32 kDdgiUpdateBinding = 9u; // 1x por frame — solo los shaders que actualizan probes

    // Binding de SSBO (espacio aparte de los de UBO): datos por probe de DDGI.
    inline constexpr u32 kProbeDataBinding = 0u;
    // Estado por probe de DDGI (edad, activa/inactiva). Solo los pases de actualizacion.
    inline constexpr u32 kProbeStateBinding = 5u;
    // Planificacion de DDGI: cursor, barrido, argumentos indirectos y la lista de probes
    // del frame. Espeja DdgiSchedule de ddgi/update.glsl (std430).
    inline constexpr u32   kScheduleBinding         = 6u;
    inline constexpr usize kScheduleTraceArgsOffset = 16u;
    inline constexpr usize kScheduleProbeArgsOffset = 32u;
    inline constexpr usize kScheduleListOffset      = 48u;
    inline constexpr usize kScheduleBytes = kScheduleListOffset + sizeof(u32) * kMaxProbesPerFrame;

    // Stats de DDGI que vuelven a la CPU con GpuReadback. Espeja DdgiStats de
    // ddgi/update.glsl (std430). deltaSum es punto fijo: Delta * kDdgiDeltaScale.
    inline constexpr u32 kDdgiStatsBinding = 7u;
    inline constexpr f32 kDdgiDeltaScale   = 1.0e4f;

    struct DdgiGpuStats {
        u32 active;
        u32 inactive;
        u32 sweep;
        u32 cursor;
        u32 listCount;
        u32 deltaSum;
        u32 deltaCount;
        u32 pad;
    };
    static_assert(sizeof(DdgiGpuStats) == 32u, "DdgiGpuStats tiene que espejar DdgiStats (std430)");

    // SSBO de las luces. Espacio de indices aparte de los UBO.
    inline constexpr u32 kLocalLightsBinding   = 1u;   // GpuLight[]
    inline constexpr u32 kVisibleLightsBinding = 2u;   // indices de las que tocan el frustum
    inline constexpr u32 kClusterLightsBinding = 3u;   // por cluster: [count, indices...]
    inline constexpr u32 kClusterAabbsBinding  = 4u;   // por cluster: min, max en vista

    inline constexpr u32 kMaxLocalLights       = 4096u;
    inline constexpr u32 kMaxDirectionalLights = 4u;

    // Unidades de sampler de los atlas de DDGI. 0-7 material, 8 sombra, 9/10/11 IBL.
    inline constexpr u32 kIrradianceAtlasUnit = 12u;
    inline constexpr u32 kDistanceAtlasUnit   = 13u;

    // Unidad del AO screen-space, atrás de los dos atlas de DDGI.
    inline constexpr u32 kAoTextureUnit       = 14u;

    // Las dos texturas que pbr.frag necesita para el upsample bilateral de las
    // señales que se resuelven a media resolucion (la indirecta de DDGI y el
    // propio AO). La guia es el prepass del AO A RESOLUCION COMPLETA: sin ella
    // los pesos no tienen contra que comparar y el upsample vuelve a ser un
    // bilineal con halos en las siluetas.
    inline constexpr u32 kIndirectTextureUnit = 15u;
    inline constexpr u32 kDepthNormalUnit     = 16u;

    // ── Mirrors C++ de los bloques std140 ──────────────────────────────────
    // Regla de std140 que gobierna todo esto: un vec3 ocupa igual 16 bytes, y un
    // array de vec3 paddea CADA elemento a 16. Promover todo a vec4 evita pelear
    // con eso y hace que el layout natural de C++ coincida byte a byte.
    // Los static_assert de sizeof/offsetof estan en ShaderBlocks.cpp.

    // Lo que no cambia en todo el frame. Lo sube Renderer::BeginScene.
    struct alignas(16) FrameBlock {
        glm::mat4  view;
        glm::mat4  projection;          // con jitter
        glm::mat4  lightSpaceMatrix;
        glm::mat4  invViewProjRot;      // para el skybox: inverse(proj * mat3(view))
        glm::vec4  viewPos;             // .xyz
        glm::vec4  shadowParams;        // x=enabled, y=biasMin, z=biasMax, w=normalOffset (m)
        glm::vec4  iblParams;           // x=hasIbl, y=intensity, z=prefilterMaxLod
        glm::mat4  invView;
        glm::mat4  invProjection;
        glm::mat4  viewProjNoJitter;
        glm::mat4  prevViewProjNoJitter;
        glm::vec4  jitter;              // xy = jitter NDC, zw = el del frame anterior
        glm::vec4  screen;              // x = ancho, y = alto, z = 1/ancho, w = 1/alto
        glm::uvec4 frameParams;         // x = frameIndex, y = jitter prendido
    };

    struct alignas(16) LightsBlock {
        glm::vec4  dirDirection[kMaxDirectionalLights];   // .xyz — hacia donde VIAJA la luz
        glm::vec4  dirColor[kMaxDirectionalLights];       // .rgb efectivo, .w = 1 si es el PrimarySun
        glm::uvec4 counts;                                // x = locales, y = direccionales, z = visibles
    };

    // std430: un elemento de LocalLights. 'reserved' es del ciclo 2 (sombras):
    // esta desde ya para que el struct no cambie de tamano.
    struct alignas(16) GpuLight {
        glm::vec4 positionRange;   // .xyz mundo, .w rango
        glm::vec4 colorRadius;     // .rgb efectivo, .w radio de la fuente
        glm::vec4 directionType;   // .xyz direccion del spot, .w tipo (0 point, 1 spot)
        glm::vec4 spotParams;      // x = scale, y = offset, z = cos exterior, w = sin exterior
        glm::vec4 reserved;        // x = shadowIndex (-1), y = flags (bit 0 castShadows)
    };

    // prevModel solo lo lee el prepass (velocidades); el resto sube prev == model.
    struct alignas(16) ObjectBlock {
        glm::mat4 model;
        glm::mat4 prevModel;
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
    // declaran (skybox, tonemap, shadow) sin que ninguno use el dato.
    struct alignas(16) DdgiBlock {
        glm::vec4  gridOrigin;    // .xyz
        glm::vec4  gridSpacing;   // .xyz
        glm::ivec4 gridCounts;    // .xyz = probes por eje, .w = total
        glm::ivec4 atlasLayout;   // x=cols, y=rows, z=irrTile(8), w=distTile(16)
        glm::ivec4 updateRange;   // reservado
        glm::vec4  params0;       // reservado, intensity, normalBias, viewBias
        glm::vec4  params1;       // enabled, chebyshevSharpness, debugView, classificationEnabled
        glm::vec4  params2;       // distanceClamp, backfaceFadeStart, backfaceFadeEnd, ablation
    };

    // PassParams (binding 4) del DepthPrepass. Corre ANTES de BeginScene, asi
    // que no puede leer el bloque Frame.
    struct alignas(16) PrepassBlock {
        glm::mat4 view;
        glm::mat4 projection;             // con jitter: la que rasteriza
        glm::mat4 viewProjNoJitter;       // velocidades
        glm::mat4 prevViewProjNoJitter;
    };

    // PassParams (binding 4) de gtao.frag y denoise.frag.
    struct alignas(16) AoPassBlock {
        glm::mat4  viewToWorld;   // inverse(view): emite el bent normal en world space
        glm::vec4  projInfo;      // xy = reconstruccion view-space, zw = 1/resolucion
        glm::vec4  params0;       // radius (m), thickness, intensity, maxScreenRadius (px)
        glm::vec4  params1;       // projScale (px por metro a 1 m), escala vs full, frame del ruido, _
        glm::ivec4 counts;        // x=slices, y=steps, z=direccion del blur (0=H,1=V), w=debugView
    };

    // PassParams (binding 4) de taa.frag.
    struct alignas(16) TaaBlock {
        glm::mat4 invViewProjNoJitter;
        glm::mat4 prevViewProjNoJitter;
        glm::vec4 jitterUv;   // xy: el jitter de este frame en UV
        glm::vec4 screen;     // ancho, alto, 1/ancho, 1/alto
        glm::vec4 params;     // x = alfa efectivo, y = ver velocidades
    };

    // PassParams (binding 4) de ddgi/indirect.frag: el pase que resuelve la
    // indirecta difusa a media resolucion.
    //
    // Corre DESPUES de BeginScene, asi que la view/proj y la posicion de camara
    // le llegan por el bloque Frame y no se repiten aca. Lo unico que Frame no
    // trae es la inversa de la view (invertir una mat4 por pixel en el shader
    // seria absurdo) y los tamanos, que dependen del target y no de la camara.
    struct alignas(16) IndirectPassBlock {
        glm::mat4  viewToWorld;   // inverse(view)
        glm::vec4  projInfo;      // xy = reconstruccion view-space, zw = 1/resolucion del target
        glm::ivec4 counts;        // x=escala vs full, y=usar bent normal, zw=resolucion full
    };

    // Constantes de AO del frame (binding 6). Bloque propio y no campos nuevos
    // en FrameBlock por el mismo motivo que DdgiBlock: extender Frame obliga a
    // tocar los cuatro shaders que lo declaran sin que ninguno use el dato.
    struct alignas(16) AoBlock {
        glm::vec4 params;   // enabled, bentNormal, multiBounce, debugView

        // Como pbr.frag reconstruye las señales que se resuelven a resolucion
        // reducida. Va en ESTE bloque y no en uno nuevo porque el binding 6 ya
        // es "lo que el forward necesita de los pases screen-space", y un bloque
        // mas por dos flags seria un binding entero para 8 bytes utiles.
        //
        //   x = la indirecta esta en la unidad 15 -> upsample en vez de
        //       samplear el volumen inline
        //   y = el AO esta a resolucion reducida -> upsample tambien
        //   z = escala: cuantos texels de resolucion completa cubre uno del
        //       target reducido por eje. La comparten los dos (ReducedRes.h).
        //   w = libre
        //
        // x e y son INDEPENDIENTES: el AO puede estar a resolucion reducida con
        // la indirecta apagada, y ahi hay que subir uno y no el otro.
        glm::vec4 upsample;
    };

    // Las 4 matrices y los 4 escalares por cascada. Los escalares van empaquetados
    // en vec4 y no en arrays de float porque std140 le da 16 bytes a cada elemento
    // de un array de escalares: seria 4x el espacio y un layout que hay que
    // recordar en vez de leer.
    struct alignas(16) CascadeBlock {
        glm::mat4 matrices[4];      // 4 == kMaxCascades
        glm::vec4 splitFar;         // corte lejano de cada cascada, en distancia de vista
        glm::vec4 normalOffsets;    // normal offset de cada cascada, en METROS
        glm::vec4 params;           // x=count (0 = apagado), y=blendRatio, z=debugView
    };

    // Constantes de la grilla de clusters (binding 8). En cero apaga la grilla:
    // pbr.frag recorre todas las visibles. Lo sube BeginScene en cero cada frame
    // y ClusterLightPass lo pisa, asi un pase que falta no deja una grilla vieja.
    struct alignas(16) ClusterBlock {
        glm::uvec4 dims;      // x = tilesX, y = tilesY, z = cortes, w = max por cluster
        glm::vec4  zParams;   // x = nearSplit, y = farLimit, z = (cortes-1)/ln(farLimit/nearSplit), w = ln(nearSplit)
        glm::vec4  screen;    // x = tile en px, y = vista de debug
    };

    // PassParams (binding 4) de voxel/voxelize.*. El motor no tiene uniforms
    // sueltos (ver Shader.h), asi que la ortografica del eje y el encuadre del
    // grid viajan por el bloque de pase como cualquier otro dato.
    struct alignas(16) VoxelizePassBlock {
        glm::mat4 viewProj;
        glm::vec4 gridOrigin;   // .xyz = esquina minima
        glm::vec4 gridParams;   // x = voxelSize (m), y = resolucion por eje
    };

    // DdgiUpdate (binding 9) de ddgi/update.glsl. La rotacion va como mat4 para no pelear con
    // el padding de mat3 en std140; el shader usa mat3(uRayRotation).
    struct alignas(16) DdgiUpdateBlock {
        glm::mat4  rayRotation;
        glm::uvec4 counts;        // x = rayos por probe, y = presupuesto, z = K, w = clasificacion
        glm::vec4  hysteresis;    // x = hMax, y = umbral de irradiancia, z = umbral de brillo, w = distanceClamp
        glm::vec4  relocation;    // x = minFrontfaceDistance, y = reubicacion, z = voxelSize, w = backfaceFadeEnd
        glm::vec4  voxelGrid;     // xyz = esquina minima del grid de voxeles, w = resolucion
        glm::vec4  voxelParams;   // x = voxelSize, y = umbral de opacidad
    };

    CascadeBlock MakeCascadeBlock(const CascadeContext& ctx);
    ClusterBlock MakeClusterBlock(const ClusterGrid& grid, u32 debugView);

    // ── Funciones puras que arman los bloques ──────────────────────────────
    // No tocan la GPU: son las que vuelven testeable headless lo que antes era
    // una tira de glUniform*.

    FrameBlock  MakeFrameBlock(const FrameView& view,
                               const ShadowContext& shadow, const IblContext& ibl);

    // params2.x es DistanceClamp(grid): el techo de las distancias del atlas de Chebyshev, no el largo del rayo.
    //
    // atlasValid apaga params1.x aunque settings.enabled este en true: es el caso
    // "no hay DdgiPass" (fallo de shader), donde pbr.frag tiene que caer a IBL
    // puro en vez de samplear una unidad de textura sin contenido.
    DdgiBlock MakeDdgiBlock(const DdgiGrid& grid, const DdgiSettings& settings,
                            bool atlasValid);

    DdgiUpdateBlock MakeDdgiUpdateBlock(const DdgiSettings& settings, const VoxelGridDesc& voxel,
                                        u32 frameIndex);

}
}
