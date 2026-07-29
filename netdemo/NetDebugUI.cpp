#include "NetDebugUI.h"

#include <efenet/Client.h>

#include <imgui.h>

#include <array>

namespace netdemo {

namespace {
    // Historial del error de reconciliacion. Es LA metrica de salud: si se
    // mantiene en cero, Step() esta realmente compartido entre cliente y
    // servidor. Si sube, hay una diferencia entre los dos lados.
    constexpr usize kErrorHistory = 180u;   // ~3 s a 60 fps

    std::array<f32, kErrorHistory> g_errorPlot {};
    usize g_errorCursor = 0u;

    f32 g_plotAcc = 0.0f;
}

    void DrawNetDebugUI(efenet::Client& client, f32 dt) {
        const efenet::Client::Stats& stats = client.GetStats();

        // Muestrear a ~60 Hz aunque el render vaya mas rapido, para que el
        // grafico represente tiempo y no frames.
        g_plotAcc += dt;
        while (g_plotAcc >= efenet::kTickDt) {
            g_errorPlot[g_errorCursor] = stats.lastReconcileError;
            g_errorCursor = (g_errorCursor + 1u) % kErrorHistory;
            g_plotAcc -= efenet::kTickDt;
        }

        ImGui::Begin("efenet");

        // ESTADO
        if (client.Connected()) {
            ImGui::TextColored(ImVec4(0.4f, 0.9f, 0.4f, 1.0f), "Conectado");
            ImGui::SameLine();
            ImGui::Text("| jugador %u", client.MyId());
        } else {
            ImGui::TextColored(ImVec4(0.9f, 0.4f, 0.4f, 1.0f), "Desconectado");
        }

        ImGui::Separator();

        // METRICAS
        if (ImGui::CollapsingHeader("Metricas", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::Text("RTT:                %u ms", stats.rttMs);
            ImGui::Text("Entrada:            %.1f KB/s", stats.bytesInPerSec / 1024.0f);
            ImGui::Text("Salida:             %.1f KB/s", stats.bytesOutPerSec / 1024.0f);
            ImGui::Text("Snapshots:          %u", stats.snapshotsReceived);
            ImGui::Text("Descartados:        %u", stats.snapshotsDiscarded);
            ImGui::Text("Inputs sin confirmar: %u", static_cast<u32>(stats.pendingInputs));
            ImGui::Text("Tiempo servidor:    %.2f s", client.ServerTime());

            ImGui::Spacing();
            ImGui::Text("Error de reconciliacion: %.4f u", stats.lastReconcileError);
            ImGui::PlotLines("##errorRecon", g_errorPlot.data(),
                             static_cast<int>(kErrorHistory),
                             static_cast<int>(g_errorCursor),
                             null, 0.0f, 0.5f, ImVec2(0.0f, 60.0f));
            ImGui::TextDisabled("Cerca de 0 = cliente y servidor coinciden.");
        }

        // SIMULADOR DE RED
        if (ImGui::CollapsingHeader("Simulador de red", ImGuiTreeNodeFlags_DefaultOpen)) {
            efenet::NetSimSettings& sim = client.netSim().settings;

            ImGui::SliderFloat("Latencia (ms)", &sim.latencyMs, 0.0f, 500.0f, "%.0f");
            ImGui::SliderFloat("Jitter (ms)",   &sim.jitterMs,  0.0f, 100.0f, "%.0f");
            ImGui::SliderFloat("Perdida (%)",   &sim.lossPct,   0.0f,  30.0f, "%.1f");

            ImGui::Text("En vuelo: %u salida / %u entrada",
                        static_cast<u32>(client.netSim().PendingOutbound()),
                        static_cast<u32>(client.netSim().PendingInbound()));
            ImGui::Text("Descartados por el simulador: %u", client.netSim().Dropped());

            if (ImGui::Button("Sin lag")) {
                sim.latencyMs = 0.0f;
                sim.jitterMs  = 0.0f;
                sim.lossPct   = 0.0f;
            }
            ImGui::SameLine();
            if (ImGui::Button("Red mala")) {
                sim.latencyMs = 200.0f;
                sim.jitterMs  = 40.0f;
                sim.lossPct   = 10.0f;
            }
        }

        // TOGGLES: la parte didactica
        if (ImGui::CollapsingHeader("Compensacion de latencia", ImGuiTreeNodeFlags_DefaultOpen)) {
            efenet::Client::Toggles& t = client.toggles();

            ImGui::Checkbox("Prediccion", &t.prediction);
            ImGui::TextDisabled("  Off: tu avatar se mueve recien cuando llega");
            ImGui::TextDisabled("  el snapshot. Se siente el RTT completo.");

            ImGui::Checkbox("Reconciliacion", &t.reconciliation);
            ImGui::TextDisabled("  Off: con prediccion sola, derivas del");
            ImGui::TextDisabled("  servidor hasta quedar en otro lado.");

            ImGui::Checkbox("Interpolacion", &t.interpolation);
            ImGui::TextDisabled("  Off: los otros saltan 20 veces por segundo.");

            ImGui::Checkbox("Extrapolacion", &t.extrapolation);
            ImGui::TextDisabled("  Off: con perdida, los otros se congelan");
            ImGui::TextDisabled("  y arrancan a tirones.");
        }

        ImGui::End();
    }

}
