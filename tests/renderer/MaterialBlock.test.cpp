// tests/renderer/MaterialBlock.test.cpp
// Material::ToBlock() es una funcion pura: empaqueta escalares y presencia de
// mapas, sin tocar la GPU. Se puede construir un Material headless (shader nulo)
// porque el UBO NO vive adentro de Material — vive en el Renderer, justo para
// que este test siga siendo posible.
#include <doctest/doctest.h>
#include <efengine/renderer/Material.h>
#include <efengine/renderer/MaterialDef.h>
#include <efengine/renderer/ShaderBlocks.h>

using namespace efengine;
using namespace efengine::renderer;

namespace {
    // ToBlock() solo compara los punteros de textura contra null: nunca los
    // desreferencia. Un puntero basura alcanza para simular "hay mapa".
    const Texture* mapaFalso() { return reinterpret_cast<const Texture*>(0x1); }

    bool tieneSlot(const MaterialBlock& b, TextureSlot slot) {
        return (b.mapMask.x & (1u << static_cast<u32>(slot))) != 0u;
    }
}

TEST_CASE("ToBlock: sin ningun mapa el bitmask queda en cero") {
    Material m(nullptr);
    CHECK(m.ToBlock().mapMask.x == 0u);
}

TEST_CASE("ToBlock: cada mapa prende el bit de SU TextureSlot") {
    Material m(nullptr);

    m.SetAlbedoMap(mapaFalso());
    CHECK(tieneSlot(m.ToBlock(), TextureSlot::Albedo));
    CHECK_FALSE(tieneSlot(m.ToBlock(), TextureSlot::Normal));

    m.SetEmissiveMap(mapaFalso());
    CHECK(tieneSlot(m.ToBlock(), TextureSlot::Emissive));
    CHECK(tieneSlot(m.ToBlock(), TextureSlot::Albedo));   // el anterior sigue
}

TEST_CASE("ToBlock: los 8 mapas prenden los 8 bits bajos y ninguno mas") {
    Material m(nullptr);
    m.SetAlbedoMap(mapaFalso());
    m.SetNormalMap(mapaFalso());
    m.SetAOMap(mapaFalso());
    m.SetRoughnessMap(mapaFalso());
    m.SetMetallicMap(mapaFalso());
    m.SetHeightMap(mapaFalso());
    m.SetOpacityMap(mapaFalso());
    m.SetEmissiveMap(mapaFalso());

    CHECK(m.ToBlock().mapMask.x == 0xFFu);
}

TEST_CASE("ToBlock: quitar un mapa apaga su bit") {
    Material m(nullptr);
    m.SetNormalMap(mapaFalso());
    REQUIRE(tieneSlot(m.ToBlock(), TextureSlot::Normal));

    m.SetNormalMap(nullptr);
    CHECK_FALSE(tieneSlot(m.ToBlock(), TextureSlot::Normal));
}

TEST_CASE("ToBlock: los tints van en .rgb") {
    Material m(nullptr);
    m.albedoTint   = glm::vec3(0.1f, 0.2f, 0.3f);
    m.emissiveTint = glm::vec3(0.4f, 0.5f, 0.6f);

    const MaterialBlock b = m.ToBlock();
    CHECK(b.albedoTint.r   == doctest::Approx(0.1f));
    CHECK(b.albedoTint.b   == doctest::Approx(0.3f));
    CHECK(b.emissiveTint.g == doctest::Approx(0.5f));
}

TEST_CASE("ToBlock: scalars0 = (metallic, roughness, aoStrength, heightScale)") {
    Material m(nullptr);
    m.metallic    = 0.11f;
    m.roughness   = 0.22f;
    m.aoStrength  = 0.33f;
    m.heightScale = 0.44f;

    const MaterialBlock b = m.ToBlock();
    CHECK(b.scalars0.x == doctest::Approx(0.11f));
    CHECK(b.scalars0.y == doctest::Approx(0.22f));
    CHECK(b.scalars0.z == doctest::Approx(0.33f));
    CHECK(b.scalars0.w == doctest::Approx(0.44f));
}

TEST_CASE("ToBlock: scalars1 = (alphaCutoff, emissiveIntensity, normalStrength, _)") {
    Material m(nullptr);
    m.alphaCutoff       = 0.55f;
    m.emissiveIntensity = 6.5f;
    m.normalStrength    = 1.75f;

    const MaterialBlock b = m.ToBlock();
    CHECK(b.scalars1.x == doctest::Approx(0.55f));
    CHECK(b.scalars1.y == doctest::Approx(6.5f));
    CHECK(b.scalars1.z == doctest::Approx(1.75f));
}

TEST_CASE("ToBlock: los defaults de Material son los que espera el shader") {
    const MaterialBlock b = Material(nullptr).ToBlock();
    CHECK(b.albedoTint.r == doctest::Approx(1.0f));
    CHECK(b.scalars0.x   == doctest::Approx(0.5f));   // metallic
    CHECK(b.scalars0.y   == doctest::Approx(1.0f));   // roughness
    CHECK(b.scalars1.y   == doctest::Approx(0.0f));   // emissiveIntensity: apagada
    CHECK(b.scalars1.z   == doctest::Approx(1.0f));   // normalStrength: el mapa tal cual
}

TEST_CASE("ToBlock: uvTransform = (tiling.x, tiling.y, offset.x, offset.y)") {
    Material m(nullptr);
    m.uvTiling = glm::vec2(4.0f, 2.0f);
    m.uvOffset = glm::vec2(0.5f, 0.25f);

    const MaterialBlock b = m.ToBlock();
    CHECK(b.uvTransform.x == doctest::Approx(4.0f));
    CHECK(b.uvTransform.y == doctest::Approx(2.0f));
    CHECK(b.uvTransform.z == doctest::Approx(0.5f));
    CHECK(b.uvTransform.w == doctest::Approx(0.25f));
}

TEST_CASE("ToBlock: el default de uvTransform es la identidad") {
    // Si este test cambia, TODA escena guardada se ve distinta sin que nadie
    // haya tocado un material: (1,1)/(0,0) es exactamente "la UV de la malla
    // tal cual", que es lo que hacia el shader antes de que existiera el tiling.
    const MaterialBlock b = Material(nullptr).ToBlock();
    CHECK(b.uvTransform.x == doctest::Approx(1.0f));
    CHECK(b.uvTransform.y == doctest::Approx(1.0f));
    CHECK(b.uvTransform.z == doctest::Approx(0.0f));
    CHECK(b.uvTransform.w == doctest::Approx(0.0f));
}
