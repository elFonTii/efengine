#include <doctest/doctest.h>
#include <efengine/scene/LightResolve.h>
#include <efengine/math/Transform.h>

#include <glm/glm.hpp>
#include <cmath>

using namespace efengine;

namespace {
    f32 Luminancia(const glm::vec3& c) { return 0.2126f * c.r + 0.7152f * c.g + 0.0722f * c.b; }
}

TEST_CASE("ResolveLight: el color efectivo es tinte x intensidad") {
    scene::LightAttachment a;
    a.color     = glm::vec3(1.0f, 0.5f, 0.25f);
    a.intensity = 8.0f;
    const renderer::Light l = scene::ResolveLight(a, glm::mat4(1.0f));
    CHECK(l.color.r == doctest::Approx(8.0f));
    CHECK(l.color.g == doctest::Approx(4.0f));
    CHECK(l.color.b == doctest::Approx(2.0f));
}

TEST_CASE("ResolveLight: la temperatura cambia el tono y conserva la luminancia") {
    scene::LightAttachment a;
    a.intensity      = 10.0f;
    a.useTemperature = true;
    a.temperatureK   = 2700.0f;
    const renderer::Light l = scene::ResolveLight(a, glm::mat4(1.0f));
    CHECK(l.color.r > l.color.g);
    CHECK(l.color.g > l.color.b);
    CHECK(Luminancia(l.color) == doctest::Approx(10.0f).epsilon(1e-3));
}

TEST_CASE("ResolveLight: sin useTemperature los Kelvin no hacen nada") {
    scene::LightAttachment a;
    a.intensity    = 10.0f;
    a.temperatureK = 2700.0f;
    const renderer::Light l = scene::ResolveLight(a, glm::mat4(1.0f));
    CHECK(l.color.r == doctest::Approx(10.0f));
    CHECK(l.color.b == doctest::Approx(10.0f));
}

TEST_CASE("ResolveLight: la direccion es el -Z del world, normalizada aunque el nodo este escalado") {
    math::Transform t;
    t.position = glm::vec3(1.0f, 2.0f, 3.0f);
    t.rotation = glm::vec3(-90.0f, 0.0f, 0.0f);
    t.scale    = glm::vec3(3.0f);
    scene::LightAttachment a;
    a.kind = scene::LightKind::Spot;
    const renderer::Light l = scene::ResolveLight(a, t.Matrix());
    CHECK(l.direction.x == doctest::Approx(0.0f).epsilon(1e-4));
    CHECK(l.direction.y == doctest::Approx(-1.0f).epsilon(1e-4));
    CHECK(l.direction.z == doctest::Approx(0.0f).epsilon(1e-4));
    CHECK(l.position.y == doctest::Approx(2.0f));
    CHECK(l.range == doctest::Approx(10.0f));   // la escala no estira el rango
}

TEST_CASE("ResolveLight: sanea conos, rango, radio, intensidad y color") {
    scene::LightAttachment a;
    a.kind         = scene::LightKind::Spot;
    a.innerConeDeg = 60.0f;
    a.outerConeDeg = 30.0f;
    a.range        = 0.0f;
    a.sourceRadius = 50.0f;
    a.intensity    = 1.0f;
    const renderer::Light l = scene::ResolveLight(a, glm::mat4(1.0f));
    CHECK(l.cosInner == doctest::Approx(l.cosOuter));
    CHECK(l.range == doctest::Approx(0.01f));
    CHECK(l.sourceRadius < l.range);

    a.outerConeDeg = 120.0f;
    CHECK(scene::ResolveLight(a, glm::mat4(1.0f)).cosOuter
          == doctest::Approx(std::cos(glm::radians(89.0f))));

    a.intensity = -5.0f;
    CHECK(scene::ResolveLight(a, glm::mat4(1.0f)).color.r == 0.0f);

    a.intensity = 1.0f;
    a.color     = glm::vec3(-1.0f, 0.5f, 0.5f);
    CHECK(scene::ResolveLight(a, glm::mat4(1.0f)).color.r == 0.0f);
}

TEST_CASE("ResolveLight: el tipo se traduce uno a uno") {
    scene::LightAttachment a;
    a.kind = scene::LightKind::Point;
    CHECK(scene::ResolveLight(a, glm::mat4(1.0f)).type == renderer::LightType::Point);
    a.kind = scene::LightKind::Spot;
    CHECK(scene::ResolveLight(a, glm::mat4(1.0f)).type == renderer::LightType::Spot);
    a.kind = scene::LightKind::Directional;
    CHECK(scene::ResolveLight(a, glm::mat4(1.0f)).type == renderer::LightType::Directional);
}
