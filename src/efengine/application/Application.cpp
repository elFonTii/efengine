#include "Application.h"

#include <efecom/RHI.h>

#include <efengine/core/Log.h>
#include <efengine/scene/SceneGraph.h>
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

        const u32 quadIndices[] = { 0, 1, 2, 2, 3, 0 };

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
        renderer::Shader* preCS = m_resources.GetComputeShader("prefilter_ggx",
                                     "assets/shaders/ibl/prefilter_ggx.comp");
        renderer::Shader* lutCS = m_resources.GetComputeShader("brdf_lut",
                                     "assets/shaders/ibl/brdf_lut.comp");
        if (eqCS && irrCS && preCS && lutCS) {
            renderer::EnvironmentDesc envDesc;
            envDesc.hdrPath = "assets/hdr/skybox.hdr";   // TODO: serializar en settings de escena

            renderer::EnvironmentShaders envShaders;
            envShaders.equirectToCube     = eqCS;
            envShaders.irradianceConvolve = irrCS;
            envShaders.prefilterGGX       = preCS;
            envShaders.brdfLut            = lutCS;

            m_environment = renderer::Environment::Create(envDesc, envShaders);
            if (!m_environment) EF_LOG_ERROR("Application: no se pudo crear el Environment IBL");
        } else {
            EF_LOG_ERROR("Application: no se pudo cargar algún compute de IBL (equirect/irradiance/prefilter/brdf)");
        }

        // DDGI. Si falta cualquier shader, m_ddgiPass queda vacio y el frame
        // sigue con IBL puro: un fallo de carga no rompe el render.
        renderer::DdgiPass::Shaders ddgiShaders;
        ddgiShaders.capture = m_resources.GetShader("ddgi_capture",
                                  "assets/shaders/pbr.vert", "assets/shaders/ddgi/capture.frag");
        ddgiShaders.captureSky = m_resources.GetShader("ddgi_capture_sky",
                                  "assets/shaders/skybox.vert", "assets/shaders/ddgi/capture_sky.frag");
        ddgiShaders.blendIrradiance = m_resources.GetComputeShader("ddgi_blend_irradiance",
                                  "assets/shaders/ddgi/blend_irradiance.comp");
        ddgiShaders.blendDistance = m_resources.GetComputeShader("ddgi_blend_distance",
                                  "assets/shaders/ddgi/blend_distance.comp");

        m_ddgiPass = renderer::DdgiPass::Create(m_renderer, m_fullscreenQuad, ddgiShaders);
        if (!m_ddgiPass) EF_LOG_ERROR("Application: no se pudo crear el DdgiPass");

        renderer::Shader* ddgiBlit = m_resources.GetShader("ddgi_debug_blit",
                "assets/shaders/screen.vert", "assets/shaders/ddgi/debug_blit.frag");
        renderer::Shader* ddgiProbe = m_resources.GetShader("ddgi_debug_probe",
                "assets/shaders/ddgi/debug_probe.vert", "assets/shaders/ddgi/debug_probe.frag");
        if (ddgiBlit && ddgiProbe) {
            m_ddgiDebug.emplace(m_renderer, m_fullscreenQuad, ddgiBlit, ddgiProbe);
        } else {
            EF_LOG_ERROR("Application: no se pudo crear el DdgiDebugPass");
        }

        // La esfera del volcado de probes. Si no carga, el modo de esferas se
        // saltea; el resto del debug de DDGI sigue funcionando.
        m_ddgiProbeMesh = m_resources.GetModel("assets/models/sphere.fbx");
        if (m_ddgiProbeMesh != null) {
            // El .fbx no viene unitario; DrawProbes divide por esto para que
            // debugRadius sea de verdad un radio en metros.
            const glm::vec3 e = m_ddgiProbeMesh->bounds().Extents();
            EF_LOG_INFO("Application: sphere.fbx semi-extents (%.3f, %.3f, %.3f)", e.x, e.y, e.z);
        }

        // AO screen-space. Mismo patron de degradacion que DDGI: si falta un
        // shader, m_aoPass queda vacio y el frame sigue sin oclusion de contacto.
        renderer::AoPass::Shaders aoShaders;
        aoShaders.depthNormal = m_resources.GetShader("ao_depth_normal",
                                    "assets/shaders/ao/depth_normal.vert",
                                    "assets/shaders/ao/depth_normal.frag");
        aoShaders.gtao = m_resources.GetShader("ao_gtao",
                                    "assets/shaders/screen.vert", "assets/shaders/ao/gtao.frag");
        aoShaders.denoise = m_resources.GetShader("ao_denoise",
                                    "assets/shaders/screen.vert", "assets/shaders/ao/denoise.frag");

        m_aoPass = renderer::AoPass::Create(m_renderer, m_fullscreenQuad, aoShaders,
                                            m_window.GetWidth(), m_window.GetHeight());
        if (!m_aoPass) EF_LOG_ERROR("Application: no se pudo crear el AoPass");

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
            m_sceneFB.Resize(w, h);
            m_postChain.Resize(w, h);
            if (m_aoPass) m_aoPass->Resize(w, h);
            // El backbuffer sigue al framebuffer de la ventana. Sin esto,
            // RenderTarget::Present() fijaria el viewport del tamano viejo.
            efecom::SetPresentExtent(w, h);
        }

        // Recalcula world-transforms y junta las listas de render una vez por frame.
        // Los dos pases de abajo (sombra y forward) leen el mismo resultado.
        scene.UpdateWorldTransforms();

        // Los cuatro contextos de iluminacion del frame, en un solo struct.
        renderer::SceneLighting lighting;

        // --- Pre-pase de sombra: profundidad de la escena desde el sol ---
        if (m_shadowPass.settings().enabled) {
            m_shadowPass.Render(scene, scene.Sun());
            lighting.shadow.map              = &m_shadowPass.DepthTexture();
            lighting.shadow.lightSpaceMatrix = m_shadowPass.lightSpaceMatrix();
            lighting.shadow.enabled          = true;
            lighting.shadow.biasMin          = m_shadowPass.settings().biasMin;
            lighting.shadow.biasMax          = m_shadowPass.settings().biasMax;

            // El normal offset se dial en texels pero viaja en metros: el
            // shader no sabe cuanto mide un texel del shadow map en el mundo, y
            // eso depende del encuadre que el pase acaba de calcular.
            const renderer::DirectionalLightFit& fit = m_shadowPass.fit();
            const f32 texel = 2.0f * fit.orthoHalfSize
                            / static_cast<f32>(m_shadowPass.resolution());
            lighting.shadow.normalOffset = m_shadowPass.settings().normalOffsetTexels * texel;
        }

        // Las 4 piezas del IBL precomputado. Sin Environment quedan en null y el
        // shader apaga el ambiente entero en vez de samplear una unidad equivocada.
        lighting.ibl.intensity = scene.iblIntensity;
        if (m_environment) {
            lighting.ibl.irradiance  = &m_environment->irradiance();
            lighting.ibl.prefiltered = &m_environment->prefiltered();
            lighting.ibl.brdfLut     = &m_environment->brdfLut();
            lighting.ibl.maxLod      = m_environment->prefilterMaxLod();
        }

        // --- DDGI: captura de probes + blend. Antes de BeginScene porque sube su
        // --- propio FrameBlock por cara, y despues del ShadowPass porque la
        // --- captura sombrea con la matriz y el depth del sol.
        if (m_ddgiPass) {
            m_ddgiPass->Update(scene, lighting.shadow, lighting.ibl,
                               m_environment ? &m_environment->env() : null);
            lighting.ddgi = m_ddgiPass->Context();
        }

        // --- AO screen-space: prepass + kernel. Antes de BeginScene porque sube
        // --- su propio bloque de binding 4, y porque pbr.frag necesita el
        // --- resultado YA calculado para modular la indirecta.
        if (m_aoPass) {
            m_aoPass->Render(scene, camera.ViewMatrix(), camera.ProjectionMatrix());
            lighting.ao = m_aoPass->Context();
        }

        m_sceneFB.Bind();
        m_renderer.Clear(m_clearColor[0], m_clearColor[1], m_clearColor[2], m_clearColor[3]);
        m_renderer.BeginScene(camera.ViewMatrix(), camera.ProjectionMatrix(), camera.Position(),
                              scene.PointLights(), scene.Sun(), lighting);

         if (m_environment) {
            m_skyboxPass.Draw(m_environment->env());
         }

        {
            EF_PROFILE_SCOPE("Forward");
            for(const scene::RenderItem& item : scene.Renderables()) {
                if(!item.model) { EF_LOG_WARNING("Se intenta renderizar un item sin modelo"); continue; }
                m_renderer.Submit(*item.model, *item.materials, item.world);
            }
        }

        // El volcado del target de captura va sobre la imagen HDR de la escena,
        // antes del post: es un instrumento de debug, no parte de la imagen.
        if (m_ddgiPass && m_ddgiDebug && m_ddgiPass->settings().debugProbes) {
            const renderer::DdgiSettings& ds = m_ddgiPass->settings();
            // Modo 3 = el mismo volcado pero mirando el alfa: sin esto la
            // distancia capturada no es verificable desde el panel.
            if (ds.debugMode >= 2u) {
                m_ddgiDebug->DrawCaptureBlit(m_ddgiPass->captureTarget(), ds.debugMode == 3u);
            } else if (m_ddgiProbeMesh != null) {
                m_ddgiDebug->DrawProbes(ds.grid, ds, *m_ddgiProbeMesh);
            }
        }

        // Ya no hay "desbindear": el post chain declara su propio destino por pase.
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
