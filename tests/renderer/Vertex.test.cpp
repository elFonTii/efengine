// tests/renderer/Vertex.test.cpp
// Valida el contrato entre el struct Vertex y el VertexLayout estándar:
// el layout que construye Mesh/ModelLoader debe describir exactamente la
// memoria de Vertex (mismo stride, mismos offsets). Lógica pura, sin OpenGL.
#include <doctest/doctest.h>
#include <efengine/renderer/Vertex.h>

using efengine::renderer::StandardVertexLayout;
using efengine::renderer::Vertex;

TEST_CASE("Vertex: el layout estándar describe exactamente el struct") {
    auto layout = StandardVertexLayout();

    // El stride del layout debe coincidir byte-a-byte con el tamaño del struct:
    // si no, AddVertexBuffer leería los vértices con un paso equivocado.
    CHECK(layout.stride() == sizeof(Vertex));

    // Cuatro atributos: position, normal, uv, tangent.
    REQUIRE(layout.attributes().size() == 4);
}

TEST_CASE("Vertex: offsets y locations del layout en el orden correcto") {
    auto layout = StandardVertexLayout();
    REQUIRE(layout.attributes().size() == 4);

    // location = orden de inserción (pos, normal, uv, tangent).
    CHECK(layout.attributes()[0].location == 0);
    CHECK(layout.attributes()[1].location == 1);
    CHECK(layout.attributes()[2].location == 2);
    CHECK(layout.attributes()[3].location == 3);

    // offsets acumulados: pos(0) → normal(12) → uv(24) → tangent(32).
    CHECK(layout.attributes()[0].offset == 0);
    CHECK(layout.attributes()[1].offset == 12);
    CHECK(layout.attributes()[2].offset == 24);
    CHECK(layout.attributes()[3].offset == 32);
}
