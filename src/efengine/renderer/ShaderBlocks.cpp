#include "efengine/renderer/ShaderBlocks.h"
#include "efengine/renderer/Renderer.h"

#include <glm/gtc/matrix_inverse.hpp>
#include <algorithm>
#include <cmath>
#include <cstddef>

namespace efengine {
namespace renderer {

    // ── Contrato std140, verificado en tiempo de compilacion ───────────────
    // Si alguno de estos falla, el shader lee basura en silencio: la GPU no avisa
    // que el mirror C++ y el bloque GLSL dejaron de coincidir.
    static_assert(sizeof(CascadeBlock) == 304u, "CascadeBlock: tamano std140 roto");
    static_assert(offsetof(CascadeBlock, matrices)      ==   0u, "CascadeBlock.matrices");
    static_assert(offsetof(CascadeBlock, splitFar)      == 256u, "CascadeBlock.splitFar");
    static_assert(offsetof(CascadeBlock, normalOffsets) == 272u, "CascadeBlock.normalOffsets");
    static_assert(offsetof(CascadeBlock, params)        == 288u, "CascadeBlock.params");

    static_assert(sizeof(ClusterBlock) == 48u, "ClusterBlock: tamano std140 roto");
    static_assert(offsetof(ClusterBlock, dims)    ==  0u, "ClusterBlock.dims");
    static_assert(offsetof(ClusterBlock, zParams) == 16u, "ClusterBlock.zParams");
    static_assert(offsetof(ClusterBlock, screen)  == 32u, "ClusterBlock.screen");

    static_assert(kMaxCascades == 4u,
                  "CascadeBlock: los arrays son de 4; sincronizar con kMaxCascades y con el shader");

    static_assert(sizeof(FrameBlock) == 608u, "FrameBlock: tamano std140 roto");
    static_assert(offsetof(FrameBlock, view)                 ==   0u, "FrameBlock.view");
    static_assert(offsetof(FrameBlock, projection)           ==  64u, "FrameBlock.projection");
    static_assert(offsetof(FrameBlock, lightSpaceMatrix)     == 128u, "FrameBlock.lightSpaceMatrix");
    static_assert(offsetof(FrameBlock, invViewProjRot)       == 192u, "FrameBlock.invViewProjRot");
    static_assert(offsetof(FrameBlock, viewPos)              == 256u, "FrameBlock.viewPos");
    static_assert(offsetof(FrameBlock, shadowParams)         == 272u, "FrameBlock.shadowParams");
    static_assert(offsetof(FrameBlock, iblParams)            == 288u, "FrameBlock.iblParams");
    static_assert(offsetof(FrameBlock, invView)              == 304u, "FrameBlock.invView");
    static_assert(offsetof(FrameBlock, invProjection)        == 368u, "FrameBlock.invProjection");
    static_assert(offsetof(FrameBlock, viewProjNoJitter)     == 432u, "FrameBlock.viewProjNoJitter");
    static_assert(offsetof(FrameBlock, prevViewProjNoJitter) == 496u, "FrameBlock.prevViewProjNoJitter");
    static_assert(offsetof(FrameBlock, jitter)               == 560u, "FrameBlock.jitter");
    static_assert(offsetof(FrameBlock, screen)               == 576u, "FrameBlock.screen");
    static_assert(offsetof(FrameBlock, frameParams)          == 592u, "FrameBlock.frameParams");

    static_assert(sizeof(VoxelizePassBlock) == 96u, "VoxelizePassBlock: tamano std140 roto");
    static_assert(offsetof(VoxelizePassBlock, viewProj)   ==  0u, "VoxelizePassBlock.viewProj");
    static_assert(offsetof(VoxelizePassBlock, gridOrigin) == 64u, "VoxelizePassBlock.gridOrigin");
    static_assert(offsetof(VoxelizePassBlock, gridParams) == 80u, "VoxelizePassBlock.gridParams");

    static_assert(sizeof(DdgiUpdateBlock) == 144u, "DdgiUpdateBlock: tamano std140 roto");
    static_assert(offsetof(DdgiUpdateBlock, counts)      ==  64u, "DdgiUpdateBlock.counts");
    static_assert(offsetof(DdgiUpdateBlock, voxelParams) == 128u, "DdgiUpdateBlock.voxelParams");

    static_assert(sizeof(LightsBlock) == 144u, "LightsBlock: tamano std140 roto");
    static_assert(offsetof(LightsBlock, dirDirection) ==   0u, "LightsBlock.dirDirection");
    static_assert(offsetof(LightsBlock, dirColor)     ==  64u, "LightsBlock.dirColor");
    static_assert(offsetof(LightsBlock, counts)       == 128u, "LightsBlock.counts");

    static_assert(sizeof(GpuLight) == 80u, "GpuLight: tamano std430 roto");
    static_assert(offsetof(GpuLight, spotParams) == 48u, "GpuLight.spotParams");
    static_assert(offsetof(GpuLight, reserved)   == 64u, "GpuLight.reserved");
    static_assert(kMaxDirectionalLights == 4u, "LightsBlock: sincronizar con uDirDirection[4] de common/lights.glsl");

    static_assert(sizeof(ObjectBlock) == 128u, "ObjectBlock: tamano std140 roto");
    static_assert(offsetof(ObjectBlock, prevModel) == 64u, "ObjectBlock.prevModel");

    static_assert(sizeof(MaterialBlock) == 96u, "MaterialBlock: tamano std140 roto");
    static_assert(offsetof(MaterialBlock, albedoTint)   ==  0u, "MaterialBlock.albedoTint");
    static_assert(offsetof(MaterialBlock, emissiveTint) == 16u, "MaterialBlock.emissiveTint");
    static_assert(offsetof(MaterialBlock, scalars0)     == 32u, "MaterialBlock.scalars0");
    static_assert(offsetof(MaterialBlock, scalars1)     == 48u, "MaterialBlock.scalars1");
    static_assert(offsetof(MaterialBlock, mapMask)      == 64u, "MaterialBlock.mapMask");
    static_assert(offsetof(MaterialBlock, uvTransform)  == 80u, "MaterialBlock.uvTransform");

    static_assert(sizeof(ShadowPassBlock) == 64u, "ShadowPassBlock: tamano std140 roto");
    static_assert(sizeof(PostParamsBlock) == 16u, "PostParamsBlock: tamano std140 roto");

    static_assert(sizeof(DdgiBlock) == 128u, "DdgiBlock: tamano std140 inesperado");
    static_assert(offsetof(DdgiBlock, gridOrigin)  ==  0u, "DdgiBlock: offset de gridOrigin");
    static_assert(offsetof(DdgiBlock, gridSpacing) == 16u, "DdgiBlock: offset de gridSpacing");
    static_assert(offsetof(DdgiBlock, gridCounts)  == 32u, "DdgiBlock: offset de gridCounts");
    static_assert(offsetof(DdgiBlock, atlasLayout) == 48u, "DdgiBlock: offset de atlasLayout");
    static_assert(offsetof(DdgiBlock, updateRange) == 64u, "DdgiBlock: offset de updateRange");
    static_assert(offsetof(DdgiBlock, params0)     == 80u, "DdgiBlock: offset de params0");
    static_assert(offsetof(DdgiBlock, params1)     == 96u, "DdgiBlock: offset de params1");
    static_assert(offsetof(DdgiBlock, params2)     == 112u, "DdgiBlock: offset de params2");

    static_assert(sizeof(PrepassBlock) == 256u, "PrepassBlock: tamano std140 roto");
    static_assert(offsetof(PrepassBlock, projection)           ==  64u, "PrepassBlock.projection");
    static_assert(offsetof(PrepassBlock, viewProjNoJitter)     == 128u, "PrepassBlock.viewProjNoJitter");
    static_assert(offsetof(PrepassBlock, prevViewProjNoJitter) == 192u, "PrepassBlock.prevViewProjNoJitter");

    static_assert(sizeof(AoPassBlock) == 128u, "AoPassBlock: tamano std140 roto");
    static_assert(offsetof(AoPassBlock, viewToWorld) ==   0u, "AoPassBlock.viewToWorld");
    static_assert(offsetof(AoPassBlock, projInfo)    ==  64u, "AoPassBlock.projInfo");
    static_assert(offsetof(AoPassBlock, params0)     ==  80u, "AoPassBlock.params0");
    static_assert(offsetof(AoPassBlock, params1)     ==  96u, "AoPassBlock.params1");
    static_assert(offsetof(AoPassBlock, counts)      == 112u, "AoPassBlock.counts");
    static_assert(sizeof(TaaBlock) == 176u, "TaaBlock: tamano std140 roto");
    static_assert(offsetof(TaaBlock, prevViewProjNoJitter) ==  64u, "TaaBlock.prevViewProjNoJitter");
    static_assert(offsetof(TaaBlock, jitterUv)             == 128u, "TaaBlock.jitterUv");
    static_assert(offsetof(TaaBlock, screen)               == 144u, "TaaBlock.screen");
    static_assert(offsetof(TaaBlock, params)               == 160u, "TaaBlock.params");


    static_assert(sizeof(IndirectPassBlock) == 96u, "IndirectPassBlock: tamano std140 roto");
    static_assert(offsetof(IndirectPassBlock, viewToWorld) ==  0u, "IndirectPassBlock.viewToWorld");
    static_assert(offsetof(IndirectPassBlock, projInfo)    == 64u, "IndirectPassBlock.projInfo");
    static_assert(offsetof(IndirectPassBlock, counts)      == 80u, "IndirectPassBlock.counts");

    static_assert(sizeof(AoBlock) == 32u, "AoBlock: tamano std140 roto");
    static_assert(offsetof(AoBlock, params)   ==  0u, "AoBlock.params");
    static_assert(offsetof(AoBlock, upsample) == 16u, "AoBlock.upsample");

    FrameBlock MakeFrameBlock(const FrameView& v,
                              const ShadowContext& shadow, const IblContext& ibl) {
        FrameBlock b {};
        b.view             = v.view;
        b.projection       = v.projection;
        b.lightSpaceMatrix = shadow.lightSpaceMatrix;

        // Sin traslacion: el entorno se ve "infinitamente lejos". Con el jitter
        // de la geometria, para que el borde del cielo tiemble junto con ella.
        b.invViewProjRot = glm::inverse(v.projection * glm::mat4(glm::mat3(v.view)));

        b.viewPos      = glm::vec4(v.viewPos, 0.0f);
        b.shadowParams = glm::vec4(shadow.enabled ? 1.0f : 0.0f,
                                   shadow.biasMin, shadow.biasMax,
                                   shadow.normalOffset);

        // Los tres mapas o ninguno: con uno solo faltante el shader tiene que
        // apagar el ambiente entero en vez de muestrear una unidad equivocada.
        const bool hasIbl = (ibl.irradiance != null
                          && ibl.prefiltered != null
                          && ibl.brdfLut != null);
        b.iblParams = glm::vec4(hasIbl ? 1.0f : 0.0f, ibl.intensity, ibl.maxLod, 0.0f);

        b.invView              = v.invView;
        b.invProjection        = v.invProjection;
        b.viewProjNoJitter     = v.viewProjNoJitter;
        b.prevViewProjNoJitter = v.prevViewProjNoJitter;
        b.jitter = glm::vec4(v.jitterNdc, v.prevJitterNdc);

        const f32 w = static_cast<f32>(v.width);
        const f32 h = static_cast<f32>(v.height);
        b.screen = glm::vec4(w, h, w > 0.0f ? 1.0f / w : 0.0f, h > 0.0f ? 1.0f / h : 0.0f);
        b.frameParams = glm::uvec4(v.frameIndex, v.jitterEnabled ? 1u : 0u, 0u, 0u);

        return b;
    }

    DdgiBlock MakeDdgiBlock(const DdgiGrid& grid, const DdgiSettings& settings,
                            UpdateRange range, bool atlasValid) {
        // Se sanea aca y no en el caller: este es el ultimo punto antes de que
        // los valores lleguen al shader, donde un cero se vuelve NaN silencioso.
        const DdgiGrid g = SanitizeGrid(grid);

        const glm::ivec2 tiles = AtlasTileCount(g);
        const i32 total = static_cast<i32>(ProbeCount(g));

        DdgiBlock b {};
        b.gridOrigin  = glm::vec4(g.origin,  0.0f);
        b.gridSpacing = glm::vec4(g.spacing, 0.0f);
        b.gridCounts  = glm::ivec4(g.counts, total);
        b.atlasLayout = glm::ivec4(tiles.x, tiles.y,
                                   static_cast<i32>(kIrradianceTile),
                                   static_cast<i32>(kDistanceTile));
        b.updateRange = glm::ivec4(static_cast<i32>(range.first),
                                   static_cast<i32>(range.count),
                                   0,
                                   static_cast<i32>(settings.probeBudget));
        b.params0 = glm::vec4(0.0f, settings.intensity, settings.normalBias, settings.viewBias);
        // params1.z lleva el modo de debug de vista. Va aca y no en FrameBlock
        // por lo que explica el comentario de DdgiBlock en el header: extender
        // Frame obliga a tocar siete shaders que no lo usan.
        b.params1 = glm::vec4((settings.enabled && atlasValid) ? 1.0f : 0.0f,
                              settings.chebyshevSharpness,
                              static_cast<f32>(settings.debugView),
                              settings.classificationEnabled ? 1.0f : 0.0f);
        // params2.w codifica el ablation test en UN float: negativo = apagado,
        // >= 0 = el valor constante que pbr.frag devuelve en vez de samplear.
        // Dos campos (flag + valor) habrian obligado a crecer el bloque y a
        // tocar los cinco shaders que lo declaran para un instrumento de medida.
        b.params2 = glm::vec4(DistanceClamp(g),
                              settings.backfaceFadeStart,
                              // smoothstep con bordes iguales o invertidos no esta definido.
                              std::max(settings.backfaceFadeEnd, settings.backfaceFadeStart + 1.0e-3f),
                              settings.ablateSample
                                  ? std::max(settings.ablateIrradiance, 0.0f)
                                  : -1.0f);
        return b;
    }


    CascadeBlock MakeCascadeBlock(const CascadeContext& ctx) {
        CascadeBlock b {};
        const u32 count = ctx.enabled ? glm::min(ctx.count, kMaxCascades) : 0u;

        // Un corte inalcanzable en los slots que no se usan: si quedaran en cero,
        // la seleccion por profundidad mandaria TODO a la primera cascada muerta.
        f32 ultimo = 0.0f;
        for (u32 i = 0; i < kMaxCascades; ++i) {
            if (i < count) {
                b.matrices[i]      = ctx.fits[i].matrix;
                b.splitFar[i]      = ctx.fits[i].splitFar;
                b.normalOffsets[i] = ctx.fits[i].texelWorldSize * ctx.normalOffsetTexels;
                ultimo = ctx.fits[i].splitFar;
            } else {
                b.matrices[i]      = glm::mat4(1.0f);
                b.splitFar[i]      = ultimo;
                b.normalOffsets[i] = 0.0f;
            }
        }

        b.params = glm::vec4(static_cast<f32>(count), ctx.blendRatio,
                             ctx.debugView ? 1.0f : 0.0f, 0.0f);
        return b;
    }

    DdgiUpdateBlock MakeDdgiUpdateBlock(const DdgiSettings& s, const VoxelGridDesc& voxel,
                                        u32 frameIndex) {
        DdgiUpdateBlock b {};
        b.rayRotation = glm::mat4(RandomRayRotation(frameIndex));
        b.counts = glm::uvec4(std::clamp(s.raysPerProbe, kMinRaysPerProbe, kMaxRaysPerProbe),
                              std::min(s.probeBudget, kMaxProbesPerFrame),
                              std::max(s.inactiveRecheckSweeps, 1u),
                              s.classificationEnabled ? 1u : 0u);
        b.hysteresis = glm::vec4(std::clamp(s.hysteresis, 0.0f, 0.995f),
                                 std::max(s.irradianceThreshold, 0.0f),
                                 std::max(s.brightnessThreshold, 0.0f),
                                 DistanceClamp(s.grid));
        // Mismo arreglo que MakeDdgiBlock: smoothstep con bordes invertidos no esta definido,
        // y schedule.comp usa este borde para decidir quien esta activa.
        b.relocation = glm::vec4(std::max(s.minFrontfaceDistance, 0.0f),
                                 s.relocationEnabled ? 1.0f : 0.0f,
                                 std::max(voxel.voxelSize, 0.0f),
                                 std::max(s.backfaceFadeEnd, s.backfaceFadeStart + 1.0e-3f));
        b.voxelGrid   = glm::vec4(voxel.origin, static_cast<f32>(voxel.resolution));
        b.voxelParams = glm::vec4(voxel.voxelSize,
                                  std::clamp(s.opacityThreshold, 1.0e-3f, 0.7f), 0.0f, 0.0f);
        return b;
    }

    ClusterBlock MakeClusterBlock(const ClusterGrid& g, u32 debugView) {
        ClusterBlock b {};
        b.dims    = glm::uvec4(g.tilesX, g.tilesY, g.slices, g.maxLightsPerCluster);
        b.zParams = glm::vec4(g.nearSplit, g.farLimit,
                              static_cast<f32>(g.slices - 1u) / std::log(g.farLimit / g.nearSplit),
                              std::log(g.nearSplit));
        b.screen  = glm::vec4(static_cast<f32>(g.tileSizePx), static_cast<f32>(debugView), 0.0f, 0.0f);
        return b;
    }

}
}
