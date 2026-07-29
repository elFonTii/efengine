// tests/resources/ShaderPreprocessor.test.cpp
// SpliceIncludes es una funcion pura sobre strings: se testea sin filesystem y
// sin GPU inyectando un resolver falso. Es lo unico que evita que la matematica
// de DDGI se duplique en cinco archivos GLSL.
#include <doctest/doctest.h>
#include <efengine/resources/ShaderPreprocessor.h>

#include <map>
#include <string>

using namespace efengine;
using namespace efengine::resources;

namespace {
    // Resolver de mentira: un mapa path -> contenido.
    IncludeResolver MapResolver(const std::map<std::string, std::string>& files) {
        return [files](const std::string& path) -> std::optional<std::string> {
            auto it = files.find(path);
            if (it == files.end()) return std::nullopt;
            return it->second;
        };
    }

    bool Contiene(const std::string& haystack, const std::string& needle) {
        return haystack.find(needle) != std::string::npos;
    }

    // Cuenta apariciones no solapadas.
    usize Veces(const std::string& haystack, const std::string& needle) {
        usize n = 0u;
        for (usize pos = haystack.find(needle); pos != std::string::npos;
             pos = haystack.find(needle, pos + needle.size())) {
            ++n;
        }
        return n;
    }
}

TEST_CASE("SpliceIncludes: fuente sin includes vuelve intacta") {
    const std::string src = "#version 450 core\nvoid main() {}\n";
    const auto out = SpliceIncludes(src, MapResolver({}));
    REQUIRE(out.has_value());
    CHECK(*out == src);
}

TEST_CASE("SpliceIncludes: un include se reemplaza por su contenido") {
    const auto out = SpliceIncludes(
        "#version 450 core\n#include \"a.glsl\"\nvoid main() {}\n",
        MapResolver({ { "a.glsl", "float A = 1.0;" } }));

    REQUIRE(out.has_value());
    CHECK(Contiene(*out, "float A = 1.0;"));
    CHECK(Contiene(*out, "#version 450 core"));
    CHECK(Contiene(*out, "void main() {}"));
    // La directiva ya no esta.
    CHECK_FALSE(Contiene(*out, "#include"));
}

TEST_CASE("SpliceIncludes: los includes anidados se resuelven") {
    const auto out = SpliceIncludes(
        "#include \"outer.glsl\"\n",
        MapResolver({ { "outer.glsl", "#include \"inner.glsl\"\nfloat OUTER = 2.0;" },
                      { "inner.glsl", "float INNER = 3.0;" } }));

    REQUIRE(out.has_value());
    CHECK(Contiene(*out, "float INNER = 3.0;"));
    CHECK(Contiene(*out, "float OUTER = 2.0;"));
}

TEST_CASE("SpliceIncludes: include-once, el mismo path no se duplica") {
    // Este es el caso real: pbr.frag incluye ddgi/common.glsl, que incluye
    // cubeface.glsl, y pbr.frag tambien lo incluye directo. Sin include-once,
    // dirForFace queda redefinida y el shader no compila.
    const auto out = SpliceIncludes(
        "#include \"cubeface.glsl\"\n#include \"common.glsl\"\n",
        MapResolver({ { "cubeface.glsl", "vec3 dirForFace() { return vec3(0.0); }" },
                      { "common.glsl",   "#include \"cubeface.glsl\"\nfloat C = 1.0;" } }));

    REQUIRE(out.has_value());
    CHECK(Veces(*out, "dirForFace") == 1u);
    CHECK(Contiene(*out, "float C = 1.0;"));
}

TEST_CASE("SpliceIncludes: un include que no resuelve devuelve nullopt") {
    const auto out = SpliceIncludes("#include \"noexiste.glsl\"\n", MapResolver({}));
    CHECK_FALSE(out.has_value());
}

TEST_CASE("SpliceIncludes: espacios antes de #include se aceptan") {
    const auto out = SpliceIncludes("   #include \"a.glsl\"\n",
                                    MapResolver({ { "a.glsl", "float A = 1.0;" } }));
    REQUIRE(out.has_value());
    CHECK(Contiene(*out, "float A = 1.0;"));
}

TEST_CASE("SpliceIncludes: #include que no arranca la linea se deja intacto") {
    // Un '#include' despues de codigo no es una directiva: no lo tocamos.
    const std::string src = "float x = 1.0; #include \"a.glsl\"\n";
    const auto out = SpliceIncludes(src, MapResolver({ { "a.glsl", "NOPE" } }));
    REQUIRE(out.has_value());
    CHECK_FALSE(Contiene(*out, "NOPE"));
    CHECK(*out == src);
}

TEST_CASE("SpliceIncludes: un ciclo se corta por profundidad, no cuelga") {
    // a incluye b, b incluye a. El include-once ya lo corta, pero si alguien
    // rompe esa parte el tope de profundidad es la segunda red.
    const auto out = SpliceIncludes(
        "#include \"a.glsl\"\n",
        MapResolver({ { "a.glsl", "#include \"b.glsl\"\nfloat A = 1.0;" },
                      { "b.glsl", "#include \"a.glsl\"\nfloat B = 2.0;" } }));

    // No cuelga. Con include-once resuelve; el CHECK es que termine y no aborte.
    REQUIRE(out.has_value());
    CHECK(Veces(*out, "float A = 1.0;") == 1u);
}

TEST_CASE("SpliceIncludes: linea de include sin comillas de cierre devuelve nullopt") {
    const auto out = SpliceIncludes("#include \"a.glsl\n", MapResolver({}));
    CHECK_FALSE(out.has_value());
}
