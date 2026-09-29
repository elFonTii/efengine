#pragma once
#include <efengine/core/Types.h>
#include <efengine/renderer/Framebuffer.h>
#include <efengine/renderer/IScenePass.h>
#include <efengine/renderer/ShaderBlocks.h>
#include <efengine/renderer/UniformBuffer.h>

#include <memory>

namespace efengine {
namespace renderer {

    class Renderer;
    class Shader;

    // Unico clear de depth del frame. Escribe la profundidad de sceneFB y el
    // depthNormal; el forward dibuja despues con GL_EQUAL. Corre antes de
    // FrameUploadPass: sube su propio bloque de pase (AoPrepassBlock).
    class DepthPrepass : public IScenePass {
        public:
            static std::unique_ptr<DepthPrepass> Create(Renderer& renderer, Shader* depthNormal,
                                                        u32 width, u32 height, Framebuffer& sceneFB);

            void Execute(FrameContext& ctx) override;
            void Resize(u32 width, u32 height) override;
            const char* Name() const override { return "Prepass"; }

            const Texture& depthNormal() const { return m_normalFb.ColorTexture(); }

        private:
            DepthPrepass(Renderer& renderer, Shader* shader, u32 width, u32 height, Framebuffer& sceneFB);

            Renderer&     m_renderer;
            Shader*       m_shader  = null;
            Framebuffer*  m_sceneFB = null;
            Framebuffer   m_normalFb;
            UniformBuffer m_ubo { sizeof(AoPrepassBlock) };
    };

}
}
