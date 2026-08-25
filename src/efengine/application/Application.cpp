#include "Application.h"

#include <efecom/RHI.h>

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
        , m_skyboxPass( m_renderer, m_fullscreenQuad,
                m_resources.GetShader("skybox", "assets/shaders/skybox.vert", "assets/shaders/skybox.frag") ) {

        // El primer eslabon del pipeline. El resto del frame sigue escrito a
        // mano en RenderScene; la frontera se corre un pase por vez.
        m_shadowPtr = static_cast<renderer::ShadowPass*>(
            m_pipeline.Add(std::make_unique<renderer::ShadowPass>(
                m_renderer,
                m_resources.GetShader("shadow_depth",
                    "assets/shaders/shadow_depth.vert",
                    "assets/shaders/shadow_depth.frag"))));


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

            // Segundo eslabon. Si Environment::Create falla, Create da null,
            // el pase no se registra y el frame sigue sin ambiente.
            m_iblPtr = static_cast<renderer::IblPass*>(
                m_pipeline.Add(renderer::IblPass::Create(
                    renderer::Environment::Create(envDesc, envShaders))));
            if (!m_iblPtr) EF_LOG_ERROR("Application: no se pudo crear el Environment IBL");
        } else {
            EF_LOG_ERROR("Application: no se pudo cargar algún compute de IBL (equirect/irradiance/prefilter/brdf)");
        }

        // DDGI. Si falta cualquier shader, m_ddgiPass queda vacio y el frame
        // sigue con IBL puro: un fallo de carga no rompe el render.
        renderer::DdgiPass::Shaders ddgiShaders;
        // Los dos vertex shaders son propios del pase y ya no los de pbr/skybox:
        // la captura dibuja TODAS las vistas del frame con un draw instanciado, y
        // cada instancia saca su vista de un SSBO en vez del bloque Frame.
        ddgiShaders.capture = m_resources.GetShader("ddgi_capture",
                                  "assets/shaders/ddgi/capture.vert",
                                  "assets/shaders/ddgi/capture.frag");
        ddgiShaders.captureSky = m_resources.GetShader("ddgi_capture_sky",
                                  "assets/shaders/ddgi/capture_sky.vert",
                                  "assets/shaders/ddgi/capture_sky.frag");
        ddgiShaders.blendIrradiance = m_resources.GetComputeShader("ddgi_blend_irradiance",
                                  "assets/shaders/ddgi/blend_irradiance.comp");
        ddgiShaders.blendDistance = m_resources.GetComputeShader("ddgi_blend_distance",
                                  "assets/shaders/ddgi/blend_distance.comp");

        m_ddgiPtr = static_cast<renderer::DdgiPass*>(
            m_pipeline.Add(renderer::DdgiPass::Create(m_renderer, m_fullscreenQuad, ddgiShaders)));
        if (!m_ddgiPtr) EF_LOG_ERROR("Application: no se pudo crear el DdgiPass");

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

        // El prepass escribe en el depth DEL FRAMEBUFFER DE ESCENA. Es lo que
        // deja al forward dibujar despues con GL_EQUAL en vez de volver a
        // resolver la visibilidad que el prepass ya resolvio.
        m_aoPass = renderer::AoPass::Create(m_renderer, m_fullscreenQuad, aoShaders,
                                            m_window.GetWidth(), m_window.GetHeight(),
                                            m_sceneFB.depthRenderbuffer());
        if (!m_aoPass) EF_LOG_ERROR("Application: no se pudo crear el AoPass");

        // Indirecta difusa a resolucion reducida. Mismo patron de degradacion
        // que DDGI y AO: si falta el shader, m_indirectPass queda vacio y
        // pbr.frag samplea el volumen inline -- la misma imagen, mas cara.
        m_indirectPass = renderer::IndirectPass::Create(
            m_renderer, m_fullscreenQuad,
            m_resources.GetShader("ddgi_indirect",
                                  "assets/shaders/screen.vert",
                                  "assets/shaders/ddgi/indirect.frag"),
            m_window.GetWidth(), m_window.GetHeight());
        if (!m_indirectPass) EF_LOG_ERROR("Application: no se pudo crear el IndirectPass");

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
            // crea un renderbuffer de profundidad nuevo y destruye el viejo, y el
            // prepass del AO lo tiene prestado: pasarle el handle despues es lo
            // unico que evita que quede enganchado a un attachment muerto.
            m_sceneFB.Resize(w, h);
            m_postChain.Resize(w, h);
            if (m_aoPass) m_aoPass->Resize(w, h, m_sceneFB.depthRenderbuffer());
            if (m_indirectPass) m_indirectPass->Resize(w, h);
            // El backbuffer sigue al framebuffer de la ventana. Sin esto,
            // RenderTarget::Present() fijaria el viewport del tamano viejo.
            efecom::SetPresentExtent(w, h);
        }

        // Recalcula world-transforms y junta las listas de render una vez por frame.
        // Los dos pases de abajo (sombra y forward) leen el mismo resultado.
        scene.UpdateWorldTransforms();

        // El contexto del frame. Los pases ya migrados publican ahi; lo que
        // todavia esta escrito a mano mas abajo lo lee de ctx.lighting.
        renderer::FrameContext ctx { scene, camera, m_renderer, m_sceneFB, w, h };
        m_pipeline.Execute(&ctx);

        renderer::SceneLighting& lighting = ctx.lighting;

        // --- AO screen-space: prepass + kernel. Antes de BeginScene porque sube
        // --- su propio bloque de binding 4, y porque pbr.frag necesita el
        // --- resultado YA calculado para modular la indirecta.
        if (m_aoPass) {
            m_aoPass->Render(scene, camera.ViewMatrix(), camera.ProjectionMatrix());
            lighting.ao = m_aoPass->Context();
        }

        m_renderer.BeginScene(camera.ViewMatrix(), camera.ProjectionMatrix(), camera.Position(),
                              scene.PointLights(), scene.Sun(), lighting);

        // --- Indirecta difusa a resolucion reducida. DESPUES de BeginScene y no
        // --- antes, a diferencia de los otros tres pre-pases: asi la view/proj y
        // --- la posicion de camara le llegan por el bloque Frame ya subido y los
        // --- atlas de DDGI ya estan en sus unidades, en vez de re-empaquetar la
        // --- camara del frame en un bloque propio y tener dos fuentes de verdad.
        //
        // --- Lee el prepass y el target del AO, asi que va despues de el; no
        // --- re-rasteriza geometria. Con el AO apagado no corre y el Context
        // --- queda vacio, que es como pbr.frag sabe que tiene que samplear el
        // --- volumen inline.
        if (m_indirectPass && m_aoPass) {
            m_indirectPass->Render(lighting.ao,
                                   camera.ViewMatrix(), camera.ProjectionMatrix());
            lighting.indirect = m_indirectPass->Context();

            // El bloque de binding 6 se arma en BeginScene, que ya corrio: hay
            // que re-subirlo con el contexto de la indirecta o pbr.frag lee
            // upsample.x en cero y samplea inline igual, tirando el pase entero.
            m_renderer.SetIndirectContext(lighting.ao, lighting.indirect);
        }

        // --- El depth prepass del forward es el prepass del AO ---
        // Si corrio, el depth del framebuffer de escena YA tiene la profundidad
        // de la escena con esta camara: el forward limpia solo color y dibuja con
        // GL_EQUAL, asi el overdraw se sombrea una sola vez y lo tapado ni entra
        // al fragment shader.
        //
        // Si no corrio (AO apagado, o fallo la carga de sus shaders), ese depth
        // tiene la profundidad del FRAME ANTERIOR y hay que limpiarlo: dibujar
        // con GL_EQUAL contra el dejaria la pantalla vacia.
        const bool prepassListo = (m_aoPass && m_aoPass->depthReady());

        m_sceneFB.Bind();
        if (prepassListo) {
            efecom::SetClearColor(m_clearColor[0], m_clearColor[1],
                                  m_clearColor[2], m_clearColor[3]);
            efecom::Clear(efecom::ClearMask::Color);
        } else {
            m_renderer.Clear(m_clearColor[0], m_clearColor[1], m_clearColor[2], m_clearColor[3]);
        }

         if (lighting.ibl.environment) {
            m_skyboxPass.Draw(*lighting.ibl.environment);
         }

        {
            EF_PROFILE_SCOPE("Forward");
            renderer::DrawOptions opciones;
            opciones.depth = prepassListo ? renderer::DepthMode::Equal
                                          : renderer::DepthMode::Write;
            for(const scene::RenderItem& item : scene.Renderables()) {
                if(!item.model) { EF_LOG_WARNING("Se intenta renderizar un item sin modelo"); continue; }
                m_renderer.Submit(*item.model, *item.materials, item.world, opciones);
            }
        }

        // El volcado del target de captura va sobre la imagen HDR de la escena,
        // antes del post: es un instrumento de debug, no parte de la imagen.
        if (m_ddgiPtr && m_ddgiDebug && m_ddgiPtr->settings().debugProbes) {
            const renderer::DdgiSettings& ds = m_ddgiPtr->settings();
            // Modo 3 = el mismo volcado pero mirando el alfa: sin esto la
            // distancia capturada no es verificable desde el panel.
            if (ds.debugMode >= 2u) {
                m_ddgiDebug->DrawCaptureBlit(m_ddgiPtr->captureTarget(), ds.debugMode == 3u);
            } else if (m_ddgiProbeMesh != null) {
                m_ddgiDebug->DrawProbes(ds.grid, ds, *m_ddgiProbeMesh);
            }
        }

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
