#include <cmath>
#include <doctest/doctest.h>
#include <efengine/scene/Camera.h>
#include <glm/gtc/matrix_transform.hpp>


using namespace efengine;

    /* 
    Entendiendo la matriz de proyeccion 
    glm::perspective(fovy, aspect, near, far) arma una matrix 4x4 ( f = 1/tan(fov/2) )
    m[0][0] = f/aspecto -> es la escala horizontal
    m[1][1] = f
    m[2][2] = (far+near)/(near-far) -> internamente es un sistema de ecuaciones que hacen que la geometría siempre esté entre el plano near y far
    m[2][3] = -1 // se usan coordenadas homogéneas (x,y,z,w), para crear ilusión de perspectiva
                    la tarjeta gráfica divide x,y,z por w después de multiplicar la matriz (división en perspectiva) TODO INTERNO GPU
                    la cámara mira al eje Z negativo.
    m[3][2] = (2*far*near)/(near-far) // idem m[2][2]
    */

TEST_CASE("Camera::ProjectionMatrix coincide con glm::perspective") {
    scene::Camera cam;
    const f32 aspect = 16.0f/9.0f;
    cam.SetAspect(aspect);

    glm::mat4 actual   = cam.ProjectionMatrix();
    glm::mat4 expected = glm::perspective(glm::radians(45.0f), 16.0f/9.0f, 0.1f, 5000.0f); // por practicidad, se hace contra los defaults de la cámara.

    for(int c = 0; c < 4; c++) {
        for(int r = 0; r < 4; r++) {
            CHECK(actual[c][r] == doctest::Approx(expected[c][r])); // Se recorre la matriz y se comparan los valores
        }
    }
}

TEST_CASE("Camera::ViewMatrix tiene que coincidir con glom::lookAt") {
    scene::Camera cam;
    
    glm::vec3 pos(2.0f, 1.0f, 3.0f);
    glm::vec3 target(0.0f);
    glm::vec3 up(0.0f, 1.f, 0.0f);

    cam.LookAt(pos, target, up);
    
    glm::mat4 actual  = cam.ViewMatrix();
    glm::mat4 expected = glm::lookAt(pos, target, up);

    // Recorremos las matrices y comparamos sus valores
    for(int c = 0; c < 4; c++) {
        for(int r = 0; r < 4; r++) {
            CHECK(actual[c][r] == doctest::Approx(expected[c][r])); // tienen que ser iguales.
        }
    }
}

TEST_CASE("Camera::Position debe reflejar el LookAt"){
    scene::Camera cam;
    glm::vec3 pos(5.0f, 0.0f, 0.0f);
    glm::vec3 target(0.0f);
    glm::vec3 up(0.0f, 1.f, 0.0f);

    cam.LookAt(pos, target, up);

    CHECK(cam.Position().x == doctest::Approx(pos.x));
    CHECK(cam.Position().y == doctest::Approx(pos.y));
    CHECK(cam.Position().z == doctest::Approx(pos.z));
}

TEST_CASE("Camera: exposición por defecto es 1.0") {
    scene::Camera cam;
    CHECK(cam.Exposure() == doctest::Approx(1.0f));
}

TEST_CASE("Camera::SetExposure hace round-trip") {
    scene::Camera cam;
    cam.SetExposure(2.5f);
    CHECK(cam.Exposure() == doctest::Approx(2.5f));
}