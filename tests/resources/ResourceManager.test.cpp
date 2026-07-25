
#include <doctest/doctest.h>
#include <efengine/resources/ResourceManager.h>
#include <efengine/renderer/Model.h>
#include <efengine/renderer/Mesh.h>

#include <vector>

TEST_CASE("ResourceManager::GetModel ruta inexistente -> null") {
    efengine::resources::ResourceManager rm;
    CHECK(rm.GetModel("assets/no_existe_xyz.fbx") == nullptr);
}

TEST_CASE("ResourceManager::GetTexture ruta inexistente -> null") {
    using namespace efengine;
    resources::ResourceManager rm;
    CHECK(rm.GetTexture("assets/no_existe.png", renderer::ColorSpace::sRGB) == nullptr);
}

TEST_CASE("ResourceManager::GetShader archivos inexistentes -> null") {
    using namespace efengine;
    resources::ResourceManager rm;
    CHECK(rm.GetShader("pbr", "assets/no.vert", "assets/no.frag") == nullptr);
}

TEST_CASE("ResourceManager::PathOf puntero nulo -> null") {
    efengine::resources::ResourceManager rm;
    CHECK(rm.PathOf(nullptr) == nullptr);
}

TEST_CASE("ResourceManager::PathOf modelo que no cargo el manager -> null") {
    using namespace efengine;
    resources::ResourceManager rm;

    // Un Model construido a mano nunca paso por el cache: no tiene path.
    renderer::Model ajeno(std::vector<renderer::Mesh>{});
    CHECK(rm.PathOf(&ajeno) == nullptr);
}