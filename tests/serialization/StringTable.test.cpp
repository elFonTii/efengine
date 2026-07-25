#include <ostream>   // doctest lo necesita para stringificar los string_view del CHECK
#include <doctest/doctest.h>
#include <efengine/serialization/StringTable.h>
#include <efengine/serialization/BinaryWriter.h>
#include <efengine/serialization/BinaryReader.h>
#include <efengine/serialization/EfeFile.h>
#include <string>

using namespace efengine;
using efengine::serialization::BinaryReader;
using efengine::serialization::BinaryWriter;
using efengine::serialization::StringTable;

TEST_CASE("EfeStringTable: el indice 0 es siempre la string vacia") {
    StringTable t;
    CHECK(t.Count() == 1u);
    CHECK(t.View(0u).empty());
    CHECK(t.Intern("") == 0u);
    CHECK(t.Count() == 1u);
}

TEST_CASE("EfeStringTable: Intern deduplica") {
    StringTable t;
    const u32 a = t.Intern("assets/models/rata.fbx");
    const u32 b = t.Intern("assets/models/lampara.fbx");
    const u32 a2 = t.Intern("assets/models/rata.fbx");

    CHECK(a != b);
    CHECK(a == a2);                 // la misma string dos veces da el mismo indice
    CHECK(t.Count() == 3u);         // "" + las dos distintas
    CHECK(t.View(a) == "assets/models/rata.fbx");
    CHECK(t.View(b) == "assets/models/lampara.fbx");
}

TEST_CASE("EfeStringTable: indice fuera de rango devuelve vacio, no OOB") {
    StringTable t;
    t.Intern("uno");

    CHECK(t.View(999u).empty());
    CHECK(t.View(serialization::kInvalidIndex).empty());
}

TEST_CASE("EfeStringTable: round-trip por el archivo preserva indices y contenido") {
    StringTable src;
    const u32 pbr    = src.Intern("pbr");
    const u32 vert   = src.Intern("assets/shaders/pbr.vert");
    const u32 vacia  = src.Intern("");
    const u32 largo  = src.Intern("assets/textures/street_rat/street_rat_diff_4k.jpg");

    BinaryWriter w;
    src.Serialize(w);

    StringTable dst;
    BinaryReader r(w.Buffer());
    dst.Serialize(r);

    CHECK(r.Ok());
    CHECK(r.Remaining() == 0u);
    CHECK(dst.Count() == src.Count());
    CHECK(dst.View(pbr)   == "pbr");
    CHECK(dst.View(vert)  == "assets/shaders/pbr.vert");
    CHECK(dst.View(vacia).empty());
    CHECK(dst.View(largo) == "assets/textures/street_rat/street_rat_diff_4k.jpg");
}

TEST_CASE("EfeStringTable: round-trip byte a byte") {
    StringTable src;
    src.Intern("a");
    src.Intern("bb");
    src.Intern("ccc");

    BinaryWriter w1;
    src.Serialize(w1);

    StringTable dst;
    BinaryReader r(w1.Buffer());
    dst.Serialize(r);
    REQUIRE(r.Ok());

    BinaryWriter w2;
    dst.Serialize(w2);

    CHECK(w2.Buffer() == w1.Buffer());
}

TEST_CASE("EfeStringTable: tras leer se puede seguir interneando sin colisionar") {
    StringTable src;
    const u32 uno = src.Intern("uno");

    BinaryWriter w;
    src.Serialize(w);

    StringTable dst;
    BinaryReader r(w.Buffer());
    dst.Serialize(r);
    REQUIRE(r.Ok());

    // El lookup se reconstruye al leer: internear algo que ya estaba no lo duplica.
    CHECK(dst.Intern("uno") == uno);
    const u32 dos = dst.Intern("dos");
    CHECK(dos != uno);
    CHECK(dst.View(dos) == "dos");
}

TEST_CASE("EfeStringTable: offsets corruptos se rechazan sin leer OOB") {
    // offsets = {0, 100} pero el blob tiene 3 bytes: el ultimo offset no coincide.
    BinaryWriter w;
    std::vector<u32> offsets = { 0u, 100u };
    std::vector<u8>  blob    = { 'a', 'b', 'c' };
    w.Array(offsets);
    w.Array(blob);

    StringTable dst;
    BinaryReader r(w.Buffer());
    dst.Serialize(r);

    CHECK_FALSE(r.Ok());
    CHECK(dst.View(1u).empty());   // no quedo una vista apuntando fuera del blob
}

TEST_CASE("EfeStringTable: offsets no monotonos se rechazan") {
    // offsets descendente: el largo de la string 0 seria negativo.
    BinaryWriter w;
    std::vector<u32> offsets = { 2u, 1u, 3u };
    std::vector<u8>  blob    = { 'a', 'b', 'c' };
    w.Array(offsets);
    w.Array(blob);

    StringTable dst;
    BinaryReader r(w.Buffer());
    dst.Serialize(r);

    CHECK_FALSE(r.Ok());
}

TEST_CASE("EfeStringTable: Clear vuelve al estado inicial") {
    StringTable t;
    t.Intern("algo");
    CHECK(t.Count() == 2u);

    t.Clear();

    CHECK(t.Count() == 1u);
    CHECK(t.View(0u).empty());
    CHECK(t.Intern("otra") == 1u);   // el indice 1 vuelve a estar libre
}
