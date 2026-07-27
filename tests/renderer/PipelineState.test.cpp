// tests/renderer/PipelineState.test.cpp
// PipelineState es solo datos: se testea headless. Lo que importa es (a) que la
// igualdad compare TODOS los campos (el backend la usa para saltear llamadas gl*
// redundantes: un campo olvidado ahi es un bug invisible) y (b) que cada estado
// con nombre diga lo que su nombre promete.
#include <doctest/doctest.h>
#include <efecom/RHI.h>
#include <efengine/renderer/PipelineStates.h>

using efecom::PipelineState;
using efecom::CullMode;
using efecom::DepthFunc;
using namespace efengine::renderer;

TEST_CASE("PipelineState: dos defaults son iguales") {
    CHECK(PipelineState{} == PipelineState{});
}

TEST_CASE("PipelineState: un solo campo distinto rompe la igualdad") {
    PipelineState a;
    PipelineState b;

    b.depthWrite = false;
    CHECK(a != b);

    b = PipelineState{};
    b.cullMode = CullMode::Front;
    CHECK(a != b);

    b = PipelineState{};
    b.blendEnable = true;
    CHECK(a != b);

    b = PipelineState{};
    b.colorWrite[3] = false;   // el array tambien cuenta
    CHECK(a != b);
}

TEST_CASE("OpaqueState: escribe y testea profundidad") {
    const PipelineState s = OpaqueState();
    CHECK(s.depthTest  == true);
    CHECK(s.depthWrite == true);
    CHECK(s.depthFunc  == DepthFunc::Less);
    CHECK(s.blendEnable == false);
}

TEST_CASE("OpaqueDoubleSidedState: identico al opaco salvo el cull") {
    PipelineState esperado = OpaqueState();
    esperado.cullMode = CullMode::None;
    CHECK(OpaqueDoubleSidedState() == esperado);
}

TEST_CASE("SkyboxState: sin depth test ni escritura de profundidad") {
    const PipelineState s = SkyboxState();
    CHECK(s.depthTest  == false);
    CHECK(s.depthWrite == false);
    CHECK(s.cullMode   == CullMode::None);
}

TEST_CASE("FullscreenState: un quad que no participa de la profundidad") {
    const PipelineState s = FullscreenState();
    CHECK(s.depthTest   == false);
    CHECK(s.depthWrite  == false);
    CHECK(s.cullMode    == CullMode::None);
    CHECK(s.blendEnable == false);
}

TEST_CASE("ShadowDepthState: escribe profundidad, igual que el opaco") {
    const PipelineState s = ShadowDepthState();
    CHECK(s.depthTest  == true);
    CHECK(s.depthWrite == true);
    CHECK(s.depthFunc  == DepthFunc::Less);
}

TEST_CASE("Todos los estados arrancan sin culling (se prende en un commit aparte)") {
    CHECK(OpaqueState().cullMode      == CullMode::None);
    CHECK(ShadowDepthState().cullMode == CullMode::None);
}
