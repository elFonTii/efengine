#include <doctest/doctest.h>
#include <efengine/resources/AssimpMaterial.h>
#include <efengine/renderer/MaterialDef.h>

using efengine::resources::MetallicPathFromSpecular;
using efengine::resources::RemapTexturePath;

/*
    Las dos son puras: no tocan disco. Si el archivo que devuelven existe o no
    lo decide el caller, porque BuildMaterial ya tolera texturas faltantes.
*/

TEST_CASE("RemapTexturePath: relativa con backslash se resuelve contra el modelo") {
    CHECK(RemapTexturePath("Textures\\Ashtray_BaseColor.dds",
                           "assets/bistro/BistroExterior.fbx")
          == "assets/bistro/Textures/Ashtray_BaseColor.png");
}

TEST_CASE("RemapTexturePath: .dds pasa a .png sin importar mayusculas") {
    CHECK(RemapTexturePath("Textures/X.DDS", "assets/bistro/m.fbx")
          == "assets/bistro/Textures/X.png");
}

TEST_CASE("RemapTexturePath: una extension que no es .dds se deja intacta") {
    // stbi_load ya lee TGA y PNG: cambiarlos mandaria a buscar un archivo que
    // el conversor nunca genero.
    CHECK(RemapTexturePath("Textures/X.tga", "assets/bistro/m.fbx")
          == "assets/bistro/Textures/X.tga");
    CHECK(RemapTexturePath("Textures/X.png", "assets/bistro/m.fbx")
          == "assets/bistro/Textures/X.png");
}

TEST_CASE("RemapTexturePath: absoluta ajena se reduce al basename") {
    // Limite conocido: se pierde el subdirectorio y el archivo no va a existir,
    // asi que el slot se omite. No hay forma general de adivinar la carpeta.
    CHECK(RemapTexturePath("/Users/kanderson/Documents/Bistro/Textures/X.dds",
                           "assets/bistro/m.fbx")
          == "assets/bistro/X.png");
}

TEST_CASE("RemapTexturePath: absoluta estilo Windows tambien se reduce") {
    CHECK(RemapTexturePath("C:\\work\\Bistro\\Textures\\X.dds", "assets/bistro/m.fbx")
          == "assets/bistro/X.png");
}

TEST_CASE("RemapTexturePath: ruta vacia devuelve vacio") {
    CHECK(RemapTexturePath("", "assets/bistro/m.fbx").empty());
}

TEST_CASE("RemapTexturePath: modelo sin directorio deja la ruta relativa tal cual") {
    CHECK(RemapTexturePath("Textures/X.dds", "m.fbx") == "Textures/X.png");
}

TEST_CASE("RemapTexturePath: sin extension no se toca el nombre") {
    CHECK(RemapTexturePath("Textures/X", "assets/bistro/m.fbx")
          == "assets/bistro/Textures/X");
}

TEST_CASE("RemapTexturePath: un punto en el directorio no cuenta como extension") {
    CHECK(RemapTexturePath("Tex.v2/X", "assets/bistro/m.fbx")
          == "assets/bistro/Tex.v2/X");
}

TEST_CASE("MetallicPathFromSpecular: inserta _Metallic antes del .png") {
    CHECK(MetallicPathFromSpecular("assets/bistro/Textures/Ashtray_Specular.png")
          == "assets/bistro/Textures/Ashtray_Specular_Metallic.png");
}

TEST_CASE("MetallicPathFromSpecular: lo que no termina en .png devuelve vacio") {
    // Si no salio del conversor no hay companion que buscar.
    CHECK(MetallicPathFromSpecular("Textures/X.dds").empty());
    CHECK(MetallicPathFromSpecular("").empty());
}

using efengine::renderer::ColorSpace;
using efengine::renderer::TextureSlot;
using efengine::resources::MapAssimpTextureType;

/*
    Los valores son los de aiTextureType: 1 DIFFUSE, 2 SPECULAR, 4 EMISSIVE,
    5 HEIGHT, 6 NORMALS. Se pasan como int para que este header no arrastre
    assimp a todo el que lo incluya.

    El volcado del Bistro (132 materiales) reporto exactamente 1, 2, 4 y 6.
*/

TEST_CASE("MapAssimpTextureType: DIFFUSE va a Albedo en sRGB") {
    const auto m = MapAssimpTextureType(1);
    REQUIRE(m.size() == 1u);
    CHECK(m[0].slot  == TextureSlot::Albedo);
    CHECK(m[0].space == ColorSpace::sRGB);
}

TEST_CASE("MapAssimpTextureType: NORMALS y HEIGHT van los dos a Normal en lineal") {
    // Muchos exportadores de FBX meten el normal map en el canal HEIGHT.
    for (const int tipo : {5, 6}) {
        const auto m = MapAssimpTextureType(tipo);
        REQUIRE(m.size() == 1u);
        CHECK(m[0].slot  == TextureSlot::Normal);
        CHECK(m[0].space == ColorSpace::Linear);
    }
}

TEST_CASE("MapAssimpTextureType: SPECULAR da DOS slots, roughness y metallic") {
    // El _Specular del Bistro es ORM empaquetado: G=roughness, B=metallic. El
    // conversor lo parte en dos PNG y una sola referencia del FBX llena los dos
    // slots. El segundo usa la ruta de MetallicPathFromSpecular.
    const auto m = MapAssimpTextureType(2);
    REQUIRE(m.size() == 2u);

    CHECK(m[0].slot      == TextureSlot::Roughness);
    CHECK(m[0].space     == ColorSpace::Linear);
    CHECK_FALSE(m[0].companionMetallic);

    CHECK(m[1].slot      == TextureSlot::Metallic);
    CHECK(m[1].space     == ColorSpace::Linear);
    CHECK(m[1].companionMetallic);
}

TEST_CASE("MapAssimpTextureType: EMISSIVE va a Emissive en sRGB") {
    const auto m = MapAssimpTextureType(4);
    REQUIRE(m.size() == 1u);
    CHECK(m[0].slot  == TextureSlot::Emissive);
    CHECK(m[0].space == ColorSpace::sRGB);
}

TEST_CASE("MapAssimpTextureType: un tipo no contemplado no devuelve nada") {
    CHECK(MapAssimpTextureType(0).empty());    // NONE
    CHECK(MapAssimpTextureType(3).empty());    // AMBIENT
    CHECK(MapAssimpTextureType(18).empty());   // UNKNOWN
}
