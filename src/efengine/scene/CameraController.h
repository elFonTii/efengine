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

            void SetInvertX(bool invert);
            void SetInvertY(bool invert);
            void SetInputEnabled(bool enabled);

        private:
            void UpdateCamera();
            glm::vec3 CalculateCameraPosition(glm::vec3 target, f32 distance, f32 pitch, f32 yaw) const;
            
             Camera* m_camera;

            glm::vec3 m_target { 0.0f, 10.0f, 0.0f };            // punto que orbita
            f32 m_distance = 30.0f;                  // radio de la órbita
            f32 m_minDistance = 5.0f;                   // distancia mínima al target
            f32 m_maxDistance = 100.0f;                 // distancia máxima al target

            f32 m_yaw      = 0.0f;
            f32 m_pitch    = glm::radians(45.0f);

            // posiciones del mouse
            f32  m_lastX = 0.0f;
            f32  m_lastY = 0.0f;

            bool m_invertX = false;
            bool m_invertY = false;

            bool m_inputEnabled = true;   // cuando false, ignora el input de mouse (ej: ImGui tiene el foco)

            // control rotación
            bool m_rotating = false;
            f32 m_rotateSpeed = 0.005f;
            f32 m_zoomSpeed   = 5.0f;
    };
}
}