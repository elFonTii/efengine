#pragma once

#include <string>

namespace efengine {
namespace resources {

    // Traduce la ruta de textura que declara el archivo de modelo a una ruta
    // usable por el motor.
    //
    // Los FBX guardan rutas absolutas de la maquina del autor y relativas al
    // propio archivo; de una absoluta ajena lo unico recuperable es el basename,
    // y ahi se pierde el subdirectorio y el archivo no va a existir. Es
    // aceptable: BuildMaterial tolera texturas faltantes. El Bistro declara las
    // 405 suyas en forma relativa, asi que no toca ese caso.
    //
    // Solo .dds se remapea a .png, porque es lo unico que sh/convert_bistro.ps1
    // genera. TGA y PNG ya los lee stbi_load.
    std::string RemapTexturePath(const std::string& assimpPath,
                                 const std::string& modelPath);

    // El companion de metallic de una ruta de specular ya remapeada.
    //
    // El _Specular del Bistro es ORM empaquetado y el conversor lo parte en dos
    // PNG; este es el nombre del segundo. Cadena vacia si la entrada no termina
    // en .png, porque entonces no salio del conversor.
    //
    //   assets/bistro/Textures/X_Specular.png
    //     -> assets/bistro/Textures/X_Specular_Metallic.png
    std::string MetallicPathFromSpecular(const std::string& specularPngPath);

}
}
