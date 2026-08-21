#pragma once
#include <efengine/core/Types.h>
#include <efengine/scene/NodeHandle.h>
#include <efengine/math/Transform.h>

namespace efengine {
namespace scene {

    class SceneGraph; 
    class Node;
    
    struct UpdateContext {
        SceneGraph& scene;
        NodeHandle  self;
        Node&       node;
        f32         dt;

        void SetLocal(const math::Transform& t); 
    };

    struct FixedUpdateContext {
        SceneGraph& scene;
        NodeHandle  self;
        Node&       node;
        f32         dt;   // CONSTANTE por contrato: siempre Time::FixedDelta()

        void SetLocal(const math::Transform& t);
    };

    class Behavior {
        public:
            bool enabled = true;

            virtual ~Behavior();
            virtual void OnUpdate(UpdateContext& ctx) = 0;

            // Corre a paso fijo, cero o mas veces por frame segun cuanto tardo el
            // anterior. Default vacio: un behavior que solo anima no tiene por que
            // implementarlo, y ninguno de los que ya existen se entera de que esto
            // aparecio.
            virtual void OnFixedUpdate(FixedUpdateContext& ctx);
    };

}
}
