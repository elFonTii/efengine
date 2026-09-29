#pragma once
#include <efengine/renderer/IScenePass.h>
#include <efengine/core/Types.h>
#include <efengine/renderer/ShaderBlocks.h>
#include <efengine/renderer/UniformBuffer.h>

namespace efengine {
namespace renderer {
    class Renderer;
    class VertexArray;
    class Shader;

    class FxaaPass: public IScenePass {
        public:
            FxaaPass(Renderer& renderer, VertexArray& fullscreenQuad, Shader* fxaaShader);

            void Execute(FrameContext& ctx) override;
            const char* Name() const override { return "FXAA"; }

            private:
                Renderer&    m_renderer;
                VertexArray& m_quad;
                Shader*      m_shader;
                UniformBuffer m_paramsUbo { sizeof(PostParamsBlock) };
    };
}
}
