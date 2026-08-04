#include "efengine/serialization/SceneSerializer.h"
#include <efengine/core/Assert.h>
#include <efengine/core/Log.h>
#include <efengine/resources/FileIO.h>
#include <efengine/resources/MaterialBuilder.h>

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <fstream>
#include <utility>

namespace efengine {
namespace serialization {

    // Los dos sentinelas tienen que valer lo mismo; resources no puede incluir el
    // header de serialization (seria un ciclo de dependencias), asi que se chequea aca.
    static_assert(resources::SceneAssets::kInvalidIndex == kInvalidIndex,
                  "SceneAssets::kInvalidIndex y serialization::kInvalidIndex divergieron");

    namespace {

        struct ExtractCtx {
            const scene::SceneGraph&          graph;
            const resources::SceneAssets&     assets;
            const resources::ResourceManager& rm;
            const SceneRegistry&              reg;
            SceneDocument&                    doc;
            u32                               sunFileIndex = kInvalidIndex;
        };

        // Un loop sobre el registro, sin un 'if' por tipo de adjunto. Que sabe
        // hacer cada componente vive en su SerializeComponent, no aca.
        void extractComponents(ExtractCtx& ctx, const scene::Node& node,
                               scene::NodeHandle handle, NodeRecord& rec) {
            for (const ComponentRegistry::Entry& entry : ctx.reg.components.All()) {
                // Un writer nuevo por componente: el payload va como blob con
                // largo, igual que el de un behavior.
                BinaryWriter    payload;
                ComponentWriter ar{ payload, ctx.doc.strings, ctx.assets, ctx.rm,
                                    node.name.c_str() };

                if (!entry.write(ar, ctx.graph.Components(), handle.index)) continue;

                ComponentRecord cr;
                cr.typeNameStr = ctx.doc.strings.Intern(entry.name);
                cr.payload     = payload.Take();
                rec.components.push_back(std::move(cr));
            }
        }

        void extractBehaviors(ExtractCtx& ctx, const scene::Node& node, NodeRecord& rec) {
            for (const std::unique_ptr<scene::Behavior>& b : node.behaviors) {
                if (!b) continue;

                const BehaviorRegistry::Entry* entry = ctx.reg.behaviors.FindByType(*b);
                if (entry == null) {
                    EF_LOG_WARNING("SceneSerializer: behavior de tipo no registrado en el "
                                   "nodo '%s', se omite", node.name.c_str());
                    continue;
                }

                BehaviorRecord br;
                br.typeNameStr = ctx.doc.strings.Intern(entry->name);
                br.enabled     = b->enabled ? 1u : 0u;

                BinaryWriter payloadWriter;
                // unique_ptr tiene const superficial: aunque 'b' sea const, *b es Behavior&.
                entry->write(payloadWriter, *b);
                br.payload = payloadWriter.Take();

                rec.behaviors.push_back(std::move(br));
            }
        }

        // Devuelve el indice de archivo del nodo que acaba de escribir.
        u32 extractNode(ExtractCtx& ctx, scene::NodeHandle handle, u32 parentFileIndex) {
            const scene::Node& node = ctx.graph.Get(handle);

            NodeRecord rec;
            rec.nameStr = ctx.doc.strings.Intern(node.name);
            rec.parent  = parentFileIndex;
            rec.local   = node.local;

            extractComponents(ctx, node, handle, rec);
            extractBehaviors(ctx, node, rec);

            // Se empuja ANTES de recursar: eso es lo que garantiza el pre-orden.
            const u32 myIndex = static_cast<u32>(ctx.doc.nodes.size());
            ctx.doc.nodes.push_back(std::move(rec));

            if (handle == ctx.graph.PrimarySun()) ctx.sunFileIndex = myIndex;

            for (scene::NodeHandle child : node.children) {
                if (ctx.graph.IsValid(child)) extractNode(ctx, child, myIndex);
            }
            return myIndex;
        }

        MaterialRecord materialDefToRecord(const renderer::MaterialDef& def, StringTable& strings) {
            MaterialRecord r;
            r.nameStr       = strings.Intern(def.name);
            r.shaderNameStr = strings.Intern(def.shaderName);
            r.vertPathStr   = strings.Intern(def.vertPath);
            r.fragPathStr   = strings.Intern(def.fragPath);
            for (const renderer::TextureDef& t : def.textures) {
                r.textures.push_back(TextureRef{
                    static_cast<u32>(t.slot),
                    strings.Intern(t.path),
                    static_cast<u32>(t.space) });
            }
            r.albedoTint  = def.albedoTint;
            r.metallic    = def.metallic;
            r.roughness   = def.roughness;
            r.aoStrength  = def.aoStrength;
            r.heightScale = def.heightScale;
            r.alphaCutoff = def.alphaCutoff;
            r.emissiveTint      = def.emissiveTint;
            r.emissiveIntensity = def.emissiveIntensity;
            r.normalStrength    = def.normalStrength;
            r.doubleSided       = def.doubleSided ? 1u : 0u;
            r.uvTiling          = def.uvTiling;
            r.uvOffset          = def.uvOffset;
            return r;
        }

        renderer::MaterialDef recordToMaterialDef(const MaterialRecord& r,
                                                  const StringTable& strings) {
            renderer::MaterialDef def;
            def.name       = std::string(strings.View(r.nameStr));
            def.shaderName = std::string(strings.View(r.shaderNameStr));
            def.vertPath   = std::string(strings.View(r.vertPathStr));
            def.fragPath   = std::string(strings.View(r.fragPathStr));
            for (const TextureRef& t : r.textures) {
                renderer::TextureDef td;
                td.slot  = static_cast<renderer::TextureSlot>(t.slot);
                td.path  = std::string(strings.View(t.pathStr));
                td.space = static_cast<renderer::ColorSpace>(t.colorSpace);
                def.textures.push_back(std::move(td));
            }
            def.albedoTint  = r.albedoTint;
            def.metallic    = r.metallic;
            def.roughness   = r.roughness;
            def.aoStrength  = r.aoStrength;
            def.heightScale = r.heightScale;
            def.alphaCutoff = r.alphaCutoff;
            def.emissiveTint      = r.emissiveTint;
            def.emissiveIntensity = r.emissiveIntensity;
            def.normalStrength    = r.normalStrength;
            def.doubleSided       = (r.doubleSided != 0u);
            def.uvTiling          = r.uvTiling;
            def.uvOffset          = r.uvOffset;
            return def;
        }

    }   // namespace anonimo

    bool SceneSerializer::Extract(const scene::SceneGraph& graph,
                                  const resources::SceneAssets& assets,
                                  const resources::ResourceManager& rm,
                                  const SceneRegistry& reg,
                                  SceneDocument& out) {
        out.Clear();
        out.iblIntensity = graph.iblIntensity;

        // Los materiales van primero: los bindings de los nodos referencian sus indices,
        // y el orden del vector de SceneAssets es el orden del chunk MATL.
        for (u32 i = 0u; i < assets.MaterialCount(); ++i) {
            const renderer::MaterialDef* def = assets.DefAt(i);
            if (def == null) continue;
            out.materials.push_back(materialDefToRecord(*def, out.strings));
        }

        ExtractCtx ctx{ graph, assets, rm, reg, out, kInvalidIndex };
        if (!graph.IsValid(graph.Root())) {
            EF_LOG_ERROR("SceneSerializer::Extract: el grafo no tiene raiz valida");
            out.Clear();
            return false;
        }
        extractNode(ctx, graph.Root(), kInvalidIndex);
        out.primarySunNode = ctx.sunFileIndex;
        return true;
    }

    bool SceneSerializer::Resolve(const SceneDocument& doc,
                                  scene::SceneGraph& outGraph,
                                  resources::SceneAssets& outAssets,
                                  resources::ResourceManager& rm,
                                  const SceneRegistry& reg) {
        outGraph.Clear();
        outAssets.Clear();

        // --- materiales: el shader es obligatorio, las texturas no ---
        for (const MaterialRecord& mr : doc.materials) {
            renderer::MaterialDef def = recordToMaterialDef(mr, doc.strings);
            std::optional<renderer::Material> mat = resources::BuildMaterial(def, rm);
            if (!mat) {
                EF_LOG_ERROR("SceneSerializer::Resolve: material '%s' sin shader, se aborta",
                             def.name.c_str());
                outGraph.Clear();
                outAssets.Clear();
                return false;
            }
            outAssets.AddMaterial(std::move(def), std::move(*mat));
        }

        // --- nodos: una sola pasada, el pre-orden ya lo valido ParseSceneDocument ---
        std::vector<scene::NodeHandle> handles;
        handles.reserve(doc.nodes.size());

        for (usize i = 0u; i < doc.nodes.size(); ++i) {
            const NodeRecord& rec = doc.nodes[i];
            const std::string name(doc.strings.View(rec.nameStr));

            scene::NodeHandle handle;
            if (i == 0u) {
                handle = outGraph.Root();       // el nodo 0 ES la raiz: no se crea
            } else {
                handle = outGraph.CreateChild(handles[rec.parent], name);
            }
            handles.push_back(handle);
            outGraph.SetLocalTransform(handle, rec.local);

            // Espejo de extractComponents: el mismo loop, sin un 'if' por tipo.
            for (const ComponentRecord& cr : rec.components) {
                const std::string typeName(doc.strings.View(cr.typeNameStr));
                const ComponentRegistry::Entry* entry = reg.components.FindByName(typeName);
                if (entry == null) {
                    // El payload tiene largo propio, asi que saltearlo no
                    // desalinea nada: el nodo se carga sin ese adjunto.
                    EF_LOG_WARNING("SceneSerializer: componente '%s' no registrado, se saltea "
                                   "(nodo '%s')", typeName.c_str(), name.c_str());
                    continue;
                }

                BinaryReader    payloadReader(cr.payload);
                ComponentReader ar{ payloadReader, doc.strings, outAssets, rm,
                                    reg.meshes, name.c_str() };

                if (!entry->read(ar, outGraph.Components(), handle.index)) continue;
                if (!payloadReader.Ok()) {
                    EF_LOG_WARNING("SceneSerializer: payload invalido para '%s' en '%s'",
                                   typeName.c_str(), name.c_str());
                }
            }

            for (const BehaviorRecord& br : rec.behaviors) {
                const std::string typeName(doc.strings.View(br.typeNameStr));
                const BehaviorRegistry::Entry* entry = reg.behaviors.FindByName(typeName);
                if (entry == null) {
                    EF_LOG_WARNING("SceneSerializer: behavior '%s' no registrado, se saltea "
                                   "(nodo '%s')", typeName.c_str(), name.c_str());
                    continue;
                }
                std::unique_ptr<scene::Behavior> beh = entry->create();
                if (!beh) continue;

                BinaryReader payloadReader(br.payload);
                entry->read(payloadReader, *beh);
                if (!payloadReader.Ok()) {
                    EF_LOG_WARNING("SceneSerializer: payload invalido para '%s' en '%s'",
                                   typeName.c_str(), name.c_str());
                }
                beh->enabled = (br.enabled != 0u);
                outGraph.AttachBehavior(handle, std::move(beh));
            }
        }

        if (doc.primarySunNode != kInvalidIndex && doc.primarySunNode < handles.size()) {
            outGraph.SetPrimarySun(handles[doc.primarySunNode]);
        }
        outGraph.iblIntensity = doc.iblIntensity;
        return true;
    }

    bool SceneSerializer::SaveToBytes(const scene::SceneGraph& graph,
                                      const resources::SceneAssets& assets,
                                      const resources::ResourceManager& rm,
                                      const SceneRegistry& reg,
                                      std::vector<u8>& out) {
        out.clear();
        SceneDocument doc;
        if (!Extract(graph, assets, rm, reg, doc)) return false;
        return WriteSceneDocument(doc, out);
    }

    bool SceneSerializer::LoadFromBytes(const u8* data, usize size,
                                        scene::SceneGraph& outGraph,
                                        resources::SceneAssets& outAssets,
                                        resources::ResourceManager& rm,
                                        const SceneRegistry& reg) {
        using Clock = std::chrono::steady_clock;

        const auto t0 = Clock::now();
        SceneDocument doc;
        // Si el parse falla, se sale ANTES de tocar el grafo: un .efe corrupto no
        // destruye la escena que ya estaba cargada.
        if (!ParseSceneDocument(data, size, doc)) return false;
        const auto t1 = Clock::now();

        if (!Resolve(doc, outGraph, outAssets, rm, reg)) return false;
        const auto t2 = Clock::now();

        const f64 parseMs   = std::chrono::duration<f64, std::milli>(t1 - t0).count();
        const f64 resolveMs = std::chrono::duration<f64, std::milli>(t2 - t1).count();
        EF_LOG_INFO("SceneSerializer: %zu nodos, %zu materiales | parse %.3f ms, resolve %.3f ms",
                    doc.nodes.size(), doc.materials.size(), parseMs, resolveMs);
        return true;
    }

    bool SceneSerializer::Save(const char* path,
                               const scene::SceneGraph& graph,
                               const resources::SceneAssets& assets,
                               const resources::ResourceManager& rm,
                               const SceneRegistry& reg) {
        EF_ASSERT(path != null, "SceneSerializer::Save: path nulo");

        std::vector<u8> bytes;
        if (!SaveToBytes(graph, assets, rm, reg, bytes)) return false;

        std::ofstream stream(path, std::ios::binary | std::ios::trunc);
        if (!stream.is_open()) {
            EF_LOG_ERROR("SceneSerializer::Save: no se pudo abrir '%s' para escribir", path);
            return false;
        }
        // Una sola escritura del archivo entero.
        if (!bytes.empty()) {
            stream.write(reinterpret_cast<const char*>(bytes.data()),
                         static_cast<std::streamsize>(bytes.size()));
        }
        if (!stream) {
            EF_LOG_ERROR("SceneSerializer::Save: escritura incompleta de '%s'", path);
            return false;
        }
        EF_LOG_INFO("SceneSerializer: escrito '%s' (%zu bytes)", path, bytes.size());
        return true;
    }

    bool SceneSerializer::Load(const char* path,
                               scene::SceneGraph& outGraph,
                               resources::SceneAssets& outAssets,
                               resources::ResourceManager& rm,
                               const SceneRegistry& reg) {
        EF_ASSERT(path != null, "SceneSerializer::Load: path nulo");

        auto bytes = resources::FileIO::ReadBytes(path);
        if (!bytes) return false;

        return LoadFromBytes(bytes->data(), bytes->size(), outGraph, outAssets, rm, reg);
    }

}
}
