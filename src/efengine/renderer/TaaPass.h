#pragma once
#include <efengine/core/Types.h>
#include <efengine/renderer/Framebuffer.h>
#include <efengine/renderer/IScenePass.h>
#include <efengine/renderer/ShaderBlocks.h>
#include <efengine/renderer/TaaSettings.h>
#include <efengine/renderer/UniformBuffer.h>

namespace efengine {
namespace renderer {

    class Renderer;
    class VertexArray;
    class Shader;

    // Resolve temporal en HDR. La historia es propia y no de PostTargets: el
    // bloom reescribe esos targets dentro del mismo frame.
    class TaaPass : public IScenePass {
        public:
            TaaPass(Renderer& renderer, VertexArray& fullscreenQuad, Shader* shader, u32 width, u32 height);

            void Execute(FrameContext& ctx) override;
            void Resize(u32 width, u32 height) override;
            const char* Name() const override { return "TAA"; }

            TaaSettings& settings()     { return m_settings; }
            void         ResetHistory() { m_resetPending = true; }

        private:
            Renderer&     m_renderer;
            VertexArray&  m_quad;
            Shader*       m_shader;
            Framebuffer   m_history[2];
            u32           m_write         = 0u;
            bool          m_resetPending  = true;   // la historia recien creada no tiene nada
            bool          m_warnedMissing = false;
            TaaSettings   m_settings;
            UniformBuffer m_ubo { sizeof(TaaBlock) };
    };

}
}
