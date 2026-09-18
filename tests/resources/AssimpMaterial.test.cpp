#include <doctest/doctest.h>
#include <efengine/resources/AssimpMaterial.h>

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
