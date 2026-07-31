#pragma once
#include <efengine/core/Types.h>
#include <efengine/renderer/ShadowMap.h>
#include <efengine/renderer/DirectionalLight.h>
#include <efengine/renderer/ShaderBlocks.h>
#include <efengine/renderer/UniformBuffer.h>
#include <glm/glm.hpp>

namespace efengine {
namespace scene { class SceneGraph; }
namespace renderer {

    class Renderer;
    class Shader;

    // Parámetros ajustables de la caja ortográfica y del bias (editables por ImGui).
    // Tuneados contra la caja de Cornell de TestScene (interior 8 x 4 x 8 m), que
    // es la escena de arranque. Los valores viejos venian de la sala de 200 m de
    // sandbox.efe y encuadraban un ortho ~9x mas grande del necesario.
    struct ShadowSettings {
        bool enabled       = true;
        f32  orthoHalfSize = 11.5f;    // encuadra la sala justa, no 100 m de vacio
        f32  distance      = 265.0f;
        f32  nearPlane     = 0.5f;
        f32  farPlane      = 450.0f;

        // Los dos bias van casi en cero, y eso DEPENDE del orthoHalfSize de
        // arriba: achicar el encuadre de 50 a 11.5 hace el texel del shadow map
        // ~4x mas chico, y el bias necesario para tapar el acne escala con el
        // texel. Con el ortho viejo, 0.0012 daba acne; con este, no.
        //
        // Bajarlos no es cosmetico: el bias despega la sombra del punto de
        // contacto y deja una franja iluminada contra el zocalo de las paredes y
        // el pie de los bloques (peter-panning). Esa franja falsa es justo la que
        // ensucia la lectura de la GI en los rincones.
        f32  biasMin       = 0.0f;
        f32  biasMax       = 0.0012f;
    };

    // Pre-pase: renderiza la profundidad de la escena desde la luz al ShadowMap.
    class ShadowPass {
        public:
            ShadowPass(Renderer& renderer, Shader* depthShader, u32 resolution = 2048);

            // Calcula la matriz light-space desde el sol + settings, renderiza la
            // profundidad y devuelve la matriz usada.
            const glm::mat4& Render(const scene::SceneGraph& scene, const DirectionalLight& sun);

            const glm::mat4& lightSpaceMatrix() const { return m_lightSpaceMatrix; }
            const Texture&   DepthTexture()     const { return m_shadowMap.DepthTexture(); }
            ShadowSettings&  settings()               { return m_settings; }

        private:
            Renderer&      m_renderer;
            Shader*        m_shader;
            ShadowMap      m_shadowMap;
            ShadowSettings m_settings;
            glm::mat4      m_lightSpaceMatrix { 1.0f };
            // PassParams (binding 4) propio: este pase corre antes de BeginScene.
            UniformBuffer  m_passUbo { sizeof(ShadowPassBlock) };
    };

}
}
