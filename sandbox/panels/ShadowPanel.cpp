#include "panels/PassPanel.h"
#include "panels/PanelUI.h"

#include "EditorUI.h"

#include <efengine/application/Application.h>
#include <efengine/renderer/ScenePipeline.h>
#include <efengine/renderer/ShadowPass.h>
#include <efengine/renderer/CascadedShadowPass.h>

#include <imgui.h>

namespace sandbox {

// Igual que EditorUI.cpp: los paneles nombran los tipos del motor como
// renderer::X, sin el efengine:: adelante.
using namespace efengine;

    void dibujarPanelSombras(EditorContext& ctx) {
            if (ImGui::CollapsingHeader("Sombras", ImGuiTreeNodeFlags_DefaultOpen)) {
                renderer::ShadowPass* pasePtr = ctx.app.GetPipeline().Find<renderer::ShadowPass>();
                if (pasePtr == null) return;
                renderer::ShadowPass&     pase = *pasePtr;
                renderer::ShadowSettings& sh   = pase.settings();

                // Sin checkbox: el pase sigue a "Habilitado" del panel de DDGI.
                ImGui::TextDisabled("Sigue a DDGI: %s", pase.Runs() ? "dibuja" : "apagado");
                // Margen: aire alrededor de la escena. El encuadre de la luz sale de
                // sus bounds y esto es lo unico a mano. Mas margen = texel mas
                // grande = mas acne.
                ImGui::SliderFloat("Margen",      &sh.padding, 0.0f, 10.0f, "%.2f m");
                // Sirve para medir: si un artefacto se afina a la mitad al duplicar
                // la resolucion, escala con el texel y es del shadow map.
                const char* resoluciones[] = { "512", "1024", "2048", "4096" };
                const u32   valores[]      = { 512u,  1024u,  2048u,  4096u  };
                int resSel = 2;
                for (int i = 0; i < IM_ARRAYSIZE(valores); ++i)
                    if (valores[i] == sh.resolution) resSel = i;
                if (ImGui::Combo("Resolucion", &resSel, resoluciones, IM_ARRAYSIZE(resoluciones))) {
                    sh.resolution = valores[resSel];
                }

                // El mecanismo principal contra el acne (lineas oscuras en zonas
                // iluminadas): subirlo. NO abre luz en los rincones; el techo es la
                // geometria fina, que empieza a filtrar.
                ImGui::SliderFloat("Normal offset", &sh.normalOffsetTexels, 0.0f, 8.0f, "%.1f texels");

                // Los dos bias son la escotilla: empujan la profundidad hacia la luz,
                // asi que cualquier valor > 0 abre una banda de luz en las aristas
                // entre paredes. Dejarlos en 0.
                ImGui::SliderFloat("Bias min", &sh.biasMin, 0.0f, 0.01f, "%.4f");
                ImGui::SliderFloat("Bias max", &sh.biasMax, 0.0f, 0.02f, "%.4f");

                // Los dos bias son fracciones de profundidad NDC, que no quiere
                // decir nada solo. Lo que se tunea de verdad es cuantos texels de
                // holgura son, asi que se muestra la conversion.
                const renderer::DirectionalLightFit& fit = pase.fit();
                const f32 texel = (pase.resolution() > 0)
                                ? 2.0f * fit.orthoHalfSize / static_cast<f32>(pase.resolution())
                                : 0.0f;
                ImGui::TextDisabled("Encuadre: half %.1f m | rango %.1f m | texel %.1f mm",
                                    fit.orthoHalfSize, fit.depthRange, texel * 1000.0f);
                ImGui::TextDisabled("Normal offset = %.1f mm | Bias Max = %.1f mm",
                                    sh.normalOffsetTexels * texel * 1000.0f,
                                    sh.biasMax * fit.depthRange * 1000.0f);
            }

            if (ImGui::CollapsingHeader("Cascadas", ImGuiTreeNodeFlags_DefaultOpen)) {
                renderer::CascadedShadowPass* csPtr =
                    ctx.app.GetPipeline().Find<renderer::CascadedShadowPass>();
                if (csPtr == null) return;
                renderer::CascadedShadowPass& cascadas = *csPtr;
                renderer::CascadeSettings&    cs       = cascadas.settings();

                ImGui::Checkbox("Activas", &cascadas.enabled);

                int count = (int)cs.count;
                if (ImGui::SliderInt("Cantidad", &count, 1, (int)renderer::kMaxCascades)) {
                    cs.count = (u32)count;
                }
                ImGui::SliderFloat("Distancia", &cs.shadowDistance, 20.0f, 1000.0f, "%.0f m");
                // 0 = uniforme, 1 = logaritmico. Lo unico que decide cuanta
                // resolucion se lleva la cascada de cerca.
                ImGui::SliderFloat("Lambda", &cs.lambda, 0.0f, 1.0f, "%.2f");
                // Si una sombra aparece de golpe al acercarse a un edificio alto,
                // es esto lo que hay que subir.
                ImGui::SliderFloat("Extension hacia la luz", &cs.lightExtension, 0.0f, 300.0f, "%.0f m");
                ImGui::SliderFloat("Normal offset##csm", &cs.normalOffsetTexels, 0.0f, 8.0f, "%.1f texels");
                ImGui::SliderFloat("Banda de transicion", &cs.blendRatio, 0.0f, 0.5f, "%.2f");
                ImGui::Checkbox("Vista de debug", &cs.debugView);

                // La tabla es lo que convierte el tuneo en lectura: con el texel en
                // metros a la vista se sabe si el problema es el reparto o el bias,
                // en vez de mover sliders hasta que algo mejore.
                const renderer::CascadeContext& cc = cascadas.context();
                if (ImGui::BeginTable("cascadas", 3, ImGuiTableFlags_Borders)) {
                    ImGui::TableSetupColumn("#");
                    ImGui::TableSetupColumn("Hasta (m)");
                    ImGui::TableSetupColumn("Texel (mm)");
                    ImGui::TableHeadersRow();
                    for (u32 i = 0; i < cc.count; ++i) {
                        ImGui::TableNextRow();
                        ImGui::TableNextColumn(); ImGui::Text("%u", i);
                        ImGui::TableNextColumn(); ImGui::Text("%.1f", cc.fits[i].splitFar);
                        ImGui::TableNextColumn(); ImGui::Text("%.1f", cc.fits[i].texelWorldSize * 1000.0f);
                    }
                    ImGui::EndTable();
                }
            }
    }

}
