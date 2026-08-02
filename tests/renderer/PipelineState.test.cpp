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

TEST_CASE("El estado opaco cullea la cara de atras; el double-sided no") {
    CHECK(OpaqueState().cullMode            == CullMode::Back);
    CHECK(ShadowDepthState().cullMode       == CullMode::Back);
    CHECK(OpaqueDoubleSidedState().cullMode == CullMode::None);
    // Los pases que dibujan un quad fullscreen no tienen cara de atras que cullear.
    CHECK(SkyboxState().cullMode     == CullMode::None);
    CHECK(FullscreenState().cullMode == CullMode::None);
}

TEST_CASE("DdgiCaptureState: no cullea, para que el probe vea las caras internas") {
    const efecom::PipelineState s = efengine::renderer::DdgiCaptureState();
    CHECK(s.cullMode == efecom::CullMode::None);
}

TEST_CASE("DdgiCaptureState: en todo lo demas es OpaqueState") {
    const efecom::PipelineState captura = efengine::renderer::DdgiCaptureState();
    const efecom::PipelineState opaco   = efengine::renderer::OpaqueState();
    CHECK(captura.depthTest   == opaco.depthTest);
    CHECK(captura.depthWrite  == opaco.depthWrite);
    CHECK(captura.depthFunc   == opaco.depthFunc);
    CHECK(captura.blendEnable == opaco.blendEnable);
}
