#pragma once

#include <efengine/renderer/MaterialDef.h>

#include <string>
#include <vector>

namespace efengine {
namespace resources {

    struct SlotMapping {
        renderer::TextureSlot slot;
        renderer::ColorSpace  space;
        // true = la ruta no es la que nombra el FBX sino la del companion que
        // devuelve MetallicPathFromSpecular.
        bool                  companionMetallic = false;
    };

    // Los slots que llena un aiTextureType. Vacio si no se mapea.
    //
    // Toma int y no aiTextureType para que este header no arrastre assimp a todo
    // el que lo incluya; el caller castea.
    //
    // SPECULAR devuelve DOS: el _Specular del Bistro es ORM empaquetado y el
    // conversor lo parte en roughness y metallic.
    std::vector<SlotMapping> MapAssimpTextureType(int assimpType);

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

    // Nombre de un aiMaterial, con "material_sin_nombre" si no trae
    // AI_MATKEY_NAME. La malla y el MaterialDef lo toman de aca para coincidir.
    std::string MaterialNameFromAssimp(const void* aiMaterialPtr);

    // Traduce un aiMaterial completo a un MaterialDef del motor.
    //
    // Toma const void* y no const aiMaterial* para que este header no obligue a
    // incluir assimp: el unico caller es ModelLoader.cpp, que ya lo incluye.
    // Shader fijo 'pbr'. Los slots que el archivo no trae quedan en el default
    // del MaterialDef, editables desde el panel.
    renderer::MaterialDef MaterialDefFromAssimp(const void* aiMaterialPtr,
                                                const std::string& modelPath);

}
}
