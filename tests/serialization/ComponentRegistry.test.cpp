#include <ostream>   // doctest lo necesita para stringificar strings en los CHECK
#include <doctest/doctest.h>
#include <efengine/serialization/ComponentRegistry.h>
#include <efengine/serialization/SceneRegistry.h>
#include <efengine/scene/ComponentStore.h>
#include <efengine/resources/ResourceManager.h>
#include <efengine/resources/SceneAssets.h>

#include <string>
#include <vector>

using namespace efengine;
using namespace efengine::serialization;

namespace {

    // Componentes de prueba: datos planos, sin herencia y sin metodos. Es la
    // forma que van a tener fisica, audio y animacion.
    struct RigidBodyTest {
        f32 masa       = 0.0f;
        u32 cinematico = 0u;
    };

    struct SonidoTest {
        f32 volumen = 0.0f;
    };

    // Sin estado: su payload tiene que quedar vacio.
    struct MarcaTest {};

    // Un componente que decide no guardarse (como una malla sin path ni
    // generador): el registro tiene que descartar el record entero.
    struct NuncaSeGuardaTest {
        f32 loQueSea = 0.0f;
    };

    // El punto de extension es una funcion libre, no un metodo: asi los tipos de
    // scene/ quedan como datos planos y la logica de archivo vive en serialization/.
    template <class Ar>
    bool SerializeComponent(Ar& ar, RigidBodyTest& c) {
        ar.Field(c.masa);
        ar.Field(c.cinematico);
        return true;
    }

    template <class Ar>
    bool SerializeComponent(Ar& ar, SonidoTest& c) {
        ar.Field(c.volumen);
        return true;
    }

    template <class Ar>
    bool SerializeComponent(Ar&, MarcaTest&) { return true; }

    template <class Ar>
    bool SerializeComponent(Ar& ar, NuncaSeGuardaTest& c) {
        if constexpr (!Ar::kIsReading) return false;   // nunca se emite
        ar.Field(c.loQueSea);
        return true;
    }

    // Contexto minimo para escribir y leer payloads sueltos. Malla es el unico
    // componente que usa assets/rm; estos tests no los tocan, pero las fachadas
    // los piden igual.
    struct Contexto {
        StringTable                strings;
        resources::SceneAssets     assets;
        resources::ResourceManager rm;
        MeshGeneratorRegistry      generadores;

        ComponentWriter writer(BinaryWriter& w) {
            return ComponentWriter{ w, strings, assets, rm, "nodoTest" };
        }
        ComponentReader reader(BinaryReader& r) {
            return ComponentReader{ r, strings, assets, rm, generadores, "nodoTest" };
        }
    };

    // Escribe el componente que 'store' tenga en 'nodo' para la entry dada.
    bool escribir(const ComponentRegistry::Entry& e, Contexto& ctx,
                  const scene::ComponentStore& store, u32 nodo, std::vector<u8>& out) {
        BinaryWriter w;
        ComponentWriter ar = ctx.writer(w);
        if (!e.write(ar, store, nodo)) return false;
        out = w.Take();
        return true;
    }

    bool leer(const ComponentRegistry::Entry& e, Contexto& ctx,
              scene::ComponentStore& store, u32 nodo, const std::vector<u8>& bytes) {
        BinaryReader r(bytes);
        ComponentReader ar = ctx.reader(r);
        return e.read(ar, store, nodo);
    }
}

TEST_CASE("EfeComponentRegistry: round-trip de los campos de un componente") {
    ComponentRegistry reg;
    reg.Register<RigidBodyTest>("test.rigidbody");

    Contexto ctx;
    scene::ComponentStore origen;
    origen.Set<RigidBodyTest>(3u, RigidBodyTest{ 12.5f, 1u });

    const ComponentRegistry::Entry* e = reg.FindByName("test.rigidbody");
    REQUIRE(e != null);

    std::vector<u8> bytes;
    REQUIRE(escribir(*e, ctx, origen, 3u, bytes));
    CHECK(bytes.size() == sizeof(f32) + sizeof(u32));

    scene::ComponentStore destino;
    REQUIRE(leer(*e, ctx, destino, 7u, bytes));    // otro indice de nodo a proposito

    const RigidBodyTest* c = destino.TryGet<RigidBodyTest>(7u);
    REQUIRE(c != null);
    CHECK(c->masa == doctest::Approx(12.5f));
    CHECK(c->cinematico == 1u);
}

TEST_CASE("EfeComponentRegistry: un nodo sin el componente no emite record") {
    ComponentRegistry reg;
    reg.Register<RigidBodyTest>("test.rigidbody");

    Contexto ctx;
    scene::ComponentStore store;        // vacio

    std::vector<u8> bytes;
    CHECK_FALSE(escribir(*reg.FindByName("test.rigidbody"), ctx, store, 0u, bytes));
}

TEST_CASE("EfeComponentRegistry: si SerializeComponent devuelve false no se emite nada") {
    ComponentRegistry reg;
    reg.Register<NuncaSeGuardaTest>("test.nunca");

    Contexto ctx;
    scene::ComponentStore store;
    store.Set<NuncaSeGuardaTest>(0u, NuncaSeGuardaTest{ 1.0f });

    // El componente ESTA en el nodo, pero se niega a guardarse. Es el camino de
    // la malla sin path ni generador.
    std::vector<u8> bytes;
    CHECK_FALSE(escribir(*reg.FindByName("test.nunca"), ctx, store, 0u, bytes));
}

TEST_CASE("EfeComponentRegistry: dos tipos distintos no colisionan") {
    ComponentRegistry reg;
    reg.Register<RigidBodyTest>("test.rigidbody");
    reg.Register<SonidoTest>("test.sonido");

    Contexto ctx;
    scene::ComponentStore store;
    store.Set<RigidBodyTest>(0u, RigidBodyTest{ 1.0f, 0u });
    store.Set<SonidoTest>(0u, SonidoTest{ 0.5f });

    std::vector<u8> bRigid, bSonido;
    REQUIRE(escribir(*reg.FindByName("test.rigidbody"), ctx, store, 0u, bRigid));
    REQUIRE(escribir(*reg.FindByName("test.sonido"), ctx, store, 0u, bSonido));

    // Cada entry escribio SU tipo: los tamanos lo delatan.
    CHECK(bRigid.size() == sizeof(f32) + sizeof(u32));
    CHECK(bSonido.size() == sizeof(f32));
}

TEST_CASE("EfeComponentRegistry: un componente sin estado produce payload vacio") {
    ComponentRegistry reg;
    reg.Register<MarcaTest>("test.marca");

    Contexto ctx;
    scene::ComponentStore store;
    store.Set<MarcaTest>(0u, MarcaTest{});

    std::vector<u8> bytes;
    REQUIRE(escribir(*reg.FindByName("test.marca"), ctx, store, 0u, bytes));
    CHECK(bytes.empty());

    scene::ComponentStore destino;
    REQUIRE(leer(*reg.FindByName("test.marca"), ctx, destino, 0u, bytes));
    CHECK(destino.Has<MarcaTest>(0u));
}

TEST_CASE("EfeComponentRegistry: un tipo no registrado devuelve null") {
    ComponentRegistry reg;
    reg.Register<RigidBodyTest>("test.rigidbody");

    CHECK(reg.FindByName("test.noexiste") == null);
    CHECK(reg.FindByName("") == null);
}

TEST_CASE("EfeComponentRegistry: All() respeta el orden de registro") {
    ComponentRegistry reg;
    reg.Register<SonidoTest>("test.sonido");
    reg.Register<RigidBodyTest>("test.rigidbody");
    reg.Register<MarcaTest>("test.marca");

    // El orden de emision es el de registro, no el de un hash: si variara, dos
    // guardados de la misma escena darian bytes distintos.
    REQUIRE(reg.All().size() == 3u);
    CHECK(reg.All()[0].name == "test.sonido");
    CHECK(reg.All()[1].name == "test.rigidbody");
    CHECK(reg.All()[2].name == "test.marca");
}

TEST_CASE("EfeComponentRegistry: FindByName y All() apuntan a la misma entry") {
    ComponentRegistry reg;
    reg.Register<SonidoTest>("test.sonido");
    reg.Register<RigidBodyTest>("test.rigidbody");

    CHECK(reg.FindByName("test.sonido") == &reg.All()[0]);
    CHECK(reg.FindByName("test.rigidbody") == &reg.All()[1]);
}

TEST_CASE("EfeComponentRegistry: un nombre repetido no pisa al primero") {
    ComponentRegistry reg;
    reg.Register<RigidBodyTest>("test.dup");
    reg.Register<SonidoTest>("test.dup");     // loguea warning y no entra

    REQUIRE(reg.All().size() == 1u);

    Contexto ctx;
    scene::ComponentStore store;
    store.Set<RigidBodyTest>(0u, RigidBodyTest{ 2.0f, 0u });
    store.Set<SonidoTest>(0u, SonidoTest{ 9.0f });

    // La entry que sobrevive es la primera: escribe el RigidBody.
    std::vector<u8> bytes;
    REQUIRE(escribir(*reg.FindByName("test.dup"), ctx, store, 0u, bytes));
    CHECK(bytes.size() == sizeof(f32) + sizeof(u32));
}

TEST_CASE("EfeComponentRegistry: un payload truncado deja el reader en falla") {
    ComponentRegistry reg;
    reg.Register<RigidBodyTest>("test.rigidbody");

    Contexto ctx;
    scene::ComponentStore store;

    std::vector<u8> cortado = { 0x00u, 0x00u };   // menos de un f32
    BinaryReader r(cortado);
    ComponentReader ar = ctx.reader(r);
    reg.FindByName("test.rigidbody")->read(ar, store, 0u);

    // Lo que importa es que se note: el serializador loguea el warning y sigue
    // con el resto del archivo, porque el payload tiene largo propio.
    CHECK_FALSE(r.Ok());
}

TEST_CASE("EfeSceneRegistry: los componentes del motor vienen registrados solos") {
    SceneRegistry reg;

    // Nadie los registro: si hubiera que acordarse, olvidarse significaria
    // cargar todas las escenas sin mallas ni luces.
    REQUIRE(reg.components.All().size() == 2u);
    CHECK(reg.components.All()[0].name == kMeshComponentName);
    CHECK(reg.components.All()[1].name == kLightComponentName);
    CHECK(reg.components.FindByName(kMeshComponentName) != null);
    CHECK(reg.components.FindByName(kLightComponentName) != null);

    // Y los otros dos registros siguen arrancando vacios: son del cliente.
    CHECK(reg.meshes.Find("sandbox.plane") == null);
}
