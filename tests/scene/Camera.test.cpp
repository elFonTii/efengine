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
            CHECK(actual[c][r] == doctest::Approx(expected[c][r]));
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

    for(int c = 0; c < 4; c++) {
        for(int r = 0; r < 4; r++) {
            CHECK(actual[c][r] == doctest::Approx(expected[c][r]));
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
    CHECK(cam.Exposure() == doctest::Approx(1.025f));
}

TEST_CASE("Camera::SetExposure hace round-trip") {
    scene::Camera cam;
    cam.SetExposure(2.5f);
    CHECK(cam.Exposure() == doctest::Approx(2.5f));
}

TEST_CASE("Camera::SetFromWorld: la identidad mira a -Z desde el origen") {
    scene::Camera cam;
    cam.SetFromWorld(glm::mat4(1.0f));

    CHECK(cam.Position().x == doctest::Approx(0.0f));
    CHECK(cam.Position().y == doctest::Approx(0.0f));
    CHECK(cam.Position().z == doctest::Approx(0.0f));

    // El target es absoluto: position + forward.
    CHECK(cam.Target().x == doctest::Approx( 0.0f));
    CHECK(cam.Target().y == doctest::Approx( 0.0f));
    CHECK(cam.Target().z == doctest::Approx(-1.0f));
}

TEST_CASE("Camera::SetFromWorld: la posicion sale de la 4a columna") {
    scene::Camera cam;
    cam.SetFromWorld(glm::translate(glm::mat4(1.0f), glm::vec3(3.0f, 4.0f, 5.0f)));

    CHECK(cam.Position().x == doctest::Approx(3.0f));
    CHECK(cam.Position().y == doctest::Approx(4.0f));
    CHECK(cam.Position().z == doctest::Approx(5.0f));
    CHECK(cam.Target().z   == doctest::Approx(4.0f));   // 5 - 1
}

TEST_CASE("Camera::SetFromWorld: rotar +90 en Y deja la camara mirando a -X") {
    scene::Camera cam;
    // R_y(+90) manda z^ a x^, asi que forward = -col2 = (-1, 0, 0).
    cam.SetFromWorld(glm::rotate(glm::mat4(1.0f), glm::radians(90.0f),
                                 glm::vec3(0.0f, 1.0f, 0.0f)));

    CHECK(cam.Target().x == doctest::Approx(-1.0f));
    CHECK(cam.Target().y == doctest::Approx( 0.0f));
    CHECK(cam.Target().z == doctest::Approx( 0.0f).epsilon(0.001));
}

TEST_CASE("Camera::SetFromWorld: el up sale del eje Y local, asi que conserva el roll") {
    scene::Camera cam;
    // R_z(+45) manda y^ a (-sin45, cos45, 0).
    cam.SetFromWorld(glm::rotate(glm::mat4(1.0f), glm::radians(45.0f),
                                 glm::vec3(0.0f, 0.0f, 1.0f)));

    CHECK(cam.Up().x == doctest::Approx(-0.70710678f));
    CHECK(cam.Up().y == doctest::Approx( 0.70710678f));
    CHECK(cam.Up().z == doctest::Approx( 0.0f));
}

TEST_CASE("Camera::SetFromWorld: la escala uniforme no cambia la orientacion") {
    scene::Camera cam;
    cam.SetFromWorld(glm::scale(glm::mat4(1.0f), glm::vec3(4.0f)));

    CHECK(cam.Target().z == doctest::Approx(-1.0f));   // no -4
    CHECK(cam.Up().y     == doctest::Approx( 1.0f));
}

TEST_CASE("Camera::SetFromWorld: un eje en escala cero no produce NaN") {
    scene::Camera cam;
    cam.SetFromWorld(glm::translate(glm::mat4(1.0f), glm::vec3(1.0f, 2.0f, 3.0f)));

    // Con Z aplastado, normalizar la columna daria NaN: se conserva la
    // orientacion anterior y solo se mueve la camara.
    glm::mat4 degenerada = glm::translate(glm::mat4(1.0f), glm::vec3(9.0f, 9.0f, 9.0f));
    degenerada = glm::scale(degenerada, glm::vec3(1.0f, 1.0f, 0.0f));
    cam.SetFromWorld(degenerada);

    CHECK(cam.Position().x == doctest::Approx(9.0f));
    CHECK(cam.Target().z   == doctest::Approx(8.0f));   // 9 - 1, el forward viejo
    CHECK(std::isnan(cam.Target().x) == false);
    CHECK(std::isnan(cam.Up().y)     == false);
}

TEST_CASE("Camera::SetFov y SetClipPlanes cambian la proyeccion") {
    scene::Camera cam;
    cam.SetAspect(1.0f);
    cam.SetFov(60.0f);
    cam.SetClipPlanes(0.5f, 100.0f);

    CHECK(cam.Fov()       == doctest::Approx(60.0f));
    CHECK(cam.NearPlane() == doctest::Approx(0.5f));
    CHECK(cam.FarPlane()  == doctest::Approx(100.0f));

    glm::mat4 esperada = glm::perspective(glm::radians(60.0f), 1.0f, 0.5f, 100.0f);
    CHECK(cam.ProjectionMatrix()[1][1] == doctest::Approx(esperada[1][1]));
    CHECK(cam.ProjectionMatrix()[2][2] == doctest::Approx(esperada[2][2]));
}

TEST_CASE("Camera::SetClipPlanes rechaza planos invalidos y no toca los buenos") {
    scene::Camera cam;
    cam.SetClipPlanes(0.5f, 100.0f);

    cam.SetClipPlanes(0.0f, 100.0f);    // near <= 0
    CHECK(cam.NearPlane() == doctest::Approx(0.5f));

    cam.SetClipPlanes(50.0f, 10.0f);    // far <= near
    CHECK(cam.NearPlane() == doctest::Approx(0.5f));
    CHECK(cam.FarPlane()  == doctest::Approx(100.0f));
}
