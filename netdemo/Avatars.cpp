#include "Avatars.h"

#include <efengine/scene/Node.h>
#include <efengine/math/Transform.h>
#include <efengine/renderer/Model.h>

#include <string>
#include <utility>

namespace netdemo {

    Avatars::Avatars(efengine::scene::SceneGraph& scene,
                     const efengine::renderer::Model* model,
                     efengine::renderer::MaterialMap materials)
        : m_scene(scene), m_model(model), m_materials(std::move(materials)) {}

    void Avatars::Sync(const std::vector<efenet::PlayerState>& players) {
        using namespace efengine;

        for (const efenet::PlayerState& p : players) {
            auto it = m_nodes.find(p.id);

            if (it == m_nodes.end()) {
                const scene::NodeHandle handle =
                    m_scene.CreateChild(m_scene.Root(), "avatar_" + std::to_string(p.id));

                scene::MeshAttachment mesh;
                mesh.model     = m_model;
                mesh.materials = m_materials;
                m_scene.AttachMesh(handle, mesh);

                it = m_nodes.emplace(p.id, handle).first;
            }

            math::Transform t;
            t.position = p.position;
            // LA UNICA conversion radianes -> grados del proyecto. La capa de
            // red trabaja en radianes porque Step() hace trigonometria;
            // math::Transform guarda euler en grados.
            t.rotation.y = p.yaw * RAD_TO_DEG;
            m_scene.SetLocalTransform(it->second, t);
        }

        // MEMBRESIA IMPLICITA: los que no vinieron en esta lista se fueron.
        for (auto it = m_nodes.begin(); it != m_nodes.end(); ) {
            bool sigue = false;
            for (const efenet::PlayerState& p : players) {
                if (p.id == it->first) { sigue = true; break; }
            }

            if (sigue) { ++it; continue; }

            m_scene.Destroy(it->second);
            it = m_nodes.erase(it);
        }
    }

}
