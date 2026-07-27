#pragma once

#include <efengine/core/Types.h>
#include <glm/glm.hpp>

namespace efengine {
namespace renderer {

    class Renderer;
    class Shader;
    class Cubemap;
    class VertexArray;

    class SkyboxPass {
        public:
            SkyboxPass(Renderer& renderer, VertexArray& fullscreenQuad, Shader* skyboxShader);

            // Sin view/projection: uInvViewProjRot sale del bloque Frame, que
            // llena Renderer::BeginScene. Llamar DESPUES de BeginScene.
            void Draw(const Cubemap& env) const;

        private:
            Renderer&    m_renderer;
            VertexArray& m_quad;
            Shader*      m_shader;
    };

}
}