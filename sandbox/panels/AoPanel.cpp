#include "panels/PassPanel.h"
#include "panels/PanelUI.h"

#include "EditorUI.h"

#include <efengine/application/Application.h>
#include <efengine/renderer/ScenePipeline.h>
#include <efengine/renderer/AoPass.h>
#include <efengine/renderer/AoSettings.h>
#include <efengine/renderer/DdgiPass.h>
#include <efengine/renderer/DdgiSettings.h>
#include <efengine/renderer/Texture.h>
#include <glm/glm.hpp>

#include <imgui.h>

namespace sandbox {

// Igual que EditorUI.cpp: los paneles nombran los tipos del motor como
// renderer::X, sin el efengine:: adelante.
using namespace efengine;

    void dibujarPanelAo(EditorContext& ctx) {
        if (!ImGui::CollapsingHeader("Oclusion ambiental (GTAO)")) return;

        renderer::AoPass* opt = ctx.app.GetPipeline().Find<renderer::AoPass>();
        if (opt == null) {
            ImGui::TextColored(kColorError, "AoPass no disponible: fallo la carga de shaders.");
            ImGui::TextWrapped("La escena esta sin oclusion de contacto. Mira la consola.");
            return;
        }
        renderer::AoSettings& s = opt->settings();

        CamposAlineados alineados;

        // El flag de encendido es del pase (IScenePass::enabled), no de sus
        // settings: es lo que el ScenePipeline consulta para saltearlo.
        ImGui::Checkbox("Habilitado", &opt->enabled);

        ImGui::SeparatorText("Trazado");
        // El radio va en METROS y tiene que quedar POR DEBAJO del espaciado de
        // probes de DDGI: lo que el AO ocluye es exactamente lo que la grilla no
        // puede ver. Por encima, los dos oscurecen la misma cosa. Radio 0 =
        // control nulo, la imagen tiene que volver a ser la de AO apagado.
        ImGui::SliderFloat("Radio", &s.radius, 0.0f, 3.0f, "%.2f m");

        // El espaciado de DDGI al lado del slider: la regla "radio < espaciado"
        // no se puede verificar de otra forma desde el panel.
        renderer::DdgiPass* ddgi = ctx.app.GetPipeline().Find<renderer::DdgiPass>();
        if (ddgi != null) {
            const glm::vec3& sp = ddgi->settings().grid.spacing;
            const f32 minSp = glm::min(sp.x, glm::min(sp.y, sp.z));
            if (s.radius >= minSp) {
                ImGui::TextColored(kColorAviso,
                                   "Radio >= espaciado de probes (%.2f m): doble oscurecimiento.", minSp);
            } else {
                ImGui::TextDisabled("espaciado de probes mas chico: %.2f m", minSp);
            }
        }

        ImGui::SliderFloat("Intensidad", &s.intensity, 0.0f, 4.0f);
        // Grosor: que tan rapido se desvanece una muestra lejana. Bajarlo hace
        // que un objeto lejano alineado en pantalla deje de ocluir a uno cercano.
        ImGui::SliderFloat("Grosor",     &s.thickness, 0.01f, 1.0f);
        ImGui::SliderInt  ("Cortes",     &s.slices, 1, 8);
        ImGui::SliderInt  ("Pasos",      &s.steps,  1, 32);
        ImGui::SliderFloat("Techo de radio", &s.maxScreenRadius, 8.0f, 512.0f, "%.0f px");

        ImGui::SeparatorText("Rendimiento");
        // Mismo criterio que el toggle de la indirecta: el plan pide medir cada
        // cambio por separado, y sin un interruptor en caliente comparar "con" y
        // "sin" pide recompilar.
        ImGui::Checkbox("Media resolucion", &s.halfRes);
        ImGui::TextDisabled("kernel y blur a %dx%d; prepass y guia siguen a %dx%d",
                            opt->aoTexture().width(),  opt->aoTexture().height(),
                            opt->normalTarget().width(), opt->normalTarget().height());
        if (s.halfRes) {
            ImGui::TextDisabled("pbr.frag lo sube con upsample bilateral (depth + normal)");
        }

        ImGui::SeparatorText("Aplicacion");
        // Bent normal: orienta el lookup de irradiancia (IBL y DDGI) hacia donde
        // el hemisferio esta abierto. En un rincon de Cornell cambia la DIRECCION
        // del color bleeding.
        ImGui::Checkbox("Bent normal",  &s.bentNormal);
        // Multi-rebote: el AO se tiñe con el albedo en vez de oscurecer a gris.
        // Sobre la pared roja la diferencia es directa.
        ImGui::Checkbox("Multi-rebote", &s.multiBounce);
        ImGui::Checkbox("Blur", &s.blur);

        ImGui::SeparatorText("Debug");
        const char* vistas[] = { "Final (normal)",
                                 "Visibilidad",
                                 "Bent normal (world)",
                                 "Normal del prepass (view)",
                                 "Profundidad (1 banda = 1 m)" };
        // Las dos ultimas vuelcan el prepass y saltean el blur. Si esta vista y
        // la de DDGI estan las dos activas, gana esta.
        int vista = static_cast<int>(s.debugView);
        if (ImGui::Combo("Vista AO", &vista, vistas, IM_ARRAYSIZE(vistas))) {
            s.debugView = static_cast<u32>(vista);
        }
        if (s.debugView != 0u) {
            ImGui::TextColored(kColorAviso, "Vista de debug activa: la imagen NO es la final.");
        }
    }

}
