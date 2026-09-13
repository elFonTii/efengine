#include "efengine/scene/Camera.h"

#include <glm/gtc/matrix_transform.hpp>
#include <efengine/core/Assert.h>
#include <efengine/core/Log.h>

namespace efengine {
namespace scene {

    glm::mat4 Camera::ViewMatrix() const {
        return glm::lookAt(m_position, m_target, m_up);
    }

    glm::mat4 Camera::ProjectionMatrix() const {
        return glm::perspective(glm::radians(m_fov), m_aspect, m_near, m_far);
    }

    void Camera::LookAt(const glm::vec3& position, const glm::vec3& target, const glm::vec3& up) {
        // solo guarda, por decision la matriz se va a calcular lazy.
        m_position = position;
        m_target = target;
        m_up = up;
    }

    void Camera::SetAspect(f32 aspect) {
        EF_ASSERT(aspect > 0.0f, "El aspect ratio debe ser positivo" );
        m_aspect = aspect;
    }

    void Camera::SetExposure(f32 exposure) { 
        EF_ASSERT(exposure >= 0.0f, "La exposición debe ser positiva" );
        m_exposure = exposure;
    }

    f32 Camera::Exposure() const {
        return m_exposure;
    }

    const glm::vec3& Camera::Position() const  { return m_position; }

    f32 Camera::Fov() const { return m_fov; }

    const glm::vec3& Camera::Target() const { return m_target; }

    void Camera::SetFov(f32 deg) { m_fov = deg; }

    void Camera::SetClipPlanes(f32 nearPlane, f32 farPlane) {
        // Un near <= 0 o un far <= near hacen singular la matriz de proyeccion.
        // Se rechazan enteros: aplicar solo uno de los dos dejaria un par
        // incoherente, que es peor que ignorar el pedido.
        if (nearPlane <= 0.0f || farPlane <= nearPlane) {
            EF_LOG_WARNING("Camera::SetClipPlanes: (%.3f, %.3f) invalido, se ignora",
                           nearPlane, farPlane);
            return;
        }
        m_near = nearPlane;
        m_far  = farPlane;
    }

    void Camera::SetFromWorld(const glm::mat4& world) {
        const glm::vec3 position = glm::vec3(world[3]);
        const glm::vec3 yAxis    = glm::vec3(world[1]);
        const glm::vec3 zAxis    = glm::vec3(world[2]);

        constexpr f32 kEpsilon = 1e-12f;   // sobre el largo AL CUADRADO
        if (glm::dot(zAxis, zAxis) < kEpsilon || glm::dot(yAxis, yAxis) < kEpsilon) {
            // Una vez y no por frame: con un nodo mal escalado esto se dispararia
            // 60 veces por segundo y taparia el resto del log.
            static bool avisado = false;
            if (!avisado) {
                EF_LOG_WARNING("Camera::SetFromWorld: matriz con un eje en escala cero, "
                               "se conserva la orientacion anterior");
                avisado = true;
            }
            const glm::vec3 forward = m_target - m_position;
            m_position = position;
            m_target   = position + forward;
            return;
        }

        m_position = position;
        m_up       = glm::normalize(yAxis);
        m_target   = position - glm::normalize(zAxis);
    }

    const glm::vec3& Camera::Up() const { return m_up; }
    f32 Camera::NearPlane() const { return m_near; }
    f32 Camera::FarPlane()  const { return m_far; }

    

}
}

