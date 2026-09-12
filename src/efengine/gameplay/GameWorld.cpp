#include "efengine/gameplay/GameWorld.h"

#include "efengine/core/Log.h"
#include "efengine/core/Time.h"
#include "efengine/physics/JoltRuntime.h"
#include "efengine/scene/Node.h"
#include "efengine/scene/SceneGraph.h"

namespace efengine {
namespace gameplay {

GameWorld::GameWorld(scene::SceneGraph& scene) : m_scene(scene) {}

GameWorld::~GameWorld() = default;

std::unique_ptr<GameWorld> GameWorld::Create(scene::SceneGraph& scene,
                                             const physics::PhysicsWorldDesc& desc) {
    std::unique_ptr<physics::JoltRuntime> runtime = physics::JoltRuntime::Create();
    if (runtime == null) {
        EF_LOG_ERROR("GameWorld::Create: no se pudo inicializar Jolt; el cliente arranca sin fisica");
        return null;
    }

    std::unique_ptr<physics::PhysicsWorld> world = physics::PhysicsWorld::Create(*runtime, desc);
    if (world == null) {
        EF_LOG_ERROR("GameWorld::Create: no se pudo crear el mundo de fisica");
        return null;
    }

    // Constructor privado: make_unique no lo alcanza. Mismo idioma que
    // PhysicsWorld::Create.
    std::unique_ptr<GameWorld> gw(new GameWorld(scene));
    gw->m_runtime = std::move(runtime);
    gw->m_world   = std::move(world);
    gw->m_binding = std::make_unique<PhysicsBinding>(scene, *gw->m_world);
    return gw;
}

void GameWorld::snapshotSubtree(scene::NodeHandle handle) {
    const scene::Node* node = m_scene.TryGet(handle);
    if (node == null) return;

    if (node->collider) m_snapshot.emplace_back(handle, node->local);

    const std::vector<scene::NodeHandle> hijos = node->children;
    for (const scene::NodeHandle h : hijos) snapshotSubtree(h);
}

void GameWorld::BeginSimulation() {
    if (m_simulating) return;

    m_snapshot.clear();
    snapshotSubtree(m_scene.Root());

    m_binding->Build();
    m_simulating = true;
}

void GameWorld::EndSimulation() {
    if (!m_simulating) return;

    m_binding->Clear();

    for (const std::pair<scene::NodeHandle, math::Transform>& guardado : m_snapshot) {
        if (m_scene.IsValid(guardado.first)) {
            m_scene.SetLocalTransform(guardado.first, guardado.second);
        }
    }
    m_snapshot.clear();
    m_simulating = false;
}

void GameWorld::Tick(core::Time& time) {
    const i32 pasos = time.FixedSteps();
    for (i32 i = 0; i < pasos; ++i) {
        m_scene.FixedUpdate(time.FixedDelta());

        if (m_simulating) {
            m_binding->PushKinematic();
            m_world->Step(time.FixedDelta());
            m_binding->WriteBack();
        }
    }

    // Despues del pump, como en Unity: los behaviors variables ven la fisica de
    // este frame y no la del anterior.
    m_scene.Update(time.DeltaTime());

    if (m_simulating) m_binding->Interpolate(time.Alpha());
}

}
}
