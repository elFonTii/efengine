#pragma once
#include <efengine/core/Types.h>
#include <efengine/renderer/ShadowMap.h>
#include <efengine/renderer/DirectionalLight.h>
#include <glm/glm.hpp>

namespace efengine {
namespace scene { class Scene; }
namespace renderer {

    class Renderer;
    class Shader;

    // Parámetros ajustables de la caja ortográfica y del bias (editables por ImGui).
    struct ShadowSettings {
        bool enabled       = true;
        f32  orthoHalfSize = 50.0f;
        f32  distance      = 100.0f;
        f32  nearPlane     = 1.0f;
        f32  farPlane      = 300.0f;
        f32  biasMin       = 0.0005f;
        f32  biasMax       = 0.0025f;
    };

    // Pre-pase: renderiza la profundidad de la escena desde la luz al ShadowMap.
    class ShadowPass {
        public:
            ShadowPass(Renderer& renderer, Shader* depthShader, u32 resolution = 2048);

            // Calcula la matriz light-space desde el sol + settings, renderiza la
            // profundidad y devuelve la matriz usada.
            const glm::mat4& Render(const scene::Scene& scene, const DirectionalLight& sun);

            const glm::mat4& lightSpaceMatrix() const { return m_lightSpaceMatrix; }
            const Texture&   DepthTexture()     const { return m_shadowMap.DepthTexture(); }
            ShadowSettings&  settings()               { return m_settings; }

        private:
            Renderer&      m_renderer;
            Shader*        m_shader;
            ShadowMap      m_shadowMap;
            ShadowSettings m_settings;
            glm::mat4      m_lightSpaceMatrix { 1.0f };
    };

}
}
