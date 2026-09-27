#include <doctest/doctest.h>
#include <efengine/renderer/ShadowPass.h>
#include <efengine/renderer/DdgiSettings.h>

using efengine::renderer::DdgiSettings;
using efengine::renderer::SceneShadowNeeded;

TEST_CASE("SceneShadowNeeded: el mapa de escena sigue a DDGI") {
    // Sin DdgiPass registrado no hay quien lo lea.
    CHECK_FALSE(SceneShadowNeeded(null));

    DdgiSettings ddgi;
    ddgi.enabled = false;
    CHECK_FALSE(SceneShadowNeeded(&ddgi));

    // Con DDGI prendido, sin el mapa la captura traza el sol sin sombra.
    ddgi.enabled = true;
    CHECK(SceneShadowNeeded(&ddgi));
}
