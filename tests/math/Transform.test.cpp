#include <doctest/doctest.h>
#include <efengine/math/Transform.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <cmath>

using namespace efengine;

/*
    Todos los casos comparan Matrix() contra la misma operación armada a mano con
    GLM. Es el patrón de Camera.test.cpp: el test no re-implementa la matemática,
    verifica que Transform componga las piezas de GLM en el orden correcto. Sin
    contexto GL: Transform es matemática pura.
*/

namespace {
    // glm::mat4 es column-major: m[columna][fila].
    // Ojo: si un CHECK de acá falla, doctest reporta ESTA línea, no la del
    // TEST_CASE. El nombre del caso te dice cuál fue.
    void CheckMat4Equal(const glm::mat4& actual, const glm::mat4& expected) {
        for (int c = 0; c < 4; c++) {
            for (int r = 0; r < 4; r++) {
                CHECK(actual[c][r] == doctest::Approx(expected[c][r]));
            }
        }
    }

    const glm::vec3 kAxisX(1.0f, 0.0f, 0.0f);
    const glm::vec3 kAxisY(0.0f, 1.0f, 0.0f);
    const glm::vec3 kAxisZ(0.0f, 0.0f, 1.0f);

    // Igual que CheckMat4Equal pero con tolerancia holgada: la ida y vuelta por
    // asin/atan2 acumula error de punto flotante que el epsilon default de
    // doctest no perdona.
    void CheckMat4Near(const glm::mat4& actual, const glm::mat4& expected) {
        for (int c = 0; c < 4; c++) {
            for (int r = 0; r < 4; r++) {
                CHECK(actual[c][r] == doctest::Approx(expected[c][r]).epsilon(0.001));
            }
        }
    }

    // El contrato de DecomposeTRS es sobre la MATRIZ, no sobre los componentes:
    // los angulos de Euler no son unicos (con pitch ±90 el yaw y el roll se
    // funden en el mismo giro), asi que la vuelta se verifica recomponiendo.
    void CheckRoundTrip(const math::Transform& t) {
        CheckMat4Near(math::DecomposeTRS(t.Matrix()).Matrix(), t.Matrix());
    }
}

TEST_CASE("Transform por default da la matriz identidad") {
    math::Transform t;

    // scale arranca en 1 y no en 0: si alguien lo inicializa en 0, todo colapsa
    // a un punto y no se dibuja nada. Este caso lo agarra.
    CheckMat4Equal(t.Matrix(), glm::mat4(1.0f));
}

TEST_CASE("Transform::Matrix solo con position coincide con glm::translate") {
    math::Transform t;
    t.position = glm::vec3(2.0f, 3.0f, -4.0f);

    CheckMat4Equal(t.Matrix(), glm::translate(glm::mat4(1.0f), t.position));
}

TEST_CASE("Transform::Matrix solo con scale coincide con glm::scale") {
    math::Transform t;
    t.scale = glm::vec3(10.0f);

    CheckMat4Equal(t.Matrix(), glm::scale(glm::mat4(1.0f), t.scale));
}

TEST_CASE("Transform::Matrix interpreta la rotacion en GRADOS, no en radianes") {
    math::Transform t;
    t.rotation = glm::vec3(0.0f, 90.0f, 0.0f);

    // Si Matrix() le pasara rotation.y crudo a glm::rotate, estaría rotando
    // 90 RADIANES (~5157 grados) y este caso se pone rojo.
    CheckMat4Equal(t.Matrix(), glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), kAxisY));
}

TEST_CASE("Transform::Matrix compone en el orden T * R * S") {
    math::Transform t;
    t.position = glm::vec3(5.0f, 0.0f, 0.0f);
    t.rotation = glm::vec3(0.0f, 90.0f, 0.0f);
    t.scale    = glm::vec3(2.0f);

    const glm::mat4 T = glm::translate(glm::mat4(1.0f), t.position);
    const glm::mat4 R = glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), kAxisY);
    const glm::mat4 S = glm::scale(glm::mat4(1.0f), t.scale);

    // El orden importa y no es conmutativo. S * R * T (que es el orden en el que
    // uno lee "escalá, rotá, movelo") rota la posición junto con el objeto y lo
    // manda a otro lado. Este caso fija el correcto.
    CheckMat4Equal(t.Matrix(), T * R * S);
}

TEST_CASE("Transform::Matrix compone los angulos de Euler en el orden Y * X * Z") {
    math::Transform t;
    t.rotation = glm::vec3(30.0f, 45.0f, 0.0f);

    const glm::mat4 Rx = glm::rotate(glm::mat4(1.0f), glm::radians(30.0f), kAxisX);
    const glm::mat4 Ry = glm::rotate(glm::mat4(1.0f), glm::radians(45.0f), kAxisY);

    // Yaw primero, después pitch: R = Ry * Rx * Rz, y acá Rz es la identidad.
    // Con position y scale por default, Matrix() se reduce a R.
    CheckMat4Equal(t.Matrix(), Ry * Rx);
}

TEST_CASE("EulerFromForward: -Z da rotacion nula") {
    const glm::vec3 e = math::EulerFromForward(glm::vec3(0.0f, 0.0f, -1.0f));
    CHECK(e.x == doctest::Approx(0.0f));
    CHECK(e.y == doctest::Approx(0.0f));
    CHECK(e.z == doctest::Approx(0.0f));
}

TEST_CASE("EulerFromForward: ida y vuelta contra Transform::Matrix") {
    // El contrato real no son los angulos, es que aplicar la rotacion resultante
    // al -Z local devuelva la direccion de entrada. Es literalmente lo que hace
    // SceneGraph::UpdateWorldTransforms para sacar la direccion del sol.
    const glm::vec3 dir = glm::normalize(glm::vec3(0.30f, -0.62f, 0.72f));

    math::Transform t;
    t.rotation = math::EulerFromForward(dir);

    const glm::vec3 fwd = glm::normalize(
        glm::vec3(t.Matrix() * glm::vec4(0.0f, 0.0f, -1.0f, 0.0f)));

    CHECK(fwd.x == doctest::Approx(dir.x).epsilon(0.0001));
    CHECK(fwd.y == doctest::Approx(dir.y).epsilon(0.0001));
    CHECK(fwd.z == doctest::Approx(dir.z).epsilon(0.0001));
}

TEST_CASE("EulerFromForward: un sol vertical no produce NaN") {
    // Mirando derecho para abajo cos(pitch) = 0, que es la division por cero de
    // la formula si se la escribe con sin/cos en vez de con atan2.
    math::Transform t;
    t.rotation = math::EulerFromForward(glm::vec3(0.0f, -1.0f, 0.0f));

    const glm::vec3 fwd = glm::normalize(
        glm::vec3(t.Matrix() * glm::vec4(0.0f, 0.0f, -1.0f, 0.0f)));

    CHECK(fwd.y == doctest::Approx(-1.0f).epsilon(0.0001));
    CHECK(std::isnan(t.rotation.x) == false);
    CHECK(std::isnan(t.rotation.y) == false);
}

TEST_CASE("EulerFromForward: un vector nulo cae en cero") {
    const glm::vec3 e = math::EulerFromForward(glm::vec3(0.0f));
    CHECK(e.x == doctest::Approx(0.0f));
    CHECK(e.y == doctest::Approx(0.0f));
    CHECK(e.z == doctest::Approx(0.0f));
}

// ---------------------------------------------------------------------------
// DecomposeTRS: la inversa de Matrix()
// ---------------------------------------------------------------------------

TEST_CASE("DecomposeTRS: la identidad da un Transform por default") {
    const math::Transform t = math::DecomposeTRS(glm::mat4(1.0f));

    CHECK(t.position.x == doctest::Approx(0.0f));
    CHECK(t.position.y == doctest::Approx(0.0f));
    CHECK(t.position.z == doctest::Approx(0.0f));
    CHECK(t.scale.x == doctest::Approx(1.0f));
    CHECK(t.scale.y == doctest::Approx(1.0f));
    CHECK(t.scale.z == doctest::Approx(1.0f));
    CHECK(t.rotation.x == doctest::Approx(0.0f));
    CHECK(t.rotation.y == doctest::Approx(0.0f));
    CHECK(t.rotation.z == doctest::Approx(0.0f));
}

TEST_CASE("DecomposeTRS: ida y vuelta con traslacion pura") {
    math::Transform t;
    t.position = glm::vec3(2.0f, -3.0f, 7.5f);
    CheckRoundTrip(t);
}

TEST_CASE("DecomposeTRS: ida y vuelta con rotacion pura en cada eje") {
    math::Transform tx; tx.rotation = glm::vec3(37.0f, 0.0f, 0.0f);
    CheckRoundTrip(tx);

    math::Transform ty; ty.rotation = glm::vec3(0.0f, 128.0f, 0.0f);
    CheckRoundTrip(ty);

    math::Transform tz; tz.rotation = glm::vec3(0.0f, 0.0f, -64.0f);
    CheckRoundTrip(tz);
}

TEST_CASE("DecomposeTRS: ida y vuelta con escala no uniforme") {
    math::Transform t;
    t.scale = glm::vec3(2.0f, 0.5f, 3.0f);
    CheckRoundTrip(t);
}

TEST_CASE("DecomposeTRS: ida y vuelta con los tres componentes juntos") {
    math::Transform t;
    t.position = glm::vec3(5.0f, -1.0f, 2.0f);
    t.rotation = glm::vec3(20.0f, -75.0f, 40.0f);
    t.scale    = glm::vec3(1.5f, 2.0f, 0.75f);
    CheckRoundTrip(t);
}

TEST_CASE("DecomposeTRS: recupera position y scale exactos, sin rotacion de por medio") {
    math::Transform t;
    t.position = glm::vec3(10.0f, 20.0f, 30.0f);
    t.scale    = glm::vec3(4.0f, 5.0f, 6.0f);

    const math::Transform d = math::DecomposeTRS(t.Matrix());

    CHECK(d.position.x == doctest::Approx(10.0f));
    CHECK(d.position.y == doctest::Approx(20.0f));
    CHECK(d.position.z == doctest::Approx(30.0f));
    CHECK(d.scale.x == doctest::Approx(4.0f));
    CHECK(d.scale.y == doctest::Approx(5.0f));
    CHECK(d.scale.z == doctest::Approx(6.0f));
}

TEST_CASE("DecomposeTRS: el gimbal a pitch +-90 no rompe la matriz recompuesta") {
    // Aca yaw y roll dejan de ser separables: cualquier par que sume lo mismo da
    // la misma rotacion. Por eso se compara la matriz y NO los angulos.
    math::Transform arriba;
    arriba.rotation = glm::vec3(90.0f, 30.0f, 0.0f);
    CheckRoundTrip(arriba);

    math::Transform abajo;
    abajo.rotation = glm::vec3(-90.0f, 30.0f, 0.0f);
    CheckRoundTrip(abajo);
}

TEST_CASE("DecomposeTRS: una matriz con escala cero no produce NaN") {
    // Con un eje colapsado la rotacion es indeterminada: no se puede normalizar
    // una columna de largo cero. El contrato es que devuelva algo usable, no NaN.
    math::Transform t;
    t.position = glm::vec3(1.0f, 2.0f, 3.0f);
    t.scale    = glm::vec3(1.0f, 0.0f, 1.0f);

    const math::Transform d = math::DecomposeTRS(t.Matrix());

    CHECK(std::isnan(d.rotation.x) == false);
    CHECK(std::isnan(d.rotation.y) == false);
    CHECK(std::isnan(d.rotation.z) == false);
    CHECK(std::isnan(d.position.x) == false);
    CHECK(d.position.x == doctest::Approx(1.0f));
}
