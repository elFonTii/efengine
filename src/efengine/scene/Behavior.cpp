#include "efengine/scene/Behavior.h"
#include <efengine/scene/SceneGraph.h>

namespace efengine {
namespace scene {

    Behavior::~Behavior() = default;

    void UpdateContext::SetLocal(const math::Transform& t) {
        scene.SetLocalTransform(self, t);
    }

}
}
