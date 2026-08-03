#include "efengine/renderer/ShadowMath.h"
#include <glm/gtc/matrix_transform.hpp>
#include <cmath>

namespace efengine {
namespace renderer {

    glm::mat4 ComputeDirectionalLightMatrix(
        const glm::vec3& direction, const glm::vec3& center,
        f32 orthoHalfSize, f32 distance, f32 nearPlane, f32 farPlane) {

        const glm::vec3 dir = glm::normalize(direction);
        const glm::vec3 eye = center - dir * distance;

        const glm::vec3 up = (std::abs(dir.y) > 0.999f)
                           ? glm::vec3(0.0f, 0.0f, 1.0f)
                           : glm::vec3(0.0f, 1.0f, 0.0f);

        const glm::mat4 view = glm::lookAt(eye, center, up);
        const glm::mat4 proj = glm::ortho(-orthoHalfSize, orthoHalfSize,
                                          -orthoHalfSize, orthoHalfSize,
                                          nearPlane, farPlane);
        return proj * view;
    }

    DirectionalLightFit FitDirectionalLight(
        const glm::vec3& direction, const AABB& bounds, f32 padding) {

        // Sin mallas no hay nada que encuadrar. Un metro en el origen mantiene
        // la matriz finita; el pase igual no va a dibujar nada.
        const bool  valida = bounds.Valid();
        const glm::vec3 center = valida ? bounds.Center() : glm::vec3(0.0f);
        // El piso de EPSILON cubre la escena de un solo punto (radio 0), que
        // haria una caja ortografica degenerada.
        const f32   radius = valida ? glm::max(bounds.Radius(), EPSILON) : 1.0f;

        const f32 margen = glm::max(padding, 0.0f);
        const f32 half   = radius + margen;

        // El ojo se para justo sobre la esfera de encuadre: la escena queda
        // entre las profundidades [margen, 2*radius + margen], con 'margen' de
        // aire contra el near y contra el far. De ahi que far == 2*half.
        DirectionalLightFit fit;
        fit.orthoHalfSize = half;
        fit.depthRange    = 2.0f * half;
        fit.matrix        = ComputeDirectionalLightMatrix(
            direction, center, half, half, 0.0f, fit.depthRange);
        return fit;
    }

}
}
