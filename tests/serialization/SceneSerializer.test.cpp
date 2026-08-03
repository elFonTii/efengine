#include <doctest/doctest.h>
#include <efengine/serialization/SceneSerializer.h>
#include <efengine/serialization/SceneRegistry.h>
#include <efengine/resources/SceneAssets.h>
#include <efengine/resources/ResourceManager.h>
#include <efengine/scene/SceneGraph.h>
#include <efengine/scene/Node.h>
#include <efengine/renderer/MaterialDef.h>
#include <efengine/renderer/Material.h>
#include <memory>
#include <vector>

using namespace efengine;
using namespace efengine::serialization;

namespace {
    // Sin contexto GL no se pueden crear Model ni Texture, asi que estos tests cubren
    // todo lo que NO es malla: jerarquia, transforms, luces, behaviors y settings.
    struct GirarSerTest : scene::Behavior {
        f32 gradosPorSeg = 0.0f;
        void OnUpdate(scene::UpdateContext&) override {}
        template <class Ar> void Serialize(Ar& ar) { ar.Field(gradosPorSeg); }
    };

    struct OrbitarSerTest : scene::Behavior {
        glm::vec3 centro{0.0f};
        f32 radio = 0.0f;
        f32 angulo = 0.0f;
        void OnUpdate(scene::UpdateContext&) override {}
        template <class Ar> void Serialize(Ar& ar) {
            ar.Field(centro);
            ar.Field(radio);
            ar.Field(angulo);   // el autor decide si el estado acumulado se guarda
        }
    };

    // Grafo de referencia: raiz -> actor -> hijo, mas una luz puntual y el sol.
    struct Armado {
        scene::NodeHandle actor, hijo, luz, sol;
    };

    Armado armarGrafo(scene::SceneGraph& g) {
        Armado a;
        g.iblIntensity = 0.33f;

        a.actor = g.CreateNode("actor");
        math::Transform t;
        t.position = glm::vec3(1.0f, 2.0f, 3.0f);
        t.rotation = glm::vec3(0.0f, 45.0f, 0.0f);
        t.scale    = glm::vec3(10.0f);
        g.SetLocalTransform(a.actor, t);

        a.hijo = g.CreateChild(a.actor, "hijo");
        math::Transform th;
        th.position = glm::vec3(0.0f, 5.0f, 0.0f);
        g.SetLocalTransform(a.hijo, th);

        a.luz = g.CreateNode("luz_1");
        math::Transform tl;
        tl.position = glm::vec3(50.0f, 80.0f, 0.0f);
        g.SetLocalTransform(a.luz, tl);
        g.AttachLight(a.luz, { scene::LightKind::Point, glm::vec3(5000.0f) });

        a.sol = g.CreateNode("sol");
        math::Transform ts;
        ts.rotation = glm::vec3(-70.0f, 56.0f, 0.0f);
        g.SetLocalTransform(a.sol, ts);
        g.AttachLight(a.sol, { scene::LightKind::Directional, glm::vec3(3.0f) });
        g.SetPrimarySun(a.sol);

        auto girar = std::make_unique<GirarSerTest>();
        girar->gradosPorSeg = 20.0f;
        g.AttachBehavior(a.actor, std::move(girar));

        auto orbitar = std::make_unique<OrbitarSerTest>();
        orbitar->centro = glm::vec3(0.0f, 80.0f, 0.0f);
        orbitar->radio  = 50.0f;
        orbitar->angulo = 1.25f;
        scene::Behavior* p = g.AttachBehavior(a.luz, std::move(orbitar));
        p->enabled = false;                    // se guarda deshabilitado a proposito

        return a;
    }

    SceneRegistry armarRegistry() {
        SceneRegistry reg;
        reg.behaviors.Register<GirarSerTest>("GirarSerTest");
        reg.behaviors.Register<OrbitarSerTest>("OrbitarSerTest");
        return reg;
    }
}

TEST_CASE("EfeSceneSerializer: round-trip de jerarquia, transforms, luces y behaviors") {
    scene::SceneGraph origen;
    armarGrafo(origen);

    resources::SceneAssets assetsOrigen;
    resources::ResourceManager rm;
    const SceneRegistry reg = armarRegistry();

    std::vector<u8> bytes;
    REQUIRE(SceneSerializer::SaveToBytes(origen, assetsOrigen, rm, reg, bytes));
    CHECK(bytes.size() > 20u);

    scene::SceneGraph destino;
    resources::SceneAssets assetsDestino;
    REQUIRE(SceneSerializer::LoadFromBytes(bytes.data(), bytes.size(),
                                           destino, assetsDestino, rm, reg));

    // --- settings ---
    CHECK(destino.iblIntensity == doctest::Approx(0.33f));

    // --- jerarquia por nombre (los handles son nuevos) ---
    scene::NodeHandle actor = destino.FindByName("actor");
    scene::NodeHandle hijo  = destino.FindByName("hijo");
    scene::NodeHandle luz   = destino.FindByName("luz_1");
    scene::NodeHandle sol   = destino.FindByName("sol");
    REQUIRE(destino.IsValid(actor));
    REQUIRE(destino.IsValid(hijo));
    REQUIRE(destino.IsValid(luz));
    REQUIRE(destino.IsValid(sol));

    CHECK(destino.Get(hijo).parent == actor);              // tercer nivel preservado
    CHECK(destino.Get(actor).parent == destino.Root());
    CHECK(destino.Get(destino.Root()).children.size() == 3u);

    // --- transforms ---
    CHECK(destino.Get(actor).local.position.x == doctest::Approx(1.0f));
    CHECK(destino.Get(actor).local.rotation.y == doctest::Approx(45.0f));
    CHECK(destino.Get(actor).local.scale.z == doctest::Approx(10.0f));
    CHECK(destino.Get(hijo).local.position.y == doctest::Approx(5.0f));

    // --- world derivado: el hijo hereda del padre ---
    destino.UpdateWorldTransforms();
    CHECK(glm::vec3(destino.Get(hijo).worldMatrix[3]).y == doctest::Approx(52.0f));  // 2 + 5*10

    // --- luces ---
    REQUIRE(destino.Get(luz).light.has_value());
    CHECK(destino.Get(luz).light->kind == scene::LightKind::Point);
    CHECK(destino.Get(luz).light->color.x == doctest::Approx(5000.0f));
    REQUIRE(destino.Get(sol).light.has_value());
    CHECK(destino.Get(sol).light->kind == scene::LightKind::Directional);

    // --- sol primario ---
    CHECK(destino.PrimarySun() == sol);
    CHECK(destino.Sun().color.x == doctest::Approx(3.0f));
    CHECK(destino.PointLights().size() == 1u);

    // --- behaviors: tipo, params y enabled ---
    REQUIRE(destino.Get(actor).behaviors.size() == 1u);
    GirarSerTest* girar = dynamic_cast<GirarSerTest*>(destino.Get(actor).behaviors[0].get());
    REQUIRE(girar != nullptr);
    CHECK(girar->gradosPorSeg == doctest::Approx(20.0f));
    CHECK(girar->enabled == true);

    REQUIRE(destino.Get(luz).behaviors.size() == 1u);
    OrbitarSerTest* orb = dynamic_cast<OrbitarSerTest*>(destino.Get(luz).behaviors[0].get());
    REQUIRE(orb != nullptr);
    CHECK(orb->centro.y == doctest::Approx(80.0f));
    CHECK(orb->radio == doctest::Approx(50.0f));
    CHECK(orb->angulo == doctest::Approx(1.25f));       // el estado acumulado se reanudo
    CHECK(orb->enabled == false);                        // el flag viajo en el archivo
}

TEST_CASE("EfeSceneSerializer: round-trip byte a byte a traves del grafo") {
    scene::SceneGraph origen;
    armarGrafo(origen);
    resources::SceneAssets assets;
    resources::ResourceManager rm;
    const SceneRegistry reg = armarRegistry();

    std::vector<u8> bytes1;
    REQUIRE(SceneSerializer::SaveToBytes(origen, assets, rm, reg, bytes1));

    scene::SceneGraph destino;
    resources::SceneAssets assetsDestino;
    REQUIRE(SceneSerializer::LoadFromBytes(bytes1.data(), bytes1.size(),
                                           destino, assetsDestino, rm, reg));

    std::vector<u8> bytes2;
    REQUIRE(SceneSerializer::SaveToBytes(destino, assetsDestino, rm, reg, bytes2));

    // Guardar -> cargar -> guardar tiene que dar exactamente los mismos bytes.
    CHECK(bytes2 == bytes1);
}

TEST_CASE("EfeSceneSerializer: un behavior con tipo no registrado se saltea al cargar") {
    scene::SceneGraph origen;
    Armado a = armarGrafo(origen);
    resources::SceneAssets assets;
    resources::ResourceManager rm;

    std::vector<u8> bytes;
    REQUIRE(SceneSerializer::SaveToBytes(origen, assets, rm, armarRegistry(), bytes));

    // Se carga con un registry que solo conoce uno de los dos tipos.
    SceneRegistry parcial;
    parcial.behaviors.Register<GirarSerTest>("GirarSerTest");

    scene::SceneGraph destino;
    resources::SceneAssets assetsDestino;
    REQUIRE(SceneSerializer::LoadFromBytes(bytes.data(), bytes.size(),
                                           destino, assetsDestino, rm, parcial));

    scene::NodeHandle actor = destino.FindByName("actor");
    scene::NodeHandle luz   = destino.FindByName("luz_1");
    REQUIRE(destino.IsValid(actor));
    REQUIRE(destino.IsValid(luz));

    CHECK(destino.Get(actor).behaviors.size() == 1u);   // el conocido si se cargo
    CHECK(destino.Get(luz).behaviors.empty());          // el desconocido se salteo
    CHECK(destino.Get(luz).light.has_value());          // y el resto del nodo quedo intacto
}

TEST_CASE("EfeSceneSerializer: un behavior sin registrar al guardar no rompe el archivo") {
    scene::SceneGraph origen;
    Armado a = armarGrafo(origen);
    resources::SceneAssets assets;
    resources::ResourceManager rm;

    // Registry que no conoce OrbitarSerTest: ese behavior no se escribe, con warning.
    SceneRegistry parcial;
    parcial.behaviors.Register<GirarSerTest>("GirarSerTest");

    std::vector<u8> bytes;
    REQUIRE(SceneSerializer::SaveToBytes(origen, assets, rm, parcial, bytes));

    scene::SceneGraph destino;
    resources::SceneAssets assetsDestino;
    REQUIRE(SceneSerializer::LoadFromBytes(bytes.data(), bytes.size(),
                                           destino, assetsDestino, rm, parcial));

    CHECK(destino.Get(destino.FindByName("actor")).behaviors.size() == 1u);
    CHECK(destino.Get(destino.FindByName("luz_1")).behaviors.empty());
}

TEST_CASE("EfeSceneSerializer: cargar sobre un grafo vivo lo reemplaza") {
    scene::SceneGraph origen;
    armarGrafo(origen);
    resources::SceneAssets assets;
    resources::ResourceManager rm;
    const SceneRegistry reg = armarRegistry();

    std::vector<u8> bytes;
    REQUIRE(SceneSerializer::SaveToBytes(origen, assets, rm, reg, bytes));

    // El destino ya tiene contenido propio, distinto del que viene en el archivo.
    scene::SceneGraph destino;
    scene::NodeHandle basura = destino.CreateNode("basura_previa");
    resources::SceneAssets assetsDestino;

    REQUIRE(SceneSerializer::LoadFromBytes(bytes.data(), bytes.size(),
                                           destino, assetsDestino, rm, reg));

    CHECK(destino.FindByName("basura_previa").IsNull());
    CHECK_FALSE(destino.IsValid(basura));                 // handle viejo invalidado
    CHECK(destino.IsValid(destino.FindByName("actor")));
}

TEST_CASE("EfeSceneSerializer: bytes corruptos no destruyen la escena cargada") {
    scene::SceneGraph origen;
    armarGrafo(origen);
    resources::SceneAssets assets;
    resources::ResourceManager rm;
    const SceneRegistry reg = armarRegistry();

    std::vector<u8> bytes;
    REQUIRE(SceneSerializer::SaveToBytes(origen, assets, rm, reg, bytes));

    scene::SceneGraph vivo;
    resources::SceneAssets assetsVivo;
    REQUIRE(SceneSerializer::LoadFromBytes(bytes.data(), bytes.size(),
                                           vivo, assetsVivo, rm, reg));
    REQUIRE(vivo.IsValid(vivo.FindByName("actor")));

    // Un archivo que no se entiende falla en el parse, ANTES del Clear del resolve.
    const std::vector<u8> basura = { 0xDEu, 0xADu, 0xBEu, 0xEFu };
    CHECK_FALSE(SceneSerializer::LoadFromBytes(basura.data(), basura.size(),
                                               vivo, assetsVivo, rm, reg));

    CHECK(vivo.IsValid(vivo.FindByName("actor")));        // la escena sigue en pie
    CHECK(vivo.iblIntensity == doctest::Approx(0.33f));
}

TEST_CASE("EfeSceneSerializer: grafo con solo la raiz round-trip") {
    scene::SceneGraph origen;
    resources::SceneAssets assets;
    resources::ResourceManager rm;
    const SceneRegistry reg;

    std::vector<u8> bytes;
    REQUIRE(SceneSerializer::SaveToBytes(origen, assets, rm, reg, bytes));

    scene::SceneGraph destino;
    resources::SceneAssets assetsDestino;
    REQUIRE(SceneSerializer::LoadFromBytes(bytes.data(), bytes.size(),
                                           destino, assetsDestino, rm, reg));

    CHECK(destino.IsValid(destino.Root()));
    CHECK(destino.Get(destino.Root()).children.empty());
    CHECK(destino.PrimarySun().IsNull());
}

TEST_CASE("EfeSceneSerializer: doubleSided del MaterialDef llega al MaterialRecord") {
    scene::SceneGraph grafo;
    grafo.CreateNode("root");

    resources::SceneAssets assets;
    renderer::MaterialDef def;
    def.name        = "vidrio";
    def.shaderName  = "pbr";
    def.doubleSided = true;
    // Material headless: Extract solo lee el DEF, nunca toca el Material.
    assets.AddMaterial(def, renderer::Material(nullptr));

    resources::ResourceManager rm;
    const SceneRegistry reg = armarRegistry();

    SceneDocument doc;
    REQUIRE(SceneSerializer::Extract(grafo, assets, rm, reg, doc));
    REQUIRE(doc.materials.size() == 1u);
    CHECK(doc.materials[0].doubleSided == 1u);
}

TEST_CASE("EfeSceneSerializer: un material de una sola cara sale con doubleSided en 0") {
    scene::SceneGraph grafo;
    grafo.CreateNode("root");

    resources::SceneAssets assets;
    renderer::MaterialDef def;
    def.name       = "pared";
    def.shaderName = "pbr";
    assets.AddMaterial(def, renderer::Material(nullptr));

    resources::ResourceManager rm;
    const SceneRegistry reg = armarRegistry();

    SceneDocument doc;
    REQUIRE(SceneSerializer::Extract(grafo, assets, rm, reg, doc));
    REQUIRE(doc.materials.size() == 1u);
    CHECK(doc.materials[0].doubleSided == 0u);
}

TEST_CASE("EfeSceneSerializer: el tiling de UV del MaterialDef llega al MaterialRecord") {
    scene::SceneGraph grafo;
    grafo.CreateNode("root");

    resources::SceneAssets assets;
    renderer::MaterialDef def;
    def.name       = "ladrillo";
    def.shaderName = "pbr";
    def.uvTiling   = glm::vec2(6.0f, 3.0f);
    def.uvOffset   = glm::vec2(0.125f, -0.25f);
    // Material headless: Extract solo lee el DEF, nunca toca el Material.
    assets.AddMaterial(def, renderer::Material(nullptr));

    resources::ResourceManager rm;
    const SceneRegistry reg = armarRegistry();

    SceneDocument doc;
    REQUIRE(SceneSerializer::Extract(grafo, assets, rm, reg, doc));
    REQUIRE(doc.materials.size() == 1u);
    CHECK(doc.materials[0].uvTiling.x == doctest::Approx(6.0f));
    CHECK(doc.materials[0].uvTiling.y == doctest::Approx(3.0f));
    CHECK(doc.materials[0].uvOffset.x == doctest::Approx(0.125f));
    CHECK(doc.materials[0].uvOffset.y == doctest::Approx(-0.25f));
}

TEST_CASE("EfeSceneSerializer: un material sin tocar sale con la UV en identidad") {
    scene::SceneGraph grafo;
    grafo.CreateNode("root");

    resources::SceneAssets assets;
    renderer::MaterialDef def;
    def.name       = "pared";
    def.shaderName = "pbr";
    assets.AddMaterial(def, renderer::Material(nullptr));

    resources::ResourceManager rm;
    const SceneRegistry reg = armarRegistry();

    SceneDocument doc;
    REQUIRE(SceneSerializer::Extract(grafo, assets, rm, reg, doc));
    REQUIRE(doc.materials.size() == 1u);
    CHECK(doc.materials[0].uvTiling.x == doctest::Approx(1.0f));
    CHECK(doc.materials[0].uvOffset.x == doctest::Approx(0.0f));
}

TEST_CASE("EfeSceneAssets: indices, lookup inverso y Clear") {
    resources::SceneAssets assets;
    CHECK(assets.MaterialCount() == 0u);
    CHECK(assets.GeneratedCount() == 0u);
    CHECK(assets.IndexOfMaterial(nullptr) == resources::SceneAssets::kInvalidIndex);
    CHECK(assets.IndexOfGenerated(nullptr) == resources::SceneAssets::kInvalidIndex);

    // AddGenerated con model nulo: alcanza para probar indices y payload sin GL.
    const std::vector<u8> payload = { 1u, 2u, 3u, 4u };
    const u32 i = assets.AddGenerated("sandbox.plane", payload, nullptr);
    CHECK(i == 0u);
    CHECK(assets.GeneratedCount() == 1u);
    CHECK(assets.GeneratorNameAt(0u) == "sandbox.plane");
    CHECK(assets.GeneratorPayloadAt(0u) == payload);

    assets.Clear();
    CHECK(assets.GeneratedCount() == 0u);
}
