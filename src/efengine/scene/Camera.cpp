#include "efengine/scene/Camera.h"

#include <glm/gtc/matrix_transform.hpp> 
#include <efengine/core/Assert.h>  

namespace efengine {
namespace scene {

    glm::mat4 Camera::ViewMatrix() const {
        return glm::lookAt(m_position, m_target, m_up);
    }

    glm::mat4 Camera::ProjectionMatrix() const {
        return glm::perspective(glm::radians(m_fov), m_aspect, m_near, m_far); // construye la perspectiva
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

    

}
}

