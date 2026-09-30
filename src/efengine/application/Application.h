#pragma once

#include <efengine/platform/Window.h>
#include <efengine/application/DebugUI.h>
#include <efengine/renderer/Context.h>
#include <efengine/renderer/Renderer.h>
#include <efengine/renderer/Framebuffer.h>
#include <efengine/renderer/PostTargets.h>
#include <efengine/resources/ResourceManager.h>
#include <efengine/core/Time.h>
#include <efengine/platform/InputCodes.h>
#include <efengine/platform/Input.h>
#include <efengine/renderer/ScenePipeline.h>
#include <efengine/renderer/GpuProfiler.h>
#include <efengine/renderer/FrameView.h>
#include <optional>

namespace efengine {
namespace scene { class SceneGraph; class Camera; }
namespace renderer { class TaaPass; }
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

            // El unico accessor de pases de escena. Los paneles encuentran el
            // suyo con Find<T>(): agregar un pase ya no agrega un accessor.
            renderer::ScenePipeline& GetPipeline() { return m_pipeline; }
            renderer::TemporalSettings& GetTemporalSettings() { return m_temporal; }
            const renderer::FrameView&  LastFrameView() const { return m_lastView; }
            // El proximo frame no reproyecta contra el anterior.
            void ResetTemporalHistory() { m_history.valid = false; }

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
            renderer::PostTargets m_postTargets;
            core::Time m_time;
            // No participa del contrato de orden: no toca GL.
            platform::Input m_input;
            // Los pases del frame, en orden. Va DESPUES de m_renderer y de
            // m_resources: sus pases guardan referencias a los dos, y el orden
            // de declaracion es el que decide quien muere primero.
            renderer::ScenePipeline m_pipeline;
            renderer::TemporalSettings m_temporal;
            renderer::FrameHistory     m_history;
            renderer::FrameView        m_lastView;
            renderer::TaaPass*         m_taa = null;   // del pipeline; null si no se pudo crear

    };

} // namespace application
} // namespace efengine
