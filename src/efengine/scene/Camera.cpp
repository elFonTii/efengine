#include "efengine/scene/Camera.h"

#include <glm/gtc/matrix_transform.hpp> 
#include <efengine/core/Assert.h>  

namespace efengine {
namespace scene {

    // stub
    glm::mat4 Camera::ViewMatrix() const       { return glm::mat4(1.0f); }
    glm::mat4 Camera::ProjectionMatrix() const { return glm::mat4(1.0f); }
    void      Camera::LookAt(const glm::vec3&, const glm::vec3&, const glm::vec3&) {}
    void      Camera::SetAspect(f32) {}
    const glm::vec3& Camera::Position() const  { return m_position; }

    

}
}

