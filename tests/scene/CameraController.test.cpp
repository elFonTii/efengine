// Escrito por Claude Opus 5
#include <doctest/doctest.h>
#include <efengine/platform/Input.h>
#include <efengine/platform/InputCodes.h>
#include <efengine/scene/Camera.h>
#include <efengine/scene/CameraController.h>
#include <glm/glm.hpp>
#include <cmath>

using namespace efengine;

namespace {
    constexpr f32 kDt = 1.0f / 60.0f;

    // Arranca un drag y lo deja listo para el segundo frame. El primer frame de
    // un drag no mueve nada por contrato (mata el salto del cursor al capturarlo),
    // asi que todo test que espere movimiento tiene que pasar por aca y recien
    // despues mover el cursor.
    void empezarDrag(platform::Input& in, platform::MouseButton b, f32 x, f32 y) {
        in.OnMouseButton((i32)b, (i32)platform::InputAction::Press, 0);
        in.OnMouseMove(x, y);
        in.Commit();
    }

    void moverCursor(platform::Input& in, f32 x, f32 y) {
        in.OnMouseMove(x, y);
        in.Commit();
    }

    void frameConTecla(platform::Input& in, platform::Key k) {
        in.SetKeyDown(k, true);
        in.Commit();
    }
}

TEST_CASE("CameraController: la pose inicial reproduce la vista orbital anterior") {
    // La orbital arrancaba en target (0,10,0) con distancia 30 y pitch 45.
    scene::Camera cam;
    scene::CameraController controller(&cam);

    CHECK(cam.Position().x == doctest::Approx(0.0f));
    CHECK(cam.Position().y == doctest::Approx(31.213f).epsilon(0.001));
    CHECK(cam.Position().z == doctest::Approx(21.213f).epsilon(0.001));
    CHECK(controller.PivotDistance() == doctest::Approx(30.0f));

    // El pivote cae exactamente en el viejo target.
    CHECK(controller.Pivot().x == doctest::Approx(0.0f).epsilon(0.001));
    CHECK(controller.Pivot().y == doctest::Approx(10.0f).epsilon(0.001));
    CHECK(controller.Pivot().z == doctest::Approx(0.0f).epsilon(0.001));
}

TEST_CASE("CameraController: W avanza sobre la linea de vista") {
    scene::Camera cam;
    scene::CameraController controller(&cam);
    const glm::vec3 p0  = cam.Position();
    const glm::vec3 fwd = controller.Forward();

    platform::Input in;
    frameConTecla(in, platform::Key::W);
    controller.Update(in, kDt);

    const glm::vec3 esperado = p0 + fwd * controller.settings().moveSpeed * kDt;
    CHECK(cam.Position().x == doctest::Approx(esperado.x));
    CHECK(cam.Position().y == doctest::Approx(esperado.y));
    CHECK(cam.Position().z == doctest::Approx(esperado.z));
}

TEST_CASE("CameraController: D estrafea sobre el eje derecho y no cambia la altura") {
    // Con yaw 0 el eje derecho es +X exacto, sin importar el pitch.
    scene::Camera cam;
    scene::CameraController controller(&cam);
    const glm::vec3 p0 = cam.Position();

    platform::Input in;
    frameConTecla(in, platform::Key::D);
    controller.Update(in, kDt);

    CHECK(cam.Position().x > p0.x);
    CHECK(cam.Position().y == doctest::Approx(p0.y));
    CHECK(cam.Position().z == doctest::Approx(p0.z));
}

TEST_CASE("CameraController: E sube y Q baja sobre el Y del mundo") {
    scene::Camera camSube, camBaja;
    scene::CameraController sube(&camSube), baja(&camBaja);
    const glm::vec3 p0 = camSube.Position();
    const f32 paso = sube.settings().moveSpeed * kDt;

    platform::Input inE;
    frameConTecla(inE, platform::Key::E);
    sube.Update(inE, kDt);

    platform::Input inQ;
    frameConTecla(inQ, platform::Key::Q);
    baja.Update(inQ, kDt);

    CHECK(camSube.Position().y == doctest::Approx(p0.y + paso));
    CHECK(camBaja.Position().y == doctest::Approx(p0.y - paso));
    // No hay componente horizontal: es el Y del mundo, no el up de la camara.
    CHECK(camSube.Position().x == doctest::Approx(p0.x));
    CHECK(camSube.Position().z == doctest::Approx(p0.z));
}

TEST_CASE("CameraController: Shift multiplica la velocidad de vuelo") {
    scene::Camera camA, camB;
    scene::CameraController a(&camA), b(&camB);
    const glm::vec3 p0 = camA.Position();

    platform::Input inA;
    frameConTecla(inA, platform::Key::W);
    a.Update(inA, kDt);

    platform::Input inB;
    inB.SetKeyDown(platform::Key::W, true);
    inB.SetKeyDown(platform::Key::LeftShift, true);
    inB.Commit();
    b.Update(inB, kDt);

    const f32 dA = glm::length(camA.Position() - p0);
    const f32 dB = glm::length(camB.Position() - p0);
    CHECK(dB == doctest::Approx(dA * b.settings().boostMultiplier).epsilon(0.001));
}

TEST_CASE("CameraController: mirar con el boton derecho rota pero no mueve la camara") {
    scene::Camera cam;
    scene::CameraController controller(&cam);

    platform::Input in;
    empezarDrag(in, platform::MouseButton::Right, 100.0f, 100.0f);
    controller.Update(in, kDt);

    const glm::vec3 p0   = cam.Position();
    const f32       yaw0 = controller.Yaw();

    moverCursor(in, 140.0f, 100.0f);   // 40 px a la derecha
    controller.Update(in, kDt);

    CHECK(controller.Yaw() != doctest::Approx(yaw0));
    CHECK(cam.Position().x == doctest::Approx(p0.x));
    CHECK(cam.Position().y == doctest::Approx(p0.y));
    CHECK(cam.Position().z == doctest::Approx(p0.z));
}

TEST_CASE("CameraController: el pitch se limita a +-89 grados") {
    scene::Camera cam;
    scene::CameraController controller(&cam);

    platform::Input in;
    empezarDrag(in, platform::MouseButton::Right, 0.0f, 0.0f);
    controller.Update(in, kDt);

    // Arrastre brutal hacia arriba: en pantalla, y decreciente es mirar arriba.
    moverCursor(in, 0.0f, -100000.0f);
    controller.Update(in, kDt);
    CHECK(controller.Pitch() == doctest::Approx(glm::radians(89.0f)).epsilon(0.001));

    moverCursor(in, 0.0f, 100000.0f);
    controller.Update(in, kDt);
    CHECK(controller.Pitch() == doctest::Approx(glm::radians(-89.0f)).epsilon(0.001));
}

TEST_CASE("CameraController: invertY invierte el signo del cambio de pitch") {
    scene::Camera camA, camB;
    scene::CameraController normal(&camA), invertida(&camB);
    invertida.settings().invertY = true;
    const f32 pitch0 = normal.Pitch();

    platform::Input inA;
    empezarDrag(inA, platform::MouseButton::Right, 0.0f, 0.0f);
    normal.Update(inA, kDt);
    moverCursor(inA, 0.0f, -50.0f);
    normal.Update(inA, kDt);

    platform::Input inB;
    empezarDrag(inB, platform::MouseButton::Right, 0.0f, 0.0f);
    invertida.Update(inB, kDt);
    moverCursor(inB, 0.0f, -50.0f);
    invertida.Update(inB, kDt);

    CHECK(normal.Pitch()    != doctest::Approx(pitch0));
    CHECK(normal.Pitch() - pitch0
          == doctest::Approx(-(invertida.Pitch() - pitch0)).epsilon(0.001));
}

TEST_CASE("CameraController: orbitar conserva la distancia y deja el pivote quieto") {
    scene::Camera cam;
    scene::CameraController controller(&cam);

    platform::Input in;
    in.SetKeyDown(platform::Key::LeftAlt, true);
    empezarDrag(in, platform::MouseButton::Left, 200.0f, 200.0f);
    controller.Update(in, kDt);

    const glm::vec3 pivot0 = controller.Pivot();
    const glm::vec3 p0     = cam.Position();
    const f32       d0     = controller.PivotDistance();

    moverCursor(in, 260.0f, 220.0f);
    controller.Update(in, kDt);

    CHECK(controller.PivotDistance() == doctest::Approx(d0));
    CHECK(glm::length(cam.Position() - controller.Pivot())
          == doctest::Approx(d0).epsilon(0.001));
    CHECK(glm::length(controller.Pivot() - pivot0)
          == doctest::Approx(0.0f).epsilon(0.001));
    CHECK(glm::length(cam.Position() - p0) > 0.01f);   // la camara si se movio
}

TEST_CASE("CameraController: el dolly acerca la camara y deja el pivote quieto") {
    scene::Camera cam;
    scene::CameraController controller(&cam);
    const glm::vec3 pivot0 = controller.Pivot();
    const f32       d0     = controller.PivotDistance();

    platform::Input in;
    in.OnMouseScroll(0.0f, 1.0f);   // un notch hacia adelante
    in.Commit();
    controller.Update(in, kDt);

    const f32 esperado = d0 * (1.0f - controller.settings().dollySpeed);
    CHECK(controller.PivotDistance() == doctest::Approx(esperado).epsilon(0.001));
    CHECK(glm::length(controller.Pivot() - pivot0)
          == doctest::Approx(0.0f).epsilon(0.001));
}

TEST_CASE("CameraController: el dolly no cruza minPivotDistance") {
    scene::Camera cam;
    scene::CameraController controller(&cam);

    platform::Input in;
    for (i32 i = 0; i < 200; ++i) {
        in.OnMouseScroll(0.0f, 1.0f);
        in.Commit();
        controller.Update(in, kDt);
    }

    CHECK(controller.PivotDistance() >= controller.settings().minPivotDistance);
}

TEST_CASE("CameraController: panear mueve la camara y el pivote juntos") {
    scene::Camera cam;
    scene::CameraController controller(&cam);

    platform::Input in;
    empezarDrag(in, platform::MouseButton::Middle, 300.0f, 300.0f);
    controller.Update(in, kDt);

    const glm::vec3 pivot0 = controller.Pivot();
    const glm::vec3 p0     = cam.Position();
    const f32       d0     = controller.PivotDistance();

    moverCursor(in, 340.0f, 300.0f);
    controller.Update(in, kDt);

    const glm::vec3 dCam   = cam.Position() - p0;
    const glm::vec3 dPivot = controller.Pivot() - pivot0;

    CHECK(controller.PivotDistance() == doctest::Approx(d0));
    CHECK(glm::length(dCam) > 0.001f);
    CHECK(dPivot.x == doctest::Approx(dCam.x).epsilon(0.001));
    CHECK(dPivot.y == doctest::Approx(dCam.y).epsilon(0.001));
    CHECK(dPivot.z == doctest::Approx(dCam.z).epsilon(0.001));
}

TEST_CASE("CameraController: Focus encuadra el objetivo sin cambiar el angulo de vista") {
    scene::Camera cam;
    scene::CameraController controller(&cam);
    const f32 yaw0   = controller.Yaw();
    const f32 pitch0 = controller.Pitch();

    const glm::vec3 centro(5.0f, 2.0f, -8.0f);
    const f32       radio = 3.0f;
    controller.Focus(centro, radio);

    const f32 esperado = (radio / std::sin(glm::radians(cam.Fov()) * 0.5f))
                       * controller.settings().focusMargin;

    CHECK(controller.Yaw()   == doctest::Approx(yaw0));
    CHECK(controller.Pitch() == doctest::Approx(pitch0));
    CHECK(controller.PivotDistance() == doctest::Approx(esperado).epsilon(0.001));
    CHECK(controller.Pivot().x == doctest::Approx(centro.x).epsilon(0.001));
    CHECK(controller.Pivot().y == doctest::Approx(centro.y).epsilon(0.001));
    CHECK(controller.Pivot().z == doctest::Approx(centro.z).epsilon(0.001));
}

TEST_CASE("CameraController: Focus sin radio centra y conserva la distancia") {
    // La sobrecarga sin radio existe para los nodos sin malla: no hay AABB del
    // que sacar un radio, asi que centrar es lo unico que se puede hacer.
    scene::Camera cam;
    scene::CameraController controller(&cam);
    const f32 d0     = controller.PivotDistance();
    const f32 yaw0   = controller.Yaw();
    const f32 pitch0 = controller.Pitch();

    const glm::vec3 centro(-4.0f, 7.0f, 12.0f);
    controller.Focus(centro);

    CHECK(controller.PivotDistance() == doctest::Approx(d0));
    CHECK(controller.Yaw()   == doctest::Approx(yaw0));
    CHECK(controller.Pitch() == doctest::Approx(pitch0));
    CHECK(controller.Pivot().x == doctest::Approx(centro.x).epsilon(0.001));
    CHECK(controller.Pivot().y == doctest::Approx(centro.y).epsilon(0.001));
    CHECK(controller.Pivot().z == doctest::Approx(centro.z).epsilon(0.001));
}

TEST_CASE("CameraController: con los gates apagados el input se ignora") {
    scene::Camera cam;
    scene::CameraController controller(&cam);
    controller.SetMouseEnabled(false);
    controller.SetKeyboardEnabled(false);

    const glm::vec3 p0   = cam.Position();
    const f32       yaw0 = controller.Yaw();

    platform::Input in;
    in.SetKeyDown(platform::Key::W, true);
    empezarDrag(in, platform::MouseButton::Right, 0.0f, 0.0f);
    controller.Update(in, kDt);
    moverCursor(in, 100.0f, 50.0f);
    controller.Update(in, kDt);

    CHECK(cam.Position().x == doctest::Approx(p0.x));
    CHECK(cam.Position().y == doctest::Approx(p0.y));
    CHECK(cam.Position().z == doctest::Approx(p0.z));
    CHECK(controller.Yaw() == doctest::Approx(yaw0));
}

TEST_CASE("CameraController: el primer frame de un drag no rota") {
    // Al capturar el cursor GLFW lo reubica: sin esta regla la camara salta
    // en cada click. El cursor "aparece" lejos justo cuando se aprieta.
    scene::Camera cam;
    scene::CameraController controller(&cam);
    const f32 yaw0   = controller.Yaw();
    const f32 pitch0 = controller.Pitch();

    platform::Input in;
    empezarDrag(in, platform::MouseButton::Right, 900.0f, 640.0f);
    controller.Update(in, kDt);

    CHECK(controller.Yaw()   == doctest::Approx(yaw0));
    CHECK(controller.Pitch() == doctest::Approx(pitch0));
}

TEST_CASE("CameraController: Tab alterna el mouselook sostenido") {
    scene::Camera cam;
    scene::CameraController controller(&cam);
    CHECK_FALSE(controller.LookToggled());

    platform::Input in;
    frameConTecla(in, platform::Key::Tab);
    controller.Update(in, kDt);
    CHECK(controller.LookToggled());
    CHECK(controller.WantsCursorCaptured());

    // Se mantiene apretada: es un flanco, no un estado.
    in.Commit();
    controller.Update(in, kDt);
    CHECK(controller.LookToggled());

    in.SetKeyDown(platform::Key::Tab, false);
    in.Commit();
    in.SetKeyDown(platform::Key::Tab, true);
    in.Commit();
    controller.Update(in, kDt);
    CHECK_FALSE(controller.LookToggled());
    CHECK_FALSE(controller.WantsCursorCaptured());
}

TEST_CASE("CameraController: con el mouselook prendido se mira sin apretar nada") {
    scene::Camera cam;
    scene::CameraController controller(&cam);

    platform::Input in;
    frameConTecla(in, platform::Key::Tab);
    controller.Update(in, kDt);

    const f32       yaw0 = controller.Yaw();
    const glm::vec3 p0   = cam.Position();

    moverCursor(in, 40.0f, 0.0f);   // ningun boton apretado
    controller.Update(in, kDt);

    CHECK(controller.Yaw() != doctest::Approx(yaw0));
    CHECK(cam.Position().x == doctest::Approx(p0.x));
    CHECK(cam.Position().z == doctest::Approx(p0.z));
}

TEST_CASE("CameraController: el frame en que arranca el mouselook no rota") {
    // Mismo motivo que el primer frame de un drag: al capturar el cursor GLFW
    // lo reubica y el delta de ese frame es basura.
    scene::Camera cam;
    scene::CameraController controller(&cam);
    const f32 yaw0 = controller.Yaw();

    platform::Input in;
    moverCursor(in, 100.0f, 100.0f);
    controller.Update(in, kDt);
    moverCursor(in, 400.0f, 100.0f);   // el cursor ya venia moviendose
    in.SetKeyDown(platform::Key::Tab, true);
    in.Commit();
    controller.Update(in, kDt);

    CHECK(controller.LookToggled());
    CHECK(controller.Yaw() == doctest::Approx(yaw0));
}

TEST_CASE("CameraController: Alt+LMB orbita aunque el mouselook este prendido") {
    // Orbitar y panear pisan al mouselook sostenido mientras duran.
    scene::Camera cam;
    scene::CameraController controller(&cam);

    platform::Input in;
    frameConTecla(in, platform::Key::Tab);
    controller.Update(in, kDt);

    in.SetKeyDown(platform::Key::LeftAlt, true);
    empezarDrag(in, platform::MouseButton::Left, 200.0f, 200.0f);
    controller.Update(in, kDt);

    const glm::vec3 pivot0 = controller.Pivot();
    const f32       d0     = controller.PivotDistance();

    moverCursor(in, 260.0f, 220.0f);
    controller.Update(in, kDt);

    CHECK(glm::length(cam.Position() - controller.Pivot())
          == doctest::Approx(d0).epsilon(0.001));
    CHECK(glm::length(controller.Pivot() - pivot0)
          == doctest::Approx(0.0f).epsilon(0.001));
}

TEST_CASE("CameraController: con el teclado tomado por la UI, Tab no alterna") {
    scene::Camera cam;
    scene::CameraController controller(&cam);
    controller.SetKeyboardEnabled(false);

    platform::Input in;
    frameConTecla(in, platform::Key::Tab);
    controller.Update(in, kDt);

    CHECK_FALSE(controller.LookToggled());
}

TEST_CASE("CameraController: WantsCursorCaptured sigue a los drags") {
    scene::Camera cam;
    scene::CameraController controller(&cam);

    platform::Input in;
    in.Commit();
    controller.Update(in, kDt);
    CHECK_FALSE(controller.WantsCursorCaptured());

    empezarDrag(in, platform::MouseButton::Right, 10.0f, 10.0f);
    controller.Update(in, kDt);
    CHECK(controller.WantsCursorCaptured());

    in.OnMouseButton((i32)platform::MouseButton::Right,
                     (i32)platform::InputAction::Release, 0);
    in.Commit();
    controller.Update(in, kDt);
    CHECK_FALSE(controller.WantsCursorCaptured());
}
