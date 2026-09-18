#pragma once
#include <efengine/core/Types.h>
#include <efengine/renderer/IScenePass.h>
#include <efengine/renderer/CascadedShadowMap.h>
#include <efengine/renderer/CascadeContext.h>
#include <efengine/renderer/ShaderBlocks.h>
#include <efengine/renderer/UniformBuffer.h>

namespace efengine {
namespace renderer {

    class Renderer;
    class Shader;

    struct CascadeSettings {
        // El flag de encendido es IScenePass::enabled, igual que en ShadowPass.
        u32 resolution = 2048u;   // lado de CADA capa
        u32 count      = 4u;      // 1..kMaxCascades

        // Hasta donde llegan las sombras, en metros. NO es el far de la camara:
        // con far=5000 tres cascadas caerian en cielo vacio.
        f32 shadowDistance = 200.0f;

        // Mezcla del reparto: 0 = uniforme, 1 = logaritmico.
        f32 lambda = 0.75f;

        // Cuanto se estira la caja hacia la luz, en metros. Lo que decide si un
        // techo fuera de la rebanada sigue proyectando sombra adentro.
        f32 lightExtension = 50.0f;

        // Un solo slider en texels para las N cascadas: la conversion a metros se
        // hace una vez por cascada porque el texel de cada una mide distinto.
        f32 normalOffsetTexels = 2.0f;

        // Fraccion final de cada cascada donde se interpola con la siguiente.
        f32 blendRatio = 0.1f;

        bool debugView = false;
    };

    class CascadedShadowPass : public IScenePass {
        public:
            CascadedShadowPass(Renderer& renderer, Shader* depthShader);

            void Execute(FrameContext& ctx) override;
            const char* Name() const override { return "Sombras en cascada"; }

            CascadeSettings&      settings()      { return m_settings; }
            const CascadeContext& context() const { return m_context; }

        private:
            Renderer&         m_renderer;
            Shader*           m_shader;
            CascadedShadowMap m_map;
            CascadeSettings   m_settings;
            CascadeContext    m_context;
            // PassParams (binding 4) propio: este pase corre antes de BeginScene,
            // igual que ShadowPass.
            UniformBuffer     m_passUbo { sizeof(ShadowPassBlock) };
    };

}
}
