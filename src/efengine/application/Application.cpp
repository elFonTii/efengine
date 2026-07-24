#include "Application.h"

#include <efengine/core/Log.h>
#include <efengine/scene/Scene.h>
#include <efengine/scene/SceneObject.h>
#include <efengine/scene/Camera.h>

namespace efengine {
namespace application {

    Application::Application()
        : m_window( platform::WindowProps{ "efengine", 1280, 720, true } )
        , m_context( m_window )
        , m_sceneFB(m_window.GetWidth(), m_window.GetHeight())
        , m_debugUI( m_window )
        , m_tonemapPass( m_renderer, m_fullscreenQuad,
                m_resources.GetShader("tonemap", "assets/shaders/screen.vert", "assets/shaders/tonemap.frag")
         )
        , m_bloomPass( m_renderer, m_fullscreenQuad,
               m_resources.GetShader("brightpass",     "assets/shaders/screen.vert", "assets/shaders/brightpass.frag"),
               m_resources.GetShader("blur",           "assets/shaders/screen.vert", "assets/shaders/blur.frag"),
               m_resources.GetShader("bloomcomposite", "assets/shaders/screen.vert", "assets/shaders/bloom_composite.frag"),
               m_window.GetWidth(), m_window.GetHeight() )
        , m_fxaaPass( m_renderer, m_fullscreenQuad,
                m_resources.GetShader("fxaa", "assets/shaders/screen.vert", "assets/shaders/fxaa.frag")
         )
        , m_postChain( m_window.GetWidth(), m_window.GetHeight())
        , m_skyboxPass( m_renderer, m_fullscreenQuad,
                m_resources.GetShader("skybox", "assets/shaders/skybox.vert", "assets/shaders/skybox.frag") )
        , m_shadowPass( m_renderer,
                m_resources.GetShader("shadow_depth", "assets/shaders/shadow_depth.vert", "assets/shaders/shadow_depth.frag") ){
        
        const f32 quadVertices[] = {
        // pos      uv
        -1.0f, -1.0f, 0.0f, 0.0f,
        1.0f, -1.0f, 1.0f, 0.0f,
        1.0f,  1.0f, 1.0f, 1.0f,
        -1.0f,  1.0f, 0.0f, 1.0f,
        };

        const u32 quadIndices[] = { 0, 1, 2, 2, 3, 0 };   // dos triángulos

        renderer::Buffer       vbo(quadVertices, sizeof(quadVertices));
        renderer::IndexBuffer  ebo(quadIndices, 6);
        m_postChain.Add(&m_bloomPass);
        m_postChain.Add(&m_tonemapPass);
        m_postChain.Add(&m_fxaaPass);   // último eslabón → escribe al backbuffer; tonemap pasa a un scratch LDR
        renderer::VertexLayout layout;
        layout.Push(renderer::ShaderDataType::Float2);
        layout.Push(renderer::ShaderDataType::Float2); 
        m_fullscreenQuad.AddVertexBuffer(std::move(vbo), layout);
        m_fullscreenQuad.SetIndexBuffer(std::move(ebo));

        renderer::Shader* eqCS = m_resources.GetComputeShader("equirect_to_cube",
                                     "assets/shaders/ibl/equirect_to_cube.comp");
        renderer::Shader* irrCS = m_resources.GetComputeShader("irradiance_convolve",
                                     "assets/shaders/ibl/irradiance_convolve.comp");
        if (eqCS && irrCS) {
            m_environment = renderer::Environment::Create(
                "assets/hdr/citrus_orchard_puresky_4k.hdr", *eqCS, *irrCS); // TODO: serializar paths, no pueden seguir creciendo...
            if (!m_environment) EF_LOG_ERROR("Application: no se pudo crear el Environment IBL");
        } else {
            EF_LOG_ERROR("Application: no se pudo cargar algún compute de IBL (equirect/irradiance)");
        }

        EF_LOG_INFO("Application inicializada");
    }

    void Application::BeginFrame() {
        m_time.Tick();
        m_window.PollEvents();
        m_debugUI.NewFrame();
    }

    void Application::EndFrame() {
        m_debugUI.Render();     // dibuja el overlay sobre la imagen final (backbuffer)
        m_window.SwapBuffers();
    }

    void Application::RenderScene(const scene::Scene& scene, const scene::Camera& camera) {
        const u32 w = m_window.GetWidth();
        const u32 h = m_window.GetHeight();
        if(w != 0 && h != 0) { m_sceneFB.Resize(w, h); m_postChain.Resize(w, h); }

        // 2 cargas:  al FBO de la escena y Backbuffer de Window
        
        // --- Pre-pase de sombra: profundidad de la escena desde el sol ---
        renderer::ShadowContext shadowCtx;
        if (m_shadowPass.settings().enabled) {
            m_shadowPass.Render(scene, scene.sun());
            shadowCtx.map              = &m_shadowPass.DepthTexture();
            shadowCtx.lightSpaceMatrix = m_shadowPass.lightSpaceMatrix();
            shadowCtx.enabled          = true;
            shadowCtx.biasMin          = m_shadowPass.settings().biasMin;
            shadowCtx.biasMax          = m_shadowPass.settings().biasMax;
        }

        // Al Framebuffer de escena
        m_sceneFB.Bind();
        m_renderer.Clear(m_clearColor[0], m_clearColor[1], m_clearColor[2], m_clearColor[3]);
        m_renderer.BeginScene(camera.ViewMatrix(), camera.ProjectionMatrix(), camera.Position(), scene.lights(), scene.ambientFactor, scene.sun(), shadowCtx,
                              m_environment ? &m_environment->irradiance() : nullptr);

         if (m_environment) {
            m_skyboxPass.Draw(m_environment->env(), camera.ViewMatrix(), camera.ProjectionMatrix());
         }

        for(const scene::SceneObject& obj : scene.objects()) {
            if(!obj.model) { EF_LOG_WARNING("Se intenta renderizar un objeto sin modelo"); continue; }
            m_renderer.Submit(*obj.model, obj.materials, obj.transform.Matrix());
        }

        m_sceneFB.Unbind();
        m_tonemapPass.SetExposure(camera.Exposure());
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
