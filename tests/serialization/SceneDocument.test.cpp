#include <ostream>   // doctest lo necesita para stringificar los string_view del CHECK
#include <doctest/doctest.h>
#include <efengine/serialization/SceneDocument.h>
#include <efengine/serialization/EfeFile.h>
#include <vector>

using namespace efengine;
using namespace efengine::serialization;

namespace {
    // Documento de referencia usado por varios tests. Cubre las cuatro formas de nodo:
    // raiz pelada, malla por path, malla por generador, y luz; mas behaviors y jerarquia
    // de tres niveles.
    SceneDocument makeDoc() {
        SceneDocument d;
        d.ambientFactor = 0.125f;

        const u32 sRoot   = d.strings.Intern("root");
        const u32 sRata   = d.strings.Intern("model_rata");
        const u32 sHijo   = d.strings.Intern("nodo_hijo");
        const u32 sPiso   = d.strings.Intern("plano");
        const u32 sSol    = d.strings.Intern("directional_light");
        const u32 sFbx    = d.strings.Intern("assets/models/street_rat_4k.fbx");
        const u32 sGen    = d.strings.Intern("sandbox.plane");
        const u32 sSub1   = d.strings.Intern("street_rat");
        const u32 sSub2   = d.strings.Intern("street_rat_hair");
        const u32 sGround = d.strings.Intern("ground");
        const u32 sMatRat = d.strings.Intern("mat_rata");
        const u32 sMatPis = d.strings.Intern("mat_piso");
        const u32 sPbr    = d.strings.Intern("pbr");
        const u32 sVert   = d.strings.Intern("assets/shaders/pbr.vert");
        const u32 sFrag   = d.strings.Intern("assets/shaders/pbr.frag");
        const u32 sTex    = d.strings.Intern("assets/textures/rata_diff.jpg");
        const u32 sRotar  = d.strings.Intern("RotarY");
        const u32 sOrbit  = d.strings.Intern("OrbitarXZ");

        // --- materiales ---
        MaterialRecord m0;
        m0.nameStr = sMatRat; m0.shaderNameStr = sPbr;
        m0.vertPathStr = sVert; m0.fragPathStr = sFrag;
        m0.textures.push_back(TextureRef{ 0u, sTex, 1u });   // slot Albedo, sRGB
        m0.albedoTint = glm::vec3(0.9f, 0.8f, 0.7f);
        m0.metallic = 0.25f; m0.roughness = 0.75f;
        m0.aoStrength = 0.5f; m0.heightScale = 0.0f; m0.alphaCutoff = 0.5f;
        d.materials.push_back(m0);

        MaterialRecord m1;
        m1.nameStr = sMatPis; m1.shaderNameStr = sPbr;
        m1.vertPathStr = sVert; m1.fragPathStr = sFrag;
        m1.metallic = 0.0f; m1.roughness = 1.0f;
        d.materials.push_back(m1);

        // --- nodo 0: la raiz ---
        NodeRecord root;
        root.nameStr = sRoot;
        root.parent  = kInvalidIndex;
        d.nodes.push_back(root);

        // --- nodo 1: malla por path, dos bindings, un behavior ---
        NodeRecord rata;
        rata.nameStr = sRata;
        rata.parent  = 0u;
        rata.local.position = glm::vec3(1.0f, 2.0f, 3.0f);
        rata.local.rotation = glm::vec3(0.0f, 45.0f, 0.0f);
        rata.local.scale    = glm::vec3(10.0f);
        rata.mesh.emplace();
        rata.mesh->kind = MeshKind::Path;
        rata.mesh->str  = sFbx;
        rata.mesh->bindings.push_back(MaterialBinding{ sSub1, 0u });
        rata.mesh->bindings.push_back(MaterialBinding{ sSub2, 0u });
        BehaviorRecord b0;
        b0.typeNameStr = sRotar;
        b0.enabled = 1u;
        b0.payload = { 0x00u, 0x00u, 0xA0u, 0x41u };   // 20.0f en little-endian
        rata.behaviors.push_back(b0);
        d.nodes.push_back(rata);

        // --- nodo 2: hijo del nodo 1 (tercer nivel), pelado ---
        NodeRecord hijo;
        hijo.nameStr = sHijo;
        hijo.parent  = 1u;
        hijo.local.position = glm::vec3(0.0f, 5.0f, 0.0f);
        d.nodes.push_back(hijo);

        // --- nodo 3: malla por generador, con payload de params ---
        NodeRecord piso;
        piso.nameStr = sPiso;
        piso.parent  = 0u;
        piso.mesh.emplace();
        piso.mesh->kind = MeshKind::Generator;
        piso.mesh->str  = sGen;
        piso.mesh->genPayload = { 1u, 2u, 3u, 4u, 5u };   // largo impar a proposito: prueba el padding
        piso.mesh->bindings.push_back(MaterialBinding{ sGround, 1u });
        d.nodes.push_back(piso);

        // --- nodo 4: luz direccional, dos behaviors, uno deshabilitado ---
        NodeRecord sol;
        sol.nameStr = sSol;
        sol.parent  = 0u;
        sol.local.rotation = glm::vec3(-70.15f, 56.3f, 0.0f);
        sol.light.emplace();
        sol.light->kind  = LightKindId::Directional;
        sol.light->color = glm::vec3(3.0f);
        BehaviorRecord b1;
        b1.typeNameStr = sOrbit;
        b1.enabled = 0u;
        b1.payload = { 0xAAu, 0xBBu };
        BehaviorRecord b2;
        b2.typeNameStr = sRotar;
        b2.enabled = 1u;
        sol.behaviors.push_back(b1);
        sol.behaviors.push_back(b2);
        d.nodes.push_back(sol);

        d.primarySunNode = 4u;
        return d;
    }

    void checkTransformEq(const math::Transform& a, const math::Transform& b) {
        CHECK(a.position.x == doctest::Approx(b.position.x));
        CHECK(a.position.y == doctest::Approx(b.position.y));
        CHECK(a.position.z == doctest::Approx(b.position.z));
        CHECK(a.rotation.x == doctest::Approx(b.rotation.x));
        CHECK(a.rotation.y == doctest::Approx(b.rotation.y));
        CHECK(a.rotation.z == doctest::Approx(b.rotation.z));
        CHECK(a.scale.x == doctest::Approx(b.scale.x));
        CHECK(a.scale.y == doctest::Approx(b.scale.y));
        CHECK(a.scale.z == doctest::Approx(b.scale.z));
    }
}

TEST_CASE("EfeSceneDocument: round-trip completo campo por campo") {
    SceneDocument src = makeDoc();

    std::vector<u8> bytes;
    REQUIRE(WriteSceneDocument(src, bytes));
    CHECK(bytes.size() % 4u == 0u);          // todo el archivo queda alineado a 4

    SceneDocument dst;
    REQUIRE(ParseSceneDocument(bytes.data(), bytes.size(), dst));

    // --- settings ---
    CHECK(dst.ambientFactor == doctest::Approx(src.ambientFactor));
    CHECK(dst.primarySunNode == src.primarySunNode);

    // --- strings ---
    CHECK(dst.strings.Count() == src.strings.Count());

    // --- materiales ---
    REQUIRE(dst.materials.size() == src.materials.size());
    const MaterialRecord& sm = src.materials[0];
    const MaterialRecord& dm = dst.materials[0];
    CHECK(dst.strings.View(dm.nameStr) == src.strings.View(sm.nameStr));
    CHECK(dst.strings.View(dm.shaderNameStr) == "pbr");
    CHECK(dst.strings.View(dm.vertPathStr) == "assets/shaders/pbr.vert");
    CHECK(dst.strings.View(dm.fragPathStr) == "assets/shaders/pbr.frag");
    REQUIRE(dm.textures.size() == 1u);
    CHECK(dm.textures[0].slot == 0u);
    CHECK(dst.strings.View(dm.textures[0].pathStr) == "assets/textures/rata_diff.jpg");
    CHECK(dm.textures[0].colorSpace == 1u);
    CHECK(dm.albedoTint.x == doctest::Approx(0.9f));
    CHECK(dm.albedoTint.z == doctest::Approx(0.7f));
    CHECK(dm.metallic == doctest::Approx(0.25f));
    CHECK(dm.roughness == doctest::Approx(0.75f));
    CHECK(dm.heightScale == doctest::Approx(0.0f));
    CHECK(dst.materials[1].textures.empty());

    // --- nodos: cantidad y jerarquia ---
    REQUIRE(dst.nodes.size() == 5u);
    CHECK(dst.nodes[0].parent == kInvalidIndex);
    CHECK(dst.nodes[1].parent == 0u);
    CHECK(dst.nodes[2].parent == 1u);        // tercer nivel preservado
    CHECK(dst.nodes[3].parent == 0u);
    CHECK(dst.nodes[4].parent == 0u);
    CHECK(dst.strings.View(dst.nodes[0].nameStr) == "root");
    CHECK(dst.strings.View(dst.nodes[2].nameStr) == "nodo_hijo");

    // --- nodo 0: sin adjuntos ---
    CHECK_FALSE(dst.nodes[0].mesh.has_value());
    CHECK_FALSE(dst.nodes[0].light.has_value());
    CHECK(dst.nodes[0].behaviors.empty());

    // --- nodo 1: malla por path + bindings + transform + behavior ---
    checkTransformEq(dst.nodes[1].local, src.nodes[1].local);
    REQUIRE(dst.nodes[1].mesh.has_value());
    CHECK(dst.nodes[1].mesh->kind == MeshKind::Path);
    CHECK(dst.strings.View(dst.nodes[1].mesh->str) == "assets/models/street_rat_4k.fbx");
    CHECK(dst.nodes[1].mesh->genPayload.empty());
    REQUIRE(dst.nodes[1].mesh->bindings.size() == 2u);
    CHECK(dst.strings.View(dst.nodes[1].mesh->bindings[0].submeshNameStr) == "street_rat");
    CHECK(dst.nodes[1].mesh->bindings[0].materialIndex == 0u);
    CHECK(dst.strings.View(dst.nodes[1].mesh->bindings[1].submeshNameStr) == "street_rat_hair");
    CHECK_FALSE(dst.nodes[1].light.has_value());
    REQUIRE(dst.nodes[1].behaviors.size() == 1u);
    CHECK(dst.strings.View(dst.nodes[1].behaviors[0].typeNameStr) == "RotarY");
    CHECK(dst.nodes[1].behaviors[0].enabled == 1u);
    CHECK(dst.nodes[1].behaviors[0].payload == src.nodes[1].behaviors[0].payload);

    // --- nodo 3: malla por generador con payload de largo impar ---
    REQUIRE(dst.nodes[3].mesh.has_value());
    CHECK(dst.nodes[3].mesh->kind == MeshKind::Generator);
    CHECK(dst.strings.View(dst.nodes[3].mesh->str) == "sandbox.plane");
    CHECK(dst.nodes[3].mesh->genPayload == src.nodes[3].mesh->genPayload);
    REQUIRE(dst.nodes[3].mesh->bindings.size() == 1u);
    CHECK(dst.nodes[3].mesh->bindings[0].materialIndex == 1u);

    // --- nodo 4: luz + dos behaviors, uno deshabilitado ---
    REQUIRE(dst.nodes[4].light.has_value());
    CHECK(dst.nodes[4].light->kind == LightKindId::Directional);
    CHECK(dst.nodes[4].light->color.x == doctest::Approx(3.0f));
    CHECK_FALSE(dst.nodes[4].mesh.has_value());
    REQUIRE(dst.nodes[4].behaviors.size() == 2u);
    CHECK(dst.nodes[4].behaviors[0].enabled == 0u);
    CHECK(dst.nodes[4].behaviors[0].payload == src.nodes[4].behaviors[0].payload);
    CHECK(dst.nodes[4].behaviors[1].enabled == 1u);
    CHECK(dst.nodes[4].behaviors[1].payload.empty());
}

TEST_CASE("EfeSceneDocument: round-trip byte a byte") {
    SceneDocument src = makeDoc();

    std::vector<u8> bytes1;
    REQUIRE(WriteSceneDocument(src, bytes1));

    SceneDocument dst;
    REQUIRE(ParseSceneDocument(bytes1.data(), bytes1.size(), dst));

    std::vector<u8> bytes2;
    REQUIRE(WriteSceneDocument(dst, bytes2));

    // Si esto falla, hay algo con orden inestable (tipicamente un unordered_map)
    // o un campo que se lee y no se escribe.
    CHECK(bytes2 == bytes1);
}

TEST_CASE("EfeSceneDocument: documento minimo (solo la raiz) round-trip") {
    SceneDocument src;
    NodeRecord root;
    root.nameStr = src.strings.Intern("root");
    root.parent  = kInvalidIndex;
    src.nodes.push_back(root);

    std::vector<u8> bytes;
    REQUIRE(WriteSceneDocument(src, bytes));

    SceneDocument dst;
    REQUIRE(ParseSceneDocument(bytes.data(), bytes.size(), dst));

    REQUIRE(dst.nodes.size() == 1u);
    CHECK(dst.nodes[0].parent == kInvalidIndex);
    CHECK(dst.strings.View(dst.nodes[0].nameStr) == "root");
    CHECK(dst.materials.empty());
    CHECK(dst.primarySunNode == kInvalidIndex);
}

TEST_CASE("EfeSceneDocument: Parse de basura falla sin crashear") {
    const std::vector<u8> basura = { 0x01u, 0x02u, 0x03u, 0x04u, 0x05u, 0x06u, 0x07u, 0x08u };
    SceneDocument dst;
    CHECK_FALSE(ParseSceneDocument(basura.data(), basura.size(), dst));
    CHECK(dst.nodes.empty());
}

TEST_CASE("EfeSceneDocument: Parse de buffer vacio falla") {
    SceneDocument dst;
    CHECK_FALSE(ParseSceneDocument(nullptr, 0u, dst));
    CHECK(dst.nodes.empty());
}

TEST_CASE("EfeSceneDocument: Clear vacia todo") {
    SceneDocument d = makeDoc();
    REQUIRE_FALSE(d.nodes.empty());

    d.Clear();

    CHECK(d.nodes.empty());
    CHECK(d.materials.empty());
    CHECK(d.primarySunNode == kInvalidIndex);
    CHECK(d.strings.Count() == 1u);        // vuelve a tener solo ""
}
