#include "panels/PassPanel.h"
#include "panels/PanelUI.h"

#include "EditorUI.h"

#include <efengine/application/Application.h>
#include <efengine/renderer/ScenePipeline.h>
#include <efengine/renderer/ShadowPass.h>

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

                // El flag de encendido es del pase (IScenePass::enabled), no de sus
                // settings: es lo que el ScenePipeline consulta para saltearlo.
                ImGui::Checkbox   ("Habilitadas", &pase.enabled);
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
    }

}
