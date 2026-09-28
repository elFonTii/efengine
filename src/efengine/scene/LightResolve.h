#pragma once
#include <efengine/scene/Node.h>
#include <efengine/renderer/Light.h>

#include <glm/glm.hpp>

namespace efengine {
namespace scene {

    // Tinte x temperatura x intensidad, con negativos a cero.
    glm::vec3 EffectiveLightColor(const LightAttachment& a);

    // Sanea aca y no en el editor: un .efe o un behavior pueden escribir
    // cualquier cosa. primarySun lo decide SceneGraph.
    renderer::Light ResolveLight(const LightAttachment& a, const glm::mat4& world);

}
}
