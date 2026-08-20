#include <doctest/doctest.h>
#include <efengine/scene/SceneGraph.h>
#include <efengine/scene/Behavior.h>
#include <efengine/scene/Node.h>
#include <efengine/core/Time.h>
#include <memory>
#include <cmath>

using namespace efengine;

namespace {
    // Cuenta las llamadas de cada canal por separado: asi se ve que Update y
    // FixedUpdate no se pisan.
    struct ContadorDobleCanal : scene::Behavior {
        int updates      = 0;
        int fixedUpdates = 0;
        f32 lastFixedDt  = -1.0f;

        void OnUpdate(scene::UpdateContext&) override { ++updates; }

        void OnFixedUpdate(scene::FixedUpdateContext& ctx) override {
            ++fixedUpdates;
            lastFixedDt = ctx.dt;
        }
    };

    // Solo implementa el canal variable: el default vacio de OnFixedUpdate tiene
    // que dejarlo pasar sin romper nada.
    struct SoloVariable : scene::Behavior {
        int updates = 0;
        void OnUpdate(scene::UpdateContext&) override { ++updates; }
    };

    // Mueve el nodo desde el canal fijo: prueba que SetLocal tambien funciona ahi.
    struct AvanzarEnX : scene::Behavior {
        f32 unitsPerSec = 0.0f;

        AvanzarEnX() = default;
        explicit AvanzarEnX(f32 speed) : unitsPerSec(speed) {}

        void OnUpdate(scene::UpdateContext&) override {}

        void OnFixedUpdate(scene::FixedUpdateContext& ctx) override {
            math::Transform t = ctx.node.local;
            t.position.x += ctx.dt * unitsPerSec;
            ctx.SetLocal(t);
        }
    };
}

TEST_CASE("SceneGraph::FixedUpdate llama OnFixedUpdate con el dt fijo") {
    scene::SceneGraph scene;
    scene::NodeHandle n = scene.CreateChild(scene.Root(), "n");
    auto* b = static_cast<ContadorDobleCanal*>(
        scene.AttachBehavior(n, std::make_unique<ContadorDobleCanal>()));

    scene.FixedUpdate(1.0f / 60.0f);

    CHECK(b->fixedUpdates == 1);
    CHECK(b->lastFixedDt == doctest::Approx(1.0f / 60.0f));
}

TEST_CASE("Update y FixedUpdate son canales independientes") {
    scene::SceneGraph scene;
    scene::NodeHandle n = scene.CreateChild(scene.Root(), "n");
    auto* b = static_cast<ContadorDobleCanal*>(
        scene.AttachBehavior(n, std::make_unique<ContadorDobleCanal>()));

    scene.FixedUpdate(0.016f);
    scene.FixedUpdate(0.016f);
    scene.Update(0.033f);

    CHECK(b->fixedUpdates == 2);
    CHECK(b->updates      == 1);
}

TEST_CASE("Un behavior que no implementa OnFixedUpdate sobrevive al canal fijo") {
    scene::SceneGraph scene;
    scene::NodeHandle n = scene.CreateChild(scene.Root(), "n");
    auto* b = static_cast<SoloVariable*>(
        scene.AttachBehavior(n, std::make_unique<SoloVariable>()));

    scene.FixedUpdate(0.016f);   // no debe romper nada
    scene.Update(0.016f);

    CHECK(b->updates == 1);
}

TEST_CASE("FixedUpdate respeta el flag enabled") {
    scene::SceneGraph scene;
    scene::NodeHandle n = scene.CreateChild(scene.Root(), "n");
    auto* b = static_cast<ContadorDobleCanal*>(
        scene.AttachBehavior(n, std::make_unique<ContadorDobleCanal>()));
    b->enabled = false;

    scene.FixedUpdate(0.016f);

    CHECK(b->fixedUpdates == 0);
}

TEST_CASE("SetLocal desde OnFixedUpdate ensucia el world del nodo") {
    scene::SceneGraph scene;
    scene::NodeHandle n = scene.CreateChild(scene.Root(), "n");
    scene.AttachBehavior(n, std::make_unique<AvanzarEnX>(60.0f));  // 60 u/s

    scene.FixedUpdate(1.0f / 60.0f);   // un paso => 1 unidad
    scene.UpdateWorldTransforms();

    CHECK(scene.Get(n).local.position.x == doctest::Approx(1.0f));
    CHECK(scene.Get(n).worldMatrix[3][0] == doctest::Approx(1.0f));
}

namespace {
    // Corre 'seconds' de tiempo simulado alimentando Time con timestamps de un
    // framerate constante, bombeando los pasos fijos igual que hace el sandbox, y
    // devuelve cuanto avanzo el nodo.
    f32 distanciaTrasCorrer(f32 fps, f64 seconds) {
        core::Time time;
        time.SetMaxDelta(1.0f);         // que el clamp del dt variable no participe
        time.SetFixedDelta(1.0f / 60.0f);
        time.SetMaxFixedSteps(64);      // ni el tope: aca no queremos saturacion

        scene::SceneGraph scene;
        scene::NodeHandle n = scene.CreateChild(scene.Root(), "mover");
        scene.AttachBehavior(n, std::make_unique<AvanzarEnX>(10.0f));  // 10 u/s

        const f64 step   = 1.0 / static_cast<f64>(fps);
        const int frames = static_cast<int>(seconds * static_cast<f64>(fps));

        f64 now = 100.0;
        time.Advance(now);              // primer tick: fija el baseline, no simula

        for (int f = 0; f < frames; ++f) {
            now += step;
            time.Advance(now);
            for (i32 s = 0; s < time.FixedSteps(); ++s) {
                scene.FixedUpdate(time.FixedDelta());
            }
        }

        return scene.Get(n).local.position.x;
    }
}

TEST_CASE("El paso fijo hace que la distancia recorrida no dependa del framerate") {
    // 2 segundos a 10 u/s = 20 unidades, corra a 30 o a 144 FPS.
    const f32 a30  = distanciaTrasCorrer(30.0f,  2.0);
    const f32 a144 = distanciaTrasCorrer(144.0f, 2.0);

    // Tolerancia: un paso fijo entero (10 u/s * 1/60 s). Al final de la corrida el
    // acumulador puede haber quedado a mitad de paso, y eso es correcto, no un bug:
    // lo que el paso fijo garantiza es que el error esta acotado por un paso, no
    // que sea cero.
    const f32 unPaso = 10.0f * (1.0f / 60.0f);

    // El margen extra es redondeo puro, no holgura de diseno: 1/60 en f32 vale
    // 0.016666668, un pelo MAS que 1/60 real, asi que en exactamente 2 s de
    // timestamps no entran 120 pasos sino 119. El error real queda en un paso
    // mas 3e-6, y comparar contra un paso exacto seria pedirle a los floats una
    // igualdad de borde. Lo que se verifica sigue siendo "acotado por un paso".
    const f32 tolerancia = unPaso * 1.001f;

    CHECK(std::abs(a30  - 20.0f) <= tolerancia);
    CHECK(std::abs(a144 - 20.0f) <= tolerancia);

    // Y lo que de verdad importa: las dos corridas coinciden entre si.
    CHECK(std::abs(a30 - a144) <= tolerancia);
}

TEST_CASE("Un frame catastrofico no teletransporta la simulacion") {
    core::Time time;
    time.SetFixedDelta(1.0f / 60.0f);
    time.SetMaxFixedSteps(5);

    scene::SceneGraph scene;
    scene::NodeHandle n = scene.CreateChild(scene.Root(), "mover");
    scene.AttachBehavior(n, std::make_unique<AvanzarEnX>(10.0f));

    time.Advance(100.0);
    time.Advance(130.0);            // 30 segundos: un breakpoint, o cargar assets

    CHECK(time.FixedStepsSaturated());

    for (i32 s = 0; s < time.FixedSteps(); ++s) scene.FixedUpdate(time.FixedDelta());

    // Sin tope, 30 s a 10 u/s serian 300 unidades de un salto. Con tope son 5
    // pasos: el personaje se atrasa respecto del reloj de pared, que es
    // exactamente la decision que queremos.
    CHECK(scene.Get(n).local.position.x == doctest::Approx(5.0f * 10.0f / 60.0f));
}
