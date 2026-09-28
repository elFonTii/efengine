#pragma once
#include <efengine/renderer/IScenePass.h>
#include <efengine/renderer/LightPacking.h>

namespace efengine {
namespace renderer {

    struct LightStats {
        u32 locals             = 0u;
        u32 visible            = 0u;
        u32 directional        = 0u;
        u32 droppedLocals      = 0u;
        u32 droppedDirectional = 0u;
    };

    // Empaqueta y sube las luces. Va PRIMERO en el frame: DdgiPass corre antes
    // de BeginScene y traza con ellas, asi que tienen que ser las de este frame.
    class LightUploadPass : public IScenePass {
        public:
            void Execute(FrameContext& ctx) override;

            const char* Name() const override { return "Luces"; }

            const LightStats& stats() const { return m_stats; }

        private:
            PackedLights m_packed;
            LightStats   m_stats;
            u32          m_avisadoLocales      = 0u;
            u32          m_avisadoDireccionales = 0u;
    };

}
}
