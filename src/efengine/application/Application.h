#pragma once

#include <efengine/platform/Window.h>
#include <efengine/application/DebugUI.h>
#include <efengine/renderer/Context.h>
#include <efengine/renderer/Renderer.h>
#include <efengine/renderer/Framebuffer.h>
#include <efengine/renderer/TonemapPass.h>
#include <efengine/renderer/BloomPass.h>
#include <efengine/renderer/FxaaPass.h>
#include <efengine/renderer/PostChain.h>
#include <efengine/resources/ResourceManager.h>
#include <efengine/core/Time.h>
#include <efengine/platform/InputCodes.h>
#include <efengine/platform/Input.h>
#include <efengine/renderer/Environment.h>
#include <efengine/renderer/SkyboxPass.h>
#include <efengine/renderer/ShadowPass.h>
#include <efengine/renderer/DdgiPass.h>
#include <efengine/renderer/DdgiDebugPass.h>
#include <efengine/renderer/AoPass.h>
#include <efengine/renderer/SceneLighting.h>
#include <efengine/renderer/GpuProfiler.h>
#include <optional>

namespace efengine {
namespace scene { class SceneGraph; class Camera; }
namespace application {

    // Bundle RAII de los subsistemas del motor. Expone accessors; el loop
    // principal vive en el cliente (sandbox). m_context no se expone: se
    // retiene solo por su lifetime (RAII de GLAD).
    class Application {
        public:
            Application();

            platform::Window&   GetWindow()   { return m_window; }
            renderer::Renderer& GetRenderer() { return m_renderer; }
            renderer::GpuProfiler& GetProfiler() { return m_profiler; }
            resources::ResourceManager& GetResources() { return m_resources; }
            core::Time& GetTime() { return m_time; }
            const platform::Input& GetInput() const { return m_input; }
            application::DebugUI& GetDebugUI() { return m_debugUI; }
            renderer::BloomPass& GetBloomPass() { return m_bloomPass; }
            renderer::FxaaPass& GetFxaaPass() { return m_fxaaPass; }
            renderer::ShadowPass& GetShadowPass() { return m_shadowPass; }
            std::optional<renderer::DdgiPass>& GetDdgiPass() { return m_ddgiPass; }
            std::optional<renderer::AoPass>&   GetAoPass()   { return m_aoPass; }

            // FRAME API
            bool Running() const { return !m_window.ShouldClose(); }
            void BeginFrame();
            void EndFrame();
            void RenderScene(scene::SceneGraph& scene, const scene::Camera& camera);
            f32  DeltaTime() const;
            f64  Elapsed() const;
            bool IsKeyPressed(platform::Key key) const;
            void Close();
            void SetClearColor(f32 r, f32 g, f32 b, f32 a = 1.0f);



        private:
            f32 m_clearColor[4] = { 0.18f, 0.18f, 0.18f, 1.0f };
            
            // ORDEN CRÍTICO (contrato, principio 11): Window 1.º (crea + activa
            // el contexto GL); Context 2.º (carga GLAD sobre ese contexto);
            // Framebuffer 3.º. necesita GL + tamaño de ventana
            // Renderer 4.º. El orden de init en C++ sigue el orden de declaración.
            // ResourceManager 4.º. (crea recursos cpu, necesita el contexto entero vivo)
            // time no participa del contrato, sólo depende de chrono y el baseline
            // del tiempo se fija en el primer tick, puede ir en cualquier lado.
            platform::Window   m_window; // 1
            renderer::Context  m_context; // 2
            renderer::Framebuffer m_sceneFB; // 3
            renderer::Renderer m_renderer; // 4
            // Despues de m_renderer: crea consultas de GL en el ctor.
            renderer::GpuProfiler m_profiler;
            resources::ResourceManager m_resources; // 5
            application::DebugUI m_debugUI;
            renderer::VertexArray m_fullscreenQuad; // 6
            renderer::TonemapPass m_tonemapPass;
            renderer::BloomPass m_bloomPass;
            renderer::FxaaPass m_fxaaPass;
            renderer::PostChain m_postChain;
            core::Time m_time;
            // No participa del contrato de orden: no toca GL.
            platform::Input m_input;
            std::optional<renderer::Environment> m_environment;
            renderer::SkyboxPass m_skyboxPass;
            renderer::ShadowPass m_shadowPass;
            // Vacios si falto algun shader de DDGI: el frame sigue con IBL puro.
            std::optional<renderer::DdgiPass>      m_ddgiPass;
            std::optional<renderer::DdgiDebugPass> m_ddgiDebug;
            // Vacio si falto algun shader de AO: el frame sigue sin oclusion.
            std::optional<renderer::AoPass>        m_aoPass;
            // Cache del ResourceManager: la esfera del volcado de probes.
            const renderer::Model* m_ddgiProbeMesh = null;

    };

} // namespace application
} // namespace efengine
