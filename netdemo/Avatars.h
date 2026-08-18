#pragma once
#include <efenet/PlayerState.h>

#include <efengine/scene/SceneGraph.h>
#include <efengine/scene/NodeHandle.h>
#include <efengine/renderer/Material.h>

#include <unordered_map>
#include <vector>

namespace efengine {
    namespace renderer { class Model; }
}

namespace netdemo {

    // Traduce el estado de red a nodos del SceneGraph. Es el UNICO lugar del
    // proyecto donde la red toca la escena.
    //
    // Tambien es el unico lugar donde el yaw pasa de radianes (toda la capa de
    // red) a grados (math::Transform).
    class Avatars {
        public:
            // 'materials' se copia a cada MeshAttachment: en el motor real
            // MeshAttachment::materials es un MaterialMap por valor, no un
            // puntero. Un mapa vacio deja el material por defecto del renderer.
            Avatars(efengine::scene::SceneGraph& scene,
                    const efengine::renderer::Model* model,
                    efengine::renderer::MaterialMap materials);

            // 'players' son las poses finales a dibujar: la propia ya predicha y
            // las ajenas ya interpoladas. Los nodos de ids que no vinieron se
            // destruyen (membresia implicita).
            void Sync(const std::vector<efenet::PlayerState>& players);

        private:
            efengine::scene::SceneGraph& m_scene;
            const efengine::renderer::Model* m_model;
            efengine::renderer::MaterialMap m_materials;

            std::unordered_map<efenet::PlayerId, efengine::scene::NodeHandle> m_nodes;
    };

}
