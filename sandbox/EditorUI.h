#pragma once

#include "FrameStats.h"

#include <efengine/core/Types.h>
#include <efengine/scene/NodeHandle.h>

namespace efengine {
    namespace application  { class Application; }
    namespace scene        { class SceneGraph; class Camera; class CameraController; }
    namespace resources    { class SceneAssets; class ResourceManager; }
    namespace serialization { struct SceneRegistry; }
}

namespace sandbox {

    // Estado que la UI necesita recordar entre frames. Vive en main y se le
    // presta al editor: el loop es el dueno, la UI solo lo lee y lo escribe.
    struct EditorState {
        // Handles cacheados por nombre. Cargar la escena los invalida a todos,
        // por eso existe RefreshHandles.
        efengine::scene::NodeHandle rat;
        efengine::scene::NodeHandle orbitLight;
        efengine::scene::NodeHandle sun;
        efengine::scene::NodeHandle selected;

        bool animate      = false;
        bool animateSun   = false;
        u32  spawnCounter = 0;

        bool showHierarchy = true;
        bool showInspector = true;
        bool showRender    = true;
        bool showStats     = true;

        FrameStats stats;
    };

    // Todo lo que la UI puede tocar, atado de una sola vez por frame.
    // Referencias y no punteros: nada de esto puede faltar ni cambiar de identidad.
    struct EditorContext {
        efengine::application::Application&           app;
        efengine::scene::SceneGraph&                  scene;
        efengine::scene::Camera&                      camera;
        efengine::scene::CameraController&            controller;
        efengine::resources::SceneAssets&             assets;
        efengine::resources::ResourceManager&         rm;
        const efengine::serialization::SceneRegistry& registry;
        EditorState&                                  state;
    };

    // Re-resuelve los handles por nombre y recalcula los flags de animacion.
    // Se llama al arrancar y despues de cada carga de escena.
    void RefreshHandles(EditorContext& ctx);

    // Encuadra el nodo seleccionado. Vive aca y no en el motor porque la
    // seleccion es un concepto de la UI: la camara no tiene que aprender que
    // es un nodo seleccionado. Sin seleccion valida, no hace nada.
    void FocusSelection(EditorContext& ctx);

    // Unico punto de entrada: dibuja barra de menu, ventanas y overlay.
    void DrawEditor(EditorContext& ctx);

}
