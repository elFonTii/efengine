#pragma once
#include <efengine/serialization/BehaviorRegistry.h>
#include <efengine/serialization/MeshGeneratorRegistry.h>

namespace efengine {
namespace serialization {

    // Todo lo que el cliente le tiene que ensenar al serializador sobre sus propios
    // tipos. Se arma una vez al arrancar y se le pasa a Save/Load.
    struct SceneRegistry {
        BehaviorRegistry      behaviors;
        MeshGeneratorRegistry meshes;
    };

}
}
