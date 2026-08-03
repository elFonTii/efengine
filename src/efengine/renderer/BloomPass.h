#pragma once
#include <efengine/renderer/IPostPass.h>
#include <efengine/renderer/Framebuffer.h>
#include <efengine/core/Types.h>
#include <efengine/renderer/ShaderBlocks.h>
#include <efengine/renderer/UniformBuffer.h>

namespace efengine {
namespace renderer {
    class Renderer;
    class VertexArray;
    class Shader;

    // Tuneados junto con la exposure de Camera y la intensidad de IBL de la
    // Cornell: los tres se leen en la misma imagen y moverlos por separado no
    // tiene sentido. El threshold bajo a 0.215 porque, con IBL casi apagado y un
    // solo rebote, casi nada de la escena pasaba de 1.0 y el bloom no existia.
    struct BloomSettings {
        f32 threshold   = 0.215f;
        f32 knee        = 0.190f;
        f32 intensity   = 0.33f;
        i32 iterations  = 6;
    };

    class BloomPass: public IPostPass {
        public:
            BloomPass(Renderer& renderer, VertexArray& fullscreenQuad,
                      Shader* brightpass, Shader* blur, Shader* composite,
                      u32 width, u32 height);

            BloomSettings& settings() { return m_settings; }

            void Apply(const Texture& input, const RenderTarget& target) override;
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
                // Un solo UBO para las tres etapas: se re-sube antes de cada draw.
                UniformBuffer m_paramsUbo { sizeof(PostParamsBlock) };
    };
}
}