// tests/renderer/RenderTarget.test.cpp
// El present target es el unico RenderTarget que se puede construir sin
// contexto GL: su handle lo da el backend y su extent es un static de TU.
// Por eso su ruteo se testea headless.
#include <doctest/doctest.h>
#include <efecom/RHI.h>
#include <efengine/renderer/RenderTarget.h>

using efengine::renderer::RenderTarget;

TEST_CASE("RenderTarget::Present: toma el extent que fijo SetPresentExtent") {
    efecom::SetPresentExtent(1280u, 720u);
    const RenderTarget t = RenderTarget::Present();
    CHECK(t.width()  == 1280u);
    CHECK(t.height() == 720u);
}

TEST_CASE("RenderTarget::Present: el extent sigue al resize de la ventana") {
    efecom::SetPresentExtent(1280u, 720u);
    efecom::SetPresentExtent(800u, 600u);
    const RenderTarget t = RenderTarget::Present();
    CHECK(t.width()  == 800u);
    CHECK(t.height() == 600u);
}

TEST_CASE("RenderTarget: se copia por valor y las copias son independientes") {
    efecom::SetPresentExtent(640u, 480u);
    const RenderTarget a = RenderTarget::Present();
    RenderTarget b = a;                  // copia
    efecom::SetPresentExtent(320u, 240u);
    // b guarda el extent del momento de la copia, no relee el backend.
    CHECK(b.width() == 640u);
    CHECK(RenderTarget::Present().width() == 320u);
}
