
#include <doctest/doctest.h>
#include <efengine/resources/ResourceManager.h>

TEST_CASE("ResourceManager::GetModel ruta inexistente -> null") {
    efengine::resources::ResourceManager rm;
    CHECK(rm.GetModel("assets/no_existe_xyz.fbx") == nullptr);
}