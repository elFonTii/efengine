#include <doctest/doctest.h>
#include <efengine/serialization/BinaryWriter.h>
#include <efengine/serialization/BinaryReader.h>
#include <glm/glm.hpp>
#include <vector>

using namespace efengine;
using efengine::serialization::BinaryReader;
using efengine::serialization::BinaryWriter;

TEST_CASE("EfeBinaryIO: round-trip de primitivas y vec3") {
    u32 a = 0xDEADBEEFu;
    f32 b = 3.5f;
    glm::vec3 c{1.0f, -2.0f, 42.0f};

    BinaryWriter w;
    w.Field(a);
    w.Field(b);
    w.Field(c);
    CHECK(w.Size() == sizeof(u32) + sizeof(f32) + sizeof(glm::vec3));

    u32 a2 = 0; f32 b2 = 0.0f; glm::vec3 c2{0.0f};
    BinaryReader r(w.Buffer());
    r.Field(a2);
    r.Field(b2);
    r.Field(c2);

    CHECK(r.Ok());
    CHECK(r.Remaining() == 0u);
    CHECK(a2 == a);
    CHECK(b2 == doctest::Approx(b));
    CHECK(c2.x == doctest::Approx(c.x));
    CHECK(c2.y == doctest::Approx(c.y));
    CHECK(c2.z == doctest::Approx(c.z));
}

TEST_CASE("EfeBinaryIO: Array de POD round-trip con count prefijado") {
    std::vector<u32> src = {1u, 2u, 3u, 4u, 5u};

    BinaryWriter w;
    w.Array(src);
    // count (u32) + 5 * u32, ya alineado a 4
    CHECK(w.Size() == sizeof(u32) + 5u * sizeof(u32));

    std::vector<u32> dst;
    BinaryReader r(w.Buffer());
    r.Array(dst);

    CHECK(r.Ok());
    CHECK(dst == src);
}

TEST_CASE("EfeBinaryIO: Array vacio round-trip") {
    std::vector<u32> src;
    BinaryWriter w;
    w.Array(src);

    std::vector<u32> dst = {99u};   // arranca sucio: el read tiene que vaciarlo
    BinaryReader r(w.Buffer());
    r.Array(dst);

    CHECK(r.Ok());
    CHECK(dst.empty());
}

TEST_CASE("EfeBinaryIO: Align4 padea al escribir y avanza al leer") {
    u8 one = 7u;
    BinaryWriter w;
    w.Field(one);
    CHECK(w.Size() == 1u);
    w.Align4();
    CHECK(w.Size() == 4u);
    u32 after = 0xABCDEF01u;
    w.Field(after);

    u8 one2 = 0; u32 after2 = 0;
    BinaryReader r(w.Buffer());
    r.Field(one2);
    r.Align4();
    r.Field(after2);

    CHECK(r.Ok());
    CHECK(one2 == one);
    CHECK(after2 == after);
}

TEST_CASE("EfeBinaryIO: leer mas alla del buffer marca error y no corrompe") {
    BinaryWriter w;
    u32 only = 5u;
    w.Field(only);

    u32 first = 0, second = 0xAAAAAAAAu;
    BinaryReader r(w.Buffer());
    r.Field(first);
    CHECK(r.Ok());
    CHECK(first == 5u);

    r.Field(second);              // ya no queda nada
    CHECK_FALSE(r.Ok());
    CHECK(second == 0u);          // la lectura fallida deja ceros, no basura
}

TEST_CASE("EfeBinaryIO: buffer cortado a mitad de un campo falla sin leer OOB") {
    BinaryWriter w;
    u32 value = 0x11223344u;
    w.Field(value);

    // Solo 2 de los 4 bytes: la lectura no debe tocar los que no existen.
    BinaryReader r(w.Buffer().data(), 2u);
    u32 out = 0x55555555u;
    r.Field(out);

    CHECK_FALSE(r.Ok());
    CHECK(out == 0u);
}

TEST_CASE("EfeBinaryIO: tras un error todas las lecturas son no-op") {
    BinaryWriter w;
    u32 a = 1u, b = 2u;
    w.Field(a);
    w.Field(b);

    BinaryReader r(w.Buffer().data(), 3u);   // ni un campo completo entra
    u32 x = 9u;
    r.Field(x);
    CHECK_FALSE(r.Ok());

    // A partir de acá nada avanza ni escribe: el cursor queda quieto.
    const usize cursorTrasFallo = r.Cursor();
    u32 y = 7u;
    r.Field(y);
    std::vector<u32> v = {1u, 2u};
    r.Array(v);

    CHECK_FALSE(r.Ok());
    CHECK(r.Cursor() == cursorTrasFallo);
    CHECK(y == 0u);
    CHECK(v.empty());
}

TEST_CASE("EfeBinaryIO: Array con count absurdo falla sin allocar") {
    // Se fabrica a mano un buffer que dice "tengo 0xFFFFFFFF elementos" y no los trae.
    BinaryWriter w;
    u32 mentira = 0xFFFFFFFFu;
    w.Field(mentira);

    std::vector<u32> dst;
    BinaryReader r(w.Buffer());
    r.Array(dst);                 // NO debe intentar resize(4 mil millones)

    CHECK_FALSE(r.Ok());
    CHECK(dst.empty());
}

TEST_CASE("EfeBinaryIO: Skip avanza y respeta los limites") {
    BinaryWriter w;
    u32 a = 1u, b = 0xC0FFEEu;
    w.Field(a);
    w.Field(b);

    BinaryReader r(w.Buffer());
    r.Skip(sizeof(u32));
    u32 out = 0;
    r.Field(out);
    CHECK(r.Ok());
    CHECK(out == b);

    r.Skip(1000u);                // fuera de rango
    CHECK_FALSE(r.Ok());
}

TEST_CASE("EfeBinaryIO: Take deja el writer vacio y entrega los bytes") {
    BinaryWriter w;
    u32 a = 123u;
    w.Field(a);
    CHECK(w.Size() == sizeof(u32));

    std::vector<u8> bytes = w.Take();
    CHECK(bytes.size() == sizeof(u32));
    CHECK(w.Size() == 0u);
}

TEST_CASE("EfeBinaryIO: bloque dimensionado round-trip con parcheo del tamano") {
    BinaryWriter w;
    u32 antes = 0xAAAAAAAAu;
    w.Field(antes);

    const usize marker = w.BeginSizedBlock();
    u32 dentro1 = 11u, dentro2 = 22u;
    w.Field(dentro1);
    w.Field(dentro2);
    w.EndSizedBlock(marker);

    u32 despues = 0xBBBBBBBBu;
    w.Field(despues);

    u32 a = 0, d = 0;
    BinaryReader r(w.Buffer());
    r.Field(a);
    const usize end = r.BeginSizedBlock();
    u32 x = 0;
    r.Field(x);                    // se lee solo el primero del bloque
    r.EndSizedBlock(end);          // y el resto se descarta prolijo
    r.Field(d);

    CHECK(r.Ok());
    CHECK(a == antes);
    CHECK(x == dentro1);
    CHECK(d == despues);           // el campo de despues del bloque quedo alineado
}

TEST_CASE("EfeBinaryIO: bloque dimensionado vacio") {
    BinaryWriter w;
    const usize marker = w.BeginSizedBlock();
    w.EndSizedBlock(marker);
    u32 despues = 0xC0FFEEu;
    w.Field(despues);

    BinaryReader r(w.Buffer());
    const usize end = r.BeginSizedBlock();
    r.EndSizedBlock(end);
    u32 d = 0;
    r.Field(d);

    CHECK(r.Ok());
    CHECK(d == despues);
}

TEST_CASE("EfeBinaryIO: bloque dimensionado padea el contenido a 4") {
    BinaryWriter w;
    const usize marker = w.BeginSizedBlock();
    u8 impar = 3u;
    w.Field(impar);                // 1 byte: el bloque tiene que padear a 4
    w.EndSizedBlock(marker);
    u32 despues = 0x12345678u;
    w.Field(despues);

    // u32 del tamano + 4 (1 byte + 3 de padding) + u32 de despues
    CHECK(w.Size() == sizeof(u32) + 4u + sizeof(u32));

    BinaryReader r(w.Buffer());
    const usize end = r.BeginSizedBlock();
    r.EndSizedBlock(end);
    u32 d = 0;
    r.Field(d);
    CHECK(r.Ok());
    CHECK(d == despues);
}

TEST_CASE("EfeBinaryIO: bloque que promete mas bytes de los que hay falla") {
    BinaryWriter w;
    u32 tamanoMentiroso = 9999u;   // dice 9999 bytes de contenido
    w.Field(tamanoMentiroso);
    u32 relleno = 1u;
    w.Field(relleno);

    BinaryReader r(w.Buffer());
    const usize end = r.BeginSizedBlock();
    CHECK_FALSE(r.Ok());           // el fin del bloque cae fuera del buffer
    CHECK(end == 0u);
}
