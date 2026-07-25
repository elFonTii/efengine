#include <ostream>   // doctest lo necesita para stringificar strings en los CHECK
#include <doctest/doctest.h>
#include <efengine/serialization/BehaviorRegistry.h>
#include <efengine/serialization/MeshGeneratorRegistry.h>
#include <efengine/serialization/SceneRegistry.h>
#include <efengine/serialization/BinaryWriter.h>
#include <efengine/serialization/BinaryReader.h>
#include <efengine/scene/Behavior.h>
#include <memory>
#include <vector>

using namespace efengine;
using namespace efengine::serialization;

namespace {
    // Behavior de prueba con dos parametros: se serializan con UNA sola funcion.
    struct GirarTest : scene::Behavior {
        f32 gradosPorSeg = 0.0f;
        u32 vueltas      = 0u;

        void OnUpdate(scene::UpdateContext&) override {}

        template <class Ar>
        void Serialize(Ar& ar) {
            ar.Field(gradosPorSeg);
            ar.Field(vueltas);
        }
    };

    // Segundo tipo, para verificar que no colisionan por typeid.
    struct MoverTest : scene::Behavior {
        f32 velocidad = 0.0f;

        void OnUpdate(scene::UpdateContext&) override {}

        template <class Ar>
        void Serialize(Ar& ar) { ar.Field(velocidad); }
    };

    // Behavior sin estado: su payload tiene que quedar vacio.
    struct PeladoTest : scene::Behavior {
        void OnUpdate(scene::UpdateContext&) override {}
        template <class Ar> void Serialize(Ar&) {}
    };

    // El generador de prueba no construye un Model de verdad (haria falta GL): devuelve
    // null pero SI consume sus params, que es lo que el test verifica.
    f32 g_ultimoHalfSize = 0.0f;
    f32 g_ultimoTiles    = 0.0f;

    struct PlanoParamsTest {
        f32 halfSize = 0.0f;
        f32 tiles    = 0.0f;
        template <class Ar> void Serialize(Ar& ar) { ar.Field(halfSize); ar.Field(tiles); }
    };
}

TEST_CASE("EfeBehaviorRegistry: round-trip de los parametros de un behavior") {
    BehaviorRegistry reg;
    reg.Register<GirarTest>("GirarTest");

    GirarTest original;
    original.gradosPorSeg = 20.0f;
    original.vueltas      = 7u;

    const BehaviorRegistry::Entry* porTipo = reg.FindByType(original);
    REQUIRE(porTipo != nullptr);
    CHECK(porTipo->name == "GirarTest");

    BinaryWriter w;
    porTipo->write(w, original);
    CHECK(w.Size() == sizeof(f32) + sizeof(u32));

    const BehaviorRegistry::Entry* porNombre = reg.FindByName("GirarTest");
    REQUIRE(porNombre != nullptr);
    std::unique_ptr<scene::Behavior> creado = porNombre->create();
    REQUIRE(creado != nullptr);

    BinaryReader r(w.Buffer());
    porNombre->read(r, *creado);

    CHECK(r.Ok());
    CHECK(r.Remaining() == 0u);
    GirarTest* tipado = dynamic_cast<GirarTest*>(creado.get());
    REQUIRE(tipado != nullptr);
    CHECK(tipado->gradosPorSeg == doctest::Approx(20.0f));
    CHECK(tipado->vueltas == 7u);
}

TEST_CASE("EfeBehaviorRegistry: create devuelve una instancia con valores por defecto") {
    BehaviorRegistry reg;
    reg.Register<GirarTest>("GirarTest");

    const BehaviorRegistry::Entry* e = reg.FindByName("GirarTest");
    REQUIRE(e != nullptr);
    std::unique_ptr<scene::Behavior> creado = e->create();
    REQUIRE(creado != nullptr);

    GirarTest* tipado = dynamic_cast<GirarTest*>(creado.get());
    REQUIRE(tipado != nullptr);
    CHECK(tipado->gradosPorSeg == doctest::Approx(0.0f));
    CHECK(tipado->vueltas == 0u);
    CHECK(tipado->enabled == true);       // el default de scene::Behavior
}

TEST_CASE("EfeBehaviorRegistry: dos tipos distintos no colisionan") {
    BehaviorRegistry reg;
    reg.Register<GirarTest>("GirarTest");
    reg.Register<MoverTest>("MoverTest");

    GirarTest girar;
    MoverTest mover;
    mover.velocidad = 3.5f;

    const BehaviorRegistry::Entry* eGirar = reg.FindByType(girar);
    const BehaviorRegistry::Entry* eMover = reg.FindByType(mover);
    REQUIRE(eGirar != nullptr);
    REQUIRE(eMover != nullptr);
    CHECK(eGirar->name == "GirarTest");
    CHECK(eMover->name == "MoverTest");

    // El write de MoverTest escribe un solo f32, no los dos campos de GirarTest.
    BinaryWriter w;
    eMover->write(w, mover);
    CHECK(w.Size() == sizeof(f32));
}

TEST_CASE("EfeBehaviorRegistry: FindByType a traves de la clase base usa el tipo real") {
    BehaviorRegistry reg;
    reg.Register<MoverTest>("MoverTest");

    MoverTest concreto;
    scene::Behavior& comoBase = concreto;      // se consulta por referencia a la base

    const BehaviorRegistry::Entry* e = reg.FindByType(comoBase);
    REQUIRE(e != nullptr);
    CHECK(e->name == "MoverTest");
}

TEST_CASE("EfeBehaviorRegistry: tipo no registrado devuelve null en las dos busquedas") {
    BehaviorRegistry reg;
    reg.Register<GirarTest>("GirarTest");

    MoverTest noRegistrado;
    CHECK(reg.FindByType(noRegistrado) == nullptr);
    CHECK(reg.FindByName("MoverTest") == nullptr);
    CHECK(reg.FindByName("") == nullptr);
}

TEST_CASE("EfeBehaviorRegistry: un behavior sin estado produce payload vacio") {
    BehaviorRegistry reg;
    reg.Register<PeladoTest>("PeladoTest");

    PeladoTest pelado;
    const BehaviorRegistry::Entry* e = reg.FindByType(pelado);
    REQUIRE(e != nullptr);

    BinaryWriter w;
    e->write(w, pelado);
    CHECK(w.Size() == 0u);

    std::unique_ptr<scene::Behavior> creado = e->create();
    BinaryReader r(w.Buffer());
    e->read(r, *creado);
    CHECK(r.Ok());
}

TEST_CASE("EfeMeshGeneratorRegistry: nombre no registrado devuelve null") {
    MeshGeneratorRegistry reg;
    CHECK(reg.Find("sandbox.plane") == nullptr);
}

TEST_CASE("EfeMeshGeneratorRegistry: CreateGenerated serializa los params y el generador los lee") {
    MeshGeneratorRegistry reg;
    reg.Register("plano.test", [](BinaryReader& r) -> std::unique_ptr<renderer::Model> {
        PlanoParamsTest p;
        p.Serialize(r);
        g_ultimoHalfSize = p.halfSize;
        g_ultimoTiles    = p.tiles;
        return nullptr;   // construir un Model de verdad necesitaria contexto GL
    });

    g_ultimoHalfSize = 0.0f;
    g_ultimoTiles    = 0.0f;

    PlanoParamsTest params;
    params.halfSize = 300.0f;
    params.tiles    = 24.0f;

    std::vector<u8> payload;
    std::unique_ptr<renderer::Model> modelo =
        CreateGenerated(reg, "plano.test", params, payload);

    CHECK(modelo == nullptr);                         // esperado en el doble de prueba
    CHECK(payload.size() == 2u * sizeof(f32));        // los params quedaron serializados
    CHECK(g_ultimoHalfSize == doctest::Approx(300.0f));
    CHECK(g_ultimoTiles == doctest::Approx(24.0f));
}

TEST_CASE("EfeMeshGeneratorRegistry: CreateGenerated con generador desconocido no deja payload") {
    MeshGeneratorRegistry reg;

    PlanoParamsTest params;
    params.halfSize = 1.0f;

    std::vector<u8> payload = { 0xFFu };
    std::unique_ptr<renderer::Model> modelo =
        CreateGenerated(reg, "no.existe", params, payload);

    CHECK(modelo == nullptr);
    CHECK(payload.empty());        // no se guarda payload de algo que no se pudo generar
}

TEST_CASE("EfeSceneRegistry: agrupa los dos registries") {
    SceneRegistry reg;
    reg.behaviors.Register<GirarTest>("GirarTest");
    reg.meshes.Register("plano.test", [](BinaryReader&) -> std::unique_ptr<renderer::Model> {
        return nullptr;
    });

    CHECK(reg.behaviors.FindByName("GirarTest") != nullptr);
    CHECK(reg.meshes.Find("plano.test") != nullptr);
}
