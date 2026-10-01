#include "panels/PassPanel.h"
#include "panels/PanelUI.h"

#include "EditorUI.h"

#include <efengine/application/Application.h>
#include <efengine/renderer/ScenePipeline.h>
#include <efengine/renderer/DdgiPass.h>
#include <efengine/renderer/DdgiSettings.h>
#include <efengine/renderer/DdgiVolume.h>
#include <efengine/renderer/DdgiGridFit.h>
#include <efengine/renderer/AoPass.h>
#include <efengine/renderer/IndirectPass.h>
#include <efengine/renderer/Bounds.h>
#include <efengine/scene/SceneGraph.h>
#include <glm/glm.hpp>

#include <imgui.h>

#include <algorithm>

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

        if (ImGui::Button("Encajar grilla a la escena", ImVec2(-kAnchoEtiqueta, 0.0f))) {
            gridChanged |= renderer::FitDdgiGridToBounds(s.grid, ctx.scene.WorldBounds());
        }
        if (gridChanged) {
            ImGui::TextColored(kColorAviso,
                               "Cambiar la grilla realoca los atlas y reinicia el barrido.");
        }

        // -- Update ------------------------------------------------------------
        ImGui::SeparatorText("Update");
        int perFrame = static_cast<int>(s.probeBudget);
        if (ImGui::SliderInt("Probes por frame", &perFrame, 0,
                             static_cast<int>(renderer::kMaxProbesPerFrame))) {
            s.probeBudget = static_cast<u32>(perFrame);
        }
        int rayos = static_cast<int>(s.raysPerProbe);
        if (ImGui::SliderInt("Rayos por probe", &rayos,
                             static_cast<int>(renderer::kMinRaysPerProbe),
                             static_cast<int>(renderer::kMaxRaysPerProbe))) {
            s.raysPerProbe = static_cast<u32>(rayos);
        }
        // Tope de la histeresis: cada probe sube hasta aca como promedio progresivo.
        ImGui::SliderFloat("Histeresis max", &s.hysteresis, 0.0f, 0.995f, "%.3f");
        ImGui::SliderFloat("Umbral de cambio", &s.irradianceThreshold, 0.0f, 1.0f, "%.2f");
        ImGui::SliderFloat("Umbral de brillo", &s.brightnessThreshold, 0.0f, 10.0f, "%.2f");
        ImGui::Checkbox("Congelar (freeze)", &s.freeze);
        ImGui::SameLine();
        if (ImGui::Button("Reset")) pass.Reset();

        const u32 total = renderer::ProbeCount(s.grid);
        const u32 framesPorBarrido = (s.probeBudget > 0u)
                                   ? (total + s.probeBudget - 1u) / s.probeBudget
                                   : 0u;
        ImGui::TextDisabled("cursor %u / %u   barridos %u", pass.cursor(), total, pass.sweepsDone());
        ImGui::TextDisabled("frames por barrido: %u", framesPorBarrido);
        // Tiempo de CPU emitiendo las llamadas, no de GPU ejecutandolas: sirve
        // para detectar que el round-robin se fue de escala, no como profiler.
        ImGui::TextDisabled("pase (CPU): %.3f ms", pass.lastMs());

        const u32 porFrame = s.freeze ? 0u : std::min(s.probeBudget, renderer::kMaxProbesPerFrame);
        const u32 probesDelFrame = std::min(porFrame, total);
        const u32 rayosPorProbe  = std::clamp(s.raysPerProbe, renderer::kMinRaysPerProbe,
                                              renderer::kMaxRaysPerProbe);
        ImGui::TextDisabled("rayos por frame: %u (%u probes x %u rayos)",
                            probesDelFrame * rayosPorProbe, probesDelFrame, rayosPorProbe);

        // -- Voxeles -----------------------------------------------------------
        ImGui::SeparatorText("Voxeles");
        // La voxelizacion NO corre por frame: se hornea una vez y sus numeros
        // son los de esa unica corrida.
        if (ImGui::Button("Revoxelizar", ImVec2(-kAnchoEtiqueta, 0.0f))) {
            pass.Voxelize(ctx.scene);
        }
        if (pass.gridValido()) {
            const renderer::VoxelGridDesc& vd = pass.voxelGrid().desc();
            ImGui::TextDisabled("resolucion: %u^3   voxel: %.3f m", vd.resolution, vd.voxelSize);
            ImGui::TextDisabled("extension: %.1f m por lado", renderer::GridExtent(vd));
            ImGui::TextDisabled("memoria: %.1f MB",
                                f64(pass.voxelGrid().memoryBytes()) / (1024.0 * 1024.0));
            ImGui::TextDisabled("ultima voxelizacion: %.1f ms, %u draws",
                                pass.voxelizeMs(), pass.voxelizeDraws());
        } else {
            ImGui::TextColored(kColorAviso, "grid sin hornear: la captura no traza nada.");
        }

        // -- Sampleo -----------------------------------------------------------
        ImGui::SeparatorText("Sampleo");
        ImGui::SliderFloat("Intensidad", &s.intensity, 0.0f, 4.0f);
        // Normal bias: subir si la luz atraviesa las paredes; bajar si los
        // rincones tienen una banda oscura.
        ImGui::SliderFloat("Normal bias", &s.normalBias, 0.0f, 1.0f, "%.3f m");
        ImGui::SliderFloat("View bias", &s.viewBias, 0.0f, 1.0f, "%.3f m");
        ImGui::SliderFloat("Chebyshev", &s.chebyshevSharpness, 1.0f, 16.0f);

        ImGui::TextDisabled("clamp de distancia: %.2f m (1,5 x diagonal de celda)",
                            renderer::DistanceClamp(s.grid));

        // Alfa minimo para que el DDA cuente un voxel como solido. En 0 la
        // condicion se cumple en el aire y el rayo muere en el primer voxel.
        // Tope 0.7: los voxeles de doble cara tienen alfa 0.75 y por encima se vuelven aire.
        ImGui::SliderFloat("Umbral de opacidad", &s.opacityThreshold, 0.0f, 0.7f, "%.2f",
                           ImGuiSliderFlags_AlwaysClamp);

        // -- Clasificacion y reubicacion -----------------------------------------
        ImGui::SeparatorText("Clasificacion y reubicacion");
        ImGui::Checkbox("Clasificacion", &s.classificationEnabled);
        ImGui::SameLine();
        ImGui::Checkbox("Reubicacion", &s.relocationEnabled);
        ImGui::SliderFloat("Backface: inicio", &s.backfaceFadeStart, 0.0f, 1.0f, "%.2f",
                           ImGuiSliderFlags_AlwaysClamp);
        ImGui::SliderFloat("Backface: fin",    &s.backfaceFadeEnd,   0.0f, 1.0f, "%.2f",
                           ImGuiSliderFlags_AlwaysClamp);
        ImGui::SliderFloat("Distancia minima", &s.minFrontfaceDistance, 0.0f, 5.0f, "%.2f m",
                           ImGuiSliderFlags_AlwaysClamp);
        ImGui::TextDisabled("Con las esferas de debug: rojas = inactivas, dibujadas donde quedaron.");

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
        const char* modos[] = { "Irradiancia", "Media de distancia", "Buffer de rayos",
                                "Buffer de rayos (distancia)" };
        int modo = static_cast<int>(s.debugMode);
        if (ImGui::Combo("Modo", &modo, modos, 4)) s.debugMode = static_cast<u32>(modo);
        ImGui::SliderFloat("Radio de esfera", &s.debugRadius, 0.02f, 0.5f, "%.3f m");
    }

}
