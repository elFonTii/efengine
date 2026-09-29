#pragma once
#include <efengine/renderer/IScenePass.h>
#include <efengine/renderer/ShaderBlocks.h>
#include <efengine/renderer/UniformBuffer.h>

namespace efengine {
namespace renderer {
    class Renderer;
    class VertexArray;
    class Shader;

    // HDR lineal -> LDR sRGB con la exposicion de la camara. FXAA corre despues:
    // su estimador de contraste asume valores perceptuales.
    class TonemapPass : public IScenePass {
        public:
            TonemapPass(Renderer& renderer, VertexArray& fullscreenQuad, Shader* shader);
            void Execute(FrameContext& ctx) override;
            const char* Name() const override { return "Tonemap"; }

        private:
            Renderer&     m_renderer;
            VertexArray&  m_quad;
            Shader*       m_shader;
            UniformBuffer m_paramsUbo { sizeof(PostParamsBlock) };
    };
}
}
