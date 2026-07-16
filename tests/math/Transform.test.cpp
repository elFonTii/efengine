#include <doctest/doctest.h>
#include <efengine/math/Transform.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

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
