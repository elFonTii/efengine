#include <doctest/doctest.h>
#include <efengine/resources/ModelLoader.h>

using efengine::resources::MetersPerUnit;

/*
    MetersPerUnit es la unica parte testeable de ModelLoader: Load necesita
    assimp y un archivo en disco, y el Model que devuelve necesita contexto GL.
    El resto se verifica con el log del sandbox.
*/

TEST_CASE("MetersPerUnit: un archivo en centimetros da 0.01 m por unidad") {
    CHECK(MetersPerUnit(100.0) == doctest::Approx(0.01f));
}

TEST_CASE("MetersPerUnit: un archivo en metros no escala") {
    CHECK(MetersPerUnit(1.0) == doctest::Approx(1.0f));
}

TEST_CASE("MetersPerUnit: factor ausente, cero o negativo cae en 1.0") {
    // El caller pasa 0.0 cuando el metadata no trae la clave. Un factor <= 0 no
    // tiene interpretacion posible y dividir por el reventaria la escala entera.
    CHECK(MetersPerUnit(0.0)   == doctest::Approx(1.0f));
    CHECK(MetersPerUnit(-5.0)  == doctest::Approx(1.0f));
}
