#include "efengine/gameplay/PhysicsBinding.h"

#include "efengine/core/Log.h"
#include "efengine/gameplay/CollisionMesh.h"
#include "efengine/math/Transform.h"
#include "efengine/renderer/Model.h"
#include "efengine/scene/Node.h"
#include "efengine/scene/SceneGraph.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

namespace efengine {
namespace gameplay {

namespace {
    physics::ShapeKind aPhysics(scene::ShapeKind k) {
        switch (k) {
            case scene::ShapeKind::Sphere:  return physics::ShapeKind::Sphere;
            case scene::ShapeKind::Capsule: return physics::ShapeKind::Capsule;
            case scene::ShapeKind::Mesh:    return physics::ShapeKind::Mesh;
            case scene::ShapeKind::Box:     break;
        }
        return physics::ShapeKind::Box;
    }

    physics::MotionType aPhysics(scene::MotionType m) {
        switch (m) {
            case scene::MotionType::Kinematic: return physics::MotionType::Kinematic;
            case scene::MotionType::Dynamic:   return physics::MotionType::Dynamic;
            case scene::MotionType::Static:    break;
        }
        return physics::MotionType::Static;
    }

    // Cuaternion de un Euler del motor. Sale de Transform::Matrix() a proposito:
    // es la unica definicion del orden Ry * Rx * Rz que hay en el repo, y
    // duplicarla seria otra convencion que mantener sincronizada.
    glm::quat quatDe(const glm::vec3& eulerGrados) {
        math::Transform soloRotacion;
        soloRotacion.rotation = eulerGrados;
        return glm::quat_cast(glm::mat3(soloRotacion.Matrix()));
    }
}

PhysicsBinding::PhysicsBinding(scene::SceneGraph& scene, physics::PhysicsWorld& world)
    : m_scene(scene), m_world(world) {}

void PhysicsBinding::Build() {
    Clear();
    addSubtree(m_scene.Root());

    // Jolt lo exige tras cargar geometria estatica de golpe: sin esto la
    // broadphase queda degenerada y no se nota hasta que la escena crece.
    if (!m_bodies.empty()) m_world.OptimizeBroadPhase();
}

void PhysicsBinding::Clear() {
    for (const Bound& b : m_bodies) m_world.DestroyBody(b.body);
    m_bodies.clear();
    m_byNode.clear();
    m_byBody.clear();
}

void PhysicsBinding::addSubtree(scene::NodeHandle handle) {
    const scene::Node* node = m_scene.TryGet(handle);
    if (node == null) return;

    if (node->collider) addBody(*node);

    // Copia: addBody no toca el grafo, pero recorrer por referencia un vector de
    // hijos mientras se llama a otra cosa es la clase de detalle que se rompe
    // solo cuando alguien agrega una linea.
    const std::vector<scene::NodeHandle> hijos = node->children;
    for (const scene::NodeHandle h : hijos) addSubtree(h);
}

void PhysicsBinding::addBody(const scene::Node& node) {
    const scene::ColliderAttachment& col = *node.collider;

    physics::ShapeDesc desc;
    desc.kind   = aPhysics(col.kind);
    desc.params = col.params;

    // Tiene que seguir viva hasta el CreateBody: ShapeDesc guarda punteros no
    // duenos y Jolt copia la geometria adentro de la forma.
    CollisionMesh geometria;
    if (desc.kind == physics::ShapeKind::Mesh) {
        if (!node.mesh || node.mesh->model == null) {
            EF_LOG_WARNING("PhysicsBinding: el nodo '%s' tiene collider Mesh pero no tiene malla; "
                           "queda sin cuerpo", node.name.c_str());
            return;
        }

        geometria = BuildCollisionMesh(*node.mesh->model);
        desc.meshPositions   = geometria.positions.data();
        desc.meshVertexCount = static_cast<u32>(geometria.positions.size() / 3u);
        desc.meshIndices     = geometria.indices.data();
        desc.meshIndexCount  = static_cast<u32>(geometria.indices.size());
    }

    const glm::mat4 bodyWorld = m_scene.WorldMatrixOf(node.self) * col.localOffset.Matrix();

    const physics::BodyHandle body = m_world.CreateBody(desc, math::DecomposeTRS(bodyWorld),
                                                        aPhysics(col.motion));
    if (body.IsNull()) return;   // physics ya logueo el porque

    Bound b;
    b.node = node.self;
    b.body = body;
    // Una malla siempre termina estatica adentro de physics; guardar el motion
    // pedido haria que el write-back le escriba al nodo su propia pose por frame.
    b.motion = desc.kind == physics::ShapeKind::Mesh ? physics::MotionType::Static
                                                     : aPhysics(col.motion);
    m_world.GetBodyPose(body, b.curr);
    b.prev = b.curr;

    m_byNode[b.node.index] = m_bodies.size();
    m_byBody[b.body.index] = m_bodies.size();
    m_bodies.push_back(b);
}

void PhysicsBinding::removeAt(usize i) {
    m_world.DestroyBody(m_bodies[i].body);
    m_bodies[i] = m_bodies.back();
    m_bodies.pop_back();
    reindex();
}

void PhysicsBinding::reindex() {
    m_byNode.clear();
    m_byBody.clear();
    for (usize i = 0; i < m_bodies.size(); ++i) {
        m_byNode[m_bodies[i].node.index] = i;
        m_byBody[m_bodies[i].body.index] = i;
    }
}

physics::BodyHandle PhysicsBinding::BodyOf(scene::NodeHandle node) const {
    const auto it = m_byNode.find(node.index);
    if (it == m_byNode.end()) return physics::BodyHandle{};

    const Bound& b = m_bodies[it->second];
    if (b.node != node) return physics::BodyHandle{};   // handle viejo
    return b.body;
}

scene::NodeHandle PhysicsBinding::NodeOf(physics::BodyHandle body) const {
    const auto it = m_byBody.find(body.index);
    if (it == m_byBody.end()) return scene::NodeHandle{};

    const Bound& b = m_bodies[it->second];
    if (b.body != body) return scene::NodeHandle{};
    return b.node;
}

u32 PhysicsBinding::BoundCount() const {
    return static_cast<u32>(m_bodies.size());
}

void PhysicsBinding::PushKinematic() {
    for (const Bound& b : m_bodies) {
        if (b.motion != physics::MotionType::Kinematic) continue;

        const scene::Node* node = m_scene.TryGet(b.node);
        if (node == null || !node->collider) continue;

        const glm::mat4 bodyWorld = m_scene.WorldMatrixOf(b.node) * node->collider->localOffset.Matrix();
        const math::Transform t   = math::DecomposeTRS(bodyWorld);

        physics::BodyPose pose;
        pose.position = t.position;
        pose.rotation = quatDe(t.rotation);
        m_world.SetBodyPose(b.body, pose);
    }
}

void PhysicsBinding::WriteBack() {
    usize i = 0;
    while (i < m_bodies.size()) {
        if (!m_scene.IsValid(m_bodies[i].node)) {
            removeAt(i);   // el nodo se destruyo con el mundo andando
            continue;
        }

        Bound& b = m_bodies[i];
        if (b.motion == physics::MotionType::Dynamic) {
            b.prev = b.curr;
            m_world.GetBodyPose(b.body, b.curr);
        }
        ++i;
    }
}

void PhysicsBinding::Interpolate(f32 alpha) {
    for (const Bound& b : m_bodies) {
        if (b.motion != physics::MotionType::Dynamic) continue;

        const scene::Node* node = m_scene.TryGet(b.node);
        if (node == null || !node->collider) continue;

        const glm::vec3 pos = glm::mix(b.prev.position, b.curr.position, alpha);
        const glm::quat rot = glm::slerp(b.prev.rotation, b.curr.rotation, alpha);
        const glm::mat4 bodyWorld = glm::translate(glm::mat4(1.0f), pos) * glm::mat4_cast(rot);

        // Solo la parte rigida del offset: su escala ya se la comio la forma en
        // CreateBody, y volver a aplicarla la contaria dos veces.
        math::Transform offset = node->collider->localOffset;
        offset.scale = glm::vec3(1.0f);

        const glm::mat4 nodeWorld   = bodyWorld * glm::inverse(offset.Matrix());
        const glm::mat4 parentWorld = m_scene.WorldMatrixOf(node->parent);
        const math::Transform t     = math::DecomposeTRS(glm::inverse(parentWorld) * nodeWorld);

        // La escala del nodo no se toca: la pose de un cuerpo es rigida y no
        // trae escala, asi que no hay nada que escribir.
        math::Transform local = node->local;
        local.position = t.position;
        local.rotation = t.rotation;
        m_scene.SetLocalTransform(b.node, local);
    }
}

}
}
