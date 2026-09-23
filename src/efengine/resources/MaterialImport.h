#pragma once

#include <efengine/renderer/Material.h>
#include <efengine/resources/ResourceManager.h>
#include <efengine/resources/SceneAssets.h>

namespace efengine {
namespace resources {

    // Registra en 'assets' un material por cada uno que declaraba el archivo del
    // modelo y devuelve el MaterialMap para el MeshAttachment del nodo.
    //
    // Dedup por nombre: si ya hay un slot con ese nombre lo reusa, asi dos
    // modelos que comparten material comparten el slot editable.
    //
    // Mapa vacio si el modelo no carga o no trae materiales; ahi el caller cae a
    // MakeUniformMaterialMap con un material default.
    renderer::MaterialMap ImportMaterials(ResourceManager& rm, SceneAssets& assets,
                                          const char* modelPath);

}
}
