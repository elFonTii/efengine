#include "panels/PassPanel.h"
#include "panels/PanelUI.h"

#include "EditorUI.h"

#include <efengine/application/Application.h>
#include <efengine/renderer/ScenePipeline.h>
#include <efengine/renderer/DdgiPass.h>
#include <efengine/renderer/DdgiSettings.h>
#include <efengine/renderer/DdgiVolume.h>
#include <efengine/renderer/AoPass.h>
#include <efengine/renderer/IndirectPass.h>
#include <efengine/renderer/Bounds.h>
#include <efengine/scene/SceneGraph.h>
#include <glm/glm.hpp>

#include <imgui.h>

namespace sandbox {

// Igual que EditorUI.cpp: los paneles nombran los tipos del motor como
// renderer::X, sin el efengine:: adelante.
using namespace efengine;

    void dibujarPanelDdgi(EditorContext& ctx) {
        if (!ImGui::CollapsingHeader("DDGI (iluminacion indirecta)")) return;

        renderer::DdgiPass* opt = ctx.app.GetPipeline().Find<renderer::DdgiPass>();
        if (opt == null) {
            ImGui::TextColored(kColorError, "DdgiPass no disponible: fallo la carga de shaders.");
            ImGui::TextWrapped("La escena esta usando IBL puro. Mira la consola.");
            return;
        }
        renderer::DdgiPass&     pass = *opt;
        renderer::DdgiSettings& s    = pass.settings();

        CamposAlineados alineados;

        // -- Lo primero que hay que mirar cuando "no se ve la GI" --------------
        if (pass.atlasValid()) {
            ImGui::TextColored(kColorOk, "pbr.frag recibe los atlas: SI");
        } else {
            ImGui::TextColored(kColorError, "pbr.frag recibe los atlas: NO");
            ImGui::TextWrapped("Hasta que corra un blend, DDGI aporta cero y la imagen es IBL puro.");
        }

        ImGui::Checkbox("Habilitado", &s.enabled);

        // -- Grilla ------------------------------------------------------------
        ImGui::SeparatorText("Grilla");
        bool gridChanged = false;
        gridChanged |= ImGui::DragFloat3("Origen",         &s.grid.origin.x,  0.1f);
        gridChanged |= ImGui::DragFloat3("Espaciado",      &s.grid.spacing.x, 0.05f, 0.05f, 10.0f);
        gridChanged |= ImGui::DragInt3  ("Probes por eje", &s.grid.counts.x,  1.0f,
                                         1, renderer::kMaxProbesPerAxis);
        ImGui::TextDisabled("total: %u probes", renderer::ProbeCount(s.grid));

        // Encajar la grilla a la escena resuelve de un click la clase entera de
        // bug "la grilla no cubre la sala", que es con la que arranco este ciclo.
        if (ImGui::Button("Encajar grilla a la escena", ImVec2(-kAnchoEtiqueta, 0.0f))) {
            const renderer::AABB& b = ctx.scene.WorldBounds();
            if (b.Valid()) {
                // Un 10% de margen hacia adentro: un probe DENTRO de una pared
                // captura su interior y contamina a sus vecinos por el peso
                // trilineal.
                const glm::vec3 ext    = b.Extents() * 0.9f;
                const glm::vec3 minPos = b.Center() - ext;
                const glm::ivec3 n     = s.grid.counts;
                s.grid.origin  = minPos;
                s.grid.spacing = glm::vec3(
                    n.x > 1 ? (2.0f * ext.x) / f32(n.x - 1) : 1.0f,
                    n.y > 1 ? (2.0f * ext.y) / f32(n.y - 1) : 1.0f,
                    n.z > 1 ? (2.0f * ext.z) / f32(n.z - 1) : 1.0f);
                gridChanged = true;
            }
        }
        if (gridChanged) {
            ImGui::TextColored(kColorAviso,
                               "Cambiar la grilla realoca los atlas y reinicia el barrido.");
        }

        // -- Update ------------------------------------------------------------
        ImGui::SeparatorText("Update");
        int perFrame = static_cast<int>(s.probesPerFrame);
        if (ImGui::SliderInt("Probes por frame", &perFrame, 0,
                             static_cast<int>(renderer::kMaxProbesPerFrame))) {
            s.probesPerFrame = static_cast<u32>(perFrame);
        }
        // Histeresis: cuanto del valor viejo se conserva. ESTO es el denoise
        // temporal de DDGI, no hace falta un denoiser aparte. Mas alto = mas
        // estable y mas lento en reaccionar.
        ImGui::SliderFloat("Histeresis", &s.hysteresis, 0.0f, 0.995f, "%.3f");
        ImGui::Checkbox("Congelar (freeze)", &s.freeze);
        ImGui::SameLine();
        if (ImGui::Button("Reset")) pass.Reset();

        const u32 total = renderer::ProbeCount(s.grid);
        const u32 framesPorBarrido = (s.probesPerFrame > 0u)
                                   ? (total + s.probesPerFrame - 1u) / s.probesPerFrame
                                   : 0u;
        ImGui::TextDisabled("cursor %u / %u   barridos %u", pass.cursor(), total, pass.sweepsDone());
        ImGui::TextDisabled("frames por barrido: %u", framesPorBarrido);
        // Tiempo de CPU emitiendo las llamadas, no de GPU ejecutandolas: sirve
        // para detectar que el round-robin se fue de escala, no como profiler.
        ImGui::TextDisabled("pase (CPU): %.3f ms", pass.lastMs());

        // -- Sampleo -----------------------------------------------------------
        ImGui::SeparatorText("Sampleo");
        ImGui::SliderFloat("Intensidad", &s.intensity, 0.0f, 4.0f);
        // Normal bias: subir si la luz atraviesa las paredes; bajar si los
        // rincones tienen una banda oscura.
        ImGui::SliderFloat("Normal bias", &s.normalBias, 0.0f, 1.0f, "%.3f m");
        ImGui::SliderFloat("View bias", &s.viewBias, 0.0f, 1.0f, "%.3f m");
        ImGui::SliderFloat("Chebyshev", &s.chebyshevSharpness, 1.0f, 16.0f);

        // El rango sale de la escena, no de un numero fijo: con un tope de 100 m
        // fijo, abrir el panel con maxDistance en 200 lo clamparia en silencio y
        // cambiaria el far plane de la captura sin que nadie toque nada.
        const renderer::AABB& bounds = ctx.scene.WorldBounds();
        const f32 topeDist = bounds.Valid() ? glm::max(4.0f * bounds.Radius(), 10.0f) : 200.0f;
        // Es el far plane de la captura de probes: muy alto tira la precision
        // del depth, muy bajo deja la captura vacia.
        ImGui::SliderFloat("Distancia max", &s.maxDistance, 1.0f, topeDist, "%.1f m");

        // -- Debug -------------------------------------------------------------
        ImGui::SeparatorText("Debug");

        // Primero de la seccion a proposito: es el unico control que contesta la
        // pregunta con la que uno abre este panel, "DDGI esta aportando algo".
        // El orden espeja DdgiSettings::DebugView.
        const char* vistas[] = { "Final (normal)",
                                 "Indirecta (irradiancia)",
                                 "Indirecta aplicada al pixel",
                                 "Solo luz directa",
                                 "DDGI ignorando el fade",
                                 "Fade del volumen",
                                 "Albedo",
                                 "Normal",
                                 "Sombra del sol (cruda)" };
        //
        // Como se leen estas vistas (era un tooltip; vive aca para no tapar la UI):
        //
        //   Eligen que termino escribe pbr.frag en vez de la imagen final.
        //
        //   'Indirecta aplicada' es lo que la indirecta le suma al pixel. OJO: ahi
        //   adentro el IBL y DDGI van MEZCLADOS por el fade, asi que no ver negro no
        //   prueba que DDGI aporte. Para leerlo, primero Render > Iluminacion >
        //   Intensidad IBL a 0: lo que quede es DDGI.
        //
        //   Y para saber por que: comparar 'DDGI ignorando el fade' con 'Fade del
        //   volumen'. Si el primero tiene color y el segundo esta negro, la GI se
        //   calcula bien y la tira el fade -- la grilla no cubre esa superficie con
        //   el margen que el fade pide.
        //
        //   Todas pasan por bloom y ACES: son cualitativas, no numeros.
        //
        int vista = static_cast<int>(s.debugView);
        if (ImGui::Combo("Vista", &vista, vistas, IM_ARRAYSIZE(vistas))) {
            s.debugView = static_cast<u32>(vista);
        }
        if (s.debugView != renderer::DdgiSettings::kDebugOff) {
            ImGui::TextColored(kColorAviso, "Vista de debug activa: la imagen NO es la final.");
        }

        // -- Diagnostico de rendimiento ---------------------------------------
        // Separado del bloque de vistas a proposito: las vistas responden "que
        // aporta DDGI", esto responde "cuanto cuesta". Es el ablation test de la
        // tarea 0 del plan de optimizacion; se mide con el panel de profiling
        // abierto, mirando el pase Forward.
        ImGui::SeparatorText("Rendimiento");

        // El interruptor del pase de indirecta a resolucion reducida. Esta aca y
        // no escondido en codigo porque el plan de optimizacion pide medir cada
        // cambio por separado: sin un toggle en caliente, comparar "con" y "sin"
        // pide recompilar, y entre las dos compilaciones cambia el estado de
        // boost de la GPU y el delta se pierde en el ruido.
        renderer::IndirectPass* ind = ctx.app.GetPipeline().Find<renderer::IndirectPass>();
        if (ind != null) {
            // El flag es del pase (IScenePass::enabled): es lo que el
            // ScenePipeline consulta para saltearlo.
            bool usaIndirecta = ind->enabled;
            if (ImGui::Checkbox("Indirecta a media resolucion", &usaIndirecta)) {
                ind->enabled = usaIndirecta;
            }
            if (usaIndirecta) {
                ImGui::TextDisabled("target %ux%u; pbr.frag sube con upsample bilateral",
                                    ind->width(), ind->height());
                // Es la dependencia que mas sorprende: sin AO no hay prepass, y
                // sin prepass no hay ni posicion ni guia para el upsample.
                renderer::AoPass* ao = ctx.app.GetPipeline().Find<renderer::AoPass>();
                if (ao == null || !ao->enabled) {
                    ImGui::TextColored(kColorAviso,
                                       "AO apagado: el pase no corre y pbr.frag samplea inline.");
                }
            } else {
                ImGui::TextDisabled("pbr.frag samplea el volumen por pixel (~16 gathers)");
            }
        } else {
            ImGui::TextColored(kColorError, "IndirectPass no disponible: fallo la carga del shader.");
        }

        ImGui::SeparatorText("Diagnostico");
        ImGui::Checkbox("Ablation: irradiancia constante", &s.ablateSample);
        if (s.ablateSample) {
            ImGui::SliderFloat("Valor del ablation", &s.ablateIrradiance, 0.0f, 2.0f, "%.3f");
            ImGui::TextColored(kColorAviso,
                               "Medicion activa: la imagen NO es la final. Mira 'Forward' en el profiler.");
        }

        ImGui::SeparatorText("Volcado");
        ImGui::Checkbox("Mostrar probes", &s.debugProbes);
        const char* modos[] = { "Irradiancia", "Media de distancia", "Target de captura",
                                "Target de captura (distancia)" };
        int modo = static_cast<int>(s.debugMode);
        if (ImGui::Combo("Modo", &modo, modos, 4)) s.debugMode = static_cast<u32>(modo);
        ImGui::SliderFloat("Radio de esfera", &s.debugRadius, 0.02f, 0.5f, "%.3f m");
    }

}
