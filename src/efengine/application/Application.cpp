#include "Application.h"

#include <efecom/RHI.h>

#include <efengine/application/FramePipeline.h>
#include <efengine/application/PassDeps.h>
#include <efengine/core/Log.h>
#include <efengine/renderer/FrameContext.h>
#include <efengine/scene/SceneGraph.h>
#include <efengine/scene/Camera.h>

namespace efengine {
namespace application {

    Application::Application()
        : m_window( platform::WindowProps{ "efengine", 1280, 720, true } )
        , m_context( m_window )
        , m_sceneFB(m_window.GetWidth(), m_window.GetHeight())
        , m_debugUI( m_window )
        , m_bloomPass( m_renderer, m_fullscreenQuad,
               m_resources.GetShader("brightpass",     "assets/shaders/screen.vert", "assets/shaders/brightpass.frag"),
               m_resources.GetShader("blur",           "assets/shaders/screen.vert", "assets/shaders/blur.frag"),
               m_resources.GetShader("bloomcomposite", "assets/shaders/screen.vert", "assets/shaders/bloom_composite.frag"),
               m_window.GetWidth(), m_window.GetHeight() )
        , m_fxaaPass( m_renderer, m_fullscreenQuad,
                m_resources.GetShader("fxaa", "assets/shaders/screen.vert", "assets/shaders/fxaa.frag")
         )
        , m_postChain( m_window.GetWidth(), m_window.GetHeight())
         {

        const f32 quadVertices[] = {
        // pos      uv
        -1.0f, -1.0f, 0.0f, 0.0f,
        1.0f, -1.0f, 1.0f, 0.0f,
        1.0f,  1.0f, 1.0f, 1.0f,
        -1.0f,  1.0f, 0.0f, 1.0f,
        };

        const u32 quadIndices[] = { 0, 1, 2, 2, 3, 0 };

        renderer::Buffer       vbo(quadVertices, sizeof(quadVertices));
        renderer::IndexBuffer  ebo(quadIndices, 6);
        // Dos eslabones y no tres: el composite del bloom tonemapea, asi que la
        // imagen sale de ahi ya en LDR sRGB. El orden efectivo sigue siendo
        // bloom -> tonemap -> FXAA, que es el correcto: FXAA estima contraste
        // asumiendo valores perceptuales.
        m_postChain.Add(&m_bloomPass);
        m_postChain.Add(&m_fxaaPass);   // ultimo eslabon -> escribe al backbuffer
        renderer::VertexLayout layout;
        layout.Push(renderer::ShaderDataType::Float2);
        layout.Push(renderer::ShaderDataType::Float2); 
        m_fullscreenQuad.AddVertexBuffer(std::move(vbo), layout);
        m_fullscreenQuad.SetIndexBuffer(std::move(ebo));

        // El frame entero, en orden. La carga de shaders de cada pase vive en
        // application/passes/, uno por archivo: este archivo ya no sabe que
        // shaders necesita ninguno.
        const PassDeps deps { m_renderer, m_resources, m_fullscreenQuad, m_sceneFB,
                              m_window.GetWidth(), m_window.GetHeight(), m_clearColor };
        BuildFramePipeline(m_pipeline, deps);

        m_window.SetEventListener(&m_input);
        renderer::SetActiveProfiler(&m_profiler);

        EF_LOG_INFO("Application inicializada");
    }

    void Application::BeginFrame() {
        m_time.Tick();
        m_window.PollEvents();
        // Después de PollEvents (que dispara los callbacks de cursor/scroll de
        // este frame) y antes de que nadie lea el input.
        m_input.NewFrame(m_window);
        m_debugUI.NewFrame();
        // ImGui_ImplOpenGL3_RenderDrawData del frame anterior piso blend, cull y
        // depth a espaldas del RHI: el cache del backend quedo mintiendo.
        efecom::ResetPipelineStateCache();
        efecom::ResetFrameCounters();
        m_profiler.BeginFrame(m_time.DeltaTime());
    }

    void Application::EndFrame() {
        m_debugUI.Render();     // dibuja el overlay sobre la imagen final (backbuffer)
        m_window.SwapBuffers();
    }

    void Application::RenderScene(scene::SceneGraph& scene, const scene::Camera& camera) {
        const u32 w = m_window.GetWidth();
        const u32 h = m_window.GetHeight();
        if(w != 0 && h != 0) {
            // ORDEN OBLIGATORIO: el framebuffer de escena PRIMERO. Su Resize
            // crea un renderbuffer de profundidad nuevo y destruye el viejo, y
            // el prepass del AO lo tiene prestado: si los pases se
            // redimensionaran antes, el AO quedaria enganchado al attachment
            // muerto. Con esto, AoPass::Resize lee el handle nuevo.
            m_sceneFB.Resize(w, h);
            m_pipeline.Resize(w, h);
            m_postChain.Resize(w, h);
            // El backbuffer sigue al framebuffer de la ventana. Sin esto,
            // RenderTarget::Present() fijaria el viewport del tamano viejo.
            efecom::SetPresentExtent(w, h);
        }

        // Recalcula world-transforms y junta las listas de render una vez por
        // frame: la sombra, la captura de DDGI y el forward leen el mismo
        // resultado.
        scene.UpdateWorldTransforms();

        // El frame entero es la lista de pases. El orden vive en el ctor, donde
        // se registran; lo que un pase le pasa a otro viaja en el contexto.
        renderer::FrameContext ctx { scene, camera, m_renderer, m_sceneFB, w, h };
        m_pipeline.Execute(&ctx);

        // Ya no hay "desbindear": el post chain declara su propio destino por pase.
        m_bloomPass.SetExposure(camera.Exposure());
        m_postChain.Run(m_sceneFB.ColorTexture());
    }

    f32 Application::DeltaTime() const { return m_time.DeltaTime(); }

    f64 Application::Elapsed() const { return m_time.Elapsed(); }
    
    bool Application::IsKeyPressed(platform::Key key) const { return m_window.IsKeyPressed(key); }

    void Application::Close() { m_window.SetShouldClose(true); }
    
    void Application::SetClearColor(f32 r, f32 g, f32 b, f32 a) {
    m_clearColor[0] = r; m_clearColor[1] = g; m_clearColor[2] = b; m_clearColor[3] = a;
}
    
}
}
