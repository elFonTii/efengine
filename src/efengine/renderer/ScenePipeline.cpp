#include "efengine/renderer/ScenePipeline.h"

#include <efengine/renderer/FrameContext.h>
#include <efengine/renderer/GpuProfiler.h>

namespace efengine {
namespace renderer {

    IScenePass* ScenePipeline::Add(std::unique_ptr<IScenePass> pass) {
        if (!pass) return null;
        IScenePass* observador = pass.get();
        m_passes.push_back(std::move(pass));
        return observador;
    }

    void ScenePipeline::Execute(FrameContext* ctx) {
        for (const std::unique_ptr<IScenePass>& p : m_passes) {
            if (!p->enabled) continue;
            // El scope del profiler lo abre el pipeline y no cada pase: el
            // nombre ya viaja en la interfaz, y asi ningun pase nuevo se puede
            // olvidar de instrumentarse.
            EF_PROFILE_SCOPE(p->Name());
            p->Execute(*ctx);
        }
    }

    void ScenePipeline::Resize(u32 width, u32 height) {
        for (const std::unique_ptr<IScenePass>& p : m_passes) {
            p->Resize(width, height);
        }
    }

}
}
