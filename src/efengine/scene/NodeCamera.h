#pragma once

#include <efengine/core/Types.h>
#include <efengine/scene/NodeHandle.h>

namespace efengine {
namespace scene {

    class Camera;
    class SceneGraph;

    // Llena 'out' con la pose del nodo y los cuatro campos de su
    // CameraAttachment, mas el aspect que le pasa el cliente (la escena no sabe
    // el tamano de la ventana). Devuelve false si el handle no es valido o el
    // nodo no tiene camara; en ese caso 'out' NO se toca.
    //
    // Vive fuera de Camera y de SceneGraph porque los junta, y ninguno de los
    // dos conoce al otro: SceneGraph no sabe que es una Camera, y Camera no sabe
    // que es un nodo. Es el unico punto de entrada que necesita un cliente
    // (el sandbox hoy, un ejecutable de juego manana):
    //
    //     scene::ApplyNodeCamera(cam, scene, scene.ActiveCamera(), aspect);
    bool ApplyNodeCamera(Camera& out, const SceneGraph& graph, NodeHandle node, f32 aspect);

}
}
