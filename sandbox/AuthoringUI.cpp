#include "AuthoringUI.h"

#include <efengine/renderer/Material.h>
#include <efengine/renderer/MaterialDef.h>
#include <efengine/renderer/Mesh.h>
#include <efengine/renderer/Model.h>
#include <efengine/resources/MaterialBuilder.h>
#include <efengine/resources/ResourceManager.h>
#include <efengine/resources/SceneAssets.h>
#include <efengine/scene/Node.h>
#include <efengine/scene/SceneGraph.h>

#include <imgui.h>
#include <imgui_stdlib.h>

#include <glm/gtc/type_ptr.hpp>

#include <algorithm>
#include <optional>
#include <string>
#include <vector>

namespace sandbox {

using namespace efengine;

namespace {

    constexpr const char* kDefaultMaterialName = "default";
    constexpr const char* kCheckerPath         = "assets/textures/default/checker.png";

    const ImVec4 kColorError { 1.0f, 0.4f, 0.4f, 1.0f };

    // Nombres de submesh DISTINTOS: dos submeshes pueden compartir
    // materialName() y el MaterialMap los unifica en una sola entrada.
    std::vector<std::string> nombresDeSubmesh(const renderer::Model& model) {
        std::vector<std::string> nombres;
        for (const renderer::Mesh& m : model.meshes()) {
            if (std::find(nombres.begin(), nombres.end(), m.materialName()) == nombres.end())
                nombres.push_back(m.materialName());
        }
        return nombres;
    }

    void adjuntarModelo(EditorContext& ctx, scene::NodeHandle handle, const std::string& path) {
        EditorState& st = ctx.state;
        st.meshError.clear();

        renderer::Model* modelo = ctx.rm.GetModel(path.c_str());
        if (modelo == nullptr) {
            st.meshError = "No se pudo cargar " + path;
            return;
        }

        const u32 mat = EnsureDefaultMaterial(ctx);
        if (mat == resources::SceneAssets::kInvalidIndex) {
            st.meshError = "No se pudo crear el material default";
            return;
        }

        ctx.scene.AttachMesh(handle, scene::MeshAttachment{
            modelo,
            renderer::MakeUniformMaterialMap(nombresDeSubmesh(*modelo), ctx.assets.MaterialAt(mat))
        });
    }

    void dibujarComboDeMaterial(EditorContext& ctx, scene::MeshAttachment& mesh,
                                const std::string& submesh) {
        const renderer::Material* actual = nullptr;
        const auto it = mesh.materials.find(submesh);
        if (it != mesh.materials.end()) actual = it->second;

        const u32 actualIdx = ctx.assets.IndexOfMaterial(actual);
        const char* etiqueta = (actualIdx == resources::SceneAssets::kInvalidIndex)
                             ? "(sin material)"
                             : ctx.assets.DefAt(actualIdx)->name.c_str();

        ImGui::PushID(submesh.c_str());
        if (ImGui::BeginCombo(submesh.c_str(), etiqueta)) {
            for (u32 i = 0u; i < ctx.assets.MaterialCount(); ++i) {
                ImGui::PushID((int)i);
                if (ImGui::Selectable(ctx.assets.DefAt(i)->name.c_str(), i == actualIdx))
                    mesh.materials[submesh] = ctx.assets.MaterialAt(i);
                ImGui::PopID();
            }
            ImGui::EndCombo();
        }
        ImGui::PopID();
    }

    struct ShaderPreset { const char* name; const char* vert; const char* frag; };

    constexpr ShaderPreset kShaders[] = {
        { "pbr",   "assets/shaders/pbr.vert",   "assets/shaders/pbr.frag"   },
        { "phong", "assets/shaders/phong.vert", "assets/shaders/phong.frag" },
    };

    // El color space va prefijado por slot (sRGB en lo que es color, Linear en
    // lo que es dato) pero queda editable por si hay que corregir una escena vieja.
    struct SlotInfo { renderer::TextureSlot slot; const char* label; renderer::ColorSpace space; };

    constexpr SlotInfo kSlots[] = {
        { renderer::TextureSlot::Albedo,    "Albedo",    renderer::ColorSpace::sRGB   },
        { renderer::TextureSlot::Normal,    "Normal",    renderer::ColorSpace::Linear },
        { renderer::TextureSlot::AO,        "AO",        renderer::ColorSpace::Linear },
        { renderer::TextureSlot::Roughness, "Roughness", renderer::ColorSpace::Linear },
        { renderer::TextureSlot::Metallic,  "Metallic",  renderer::ColorSpace::Linear },
        { renderer::TextureSlot::Height,    "Height",    renderer::ColorSpace::Linear },
        { renderer::TextureSlot::Opacity,   "Opacity",   renderer::ColorSpace::sRGB   },
        { renderer::TextureSlot::Emissive,  "Emissive",  renderer::ColorSpace::sRGB   },
    };

    renderer::TextureDef* buscarTextura(renderer::MaterialDef& def, renderer::TextureSlot slot) {
        for (renderer::TextureDef& t : def.textures) {
            if (t.slot == slot) return &t;
        }
        return nullptr;
    }

    void quitarTextura(renderer::MaterialDef& def, renderer::TextureSlot slot) {
        for (usize i = 0u; i < def.textures.size(); ++i) {
            if (def.textures[i].slot == slot) { def.textures.erase(def.textures.begin() + i); return; }
        }
    }

    bool dibujarComboDeShader(renderer::MaterialDef& def) {
        bool changed = false;
        if (ImGui::BeginCombo("Shader", def.shaderName.c_str())) {
            for (const ShaderPreset& p : kShaders) {
                if (ImGui::Selectable(p.name, def.shaderName == p.name)) {
                    def.shaderName = p.name;
                    def.vertPath   = p.vert;
                    def.fragPath   = p.frag;
                    changed = true;
                }
            }
            ImGui::EndCombo();
        }
        return changed;
    }

    bool dibujarSlotDeTextura(EditorContext& ctx, renderer::MaterialDef& def, const SlotInfo& info) {
        bool changed = false;
        renderer::TextureDef* tex = buscarTextura(def, info.slot);

        ImGui::PushID(info.label);
        if (ImGui::BeginCombo(info.label, tex ? tex->path.c_str() : "(ninguna)")) {
            if (ImGui::Selectable("(ninguna)", tex == nullptr) && tex != nullptr) {
                quitarTextura(def, info.slot);
                changed = true;
            }
            for (const std::string& path : ctx.state.catalog.Textures()) {
                const bool seleccionada = tex != nullptr && tex->path == path;
                if (ImGui::Selectable(path.c_str(), seleccionada)) {
                    if (tex != nullptr) tex->path = path;
                    else def.textures.push_back(renderer::TextureDef{ info.slot, path, info.space });
                    changed = true;
                }
            }
            ImGui::EndCombo();
        }

        // Re-buscar: si se agrego o borro un TextureDef, el puntero de arriba
        // apunta a un vector que se reallocó o se movió. Usarlo seria UB.
        tex = buscarTextura(def, info.slot);
        if (tex != nullptr) {
            ImGui::SameLine();
            bool srgb = tex->space == renderer::ColorSpace::sRGB;
            if (ImGui::Checkbox("sRGB", &srgb)) {
                tex->space = srgb ? renderer::ColorSpace::sRGB : renderer::ColorSpace::Linear;
                changed = true;
            }
        }
        ImGui::PopID();
        return changed;
    }

    // Reconstruye el material del slot activo desde el draft. Si falla, NO
    // aplica nada: el draft conserva la edicion para poder corregirla.
    void aplicarDraft(EditorContext& ctx) {
        EditorState& st = ctx.state;

        std::optional<renderer::Material> rebuilt = resources::BuildMaterial(st.materialDraft, ctx.rm);
        if (!rebuilt) {
            st.materialError = "No se pudo construir el material: el cambio no se aplico";
            return;
        }
        st.materialError.clear();
        ctx.assets.UpdateMaterial(st.activeMaterial, st.materialDraft, std::move(*rebuilt));
    }

    void crearMaterial(EditorContext& ctx, bool duplicandoElActivo) {
        EditorState& st = ctx.state;

        renderer::MaterialDef def;
        if (duplicandoElActivo && st.activeMaterial < ctx.assets.MaterialCount()) {
            def = *ctx.assets.DefAt(st.activeMaterial);
            def.name += "_copia";
        } else {
            def.name       = "material_" + std::to_string(ctx.assets.MaterialCount());
            def.shaderName = kShaders[0].name;
            def.vertPath   = kShaders[0].vert;
            def.fragPath   = kShaders[0].frag;
        }

        std::optional<renderer::Material> mat = resources::BuildMaterial(def, ctx.rm);
        if (!mat) {
            st.materialError = "No se pudo construir el material nuevo";
            return;
        }

        st.materialError.clear();
        st.activeMaterial = ctx.assets.AddMaterial(def, std::move(*mat));
        st.materialDraft  = def;
        st.draftFor       = st.activeMaterial;
    }

} // namespace

u32 EnsureDefaultMaterial(EditorContext& ctx) {
    // Busqueda por NOMBRE: es la unica del editor. Todo lo demas va por indice.
    for (u32 i = 0u; i < ctx.assets.MaterialCount(); ++i) {
        if (ctx.assets.DefAt(i)->name == kDefaultMaterialName) return i;
    }

    renderer::MaterialDef def;
    def.name       = kDefaultMaterialName;
    def.shaderName = "pbr";
    def.vertPath   = "assets/shaders/pbr.vert";
    def.fragPath   = "assets/shaders/pbr.frag";
    def.textures.push_back(renderer::TextureDef{
        renderer::TextureSlot::Albedo, kCheckerPath, renderer::ColorSpace::sRGB });
    def.metallic  = 0.0f;
    def.roughness = 0.6f;

    std::optional<renderer::Material> mat = resources::BuildMaterial(def, ctx.rm);
    if (!mat) return resources::SceneAssets::kInvalidIndex;

    return ctx.assets.AddMaterial(std::move(def), std::move(*mat));
}

void DrawMeshSection(EditorContext& ctx, scene::NodeHandle handle) {
    EditorState& st = ctx.state;
    if (!ctx.scene.IsValid(handle)) return;

    scene::Node& node = ctx.scene.Get(handle);

    ImGui::SeparatorText("Malla");

    if (node.mesh && node.mesh->model) {
        const std::string* path = ctx.rm.PathOf(node.mesh->model);
        ImGui::TextDisabled("%s", path ? path->c_str() : "(malla generada)");

        if (ImGui::Button("Quitar malla")) {
            ctx.scene.DetachMesh(handle);
            return;   // el adjunto ya no existe: no seguir dibujandolo este frame
        }
        if (path == nullptr) {
            ImGui::SameLine();
            ImGui::TextDisabled("(no se puede volver a adjuntar desde el editor)");
        }

        for (const std::string& submesh : nombresDeSubmesh(*node.mesh->model)) {
            dibujarComboDeMaterial(ctx, *node.mesh, submesh);
        }
        return;
    }

    // Sin malla: elegir un modelo del catalogo y adjuntarlo.
    st.catalog.EnsureScanned();
    const std::vector<std::string>& modelos = st.catalog.Models();

    if (modelos.empty()) {
        ImGui::TextDisabled("assets/models esta vacio");
    } else {
        if (st.modelPick >= (int)modelos.size()) st.modelPick = 0;
        if (ImGui::BeginCombo("Modelo", modelos[st.modelPick].c_str())) {
            for (int i = 0; i < (int)modelos.size(); ++i) {
                if (ImGui::Selectable(modelos[i].c_str(), i == st.modelPick)) st.modelPick = i;
            }
            ImGui::EndCombo();
        }
        if (ImGui::Button("Adjuntar")) adjuntarModelo(ctx, handle, modelos[st.modelPick]);
        ImGui::SameLine();
    }

    if (ImGui::SmallButton("Refrescar")) st.catalog.Rescan();
    if (!st.meshError.empty()) ImGui::TextColored(kColorError, "%s", st.meshError.c_str());
}

void DrawMaterialsPanel(EditorContext& ctx) {
    EditorState& st = ctx.state;

    if (!ImGui::Begin("Materiales", &st.showMaterials)) { ImGui::End(); return; }

    st.catalog.EnsureScanned();

    // El indice es la identidad (es lo que guardan los bindings del .efe); el
    // nombre es etiqueta y puede repetirse, por eso el ID de ImGui es el indice.
    const ImVec2 alto { -FLT_MIN, 6.0f * ImGui::GetTextLineHeightWithSpacing() };
    if (ImGui::BeginListBox("##materiales", alto)) {
        for (u32 i = 0u; i < ctx.assets.MaterialCount(); ++i) {
            ImGui::PushID((int)i);
            if (ImGui::Selectable(ctx.assets.DefAt(i)->name.c_str(), i == st.activeMaterial))
                st.activeMaterial = i;
            ImGui::PopID();
        }
        ImGui::EndListBox();
    }

    if (ImGui::Button("+ Nuevo")) crearMaterial(ctx, false);
    ImGui::SameLine();
    if (ImGui::Button("Duplicar")) crearMaterial(ctx, true);
    ImGui::SameLine();
    if (ImGui::SmallButton("Refrescar assets")) st.catalog.Rescan();

    if (st.activeMaterial >= ctx.assets.MaterialCount()) {
        ImGui::TextDisabled("Ningun material seleccionado");
        if (!st.materialError.empty()) ImGui::TextColored(kColorError, "%s", st.materialError.c_str());
        ImGui::End();
        return;
    }

    // El draft se recarga SOLO al cambiar de material: recargarlo cada frame
    // pisaria la edicion que todavia no se pudo aplicar.
    if (st.draftFor != st.activeMaterial) {
        st.materialDraft = *ctx.assets.DefAt(st.activeMaterial);
        st.draftFor      = st.activeMaterial;
        st.materialError.clear();
    }

    ImGui::SeparatorText(ctx.assets.DefAt(st.activeMaterial)->name.c_str());

    renderer::MaterialDef& def = st.materialDraft;
    bool changed = false;

    changed |= ImGui::InputText("Nombre", &def.name);
    changed |= dibujarComboDeShader(def);

    ImGui::SeparatorText("Texturas");
    for (const SlotInfo& info : kSlots) changed |= dibujarSlotDeTextura(ctx, def, info);

    ImGui::SeparatorText("Escalares");
    changed |= ImGui::ColorEdit3 ("Tint",      glm::value_ptr(def.albedoTint));
    changed |= ImGui::SliderFloat("Metallic",  &def.metallic,    0.0f,  1.0f);
    changed |= ImGui::SliderFloat("Roughness", &def.roughness,   0.02f, 1.0f);
    changed |= ImGui::SliderFloat("AO",        &def.aoStrength,  0.0f,  1.0f);
    changed |= ImGui::SliderFloat("Height",    &def.heightScale, 0.0f,  0.2f);
    changed |= ImGui::SliderFloat("Alpha cut", &def.alphaCutoff, 0.0f,  1.0f);
    changed |= ImGui::SliderFloat("Normal str", &def.normalStrength, 0.0f, 2.0f);

    ImGui::SeparatorText("Emision");
    changed |= ImGui::ColorEdit3 ("Emis tint", glm::value_ptr(def.emissiveTint));
    // Rango HDR: para que el bloom lo agarre, la emision tiene que pasar el
    // threshold del brightpass, que trabaja sobre radiancia lineal sin tonemapear.
    changed |= ImGui::SliderFloat("Emis int",  &def.emissiveIntensity, 0.0f, 20.0f);

    if (changed) aplicarDraft(ctx);

    if (!st.materialError.empty()) ImGui::TextColored(kColorError, "%s", st.materialError.c_str());
    ImGui::TextDisabled("Un material es de la escena: editarlo cambia todos los nodos que lo usan.");

    ImGui::End();
}

} // namespace sandbox
