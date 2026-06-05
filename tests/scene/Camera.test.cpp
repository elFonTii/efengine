#include <doctest/doctest.h>
#include <efengine/scene/Camera.h>
#include <glm/gtc/matrix_transform.hpp>

using namespace efengine;

    /* 
    Entendiendo la matriz de proyeccion 
    glm::perspective(fovy, aspect, near, far) arma una matrix 4x4 ( f = 1/tan(fov/2) )
    m[0][0] = f/aspecto -> es la escala horizontal y depende del aspecto
    m[1][1] = f
    m[2][2] = (far+near)/(near-far) -> de aca para abajo ni puta idea
    m[2][3] = -1
    m[3][2] = (2*far*near)/(near-far)
    */

TEST_CASE("Camera::ProjectionMatrix tiene que cambiar con la relación de aspecto") {
    scene::Camera cam;

    cam.SetAspect(1.0f);
    glm::mat4 p1 = cam.ProjectionMatrix();

    cam.SetAspect(2.0f);
    glm::mat4 p2 = cam.ProjectionMatrix();

    // Verificamos que los valores en la posición [0][0] de las matrices difieran
    CHECK(p1[0][0] != doctest::Approx(p2[0][0]));
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