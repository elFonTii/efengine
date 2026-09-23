#pragma once
#include <efengine/core/Types.h>
#include <efengine/gameplay/PhysicsBinding.h>
#include <efengine/math/Transform.h>
#include <efengine/physics/PhysicsWorld.h>
#include <efengine/scene/NodeHandle.h>

#include <memory>
#include <utility>
#include <vector>

namespace efengine {

namespace core  { class Time; }
namespace scene { class SceneGraph; }
namespace physics { class JoltRuntime; }

namespace gameplay {

    // Junta el runtime de Jolt, el mundo, el binding y el pump del frame. El
    // dueno es el cliente (el sandbox), no Application.
    class GameWorld {
        public:
            // nullptr si Jolt no arranca: el cliente sigue andando sin fisica.
            // 'scene' tiene que sobrevivir al GameWorld.
            static std::unique_ptr<GameWorld> Create(scene::SceneGraph& scene,
                                                     const physics::PhysicsWorldDesc& desc = {});
            ~GameWorld();

            GameWorld(const GameWorld&)            = delete;
            GameWorld& operator=(const GameWorld&) = delete;

            bool Simulating() const { return m_simulating; }

            // Guarda el local de cada nodo con collider y arma los cuerpos.
            // No-op si ya esta simulando.
            void BeginSimulation();
            // Destruye los cuerpos y devuelve los locals guardados. No-op si no
            // estaba simulando.
            void EndSimulation();

            // El frame entero: pump de paso fijo, Update variable e
            // interpolacion. Sin simular corre solo FixedUpdate y Update, que es
            // exactamente lo que hacia el cliente antes de que existiera fisica.
            void Tick(core::Time& time);

            const PhysicsBinding& Binding() const { return *m_binding; }

        private:
            explicit GameWorld(scene::SceneGraph& scene);

            void snapshotSubtree(scene::NodeHandle handle);

            scene::SceneGraph& m_scene;

            // El orden importa: se destruyen al reves, y el binding le habla al
            // mundo, que le habla al runtime.
            std::unique_ptr<physics::JoltRuntime>  m_runtime;
            std::unique_ptr<physics::PhysicsWorld> m_world;
            std::unique_ptr<PhysicsBinding>        m_binding;

            std::vector<std::pair<scene::NodeHandle, math::Transform>> m_snapshot;
            bool m_simulating = false;
    };

}
}
