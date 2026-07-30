#include <doctest/doctest.h>
#include <efengine/renderer/BoxMesh.h>
#include <efengine/renderer/Bounds.h>

#include <glm/glm.hpp>

#include <algorithm>

using namespace efengine;
using namespace efengine::renderer;

namespace {
    // Junta los vertices de todas las caras. La AABB del modelo entero es la
    // union, y varios casos la necesitan.
    std::vector<Vertex> todosLosVertices(const std::vector<BoxFace>& caras) {
        std::vector<Vertex> out;
        for (const BoxFace& c : caras) out.insert(out.end(), c.vertices.begin(), c.vertices.end());
        return out;
    }
}

TEST_CASE("BoxMeshData: devuelve las 6 caras con sus nombres y en orden") {
    BoxParams p;
    const std::vector<BoxFace> caras = BoxMeshData(p);

    REQUIRE(caras.size() == kBoxFaceCount);
    CHECK(caras[0].name == "pared_xneg");
    CHECK(caras[1].name == "pared_xpos");
    CHECK(caras[2].name == "piso");
    CHECK(caras[3].name == "techo");
    CHECK(caras[4].name == "pared_zneg");
    CHECK(caras[5].name == "pared_zpos");
}

TEST_CASE("BoxMeshData: sin abertura, cada cara es un quad") {
    BoxParams p;
    p.holeFace = 6u;   // ninguna

    for (const BoxFace& c : BoxMeshData(p)) {
        CHECK(c.vertices.size() == 4u);
        CHECK(c.indices.size()  == 6u);
    }
}

TEST_CASE("BoxMeshData: la AABB de los vertices son los semi-extents pedidos") {
    BoxParams p;
    p.half = glm::vec3(4.0f, 2.0f, 4.0f);
    p.holeFace = 6u;

    const AABB b = ComputeBounds(todosLosVertices(BoxMeshData(p)));

    CHECK(b.min.x == doctest::Approx(-4.0f));
    CHECK(b.min.y == doctest::Approx(-2.0f));
    CHECK(b.min.z == doctest::Approx(-4.0f));
    CHECK(b.max.x == doctest::Approx( 4.0f));
    CHECK(b.max.y == doctest::Approx( 2.0f));
    CHECK(b.max.z == doctest::Approx( 4.0f));
}

TEST_CASE("BoxMeshData: con inward toda normal apunta al centro") {
    // dot(normal, -posLocal) > 0 exactamente cuando la normal mira hacia adentro:
    // en la cara de eje A, dot(normalSaliente, pos) == half[A] > 0, asi que la
    // normal invertida da +half[A].
    BoxParams p;
    p.half   = glm::vec3(4.0f, 2.0f, 4.0f);
    p.inward = 1u;
    p.holeFace = 6u;

    for (const BoxFace& c : BoxMeshData(p)) {
        for (const Vertex& v : c.vertices) {
            CHECK(glm::dot(v.normal, -v.position) > 0.0f);
        }
    }
}

TEST_CASE("BoxMeshData: sin inward toda normal apunta hacia afuera") {
    BoxParams p;
    p.half   = glm::vec3(0.6f, 1.2f, 0.6f);
    p.inward = 0u;
    p.holeFace = 6u;

    for (const BoxFace& c : BoxMeshData(p)) {
        for (const Vertex& v : c.vertices) {
            CHECK(glm::dot(v.normal, -v.position) < 0.0f);
        }
    }
}

TEST_CASE("BoxMeshData: la cara con abertura es un marco de 4 quads y no tiene vertices dentro del hueco") {
    BoxParams p;
    p.half     = glm::vec3(4.0f, 2.0f, 4.0f);
    p.inward   = 1u;
    p.holeFace = static_cast<u32>(BoxFaceIndex::ZNeg);
    p.hole     = glm::vec4(-1.5f, -0.8f, 1.5f, 1.2f);

    const std::vector<BoxFace> caras = BoxMeshData(p);
    const BoxFace& conHueco = caras[4];

    CHECK(conHueco.vertices.size() == 16u);   // 4 quads
    CHECK(conHueco.indices.size()  == 24u);   // 4 quads * 2 triangulos * 3

    // Ningun vertice cae en el interior ABIERTO del hueco. Los bordes SI son
    // vertices del marco, por eso la comparacion es estricta.
    for (const Vertex& v : conHueco.vertices) {
        const bool dentroX = v.position.x > -1.5f + 1e-4f && v.position.x < 1.5f - 1e-4f;
        const bool dentroY = v.position.y > -0.8f + 1e-4f && v.position.y < 1.2f - 1e-4f;
        // El && se resuelve fuera del CHECK: doctest no descompone expresiones
        // logicas compuestas.
        const bool dentroDelHueco = dentroX && dentroY;
        CHECK_FALSE(dentroDelHueco);
    }

    // Las otras cinco caras siguen siendo quads simples.
    for (u32 i = 0u; i < kBoxFaceCount; ++i) {
        if (i == 4u) continue;
        CHECK(caras[i].vertices.size() == 4u);
    }
}

TEST_CASE("BoxMeshData: una abertura que cubre la cara entera la deja sin geometria") {
    // Los 4 quads del marco quedan degenerados y se saltean: es el caso de
    // "abrir la pared entera", que el generador tiene que aceptar sin producir
    // triangulos de area cero.
    BoxParams p;
    p.half     = glm::vec3(4.0f, 2.0f, 4.0f);
    p.holeFace = static_cast<u32>(BoxFaceIndex::ZNeg);
    p.hole     = glm::vec4(-9.0f, -9.0f, 9.0f, 9.0f);

    const std::vector<BoxFace> caras = BoxMeshData(p);
    REQUIRE(caras.size() == kBoxFaceCount);
    CHECK(caras[4].vertices.empty());
    CHECK(caras[4].indices.empty());
    CHECK(caras[4].name == "pared_zneg");   // la entrada sigue existiendo
}

TEST_CASE("BoxMeshData: los indices siempre caen dentro del vector de vertices de su cara") {
    BoxParams p;
    p.holeFace = static_cast<u32>(BoxFaceIndex::YPos);
    p.hole     = glm::vec4(-1.0f, -1.0f, 1.0f, 1.0f);

    for (const BoxFace& c : BoxMeshData(p)) {
        for (u32 i : c.indices) CHECK(i < c.vertices.size());
        // El resto se calcula fuera del CHECK: doctest no puede descomponer una
        // expresion con % adentro.
        const size_t resto = c.indices.size() % 3u;
        CHECK(resto == 0u);
    }
}
