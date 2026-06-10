#pragma once

#include <efengine/core/Types.h>
#include <efengine/platform/IEventListener.h>
#include <glm/glm.hpp>

namespace efengine {
namespace scene {
    
    class Camera; // forward dec

    class CameraController : public platform::IEventListener {
        public: 
            explicit CameraController(Camera* cam);

            void OnWindowResize(u32 width, u32 height)             override;
            void OnMouseMove(f32 x, f32 y)                         override;
            void OnMouseButton(i32 button, i32 action, i32 mods)   override;
            void OnMouseScroll(f32 xOffset, f32 yOffset)           override;

        private:
            void UpdateCamera();
            glm::vec3 CalculateCameraPosition(glm::vec3 target, f32 distance, f32 pitch, f32 yaw);
            
             Camera* m_camera;

            glm::vec3 m_target { 0.0f };            // punto que orbita
            f32 m_distance = 2.5f;                  // radio de la órbita

            f32 m_yaw      = 0.0f;
            f32 m_pitch    = glm::radians(45.0f);

            // posiciones del mouse
            f32  m_lastX = 0.0f;
            f32  m_lastY = 0.0f;

            // control rotación
            bool m_rotating = false;
            f32 m_rotateSpeed = 0.005f;
            f32 m_zoomSpeed   = 0.5f;
    };
}
}