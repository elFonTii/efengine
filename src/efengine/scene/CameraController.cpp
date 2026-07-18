#include "CameraController.h"
#include <cmath>

#include <efengine/platform/InputCodes.h>
#include <efengine/core/Types.h>
#include "Camera.h"


namespace efengine {
namespace scene {

    glm::vec3 CameraController::CalculateCameraPosition(glm::vec3 target, f32 distance, f32 pitch, f32 yaw) const {
        glm::vec3 pos;

        pos.x = target.x + distance * std::cos(pitch) * std::sin(yaw);
        pos.y = target.y + distance * std::sin(pitch);
        pos.z = target.z + distance * std::cos(pitch) * std::cos(yaw);

        return pos;
    }

    void CameraController::UpdateCamera() {
        glm::vec3 pos = CalculateCameraPosition(m_target, m_distance, m_pitch, m_yaw);
        m_camera->LookAt(pos, m_target);
    }

    CameraController::CameraController(Camera* cam) {
        m_camera = cam;

        UpdateCamera(); // bootstrap de cámara
    }

    void CameraController::OnMouseButton(i32 button, i32 action, i32 mods) {
        if(!m_inputEnabled) return;
        if(button == (i32)platform::MouseButton::Left) {
             m_rotating = (action == (i32)platform::InputAction::Press);
        }
    }

    void CameraController::OnMouseMove(f32 x, f32 y) {
        if(!m_inputEnabled) return;
        if(m_rotating) {
            f32 signX = m_invertX ? -1.0f : 1.0f;
            f32 signY = m_invertY ? -1.0f : 1.0f;

            f32 dirX = x - m_lastX;
            f32 dirY = y - m_lastY;
            
            m_yaw -= dirX * m_rotateSpeed * signX;
            m_pitch += dirY * m_rotateSpeed * signY;

            f32 limit = glm::radians(89.0f);
            m_pitch = glm::clamp(m_pitch, -limit, limit);

            UpdateCamera();
        }
        m_lastX = x;
        m_lastY = y;
    }

    void CameraController::OnMouseScroll(f32 xOffset, f32 yOffset) {
        if(!m_inputEnabled) return;
        m_distance -= yOffset * m_zoomSpeed; // controlar distancia hasta cubo
        m_distance = glm::clamp(m_distance, m_minDistance, m_maxDistance); // limite

        UpdateCamera();
    }

    void CameraController::OnWindowResize(u32 width, u32 height) {
        if(height > 0) {
            m_camera->SetAspect((f32)width / (f32)height);
        }
    }

    void CameraController::SetInvertX(bool invert) { 
        m_invertX = invert;
    }

    void CameraController::SetInvertY(bool invert) {
        m_invertY = invert;
    }

    void CameraController::SetInputEnabled(bool enabled) { m_inputEnabled = enabled; }
}
}