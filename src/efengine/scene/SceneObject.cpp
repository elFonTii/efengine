#include <efengine/scene/SceneObject.h>
#include <efengine/core/Assert.h>
#include <utility>

namespace efengine {
namespace scene {

    SceneObject::SceneObject(const renderer::Model* model,
                             renderer::MaterialMap  materials,
                             math::Transform        transform)
        : model(model)
        , transform(transform)
        , materials(std::move(materials)) {
        EF_ASSERT(model != null, "SceneObject::SceneObject: model nulo");
    }

}
}