#pragma once
#include <efengine/renderer/IPostPass.h>
#include <efengine/core/Types.h>

namespace efengine {
namespace renderer {
    class Renderer;
    class VertexArray;
    class Shader;

    class TonemapPass: public IPostPass {
        public:
            TonemapPass(Renderer& renderer, VertexArray& fullscreenQuad, Shader* tonemapShader);

            void SetExposure(f32 exposure);

            void Apply(const Texture& input, const Framebuffer* target) override;
            void Resize(u32 width, u32 height) override;

            private:
                Renderer&    m_renderer;
                VertexArray& m_quad;
                Shader*      m_shader;
                f32          m_exposure = 1.0f;
    };
}
}