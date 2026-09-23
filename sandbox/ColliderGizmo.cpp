#include "ColliderGizmo.h"

#include "EditorUI.h"

#include <efengine/math/Math.h>
#include <efengine/math/Transform.h>
#include <efengine/renderer/Bounds.h>
#include <efengine/renderer/Model.h>
#include <efengine/scene/Camera.h>
#include <efengine/scene/SceneGraph.h>

#include <imgui.h>
#include <imgui_internal.h>   // DockBuilderGetCentralNode

#include <cmath>
#include <optional>

namespace sandbox {

using namespace efengine;

namespace {
    // Todo lo que hace falta para pintar una arista, atado una vez por frame.
    struct Lienzo {
        ImDrawList*       dl;
        glm::mat4         viewProj;
        math::ScreenRect  rect;
    };

    ImU32 colorDe(scene::MotionType m) {
        switch (m) {
            case scene::MotionType::Kinematic: return IM_COL32(120, 200, 255, 255);
            case scene::MotionType::Dynamic:   return IM_COL32(255, 170,  80, 255);
            case scene::MotionType::Static:    break;
        }
        return IM_COL32(120, 230, 120, 255);
    }

    // 'a' y 'b' van en espacio de la forma; 'm' los lleva a mundo. Dos pasadas,
    // negro grueso y color fino, para que se lea sobre cualquier fondo.
    void arista(const Lienzo& l, const glm::mat4& m, const glm::vec3& a, const glm::vec3& b,
                ImU32 color) {
        const std::optional<glm::vec2> pa =
            math::ProjectToScreen(l.viewProj, glm::vec3(m * glm::vec4(a, 1.0f)), l.rect);
        const std::optional<glm::vec2> pb =
            math::ProjectToScreen(l.viewProj, glm::vec3(m * glm::vec4(b, 1.0f)), l.rect);
        if (!pa || !pb) return;

        const ImVec2 va { pa->x, pa->y };
        const ImVec2 vb { pb->x, pb->y };
        l.dl->AddLine(va, vb, IM_COL32(0, 0, 0, 200), 3.0f);
        l.dl->AddLine(va, vb, color, 1.5f);
    }

    void caja(const Lienzo& l, const glm::mat4& m, const glm::vec3& centro, const glm::vec3& half,
              ImU32 color) {
        const glm::vec3 e[8] = {
            centro + glm::vec3(-half.x, -half.y, -half.z),
            centro + glm::vec3( half.x, -half.y, -half.z),
            centro + glm::vec3( half.x, -half.y,  half.z),
            centro + glm::vec3(-half.x, -half.y,  half.z),
            centro + glm::vec3(-half.x,  half.y, -half.z),
            centro + glm::vec3( half.x,  half.y, -half.z),
            centro + glm::vec3( half.x,  half.y,  half.z),
            centro + glm::vec3(-half.x,  half.y,  half.z),
        };
        const int aristas[12][2] = {
            {0,1},{1,2},{2,3},{3,0},
            {4,5},{5,6},{6,7},{7,4},
            {0,4},{1,5},{2,6},{3,7},
        };
        for (const int (&ar)[2] : aristas) arista(l, m, e[ar[0]], e[ar[1]], color);
    }

    // ejeNormal: 0 = el circulo vive en YZ, 1 = en XZ, 2 = en XY.
    void circulo(const Lienzo& l, const glm::mat4& m, const glm::vec3& centro, f32 radio,
                 i32 ejeNormal, ImU32 color) {
        constexpr i32 kSegmentos = 24;

        glm::vec3 previo { 0.0f };
        for (i32 i = 0; i <= kSegmentos; ++i) {
            const f32 a = TAU * static_cast<f32>(i) / static_cast<f32>(kSegmentos);
            const f32 c = std::cos(a) * radio;
            const f32 s = std::sin(a) * radio;

            glm::vec3 p = centro;
            if (ejeNormal == 0)      p += glm::vec3(0.0f, c, s);
            else if (ejeNormal == 1) p += glm::vec3(c, 0.0f, s);
            else                     p += glm::vec3(c, s, 0.0f);

            if (i > 0) arista(l, m, previo, p, color);
            previo = p;
        }
    }

    void dibujarNodo(const Lienzo& l, const scene::SceneGraph& scene, const scene::Node& node) {
        const scene::ColliderAttachment& col = *node.collider;
        const glm::mat4 m = scene.WorldMatrixOf(node.self) * col.localOffset.Matrix();
        const ImU32 color = colorDe(col.motion);

        switch (col.kind) {
            case scene::ShapeKind::Box:
                caja(l, m, glm::vec3(0.0f), col.params, color);
                break;

            case scene::ShapeKind::Sphere:
                circulo(l, m, glm::vec3(0.0f), col.params.x, 0, color);
                circulo(l, m, glm::vec3(0.0f), col.params.x, 1, color);
                circulo(l, m, glm::vec3(0.0f), col.params.x, 2, color);
                break;

            case scene::ShapeKind::Capsule: {
                const f32 r = col.params.x;
                const f32 h = col.params.y;
                circulo(l, m, glm::vec3(0.0f,  h, 0.0f), r, 1, color);
                circulo(l, m, glm::vec3(0.0f, -h, 0.0f), r, 1, color);
                arista(l, m, glm::vec3( r,  h, 0.0f), glm::vec3( r, -h, 0.0f), color);
                arista(l, m, glm::vec3(-r,  h, 0.0f), glm::vec3(-r, -h, 0.0f), color);
                arista(l, m, glm::vec3(0.0f,  h,  r), glm::vec3(0.0f, -h,  r), color);
                arista(l, m, glm::vec3(0.0f,  h, -r), glm::vec3(0.0f, -h, -r), color);
                break;
            }

            case scene::ShapeKind::Mesh: {
                if (!node.mesh || node.mesh->model == null) break;

                const renderer::AABB& b = node.mesh->model->bounds();
                if (!b.Valid()) break;
                // La AABB es local a la malla: su centro no tiene por que estar
                // en el origen del nodo.
                caja(l, m, b.Center(), b.Extents(), color);
                break;
            }
        }
    }

    void dibujarSubarbol(const Lienzo& l, const scene::SceneGraph& scene, scene::NodeHandle handle) {
        const scene::Node* node = scene.TryGet(handle);
        if (node == null) return;

        if (node->collider) dibujarNodo(l, scene, *node);
        for (const scene::NodeHandle h : node->children) dibujarSubarbol(l, scene, h);
    }
}

void DrawColliderGizmos(EditorContext& ctx, ImGuiID dockId) {
    if (!ctx.state.showColliders) return;

    const ImGuiViewport* vp      = ImGui::GetMainViewport();
    const ImGuiDockNode* central = ImGui::DockBuilderGetCentralNode(dockId);
    const ImVec2 pos  = central ? central->Pos  : vp->WorkPos;
    const ImVec2 size = central ? central->Size : vp->WorkSize;
    if (size.x <= 0.0f || size.y <= 0.0f) return;

    Lienzo l;
    l.dl       = ImGui::GetForegroundDrawList();
    l.viewProj = ctx.camera.ProjectionMatrix() * ctx.camera.ViewMatrix();
    l.rect     = math::ScreenRect { pos.x, pos.y, size.x, size.y };

    l.dl->PushClipRect(pos, ImVec2(pos.x + size.x, pos.y + size.y), true);
    dibujarSubarbol(l, ctx.scene, ctx.scene.Root());
    l.dl->PopClipRect();
}

} // namespace sandbox
