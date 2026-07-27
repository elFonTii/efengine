#include "efengine/renderer/ShaderBlocks.h"
#include "efengine/renderer/Renderer.h"

#include <glm/gtc/matrix_inverse.hpp>
#include <cstddef>

namespace efengine {
namespace renderer {

    // ── Contrato std140, verificado en tiempo de compilacion ───────────────
    // Si alguno de estos falla, el shader lee basura en silencio: la GPU no avisa
    // que el mirror C++ y el bloque GLSL dejaron de coincidir.
    static_assert(sizeof(FrameBlock) == 304u, "FrameBlock: tamano std140 roto");
    static_assert(offsetof(FrameBlock, view)             ==   0u, "FrameBlock.view");
    static_assert(offsetof(FrameBlock, projection)       ==  64u, "FrameBlock.projection");
    static_assert(offsetof(FrameBlock, lightSpaceMatrix) == 128u, "FrameBlock.lightSpaceMatrix");
    static_assert(offsetof(FrameBlock, invViewProjRot)   == 192u, "FrameBlock.invViewProjRot");
    static_assert(offsetof(FrameBlock, viewPos)          == 256u, "FrameBlock.viewPos");
    static_assert(offsetof(FrameBlock, shadowParams)     == 272u, "FrameBlock.shadowParams");
    static_assert(offsetof(FrameBlock, iblParams)        == 288u, "FrameBlock.iblParams");

    static_assert(sizeof(LightsBlock) == 176u, "LightsBlock: tamano std140 roto");
    static_assert(offsetof(LightsBlock, positions)    ==   0u, "LightsBlock.positions");
    static_assert(offsetof(LightsBlock, colors)       ==  64u, "LightsBlock.colors");
    static_assert(offsetof(LightsBlock, dirDirection) == 128u, "LightsBlock.dirDirection");
    static_assert(offsetof(LightsBlock, dirColor)     == 144u, "LightsBlock.dirColor");
    static_assert(offsetof(LightsBlock, counts)       == 160u, "LightsBlock.counts");

    static_assert(sizeof(ObjectBlock) == 64u, "ObjectBlock: tamano std140 roto");

    static_assert(sizeof(MaterialBlock) == 80u, "MaterialBlock: tamano std140 roto");
    static_assert(offsetof(MaterialBlock, albedoTint)   ==  0u, "MaterialBlock.albedoTint");
    static_assert(offsetof(MaterialBlock, emissiveTint) == 16u, "MaterialBlock.emissiveTint");
    static_assert(offsetof(MaterialBlock, scalars0)     == 32u, "MaterialBlock.scalars0");
    static_assert(offsetof(MaterialBlock, scalars1)     == 48u, "MaterialBlock.scalars1");
    static_assert(offsetof(MaterialBlock, mapMask)      == 64u, "MaterialBlock.mapMask");

    static_assert(sizeof(ShadowPassBlock) == 64u, "ShadowPassBlock: tamano std140 roto");
    static_assert(sizeof(PostParamsBlock) == 16u, "PostParamsBlock: tamano std140 roto");

    // El array del bloque tiene que tener exactamente los slots que el motor cree.
    static_assert(Renderer::kMaxLights == 4u,
                  "LightsBlock: los arrays son de 4; sincronizar con kMaxLights y con MAX_LIGHTS del shader");

    FrameBlock MakeFrameBlock(const glm::mat4& view, const glm::mat4& projection,
                              const glm::vec3& viewPos,
                              const ShadowContext& shadow, const IblContext& ibl) {
        FrameBlock b {};
        b.view             = view;
        b.projection       = projection;
        b.lightSpaceMatrix = shadow.lightSpaceMatrix;

        // Sin traslacion: el entorno se ve "infinitamente lejos". Antes lo
        // calculaba SkyboxPass; vive aca para que el skybox no tenga uniforms propios.
        b.invViewProjRot = glm::inverse(projection * glm::mat4(glm::mat3(view)));

        b.viewPos      = glm::vec4(viewPos, 0.0f);
        b.shadowParams = glm::vec4(shadow.enabled ? 1.0f : 0.0f,
                                   shadow.biasMin, shadow.biasMax, 0.0f);

        // Los tres mapas o ninguno: con uno solo faltante el shader tiene que
        // apagar el ambiente entero en vez de muestrear una unidad equivocada.
        const bool hasIbl = (ibl.irradiance != null
                          && ibl.prefiltered != null
                          && ibl.brdfLut != null);
        b.iblParams = glm::vec4(hasIbl ? 1.0f : 0.0f, ibl.intensity, ibl.maxLod, 0.0f);

        return b;
    }

    LightsBlock MakeLightsBlock(const std::vector<PointLight>& lights,
                                const DirectionalLight& sun) {
        LightsBlock b {};

        const usize count = (lights.size() < Renderer::kMaxLights)
                          ? lights.size()
                          : static_cast<usize>(Renderer::kMaxLights);

        for (usize i = 0u; i < count; ++i) {
            b.positions[i] = glm::vec4(lights[i].position, 0.0f);
            b.colors[i]    = glm::vec4(lights[i].color, 0.0f);
        }

        b.dirDirection = glm::vec4(sun.direction, 0.0f);
        b.dirColor     = glm::vec4(sun.color, 0.0f);
        b.counts       = glm::ivec4(static_cast<i32>(count), 0, 0, 0);

        return b;
    }

}
}
