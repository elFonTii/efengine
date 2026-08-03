#pragma once
#include <efengine/core/Types.h>
#include <efengine/renderer/ShadowMap.h>
#include <efengine/renderer/ShadowMath.h>
#include <efengine/renderer/DirectionalLight.h>
#include <efengine/renderer/ShaderBlocks.h>
#include <efengine/renderer/UniformBuffer.h>
#include <glm/glm.hpp>

namespace efengine {
namespace scene { class SceneGraph; }
namespace renderer {

    class Renderer;
    class Shader;

    // Parámetros ajustables del bias (editables por ImGui). El encuadre de la
    // caja ortográfica NO esta acá: lo calcula FitDirectionalLight a partir de
    // SceneGraph::WorldBounds() en cada Render.
    //
    // Antes eran cuatro sliders a ojo (orthoHalfSize/distance/near/far) y ese
    // era el bug: con near=0.5 y far=450 el rango de profundidad valia 449.5 m
    // para una sala de 8 m, y como el bias se compara contra profundidad NDC
    // [0,1], el escalon mas chico del slider (0.0001) ya valia 4.5 cm de holgura
    // a lo largo del rayo de luz. Eso despegaba la sombra del punto de contacto
    // y abria una banda de luz en cada arista entre paredes (peter-panning), sin
    // que hubiera ningun valor intermedio que tapara el acne sin abrir la banda.
    struct ShadowSettings {
        bool enabled = true;

        // Aire alrededor de la AABB de la escena, en metros. Sube el rango de
        // profundidad y el tamaño del texel, asi que conviene chico.
        f32  padding = 1.0f;

        // Lado del shadow map. Cambiarlo recrea el FBO en el proximo Render.
        // Existe como control porque es la unica forma de medir si un artefacto
        // escala con el texel: si al duplicar la resolucion una linea se afina a
        // la mitad, es cuantizacion del shadow map; si no se mueve, es otra cosa.
        u32  resolution = 2048u;

        // El mecanismo principal: corre el punto de muestreo a lo largo de la
        // normal, en texels del shadow map (ShadowFactor lo recibe convertido a
        // metros). Se expresa en texels y no en metros porque lo que tiene que
        // tapar es la cuantizacion del shadow map, que mide exactamente eso.
        //
        // A diferencia del bias de profundidad, subirlo NO abre banda de luz en
        // las aristas: el punto corrido se despega de su propia superficie pero
        // sigue tapado por la pared vecina. El limite por arriba es la geometria
        // fina — un offset mas grueso que una pared la atraviesa.
        f32  normalOffsetTexels = 2.0f;

        // Bias de profundidad clasico, en fraccion del rango de profundidad
        // (que ahora es 2*(radio+padding)). Queda como escotilla y va en cero:
        // es el que causaba el peter-panning en los rincones. La conversion a
        // texels es independiente de la escena — bias_en_texels == biasMax *
        // resolucion — porque el rango y el texel escalan los dos con el radio.
        f32  biasMin = 0.0f;
        f32  biasMax = 0.0f;
    };

    // Pre-pase: renderiza la profundidad de la escena desde la luz al ShadowMap.
    class ShadowPass {
        public:
            ShadowPass(Renderer& renderer, Shader* depthShader, u32 resolution = 2048);

            // Calcula la matriz light-space desde el sol + settings, renderiza la
            // profundidad y devuelve la matriz usada.
            const glm::mat4& Render(const scene::SceneGraph& scene, const DirectionalLight& sun);

            const glm::mat4& lightSpaceMatrix() const { return m_fit.matrix; }
            const Texture&   DepthTexture()     const { return m_shadowMap.DepthTexture(); }
            ShadowSettings&  settings()               { return m_settings; }

            // Encuadre calculado en el ultimo Render. El panel lo muestra para
            // poder leer el bias en metros sin adivinar.
            const DirectionalLightFit& fit() const { return m_fit; }
            u32 resolution() const { return m_shadowMap.resolution(); }

        private:
            Renderer&           m_renderer;
            Shader*             m_shader;
            ShadowMap           m_shadowMap;
            ShadowSettings      m_settings;
            DirectionalLightFit m_fit;
            // PassParams (binding 4) propio: este pase corre antes de BeginScene.
            UniformBuffer  m_passUbo { sizeof(ShadowPassBlock) };
    };

}
}
