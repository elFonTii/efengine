#pragma once

#include <efengine/core/Types.h>
#include <efengine/platform/Input.h>

#include <glm/glm.hpp>

namespace efengine {
namespace scene {

    class Camera;

    struct CameraSettings {
        f32  moveSpeed        = 20.0f;    // unidades/segundo
        f32  boostMultiplier  = 4.0f;
        f32  lookSensitivity  = 0.005f;   // radianes por pixel
        f32  panSpeed         = 0.001f;   // fraccion de la distancia al pivote, por pixel
        f32  dollySpeed       = 0.15f;    // fraccion de la distancia al pivote, por notch
        f32  minPivotDistance = 0.5f;
        f32  focusMargin      = 1.2f;
        bool invertX = false;
        bool invertY = false;
    };

    // Camara hibrida de editor: volar (WASD/QE), mirar (RMB), orbitar
    // (Alt+LMB), panear (MMB), dolly (scroll) y encuadrar (Focus).
    //
    // El estado son cuatro numeros: posicion, yaw, pitch y distancia al pivote.
    // El PIVOTE ES DERIVADO, no guardado: siempre es el punto a
    // m_pivotDistance sobre la linea de vista. Guardarlo obligaria a
    // mantenerlo sincronizado con la posicion en cada modo, y ahi es donde
    // este tipo de camara se rompe.
    //
    // No depende de la plataforma: no captura el cursor, solo declara si lo
    // quiere capturado (WantsCursorCaptured) y el loop lo aplica.
    class CameraController {
        public:
            explicit CameraController(Camera* cam);

            void Update(const platform::Input& input, f32 dt);

            // Encuadra una esfera de radio 'radius' alrededor de 'center'.
            // No toca yaw ni pitch: encuadrar no es reorientar.
            void Focus(const glm::vec3& center, f32 radius);

            // Sobrecarga para lo que no tiene AABB (una luz, un nodo pelado):
            // centra conservando la distancia actual.
            void Focus(const glm::vec3& center);

            void SetPose(const glm::vec3& position, f32 yawRad, f32 pitchRad);

            bool WantsCursorCaptured() const { return m_dragMode != DragMode::None; }

            // Mouselook sostenido: se mira sin mantener ningun boton. Lo
            // alterna Tab. Orbitar y panear lo pisan mientras duran.
            bool LookToggled() const { return m_lookToggled; }
            void SetLookToggled(bool on) { m_lookToggled = on; }

            void SetMouseEnabled(bool enabled)    { m_mouseEnabled = enabled; }
            void SetKeyboardEnabled(bool enabled) { m_keyboardEnabled = enabled; }

            CameraSettings&       settings()       { return m_settings; }
            const CameraSettings& settings() const { return m_settings; }

            const glm::vec3& Position() const { return m_position; }
            glm::vec3 Forward() const;
            glm::vec3 Pivot() const;
            f32 Yaw() const           { return m_yaw; }
            f32 Pitch() const         { return m_pitch; }
            f32 PivotDistance() const { return m_pivotDistance; }

        private:
            enum class DragMode { None, Look, Orbit, Pan };

            glm::vec3 Right() const;
            glm::vec3 Up() const;
            void      Apply();

            Camera* m_camera;

            glm::vec3 m_position { 0.0f, 31.213f, 21.213f };
            f32       m_yaw   = 0.0f;
            f32       m_pitch = glm::radians(-45.0f);
            f32       m_pivotDistance = 30.0f;

            DragMode m_dragMode = DragMode::None;
            // El modo del frame anterior: cuando cambia, el delta de mouse se
            // descarta. Cubre tanto el click inicial de un drag como el frame
            // en que se prende el mouselook.
            DragMode m_prevDragMode = DragMode::None;
            bool     m_lookToggled  = false;

            bool m_mouseEnabled    = true;
            bool m_keyboardEnabled = true;

            CameraSettings m_settings;
    };
}
}
