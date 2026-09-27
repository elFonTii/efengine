#pragma once
#include <efengine/core/Types.h>
#include <efengine/physics/BodyHandle.h>
#include <efengine/physics/PhysicsWorld.h>
#include <efengine/physics/ShapeDesc.h>
#include <efengine/scene/NodeHandle.h>

#include <unordered_map>
#include <vector>

namespace efengine {

namespace scene { class SceneGraph; class Node; }

namespace gameplay {

    // Traduce los ColliderAttachment de la escena a cuerpos, y las poses
    // simuladas de vuelta a los transforms locales de los nodos.
    //
    // No se re-sincroniza solo: un collider agregado o cambiado despues del
    // Build recien tiene efecto en el proximo Build. Lo unico que si atiende, y
    // de forma perezosa, es el nodo que se destruye con el mundo andando.
    class PhysicsBinding {
        public:
            // Las dos referencias tienen que sobrevivir al binding.
            PhysicsBinding(scene::SceneGraph& scene, physics::PhysicsWorld& world);

            PhysicsBinding(const PhysicsBinding&)            = delete;
            PhysicsBinding& operator=(const PhysicsBinding&) = delete;

            // Un cuerpo por nodo con collider, recorriendo desde la raiz. Hace
            // Clear() primero, asi que llamarlo dos veces no duplica nada.
            void Build();
            void Clear();

            void PushKinematic();
            void WriteBack();
            void Interpolate(f32 alpha);

            // Handle nulo si el nodo no tiene cuerpo, o si el handle es viejo.
            physics::BodyHandle BodyOf(scene::NodeHandle node) const;
            scene::NodeHandle   NodeOf(physics::BodyHandle body) const;

            u32 BoundCount() const;

        private:
            struct Bound {
                scene::NodeHandle   node;
                physics::BodyHandle body;
                physics::MotionType motion = physics::MotionType::Static;
                physics::BodyPose   prev;
                physics::BodyPose   curr;
            };

            void addSubtree(scene::NodeHandle handle);
            void addBody(const scene::Node& node);
            void removeAt(usize i);
            void reindex();

            scene::SceneGraph&     m_scene;
            physics::PhysicsWorld& m_world;

            std::vector<Bound> m_bodies;
            // Por INDICE de handle, no por el handle entero: asi no hace falta
            // especializar std::hash. La generacion se verifica contra el Bound.
            std::unordered_map<u32, usize> m_byNode;
            std::unordered_map<u32, usize> m_byBody;
    };

}
}
