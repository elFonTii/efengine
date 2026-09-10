#include <efengine/physics/PhysicsWorld.h>

#include <efengine/core/Log.h>
#include <efengine/physics/JoltRuntime.h>

#include <Jolt/Jolt.h>
#include <Jolt/Core/JobSystemSingleThreaded.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <Jolt/Physics/Collision/Shape/MeshShape.h>
#include <Jolt/Physics/Collision/Shape/ScaledShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/PhysicsSettings.h>
#include <Jolt/Physics/PhysicsSystem.h>

#include <algorithm>
#include <vector>

namespace efengine {
namespace physics {

namespace {

    // Dos capas: lo estatico no colisiona contra lo estatico, todo lo demas si.
    namespace Capas {
        static constexpr JPH::ObjectLayer kQuieto  = 0;
        static constexpr JPH::ObjectLayer kMovil   = 1;
        static constexpr JPH::ObjectLayer kCuantas = 2;
    }

    namespace CapasBroad {
        static constexpr JPH::BroadPhaseLayer kQuieto(0);
        static constexpr JPH::BroadPhaseLayer kMovil(1);
        static constexpr JPH::uint            kCuantas = 2;
    }

    class InterfazCapasBroad final : public JPH::BroadPhaseLayerInterface {
        public:
            JPH::uint GetNumBroadPhaseLayers() const override { return CapasBroad::kCuantas; }

            JPH::BroadPhaseLayer GetBroadPhaseLayer(JPH::ObjectLayer capa) const override {
                return capa == Capas::kQuieto ? CapasBroad::kQuieto : CapasBroad::kMovil;
            }

#if defined(JPH_EXTERNAL_PROFILE) || defined(JPH_PROFILE_ENABLED)
            const char* GetBroadPhaseLayerName(JPH::BroadPhaseLayer capa) const override {
                return capa == CapasBroad::kQuieto ? "QUIETO" : "MOVIL";
            }
#endif
    };

    class FiltroObjetoVsBroad final : public JPH::ObjectVsBroadPhaseLayerFilter {
        public:
            bool ShouldCollide(JPH::ObjectLayer objeto, JPH::BroadPhaseLayer broad) const override {
                if (objeto == Capas::kQuieto) return broad == CapasBroad::kMovil;
                return true;
            }
    };

    class FiltroParDeCapas final : public JPH::ObjectLayerPairFilter {
        public:
            bool ShouldCollide(JPH::ObjectLayer a, JPH::ObjectLayer b) const override {
                if (a == Capas::kQuieto) return b == Capas::kMovil;
                return true;
            }
    };

    JPH::EMotionType aJolt(MotionType motion) {
        switch (motion) {
            case MotionType::Kinematic: return JPH::EMotionType::Kinematic;
            case MotionType::Dynamic:   return JPH::EMotionType::Dynamic;
            case MotionType::Static:
            default:                    return JPH::EMotionType::Static;
        }
    }

    JPH::ObjectLayer capaDe(MotionType motion) {
        return motion == MotionType::Static ? Capas::kQuieto : Capas::kMovil;
    }

    // Tiene que seguir el orden de math::Transform::Matrix(): Ry * Rx * Rz.
    JPH::Quat quatDeEuler(const glm::vec3& eulerGrados) {
        const JPH::Quat y = JPH::Quat::sRotation(JPH::Vec3::sAxisY(),
                                                 JPH::DegreesToRadians(eulerGrados.y));
        const JPH::Quat x = JPH::Quat::sRotation(JPH::Vec3::sAxisX(),
                                                 JPH::DegreesToRadians(eulerGrados.x));
        const JPH::Quat z = JPH::Quat::sRotation(JPH::Vec3::sAxisZ(),
                                                 JPH::DegreesToRadians(eulerGrados.z));
        return y * x * z;
    }

    JPH::RefConst<JPH::Shape> construirForma(const ScaledShape& s) {
        JPH::ShapeSettings::ShapeResult resultado;

        switch (s.desc.kind) {
            case ShapeKind::Box: {
                const JPH::Vec3 mitades(s.desc.params.x, s.desc.params.y, s.desc.params.z);
                // El convex radius no puede pasar la mitad de la extension mas
                // chica: si la pasa, Jolt rechaza la caja.
                const f32 menor = std::min(s.desc.params.x,
                                           std::min(s.desc.params.y, s.desc.params.z));
                const f32 radio = std::min(JPH::cDefaultConvexRadius, menor * 0.5f);
                resultado = JPH::BoxShapeSettings(mitades, radio).Create();
                break;
            }

            case ShapeKind::Sphere:
                resultado = JPH::SphereShapeSettings(s.desc.params.x).Create();
                break;

            case ShapeKind::Capsule:
                // Jolt toma (medio alto, radio), en ese orden.
                resultado = JPH::CapsuleShapeSettings(s.desc.params.y, s.desc.params.x).Create();
                break;

            case ShapeKind::Mesh: {
                const ShapeDesc& d = s.desc;

                if (d.meshPositions == nullptr || d.meshVertexCount == 0
                    || d.meshIndices == nullptr || d.meshIndexCount < 3
                    || (d.meshIndexCount % 3) != 0) {
                    EF_LOG_ERROR("PhysicsWorld: collider Mesh sin geometria valida "
                                 "(%u vertices, %u indices)",
                                 d.meshVertexCount, d.meshIndexCount);
                    return nullptr;
                }

                JPH::VertexList vertices;
                vertices.reserve(d.meshVertexCount);
                for (u32 i = 0; i < d.meshVertexCount; ++i) {
                    const f32* p = d.meshPositions + static_cast<usize>(i) * 3u;
                    vertices.push_back(JPH::Float3(p[0], p[1], p[2]));
                }

                JPH::IndexedTriangleList triangulos;
                triangulos.reserve(d.meshIndexCount / 3u);
                for (u32 i = 0; i < d.meshIndexCount; i += 3u) {
                    const u32 a = d.meshIndices[i];
                    const u32 b = d.meshIndices[i + 1u];
                    const u32 c = d.meshIndices[i + 2u];

                    // Un indice fuera de rango adentro de Jolt es un acceso a
                    // memoria ajena, no un error que devuelva algo.
                    if (a >= d.meshVertexCount || b >= d.meshVertexCount
                        || c >= d.meshVertexCount) {
                        EF_LOG_ERROR("PhysicsWorld: collider Mesh con el indice %u fuera de "
                                     "rango (%u vertices)",
                                     std::max(a, std::max(b, c)), d.meshVertexCount);
                        return nullptr;
                    }

                    triangulos.push_back(JPH::IndexedTriangle(a, b, c, 0));
                }

                resultado = JPH::MeshShapeSettings(vertices, triangulos).Create();
                break;
            }
        }

        if (resultado.HasError()) {
            EF_LOG_ERROR("PhysicsWorld: Jolt rechazo la forma: %s", resultado.GetError().c_str());
            return nullptr;
        }

        JPH::RefConst<JPH::Shape> forma = resultado.Get();

        // Solo la malla llega con escala sin absorber.
        if (s.residualScale != glm::vec3(1.0f)) {
            const JPH::ShapeSettings::ShapeResult escalada =
                JPH::ScaledShapeSettings(forma, JPH::Vec3(s.residualScale.x,
                                                          s.residualScale.y,
                                                          s.residualScale.z)).Create();
            if (escalada.HasError()) {
                EF_LOG_ERROR("PhysicsWorld: Jolt rechazo el escalado de la forma: %s",
                             escalada.GetError().c_str());
                return nullptr;
            }
            forma = escalada.Get();
        }

        return forma;
    }
}

struct PhysicsWorld::Impl {
    struct Slot {
        JPH::BodyID id;
        u32         generation = 0;
        bool        alive      = false;
    };

    // El orden de declaracion importa: PhysicsSystem guarda referencias a los
    // tres filtros, tienen que construirse antes y morir despues.
    InterfazCapasBroad  capasBroad;
    FiltroObjetoVsBroad filtroObjetoVsBroad;
    FiltroParDeCapas    filtroParDeCapas;

    std::unique_ptr<JPH::TempAllocatorImpl>       tempAllocator;
    std::unique_ptr<JPH::JobSystemSingleThreaded> jobSystem;
    JPH::PhysicsSystem                            system;

    std::vector<Slot> slots;
    std::vector<u32>  libres;
    u32               vivos = 0;

    Slot* resolver(BodyHandle h) {
        if (h.IsNull() || h.index >= slots.size()) return nullptr;
        Slot& s = slots[h.index];
        if (!s.alive || s.generation != h.generation) return nullptr;
        return &s;
    }

    const Slot* resolver(BodyHandle h) const {
        return const_cast<Impl*>(this)->resolver(h);
    }

    BodyHandle ocupar(JPH::BodyID id) {
        u32 idx;
        if (!libres.empty()) {
            idx = libres.back();
            libres.pop_back();
        } else {
            idx = static_cast<u32>(slots.size());
            slots.push_back(Slot{});
        }

        Slot& s = slots[idx];
        s.id    = id;
        s.alive = true;
        ++s.generation;
        if (s.generation == 0) s.generation = 1;   // 0 esta reservado

        ++vivos;
        return BodyHandle{ idx, s.generation };
    }
};

PhysicsWorld::PhysicsWorld() : m_impl(std::make_unique<Impl>()) {}

std::unique_ptr<PhysicsWorld> PhysicsWorld::Create(JoltRuntime& runtime,
                                                   const PhysicsWorldDesc& desc) {
    // No se guarda: pedirlo por referencia es lo que vuelve imposible construir
    // un mundo sin runtime vivo.
    (void)runtime;

    if (desc.maxBodies == 0) {
        EF_LOG_ERROR("PhysicsWorld::Create: maxBodies en 0");
        return nullptr;
    }

    std::unique_ptr<PhysicsWorld> mundo(new PhysicsWorld());
    Impl& impl = *mundo->m_impl;

    impl.tempAllocator = std::make_unique<JPH::TempAllocatorImpl>(10 * 1024 * 1024);
    // Single-threaded a proposito: a esta escala un pool no compra nada y trae
    // no-determinismo.
    impl.jobSystem = std::make_unique<JPH::JobSystemSingleThreaded>(JPH::cMaxPhysicsJobs);

    impl.system.Init(desc.maxBodies,
                     0,
                     desc.maxBodyPairs,
                     desc.maxContactConstraints,
                     impl.capasBroad,
                     impl.filtroObjetoVsBroad,
                     impl.filtroParDeCapas);

    impl.system.SetGravity(JPH::Vec3(desc.gravity.x, desc.gravity.y, desc.gravity.z));

    EF_LOG_INFO("PhysicsWorld: mundo listo (max %u cuerpos, gravedad %.2f)",
                desc.maxBodies, static_cast<double>(desc.gravity.y));
    return mundo;
}

PhysicsWorld::~PhysicsWorld() {
    // Los cuerpos se sacan antes de que muera el sistema: dejarlos adentro es un
    // assert de Jolt en Debug y una fuga en Release.
    JPH::BodyInterface& bi = m_impl->system.GetBodyInterface();
    for (Impl::Slot& s : m_impl->slots) {
        if (!s.alive) continue;
        bi.RemoveBody(s.id);
        bi.DestroyBody(s.id);
        s.alive = false;
    }
}

BodyHandle PhysicsWorld::CreateBody(const ShapeDesc& shape, const math::Transform& world,
                                    MotionType motion) {
    const ScaledShape escalada = ScaleShape(shape, world.scale);

    if (escalada.degenerate) {
        EF_LOG_WARNING("PhysicsWorld::CreateBody: forma degenerada (escala %.3f, %.3f, %.3f); "
                       "el nodo queda sin cuerpo",
                       static_cast<double>(world.scale.x),
                       static_cast<double>(world.scale.y),
                       static_cast<double>(world.scale.z));
        return BodyHandle{};
    }

    if (escalada.approximated) {
        EF_LOG_WARNING("PhysicsWorld::CreateBody: escala no uniforme (%.3f, %.3f, %.3f) en una "
                       "forma que no la admite; se aproxima al eje dominante",
                       static_cast<double>(world.scale.x),
                       static_cast<double>(world.scale.y),
                       static_cast<double>(world.scale.z));
    }

    const JPH::RefConst<JPH::Shape> forma = construirForma(escalada);
    if (forma == nullptr) return BodyHandle{};

    // MeshShape de Jolt no puede tener masa: pedirle Kinematic o Dynamic es un
    // assert adentro de la libreria.
    MotionType motionEfectivo = motion;
    if (shape.kind == ShapeKind::Mesh && motion != MotionType::Static) {
        EF_LOG_WARNING("PhysicsWorld::CreateBody: un collider Mesh no puede ser no-estatico; "
                       "se crea estatico");
        motionEfectivo = MotionType::Static;
    }

    JPH::BodyCreationSettings settings(forma,
                                       JPH::RVec3(world.position.x, world.position.y,
                                                  world.position.z),
                                       quatDeEuler(world.rotation),
                                       aJolt(motionEfectivo),
                                       capaDe(motionEfectivo));

    JPH::BodyInterface& bi = m_impl->system.GetBodyInterface();
    const JPH::BodyID id = bi.CreateAndAddBody(
        settings,
        motionEfectivo == MotionType::Static ? JPH::EActivation::DontActivate
                                             : JPH::EActivation::Activate);

    if (id.IsInvalid()) {
        EF_LOG_ERROR("PhysicsWorld::CreateBody: Jolt rechazo el cuerpo (se lleno maxBodies?)");
        return BodyHandle{};
    }

    return m_impl->ocupar(id);
}

void PhysicsWorld::DestroyBody(BodyHandle handle) {
    Impl::Slot* s = m_impl->resolver(handle);
    if (s == nullptr) return;

    JPH::BodyInterface& bi = m_impl->system.GetBodyInterface();
    bi.RemoveBody(s->id);
    bi.DestroyBody(s->id);

    s->alive = false;
    m_impl->libres.push_back(handle.index);
    --m_impl->vivos;
}

bool PhysicsWorld::IsValid(BodyHandle handle) const {
    return m_impl->resolver(handle) != nullptr;
}

u32 PhysicsWorld::BodyCount() const {
    return m_impl->vivos;
}

void PhysicsWorld::Step(f32 fixedDt) {
    if (fixedDt <= 0.0f) return;
    m_impl->system.Update(fixedDt, 1, m_impl->tempAllocator.get(), m_impl->jobSystem.get());
}

void PhysicsWorld::OptimizeBroadPhase() {
    m_impl->system.OptimizeBroadPhase();
}

bool PhysicsWorld::GetBodyPose(BodyHandle handle, BodyPose& out) const {
    const Impl::Slot* s = m_impl->resolver(handle);
    if (s == nullptr) return false;

    JPH::RVec3 posicion;
    JPH::Quat  rotacion;
    m_impl->system.GetBodyInterface().GetPositionAndRotation(s->id, posicion, rotacion);

    out.position = glm::vec3(posicion.GetX(), posicion.GetY(), posicion.GetZ());
    out.rotation = glm::quat(rotacion.GetW(), rotacion.GetX(),
                             rotacion.GetY(), rotacion.GetZ());
    return true;
}

void PhysicsWorld::SetBodyPose(BodyHandle handle, const BodyPose& pose) {
    const Impl::Slot* s = m_impl->resolver(handle);
    if (s == nullptr) return;

    m_impl->system.GetBodyInterface().SetPositionAndRotation(
        s->id,
        JPH::RVec3(pose.position.x, pose.position.y, pose.position.z),
        JPH::Quat(pose.rotation.x, pose.rotation.y, pose.rotation.z, pose.rotation.w),
        JPH::EActivation::Activate);
}

}
}
