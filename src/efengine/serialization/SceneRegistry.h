#pragma once
#include <efengine/serialization/BehaviorRegistry.h>
#include <efengine/serialization/ComponentRegistry.h>
#include <efengine/serialization/CoreComponents.h>
#include <efengine/serialization/MeshGeneratorRegistry.h>

namespace efengine {
namespace serialization {

    // Todo lo que el cliente le tiene que ensenar al serializador sobre sus propios
    // tipos. Se arma una vez al arrancar y se le pasa a Save/Load.
    struct SceneRegistry {
        BehaviorRegistry      behaviors;
        ComponentRegistry     components;
        MeshGeneratorRegistry meshes;

        // Malla y luz son del motor, no del cliente: se registran solas. Si
        // hubiera que acordarse de registrarlas, olvidarse significaria cargar
        // todas las escenas sin mallas ni luces.
        SceneRegistry() { RegisterCoreComponents(components); }
    };

}
}
