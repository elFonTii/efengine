#pragma once

#include <efengine/core/Types.h>
#include <efengine/renderer/IScenePass.h>
#include <glm/glm.hpp>

namespace efengine {
namespace renderer {

    class Renderer;
    class Shader;
    class Cubemap;
    class VertexArray;

    class SkyboxPass : public IScenePass {
        public:
            SkyboxPass(Renderer& renderer, VertexArray& fullscreenQuad, Shader* skyboxShader);

            // Sin Environment no hay cielo: el pase no dibuja y el fondo queda
            // en el color de limpieza.
            //
            // Sin view/projection: uInvViewProjRot sale del bloque Frame, que
            // llena FrameUploadPass. Por eso va DESPUES de ese pase.
            void Execute(FrameContext& ctx) override;

            const char* Name() const override { return "Skybox"; }

            void Draw(const Cubemap& env) const;

        private:
            Renderer&    m_renderer;
            VertexArray& m_quad;
            Shader*      m_shader;
    };

}
}