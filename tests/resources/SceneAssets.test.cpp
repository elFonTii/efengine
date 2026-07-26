#include <doctest/doctest.h>
#include <efengine/renderer/Material.h>
#include <efengine/renderer/MaterialDef.h>
#include <efengine/resources/SceneAssets.h>

namespace {
    using namespace efengine;

    // Shader nulo a proposito: nada de esto llama a GL mientras no se haga Bind().
    renderer::Material materialDePrueba(f32 roughness) {
        renderer::Material m(nullptr);
        m.roughness = roughness;
        return m;
    }

    renderer::MaterialDef defDePrueba(const char* nombre, f32 roughness) {
        renderer::MaterialDef d;
        d.name      = nombre;
        d.roughness = roughness;
        return d;
    }
}

TEST_CASE("SceneAssets::AddMaterial devuelve indices crecientes desde 0") {
    resources::SceneAssets assets;
    CHECK(assets.AddMaterial(defDePrueba("a", 1.0f), materialDePrueba(1.0f)) == 0u);
    CHECK(assets.AddMaterial(defDePrueba("b", 1.0f), materialDePrueba(1.0f)) == 1u);
    CHECK(assets.MaterialCount() == 2u);
}

TEST_CASE("SceneAssets::UpdateMaterial NO mueve la direccion del Material") {
    resources::SceneAssets assets;
    const u32 i = assets.AddMaterial(defDePrueba("piso", 1.0f), materialDePrueba(1.0f));

    const renderer::Material* antes = assets.MaterialAt(i);
    REQUIRE(antes != nullptr);

    REQUIRE(assets.UpdateMaterial(i, defDePrueba("piso editado", 0.25f), materialDePrueba(0.25f)));

    // Esto es lo que hace que los MeshAttachment::materials de los nodos
    // sigan siendo validos despues de editar un material.
    CHECK(assets.MaterialAt(i) == antes);
    CHECK(antes->roughness == doctest::Approx(0.25f));
    CHECK(assets.DefAt(i)->name == "piso editado");
    CHECK(assets.IndexOfMaterial(antes) == i);
}

TEST_CASE("SceneAssets::UpdateMaterial con indice invalido -> false y no toca nada") {
    resources::SceneAssets assets;
    const u32 i = assets.AddMaterial(defDePrueba("unico", 1.0f), materialDePrueba(1.0f));

    CHECK_FALSE(assets.UpdateMaterial(i + 1u, defDePrueba("fantasma", 0.0f), materialDePrueba(0.0f)));
    CHECK(assets.MaterialCount() == 1u);
    CHECK(assets.DefAt(i)->name == "unico");
    CHECK(assets.MaterialAt(i)->roughness == doctest::Approx(1.0f));
}
