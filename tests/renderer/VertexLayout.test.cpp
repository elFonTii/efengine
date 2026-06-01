// tests/renderer/VertexLayout.test.cpp
// Valida la lógica pura de VertexLayout (stride, offset, location).
// No necesita contexto OpenGL.
#include <doctest/doctest.h>
#include <efengine/renderer/VertexLayout.h>

using efengine::renderer::ShaderDataType;
using efengine::renderer::VertexLayout;

TEST_CASE("VertexLayout: layout vacío") {
    VertexLayout layout;
    CHECK(layout.stride() == 0);
    CHECK(layout.attributes().empty());
}

TEST_CASE("VertexLayout: un atributo Float3") {
    VertexLayout layout;
    layout.Push(ShaderDataType::Float3);

    CHECK(layout.stride() == 12);
    REQUIRE(layout.attributes().size() == 1);
    CHECK(layout.attributes()[0].location == 0);
    CHECK(layout.attributes()[0].offset == 0);
}

TEST_CASE("VertexLayout: Float3 + Float4 acumula stride/offset/location") {
    VertexLayout layout;
    layout.Push(ShaderDataType::Float3);
    layout.Push(ShaderDataType::Float4);

    CHECK(layout.stride() == 28);
    REQUIRE(layout.attributes().size() == 2);
    CHECK(layout.attributes()[0].location == 0);
    CHECK(layout.attributes()[0].offset   == 0);
    CHECK(layout.attributes()[1].location == 1);
    CHECK(layout.attributes()[1].offset   == 12);
}
