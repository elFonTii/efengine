#include "efengine/renderer/IblPass.h"

#include <efengine/renderer/FrameContext.h>
#include <efengine/scene/SceneGraph.h>

namespace efengine {
namespace renderer {

    std::unique_ptr<IblPass> IblPass::Create(std::optional<Environment>&& env) {
        if (!env.has_value()) return null;
        // El ctor es privado, asi que make_unique no lo alcanza. Es el mismo
        // motivo por el que AoPass::Create usa un ctor privado con optional.
        return std::unique_ptr<IblPass>(new IblPass(std::move(*env)));
    }

    void IblPass::Execute(FrameContext& ctx) {
        // La intensidad es de la ESCENA y no del pase: viaja en el .efe.
        ctx.lighting.ibl.intensity   = ctx.scene.iblIntensity;

        ctx.lighting.ibl.environment = &m_env.env();
        ctx.lighting.ibl.irradiance  = &m_env.irradiance();
        ctx.lighting.ibl.prefiltered = &m_env.prefiltered();
        ctx.lighting.ibl.brdfLut     = &m_env.brdfLut();
        ctx.lighting.ibl.maxLod      = m_env.prefilterMaxLod();
    }

}
}
