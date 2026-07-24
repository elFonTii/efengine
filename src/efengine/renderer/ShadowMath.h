#pragma once
#include <efengine/core/Types.h>
#include <glm/glm.hpp>

namespace efengine {
namespace renderer {

    // Matriz light-space de una luz direccional: ortho * lookAt.
    // 'center' es el punto que la caja ortográfica encuadra (origen de escena
    // para una cascada). 'distance' es cuán atrás se ubica el ojo de la luz a lo
    // largo de -direction. Función pura: sin estado ni llamadas GL.
    glm::mat4 ComputeDirectionalLightMatrix(
        const glm::vec3& direction,
        const glm::vec3& center,
        f32 orthoHalfSize,
        f32 distance,
        f32 nearPlane,
        f32 farPlane);

}
}
