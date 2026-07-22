#pragma once
#include <efengine/renderer/IPostPass.h>
#include <efengine/renderer/Framebuffer.h>
#include <efengine/core/Types.h>

namespace efengine {
namespace renderer {
    class Renderer;
    class VertexArray;
    class Shader;

    struct BloomSettings {
        f32 threshold   = 1.0f;
        f32 knee        = 0.5f;
        f32 intensity   = 0.08f;
        i32 iterations  = 5;
    };

    class BloomPass: public IPostPass {
        public:
            BloomPass(Renderer& renderer, VertexArray& fullscreenQuad,
                      Shader* brightpass, Shader* blur, Shader* composite,
                      u32 width, u32 height);

            BloomSettings& settings() { return m_settings; }

            void Apply(const Texture& input, const Framebuffer* target) override;
            void Resize(u32 width, u32 height) override;

            private:
                Renderer&    m_renderer;
                VertexArray& m_quad;
                Shader*      m_brightpass;
                Shader*      m_blur;
                Shader*      m_composite;
                Framebuffer  m_fboA;
                Framebuffer  m_fboB;
                BloomSettings m_settings;
    };
}
}