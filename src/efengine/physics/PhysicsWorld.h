#pragma once
#include <efengine/core/Types.h>
#include <efengine/math/Transform.h>
#include <efengine/physics/BodyHandle.h>
#include <efengine/physics/ShapeDesc.h>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include <memory>

namespace efengine {
namespace physics {

    class JoltRuntime;

    // Sin escala: los cuerpos de Jolt no la tienen, se la come la forma.
    // La rotacion es cuaternion porque la interpolacion del ciclo 3b es un
    // slerp, y con Euler eso da volteretas cerca del gimbal lock.
    struct BodyPose {
        glm::vec3 position { 0.0f };
        glm::quat rotation { 1.0f, 0.0f, 0.0f, 0.0f };   // (w, x, y, z)
    };

    struct PhysicsWorldDesc {
        glm::vec3 gravity { 0.0f, -9.81f, 0.0f };
        u32 maxBodies             = 1024;
        u32 maxBodyPairs          = 1024;
        u32 maxContactConstraints = 1024;
    };

    // Ningun tipo JPH:: se asoma por este header.
    class PhysicsWorld {
        public:
            // 'runtime' tiene que sobrevivir al mundo.
            static std::unique_ptr<PhysicsWorld> Create(JoltRuntime& runtime,
                                                        const PhysicsWorldDesc& desc = {});
            ~PhysicsWorld();

            PhysicsWorld(const PhysicsWorld&)            = delete;
            PhysicsWorld& operator=(const PhysicsWorld&) = delete;
            PhysicsWorld(PhysicsWorld&&)                 = delete;
            PhysicsWorld& operator=(PhysicsWorld&&)      = delete;

            // 'world' es el transform de mundo completo, con escala: el mundo le
            // aplica ScaleShape el mismo. Handle nulo si la forma no se pudo crear.
            BodyHandle CreateBody(const ShapeDesc& shape, const math::Transform& world,
                                  MotionType motion);

            void DestroyBody(BodyHandle handle);

            bool IsValid(BodyHandle handle) const;
            u32  BodyCount() const;

            // dt <= 0 es no-op.
            void Step(f32 fixedDt);

            // Jolt lo exige tras cargar geometria estatica de golpe. Una vez, no
            // por frame.
            void OptimizeBroadPhase();

            // false si el handle no vale; en ese caso 'out' no se toca.
            bool GetBodyPose(BodyHandle handle, BodyPose& out) const;

            void SetBodyPose(BodyHandle handle, const BodyPose& pose);

        private:
            PhysicsWorld();

            struct Impl;
            std::unique_ptr<Impl> m_impl;
    };

}
}
