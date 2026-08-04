#include <cstring>
#include <ostream>   // doctest lo necesita para stringificar los string_view del CHECK
#include <doctest/doctest.h>
#include <efengine/serialization/SceneDocument.h>
#include <efengine/serialization/ComponentPayloads.h>
#include <optional>
#include <efengine/serialization/EfeFile.h>
#include <vector>

using namespace efengine;
using namespace efengine::serialization;

namespace {

    // Desde v5 el documento no sabe que es una malla: ve un nombre de tipo y un
    // blob. Estos dos helpers arman y leen ese blob para poder seguir testeando
    // el contenido, que es lo que estos tests siempre verificaron.
    template <class Payload>
    ComponentRecord componente(Payload p, const char* typeName, StringTable& strings) {
        BinaryWriter w;
        SerializePayload(w, p);

        ComponentRecord rec;
        rec.typeNameStr = strings.Intern(typeName);
        rec.payload     = w.Take();
        return rec;
    }

    template <class Payload>
    Payload leerPayload(const ComponentRecord& rec) {
        BinaryReader r(rec.payload);
        Payload p;
        SerializePayload(r, p);
        return p;
    }

    // Busca el componente de un tipo en un nodo; null si no lo tiene.
    const ComponentRecord* buscar(const NodeRecord& n, const StringTable& strings,
                                  const char* typeName) {
        for (const ComponentRecord& c : n.components) {
            if (strings.View(c.typeNameStr) == typeName) return &c;
        }
        return null;
    }
    // Documento de referencia usado por varios tests. Cubre las cuatro formas de nodo:
    // raiz pelada, malla por path, malla por generador, y luz; mas behaviors y jerarquia
    // de tres niveles.
    SceneDocument makeDoc() {
        SceneDocument d;
        d.iblIntensity = 0.125f;

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
        {
            MeshPayload mp;
            mp.kind = MeshKind::Path;
            mp.str  = sFbx;
            mp.bindings.push_back(MaterialBinding{ sSub1, 0u });
            mp.bindings.push_back(MaterialBinding{ sSub2, 0u });
            rata.components.push_back(componente(std::move(mp), kMeshComponentName, d.strings));
        }
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
        {
            MeshPayload mp;
            mp.kind = MeshKind::Generator;
            mp.str  = sGen;
            mp.genPayload = { 1u, 2u, 3u, 4u, 5u };   // largo impar a proposito: prueba el padding
            mp.bindings.push_back(MaterialBinding{ sGround, 1u });
            piso.components.push_back(componente(std::move(mp), kMeshComponentName, d.strings));
        }
        d.nodes.push_back(piso);

        // --- nodo 4: luz direccional, dos behaviors, uno deshabilitado ---
        NodeRecord sol;
        sol.nameStr = sSol;
        sol.parent  = 0u;
        sol.local.rotation = glm::vec3(-70.15f, 56.3f, 0.0f);
        {
            LightPayload lp;
            lp.kind  = LightKindId::Directional;
            lp.color = glm::vec3(3.0f);
            sol.components.push_back(componente(lp, kLightComponentName, d.strings));
        }
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

    // --- layout de nodo v<=4, a mano ---------------------------------------
    // Hasta v4 el nodo llevaba un bitfield de flags y los cuerpos de mesh y luz
    // incrustados. El writer de produccion ya no sabe emitirlo (desde v5 los
    // adjuntos son components), asi que los tests de compatibilidad se lo
    // arman ellos.
    struct NodoV4 {
        u32 nameStr = 0u;
        u32 parent  = kInvalidIndex;
        math::Transform local;
        std::optional<MeshPayload>  mesh;
        std::optional<LightPayload> light;
        std::vector<BehaviorRecord> behaviors;
    };

    void escribirNodosV4(BinaryWriter& w, std::vector<NodoV4>& nodos) {
        u32 count = static_cast<u32>(nodos.size());
        w.Count(count, kMinEncodedNode);
        for (NodoV4& n : nodos) {
            w.Field(n.nameStr);
            w.Field(n.parent);

            u32 flags = 0u;
            if (n.mesh)  flags |= 1u << 0;   // NodeFlags::HasMesh
            if (n.light) flags |= 1u << 1;   // NodeFlags::HasLight
            w.Field(flags);

            Serialize(w, n.local);
            if (n.mesh)  SerializePayload(w, *n.mesh);
            if (n.light) SerializePayload(w, *n.light);
            SerializeVector(w, n.behaviors, kMinEncodedBehavior);
        }
    }

    std::vector<NodoV4> raizSolaV4(u32 nameStr) {
        NodoV4 root;
        root.nameStr = nameStr;
        root.parent  = kInvalidIndex;
        return { root };
    }

    // Arma a mano un .efe v1: header con version 1 y los records con el layout viejo
    // (material SIN los 3 escalares nuevos, chunk SCNE con el ambientFactor original).
    // No se puede usar WriteSceneDocument: el writer siempre emite kCurrentVersion.
    std::vector<u8> documentoV1() {
        BinaryWriter w;

        // Header a mano, con version 1.
        w.Bytes(kMagic, 4u);
        u32 endian  = kEndianCheck;
        u32 version = 1u;
        u32 content = static_cast<u32>(ContentType::Scene);
        u32 chunks  = 4u;
        w.Field(endian);
        w.Field(version);
        w.Field(content);
        w.Field(chunks);

        StringTable strings;
        const u32 sMat  = strings.Intern("mat_v1");
        const u32 sShad = strings.Intern("pbr");
        const u32 sVert = strings.Intern("assets/shaders/pbr.vert");
        const u32 sFrag = strings.Intern("assets/shaders/pbr.frag");
        const u32 sRoot = strings.Intern("root");

        usize marker = BeginChunk(w, ChunkId::Strings);
        strings.Serialize(w);
        EndChunk(w, marker);

        // SCNE v1: el primer f32 era ambientFactor.
        marker = BeginChunk(w, ChunkId::Settings);
        f32 ambientFactorViejo = 0.08f;
        u32 primarySun         = kInvalidIndex;
        w.Field(ambientFactorViejo);
        w.Field(primarySun);
        EndChunk(w, marker);

        // MATL v1: 4*u32 + array de texturas + vec3 + 5*f32. Nada mas.
        marker = BeginChunk(w, ChunkId::Materials);
        u32 materialCount = 1u;
        w.Count(materialCount, 52u);
        u32 nameStr = sMat, shaderStr = sShad, vertStr = sVert, fragStr = sFrag;
        w.Field(nameStr);
        w.Field(shaderStr);
        w.Field(vertStr);
        w.Field(fragStr);
        std::vector<TextureRef> sinTexturas;
        w.Array(sinTexturas);
        glm::vec3 tint(1.0f);
        f32 metallic = 0.25f, roughness = 0.5f, ao = 0.5f, height = 0.05f, cutoff = 0.5f;
        w.Field(tint);
        w.Field(metallic);
        w.Field(roughness);
        w.Field(ao);
        w.Field(height);
        w.Field(cutoff);
        EndChunk(w, marker);

        // NODE con el layout de v1: flags + cuerpos incrustados.
        marker = BeginChunk(w, ChunkId::Nodes);
        std::vector<NodoV4> nodes = raizSolaV4(sRoot);
        escribirNodosV4(w, nodes);
        EndChunk(w, marker);

        return w.Take();
    }

    // Arma a mano un .efe v2: material CON los 3 escalares de v2 pero SIN el
    // doubleSided de v3. Igual que documentoV1(), no se puede usar
    // WriteSceneDocument porque el writer siempre emite kCurrentVersion.
    std::vector<u8> documentoV2() {
        BinaryWriter w;

        // Header a mano, con version 2.
        w.Bytes(kMagic, 4u);
        u32 endian  = kEndianCheck;
        u32 version = 2u;
        u32 content = static_cast<u32>(ContentType::Scene);
        u32 chunks  = 4u;
        w.Field(endian);
        w.Field(version);
        w.Field(content);
        w.Field(chunks);

        StringTable strings;
        const u32 sMat  = strings.Intern("mat_v2");
        const u32 sShad = strings.Intern("pbr");
        const u32 sVert = strings.Intern("assets/shaders/pbr.vert");
        const u32 sFrag = strings.Intern("assets/shaders/pbr.frag");
        const u32 sRoot = strings.Intern("root");

        usize marker = BeginChunk(w, ChunkId::Strings);
        strings.Serialize(w);
        EndChunk(w, marker);

        // SCNE v2: el primer f32 ya es iblIntensity, no el ambientFactor de v1.
        marker = BeginChunk(w, ChunkId::Settings);
        f32 iblIntensity = 1.0f;
        u32 primarySun   = kInvalidIndex;
        w.Field(iblIntensity);
        w.Field(primarySun);
        EndChunk(w, marker);

        // MATL v2: lo de v1 + vec3 emissiveTint + f32 intensity + f32 strength.
        marker = BeginChunk(w, ChunkId::Materials);
        u32 materialCount = 1u;
        w.Count(materialCount, 72u);
        u32 nameStr = sMat, shaderStr = sShad, vertStr = sVert, fragStr = sFrag;
        w.Field(nameStr);
        w.Field(shaderStr);
        w.Field(vertStr);
        w.Field(fragStr);
        std::vector<TextureRef> sinTexturas;
        w.Array(sinTexturas);
        glm::vec3 tint(1.0f);
        f32 metallic = 0.25f, roughness = 0.5f, ao = 0.5f, height = 0.05f, cutoff = 0.5f;
        w.Field(tint);
        w.Field(metallic);
        w.Field(roughness);
        w.Field(ao);
        w.Field(height);
        w.Field(cutoff);
        glm::vec3 emisTint(1.0f);
        f32 emisIntensity = 3.0f, normalStrength = 1.5f;
        w.Field(emisTint);
        w.Field(emisIntensity);
        w.Field(normalStrength);
        EndChunk(w, marker);

        // NODE con el layout de v2: flags + cuerpos incrustados.
        marker = BeginChunk(w, ChunkId::Nodes);
        std::vector<NodoV4> nodes = raizSolaV4(sRoot);
        escribirNodosV4(w, nodes);
        EndChunk(w, marker);

        return w.Take();
    }

    // Arma a mano un .efe v3: material CON el doubleSided de v3 pero SIN el
    // uvTiling/uvOffset de v4. Igual que sus hermanos, no se puede usar
    // WriteSceneDocument porque el writer siempre emite kCurrentVersion.
    std::vector<u8> documentoV3() {
        BinaryWriter w;

        // Header a mano, con version 3.
        w.Bytes(kMagic, 4u);
        u32 endian  = kEndianCheck;
        u32 version = 3u;
        u32 content = static_cast<u32>(ContentType::Scene);
        u32 chunks  = 4u;
        w.Field(endian);
        w.Field(version);
        w.Field(content);
        w.Field(chunks);

        StringTable strings;
        const u32 sMat  = strings.Intern("mat_v3");
        const u32 sShad = strings.Intern("pbr");
        const u32 sVert = strings.Intern("assets/shaders/pbr.vert");
        const u32 sFrag = strings.Intern("assets/shaders/pbr.frag");
        const u32 sRoot = strings.Intern("root");

        usize marker = BeginChunk(w, ChunkId::Strings);
        strings.Serialize(w);
        EndChunk(w, marker);

        marker = BeginChunk(w, ChunkId::Settings);
        f32 iblIntensity = 1.0f;
        u32 primarySun   = kInvalidIndex;
        w.Field(iblIntensity);
        w.Field(primarySun);
        EndChunk(w, marker);

        // MATL v3: lo de v2 + u32 doubleSided. Nada de UV.
        marker = BeginChunk(w, ChunkId::Materials);
        u32 materialCount = 1u;
        w.Count(materialCount, 76u);
        u32 nameStr = sMat, shaderStr = sShad, vertStr = sVert, fragStr = sFrag;
        w.Field(nameStr);
        w.Field(shaderStr);
        w.Field(vertStr);
        w.Field(fragStr);
        std::vector<TextureRef> sinTexturas;
        w.Array(sinTexturas);
        glm::vec3 tint(1.0f);
        f32 metallic = 0.25f, roughness = 0.5f, ao = 0.5f, height = 0.05f, cutoff = 0.5f;
        w.Field(tint);
        w.Field(metallic);
        w.Field(roughness);
        w.Field(ao);
        w.Field(height);
        w.Field(cutoff);
        glm::vec3 emisTint(1.0f);
        f32 emisIntensity = 3.0f, normalStrength = 1.5f;
        w.Field(emisTint);
        w.Field(emisIntensity);
        w.Field(normalStrength);
        u32 doubleSided = 1u;
        w.Field(doubleSided);
        EndChunk(w, marker);

        // NODE con el layout de v3: flags + cuerpos incrustados.
        marker = BeginChunk(w, ChunkId::Nodes);
        std::vector<NodoV4> nodes = raizSolaV4(sRoot);
        escribirNodosV4(w, nodes);
        EndChunk(w, marker);

        return w.Take();
    }

    // Los payloads que se esperan del nodo 1 de documentoV4ConAdjuntos(), para
    // comparar byte a byte contra lo que produjo el shim.
    struct AdjuntosV4Esperados {
        MeshPayload  mesh;
        LightPayload light;
    };

    AdjuntosV4Esperados adjuntosEsperados(u32 sFbx, u32 sSub) {
        AdjuntosV4Esperados e;
        e.mesh.kind = MeshKind::Path;
        e.mesh.str  = sFbx;
        e.mesh.genPayload = { 7u, 8u, 9u };          // largo impar: ejercita el Align4
        e.mesh.bindings.push_back(MaterialBinding{ sSub, 0u });
        e.light.kind  = LightKindId::Directional;
        e.light.color = glm::vec3(1.5f, 2.5f, 3.5f);
        return e;
    }

    // Un .efe v4 cuyo nodo 1 tiene malla Y luz a la vez (flags == 3). Esa
    // combinacion no existe en ninguna escena versionada, asi que es justo el
    // caso que el formato viejo soportaba pero nadie ejercitaba. Sin materiales:
    // lo que se testea es el layout del nodo, no el del material.
    std::vector<u8> documentoV4ConAdjuntos() {
        BinaryWriter w;

        w.Bytes(kMagic, 4u);
        u32 endian  = kEndianCheck;
        u32 version = 4u;
        u32 content = static_cast<u32>(ContentType::Scene);
        u32 chunks  = 4u;
        w.Field(endian);
        w.Field(version);
        w.Field(content);
        w.Field(chunks);

        StringTable strings;
        const u32 sRoot = strings.Intern("root");
        const u32 sAmbos = strings.Intern("malla_y_luz");
        const u32 sFbx  = strings.Intern("assets/models/x.fbx");
        const u32 sSub  = strings.Intern("submesh");

        usize marker = BeginChunk(w, ChunkId::Strings);
        strings.Serialize(w);
        EndChunk(w, marker);

        marker = BeginChunk(w, ChunkId::Settings);
        f32 iblIntensity = 0.75f;
        u32 primarySun   = 1u;
        w.Field(iblIntensity);
        w.Field(primarySun);
        EndChunk(w, marker);

        marker = BeginChunk(w, ChunkId::Materials);
        u32 materialCount = 0u;
        w.Count(materialCount, MinEncodedMaterial(4u));
        EndChunk(w, marker);

        marker = BeginChunk(w, ChunkId::Nodes);
        const AdjuntosV4Esperados esperados = adjuntosEsperados(sFbx, sSub);

        std::vector<NodoV4> nodes = raizSolaV4(sRoot);
        NodoV4 ambos;
        ambos.nameStr = sAmbos;
        ambos.parent  = 0u;
        ambos.local.position = glm::vec3(4.0f, 5.0f, 6.0f);
        ambos.mesh  = esperados.mesh;
        ambos.light = esperados.light;
        BehaviorRecord b;
        b.typeNameStr = sRoot;      // el nombre no importa aca, si que sobreviva
        b.enabled = 0u;
        b.payload = { 0xDEu, 0xADu };
        ambos.behaviors.push_back(b);
        nodes.push_back(ambos);

        escribirNodosV4(w, nodes);
        EndChunk(w, marker);

        return w.Take();
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
    CHECK(dst.iblIntensity == doctest::Approx(src.iblIntensity));
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
    CHECK(dst.nodes[0].components.empty());
    CHECK(dst.nodes[0].behaviors.empty());

    // --- nodo 1: malla por path + bindings + transform + behavior ---
    checkTransformEq(dst.nodes[1].local, src.nodes[1].local);
    const ComponentRecord* cMalla = buscar(dst.nodes[1], dst.strings, kMeshComponentName);
    REQUIRE(cMalla != null);
    const MeshPayload mallaRata = leerPayload<MeshPayload>(*cMalla);
    CHECK(mallaRata.kind == MeshKind::Path);
    CHECK(dst.strings.View(mallaRata.str) == "assets/models/street_rat_4k.fbx");
    CHECK(mallaRata.genPayload.empty());
    REQUIRE(mallaRata.bindings.size() == 2u);
    CHECK(dst.strings.View(mallaRata.bindings[0].submeshNameStr) == "street_rat");
    CHECK(mallaRata.bindings[0].materialIndex == 0u);
    CHECK(dst.strings.View(mallaRata.bindings[1].submeshNameStr) == "street_rat_hair");
    CHECK(buscar(dst.nodes[1], dst.strings, kLightComponentName) == null);
    REQUIRE(dst.nodes[1].behaviors.size() == 1u);
    CHECK(dst.strings.View(dst.nodes[1].behaviors[0].typeNameStr) == "RotarY");
    CHECK(dst.nodes[1].behaviors[0].enabled == 1u);
    CHECK(dst.nodes[1].behaviors[0].payload == src.nodes[1].behaviors[0].payload);

    // --- nodo 3: malla por generador con payload de largo impar ---
    const ComponentRecord* cGen = buscar(dst.nodes[3], dst.strings, kMeshComponentName);
    REQUIRE(cGen != null);
    const MeshPayload mallaPiso = leerPayload<MeshPayload>(*cGen);
    CHECK(mallaPiso.kind == MeshKind::Generator);
    CHECK(dst.strings.View(mallaPiso.str) == "sandbox.plane");
    CHECK(mallaPiso.genPayload == std::vector<u8>{ 1u, 2u, 3u, 4u, 5u });
    REQUIRE(mallaPiso.bindings.size() == 1u);
    CHECK(mallaPiso.bindings[0].materialIndex == 1u);

    // --- nodo 4: luz + dos behaviors, uno deshabilitado ---
    const ComponentRecord* cLuz = buscar(dst.nodes[4], dst.strings, kLightComponentName);
    REQUIRE(cLuz != null);
    const LightPayload luzSol = leerPayload<LightPayload>(*cLuz);
    CHECK(luzSol.kind == LightKindId::Directional);
    CHECK(luzSol.color.x == doctest::Approx(3.0f));
    CHECK(buscar(dst.nodes[4], dst.strings, kMeshComponentName) == null);
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

TEST_CASE("SceneDocument: los escalares v2 del material sobreviven el round-trip") {
    SceneDocument src;
    const u32 sMat = src.strings.Intern("mat");

    MaterialRecord m;
    m.nameStr           = sMat;
    m.emissiveTint      = glm::vec3(0.25f, 0.5f, 0.75f);
    m.emissiveIntensity = 12.5f;
    m.normalStrength    = 1.75f;
    src.materials.push_back(m);

    NodeRecord root;
    root.nameStr = src.strings.Intern("root");
    root.parent  = kInvalidIndex;
    src.nodes.push_back(root);

    std::vector<u8> bytes;
    REQUIRE(WriteSceneDocument(src, bytes));

    SceneDocument dst;
    REQUIRE(ParseSceneDocument(bytes.data(), bytes.size(), dst));
    REQUIRE(dst.materials.size() == 1u);
    CHECK(dst.materials[0].emissiveTint.x == doctest::Approx(0.25f));
    CHECK(dst.materials[0].emissiveTint.y == doctest::Approx(0.5f));
    CHECK(dst.materials[0].emissiveTint.z == doctest::Approx(0.75f));
    CHECK(dst.materials[0].emissiveIntensity == doctest::Approx(12.5f));
    CHECK(dst.materials[0].normalStrength == doctest::Approx(1.75f));
}

TEST_CASE("SceneDocument: doubleSided sobrevive el round-trip") {
    SceneDocument src;
    const u32 sMat = src.strings.Intern("mat");

    MaterialRecord m;
    m.nameStr      = sMat;
    m.doubleSided  = 1u;
    src.materials.push_back(m);

    NodeRecord root;
    root.nameStr = src.strings.Intern("root");
    root.parent  = kInvalidIndex;
    src.nodes.push_back(root);

    std::vector<u8> bytes;
    REQUIRE(WriteSceneDocument(src, bytes));

    SceneDocument dst;
    REQUIRE(ParseSceneDocument(bytes.data(), bytes.size(), dst));
    REQUIRE(dst.materials.size() == 1u);
    CHECK(dst.materials[0].doubleSided == 1u);
}

TEST_CASE("SceneDocument: uvTiling y uvOffset sobreviven el round-trip") {
    SceneDocument src;
    const u32 sMat = src.strings.Intern("mat");

    MaterialRecord m;
    m.nameStr  = sMat;
    m.uvTiling = glm::vec2(8.0f, 4.0f);
    m.uvOffset = glm::vec2(0.25f, -0.5f);
    src.materials.push_back(m);

    NodeRecord root;
    root.nameStr = src.strings.Intern("root");
    root.parent  = kInvalidIndex;
    src.nodes.push_back(root);

    std::vector<u8> bytes;
    REQUIRE(WriteSceneDocument(src, bytes));

    SceneDocument dst;
    REQUIRE(ParseSceneDocument(bytes.data(), bytes.size(), dst));
    REQUIRE(dst.materials.size() == 1u);
    CHECK(dst.materials[0].uvTiling.x == doctest::Approx(8.0f));
    CHECK(dst.materials[0].uvTiling.y == doctest::Approx(4.0f));
    CHECK(dst.materials[0].uvOffset.x == doctest::Approx(0.25f));
    CHECK(dst.materials[0].uvOffset.y == doctest::Approx(-0.5f));
}

TEST_CASE("SceneDocument: un archivo v3 migra con la UV en identidad") {
    const std::vector<u8> bytes = documentoV3();

    SceneDocument dst;
    REQUIRE(ParseSceneDocument(bytes.data(), bytes.size(), dst));
    REQUIRE(dst.materials.size() == 1u);

    // Lo que v3 SI traia se lee igual.
    CHECK(dst.materials[0].metallic == doctest::Approx(0.25f));
    CHECK(dst.materials[0].normalStrength == doctest::Approx(1.5f));
    CHECK(dst.materials[0].doubleSided == 1u);

    // Lo que v3 no traia queda en la identidad: la escena vieja se ve igual que
    // siempre, sin que nadie tenga que reabrir y reguardar nada.
    CHECK(dst.materials[0].uvTiling.x == doctest::Approx(1.0f));
    CHECK(dst.materials[0].uvTiling.y == doctest::Approx(1.0f));
    CHECK(dst.materials[0].uvOffset.x == doctest::Approx(0.0f));
    CHECK(dst.materials[0].uvOffset.y == doctest::Approx(0.0f));
}

TEST_CASE("SceneDocument: un archivo v2 migra con doubleSided en false") {
    const std::vector<u8> bytes = documentoV2();

    SceneDocument dst;
    REQUIRE(ParseSceneDocument(bytes.data(), bytes.size(), dst));
    REQUIRE(dst.materials.size() == 1u);

    // Lo que v2 SI traia se lee igual.
    CHECK(dst.materials[0].metallic == doctest::Approx(0.25f));
    CHECK(dst.materials[0].normalStrength == doctest::Approx(1.5f));
    CHECK(dst.materials[0].emissiveIntensity == doctest::Approx(3.0f));

    // Lo que v2 no traia queda en el default: material de una cara sola.
    CHECK(dst.materials[0].doubleSided == 0u);
}

TEST_CASE("MinEncodedMaterial: cada version suma lo suyo al minimo") {
    CHECK(MinEncodedMaterial(1u) == 52u);
    CHECK(MinEncodedMaterial(2u) == 72u);
    CHECK(MinEncodedMaterial(3u) == 76u);
    CHECK(MinEncodedMaterial(4u) == 92u);   // + 2*vec2
}

TEST_CASE("SceneDocument: un archivo v1 carga con los defaults de los campos v2") {
    const std::vector<u8> bytes = documentoV1();

    SceneDocument dst;
    REQUIRE(ParseSceneDocument(bytes.data(), bytes.size(), dst));
    REQUIRE(dst.materials.size() == 1u);

    // Lo que v1 SI traia se lee igual.
    CHECK(dst.materials[0].metallic == doctest::Approx(0.25f));
    CHECK(dst.materials[0].roughness == doctest::Approx(0.5f));

    // Lo que v1 no traia queda en el default: emision apagada, normal sin escalar.
    CHECK(dst.materials[0].emissiveTint.x == doctest::Approx(1.0f));
    CHECK(dst.materials[0].emissiveIntensity == doctest::Approx(0.0f));
    CHECK(dst.materials[0].normalStrength == doctest::Approx(1.0f));
}

TEST_CASE("SceneDocument: un archivo v1 descarta el ambientFactor viejo") {
    const std::vector<u8> bytes = documentoV1();   // guarda 0.08f en el primer f32 de SCNE

    SceneDocument dst;
    REQUIRE(ParseSceneDocument(bytes.data(), bytes.size(), dst));

    // El 0.08 de v1 significaba "ambiente constante" y no se puede reinterpretar como
    // intensidad de IBL: la escena entera se veria casi negra. Se descarta.
    CHECK(dst.iblIntensity == doctest::Approx(1.0f));
    CHECK(dst.primarySunNode == kInvalidIndex);   // el campo que sigue quedo alineado
}

TEST_CASE("SceneDocument: iblIntensity sobrevive el round-trip v2") {
    SceneDocument src;
    src.iblIntensity = 1.75f;
    NodeRecord root;
    root.nameStr = src.strings.Intern("root");
    root.parent  = kInvalidIndex;
    src.nodes.push_back(root);

    std::vector<u8> bytes;
    REQUIRE(WriteSceneDocument(src, bytes));

    SceneDocument dst;
    REQUIRE(ParseSceneDocument(bytes.data(), bytes.size(), dst));
    CHECK(dst.iblIntensity == doctest::Approx(1.75f));
}

// ---------------------------------------------------------------------------
// v<=4 -> v5: mesh y light dejan de ser campos del nodo y pasan a ser
// componentes. Estos tests anclan el shim: si se rompe, los .efe versionados
// (sandbox.efe es v3, house.efe es v4) dejan de cargar.
// ---------------------------------------------------------------------------

TEST_CASE("SceneDocument: un nodo v4 con malla y luz sube a dos componentes") {
    const std::vector<u8> bytes = documentoV4ConAdjuntos();

    SceneDocument dst;
    REQUIRE(ParseSceneDocument(bytes.data(), bytes.size(), dst));

    REQUIRE(dst.nodes.size() == 2u);
    CHECK(dst.nodes[0].components.empty());            // la raiz no tenia flags

    const NodeRecord& n = dst.nodes[1];
    REQUIRE(n.components.size() == 2u);

    // El orden tiene que ser el de registro (malla, luz): si se invirtiera,
    // abrir y guardar un archivo viejo moveria bytes sin cambiar la escena.
    CHECK(dst.strings.View(n.components[0].typeNameStr) == kMeshComponentName);
    CHECK(dst.strings.View(n.components[1].typeNameStr) == kLightComponentName);

    // El resto del nodo cruza intacto.
    CHECK(dst.strings.View(n.nameStr) == "malla_y_luz");
    CHECK(n.parent == 0u);
    CHECK(n.local.position.x == doctest::Approx(4.0f));
    CHECK(n.local.position.z == doctest::Approx(6.0f));
    REQUIRE(n.behaviors.size() == 1u);
    CHECK(n.behaviors[0].enabled == 0u);
    CHECK(n.behaviors[0].payload == std::vector<u8>{ 0xDEu, 0xADu });

    CHECK(dst.iblIntensity == doctest::Approx(0.75f));
    CHECK(dst.primarySunNode == 1u);
}

TEST_CASE("SceneDocument: el payload que produce el shim es el cuerpo v4 tal cual") {
    const std::vector<u8> bytes = documentoV4ConAdjuntos();

    SceneDocument dst;
    REQUIRE(ParseSceneDocument(bytes.data(), bytes.size(), dst));
    REQUIRE(dst.nodes.size() == 2u);
    REQUIRE(dst.nodes[1].components.size() == 2u);

    // Los indices de string sobreviven porque el shim reusa la tabla del
    // documento: no re-internar nada es lo que mantiene los bytes iguales.
    const u32 sFbx = dst.strings.Intern("assets/models/x.fbx");
    const u32 sSub = dst.strings.Intern("submesh");
    AdjuntosV4Esperados esperados = adjuntosEsperados(sFbx, sSub);

    BinaryWriter wMesh;
    SerializePayload(wMesh, esperados.mesh);
    CHECK(dst.nodes[1].components[0].payload == wMesh.Buffer());

    BinaryWriter wLuz;
    SerializePayload(wLuz, esperados.light);
    CHECK(dst.nodes[1].components[1].payload == wLuz.Buffer());

    // Y decodifican a los valores originales.
    const MeshPayload m = leerPayload<MeshPayload>(dst.nodes[1].components[0]);
    CHECK(m.kind == MeshKind::Path);
    CHECK(dst.strings.View(m.str) == "assets/models/x.fbx");
    CHECK(m.genPayload == std::vector<u8>{ 7u, 8u, 9u });
    REQUIRE(m.bindings.size() == 1u);
    CHECK(dst.strings.View(m.bindings[0].submeshNameStr) == "submesh");

    const LightPayload l = leerPayload<LightPayload>(dst.nodes[1].components[1]);
    CHECK(l.kind == LightKindId::Directional);
    CHECK(l.color.x == doctest::Approx(1.5f));
    CHECK(l.color.z == doctest::Approx(3.5f));
}

TEST_CASE("SceneDocument: guardar un archivo v4 lo migra a v5 y queda estable") {
    const std::vector<u8> viejos = documentoV4ConAdjuntos();

    SceneDocument dst;
    REQUIRE(ParseSceneDocument(viejos.data(), viejos.size(), dst));

    std::vector<u8> nuevos;
    REQUIRE(WriteSceneDocument(dst, nuevos));

    // El writer siempre emite la version actual: abrir y guardar migra.
    u32 version = 0u;
    std::memcpy(&version, nuevos.data() + 8u, sizeof(u32));
    CHECK(version == kCurrentVersion);

    // Y el v5 resultante es punto fijo: releerlo y reescribirlo da lo mismo.
    SceneDocument round;
    REQUIRE(ParseSceneDocument(nuevos.data(), nuevos.size(), round));
    std::vector<u8> otraVez;
    REQUIRE(WriteSceneDocument(round, otraVez));
    CHECK(otraVez == nuevos);
}

TEST_CASE("SceneDocument: un componente de tipo desconocido no desalinea el nodo") {
    // Justo lo que el formato v<=4 no podia hacer: mesh y light iban
    // posicionales y sin tamano, asi que un adjunto no reconocido se comia el
    // resto del nodo. Con el payload dimensionado se puede saltear.
    SceneDocument src;

    NodeRecord root;
    root.nameStr = src.strings.Intern("root");
    root.parent  = kInvalidIndex;
    src.nodes.push_back(root);

    NodeRecord n;
    n.nameStr = src.strings.Intern("del_futuro");
    n.parent  = 0u;
    n.local.position = glm::vec3(9.0f, 8.0f, 7.0f);

    LightPayload lp;
    lp.kind  = LightKindId::Point;
    lp.color = glm::vec3(11.0f);
    n.components.push_back(componente(lp, kLightComponentName, src.strings));

    // Un componente que este build no conoce, con un payload de largo impar.
    ComponentRecord futuro;
    futuro.typeNameStr = src.strings.Intern("cliente.rigidbody");
    futuro.payload     = { 1u, 2u, 3u, 4u, 5u, 6u, 7u };
    n.components.push_back(futuro);

    BehaviorRecord b;
    b.typeNameStr = src.strings.Intern("Girar");
    b.payload = { 0x11u, 0x22u };
    n.behaviors.push_back(b);
    src.nodes.push_back(n);

    std::vector<u8> bytes;
    REQUIRE(WriteSceneDocument(src, bytes));

    SceneDocument dst;
    REQUIRE(ParseSceneDocument(bytes.data(), bytes.size(), dst));

    REQUIRE(dst.nodes.size() == 2u);
    const NodeRecord& d = dst.nodes[1];

    // El desconocido viaja entero, sin interpretarse.
    REQUIRE(d.components.size() == 2u);
    CHECK(dst.strings.View(d.components[1].typeNameStr) == "cliente.rigidbody");
    CHECK(d.components[1].payload == std::vector<u8>{ 1u, 2u, 3u, 4u, 5u, 6u, 7u });

    // Y lo que viene DESPUES del desconocido sigue alineado.
    REQUIRE(d.behaviors.size() == 1u);
    CHECK(dst.strings.View(d.behaviors[0].typeNameStr) == "Girar");
    CHECK(d.behaviors[0].payload == std::vector<u8>{ 0x11u, 0x22u });
    CHECK(d.local.position.x == doctest::Approx(9.0f));
}
