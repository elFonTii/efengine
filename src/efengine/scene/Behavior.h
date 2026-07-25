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

    class Behavior {
        public:
            bool enabled = true;

            virtual ~Behavior();
            virtual void OnUpdate(UpdateContext& ctx) = 0;
    };

}
}
