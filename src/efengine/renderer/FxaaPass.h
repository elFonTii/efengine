#pragma once
#include <efengine/renderer/IPostPass.h>
#include <efengine/core/Types.h>
#include <efengine/renderer/ShaderBlocks.h>
#include <efengine/renderer/UniformBuffer.h>

namespace efengine {
namespace renderer {
    class Renderer;
    class VertexArray;
    class Shader;

    struct FxaaSettings {
        bool enabled = true;
    };

    class FxaaPass: public IPostPass {
        public:
            FxaaPass(Renderer& renderer, VertexArray& fullscreenQuad, Shader* fxaaShader);

            FxaaSettings& settings() { return m_settings; }

            void Apply(const Texture& input, const RenderTarget& target) override;
            void Resize(u32 width, u32 height) override;

            private:
                Renderer&    m_renderer;
                VertexArray& m_quad;
                Shader*      m_shader;
                FxaaSettings m_settings;
                UniformBuffer m_paramsUbo { sizeof(PostParamsBlock) };
    };
}
}
