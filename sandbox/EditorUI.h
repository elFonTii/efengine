#pragma once

#include "AssetCatalog.h"
#include "FrameStats.h"

#include <efengine/core/Types.h>
#include <efengine/renderer/MaterialDef.h>
#include <efengine/scene/NodeHandle.h>

#include <string>

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

        // Reparent pedido por drag & drop, pendiente de aplicar. Mutar la
        // jerarquia en medio de la recursion que la dibuja rompe el recorrido,
        // asi que el drop solo anota y drawHierarchy aplica al final del frame.
        // Child nulo = no hay pedido. Uno por frame alcanza: no se pueden soltar
        // dos cosas a la vez.
        efengine::scene::NodeHandle pendingReparentChild;
        efengine::scene::NodeHandle pendingReparentParent;

        bool animate      = false;
        bool animateSun   = false;
        u32  spawnCounter = 0;

        // Ruta del .efe abierto. Vacia = la escena no tiene archivo: es la sala
        // de Cornell recien armada, o un arranque en el que el Load fallo. Sin
        // ruta, "Guardar" no tiene destino.
        std::string currentScenePath;

        // Estado del modal "Guardar como...". openSaveAs es el pedido de abrirlo:
        // ImGui::OpenPopup llamado desde adentro de un BeginMenu queda en otro
        // nivel del ID stack y el BeginPopupModal no lo encuentra, asi que el menu
        // solo prende este flag y el modal se abre afuera.
        // saveAsConfirm guarda la ruta que ya se aviso que existe: el segundo
        // click sobre el mismo nombre es el que pisa el archivo.
        bool        openSaveAs = false;
        std::string saveAsName;
        std::string saveAsConfirm;
        std::string saveAsError;

        bool showUI        = false;
        bool showHierarchy = true;
        bool showInspector = true;
        bool showMaterials = true;
        bool showRender    = true;
        bool showStats     = true;

        // Pedido de reconstruir el layout de paneles desde cero. Lo prende el menu
        // "Ventanas" y lo apaga el editor en el mismo frame en que lo atiende.
        // Sin esto no habria vuelta atras: una vez que arrastraste un panel, tu
        // acomodo queda guardado en imgui.ini y el default no se aplica nunca mas.
        bool resetLayout = false;

        // Autoria de mallas: catalogo de disco + el modelo elegido en el combo.
        AssetCatalog catalog;
        int          modelPick = 0;
        std::string  meshError;

        // Autoria de materiales. activeMaterial es un indice en SceneAssets
        // (kInvalidIndex de SceneAssets = 0xFFFFFFFF cuando no hay ninguno).
        // El draft es la copia que se edita; draftFor recuerda de que indice
        // se cargo, para no recargarlo cada frame y perder la edicion.
        u32 activeMaterial = 0xFFFFFFFFu;
        u32 draftFor       = 0xFFFFFFFFu;
        efengine::renderer::MaterialDef materialDraft;
        std::string materialError;

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
