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

            void Draw(const Cubemap& env, const glm::mat4& view, const glm::mat4& projection) const;

        private:
            Renderer&    m_renderer;
            VertexArray& m_quad;
            Shader*      m_shader;
    };

}
}