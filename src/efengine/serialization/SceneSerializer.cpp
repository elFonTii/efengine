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

        void extractMesh(ExtractCtx& ctx, const scene::MeshAttachment& att, NodeRecord& rec) {
            rec.mesh.emplace();
            MeshRecord& mesh = *rec.mesh;

            // Primero se pregunta si es una malla generada (la escena es su duena);
            // si no, tiene que venir de un path del ResourceManager.
            const u32 genIndex = ctx.assets.IndexOfGenerated(att.model);
            if (genIndex != kInvalidIndex) {
                mesh.kind       = MeshKind::Generator;
                mesh.str        = ctx.doc.strings.Intern(ctx.assets.GeneratorNameAt(genIndex));
                mesh.genPayload = ctx.assets.GeneratorPayloadAt(genIndex);
            } else if (const std::string* path = ctx.rm.PathOf(att.model)) {
                mesh.kind = MeshKind::Path;
                mesh.str  = ctx.doc.strings.Intern(*path);
            } else {
                EF_LOG_WARNING("SceneSerializer: malla sin path ni generador, se omite");
                rec.mesh.reset();
                return;
            }

            for (const auto& entry : att.materials) {
                const u32 matIndex = ctx.assets.IndexOfMaterial(entry.second);
                if (matIndex == kInvalidIndex) {
                    EF_LOG_WARNING("SceneSerializer: el material del submesh '%s' no esta "
                                   "en SceneAssets, se omite", entry.first.c_str());
                    continue;
                }
                mesh.bindings.push_back(MaterialBinding{
                    ctx.doc.strings.Intern(entry.first), matIndex });
            }

            // MaterialMap es unordered: sin ordenar, dos guardados dan bytes distintos.
            std::sort(mesh.bindings.begin(), mesh.bindings.end(),
                      [&ctx](const MaterialBinding& a, const MaterialBinding& b) {
                          return ctx.doc.strings.View(a.submeshNameStr)
                               < ctx.doc.strings.View(b.submeshNameStr);
                      });
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

            if (const scene::MeshAttachment* mesh = ctx.graph.TryGet<scene::MeshAttachment>(handle)) {
                extractMesh(ctx, *mesh, rec);
            }
            if (const scene::LightAttachment* light = ctx.graph.TryGet<scene::LightAttachment>(handle)) {
                rec.light.emplace();
                rec.light->kind  = (light->kind == scene::LightKind::Directional)
                                       ? LightKindId::Directional : LightKindId::Point;
                rec.light->color = light->color;
            }
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

            if (rec.mesh) {
                const std::string ref(doc.strings.View(rec.mesh->str));
                const renderer::Model* model = null;

                if (rec.mesh->kind == MeshKind::Generator) {
                    MeshGeneratorRegistry::CreateFn fn = reg.meshes.Find(ref);
                    if (fn == null) {
                        EF_LOG_WARNING("SceneSerializer: generador '%s' no registrado, "
                                       "el nodo '%s' queda sin malla",
                                       ref.c_str(), name.c_str());
                    } else {
                        BinaryReader payloadReader(rec.mesh->genPayload);
                        std::unique_ptr<renderer::Model> generado = fn(payloadReader);
                        if (generado == nullptr) {
                            EF_LOG_WARNING("SceneSerializer: el generador '%s' no produjo "
                                           "malla para '%s'", ref.c_str(), name.c_str());
                        } else {
                            const u32 idx = outAssets.AddGenerated(
                                ref, rec.mesh->genPayload, std::move(generado));
                            model = outAssets.GeneratedAt(idx);
                        }
                    }
                } else {
                    model = rm.GetModel(ref.c_str());
                    if (model == null) {
                        EF_LOG_WARNING("SceneSerializer: no se pudo cargar '%s', el nodo "
                                       "'%s' queda sin malla", ref.c_str(), name.c_str());
                    }
                }

                if (model != null) {
                    renderer::MaterialMap materials;
                    for (const MaterialBinding& b : rec.mesh->bindings) {
                        const renderer::Material* m = outAssets.MaterialAt(b.materialIndex);
                        if (m == null) {
                            EF_LOG_WARNING("SceneSerializer: binding con material %u "
                                           "invalido en '%s'", b.materialIndex, name.c_str());
                            continue;
                        }
                        materials[std::string(doc.strings.View(b.submeshNameStr))] = m;
                    }
                    outGraph.AttachMesh(handle, scene::MeshAttachment{ model,
                                                                       std::move(materials) });
                }
            }

            if (rec.light) {
                const scene::LightKind kind = (rec.light->kind == LightKindId::Directional)
                                                  ? scene::LightKind::Directional
                                                  : scene::LightKind::Point;
                outGraph.AttachLight(handle, scene::LightAttachment{ kind, rec.light->color });
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
