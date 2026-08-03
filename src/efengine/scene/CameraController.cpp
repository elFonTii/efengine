#include "CameraController.h"

#include "Camera.h"

#include <algorithm>
#include <cmath>

namespace efengine {
namespace scene {

    namespace {
        // Con el pitch clampeado acá la línea de vista nunca queda paralela al
        // up del mundo, así que glm::lookAt resuelve la orientación sin roll y
        // sin casos degenerados.
        constexpr f32 kMaxPitch = 1.55334306f;   // radians(89)

        const glm::vec3 kWorldUp { 0.0f, 1.0f, 0.0f };

        f32 ClampPitch(f32 pitch) {
            return std::max(-kMaxPitch, std::min(kMaxPitch, pitch));
        }
    }

    CameraController::CameraController(Camera* cam) : m_camera(cam) {
        Apply();
    }

    glm::vec3 CameraController::Forward() const {
        const f32 cp = std::cos(m_pitch);
        return glm::vec3(std::sin(m_yaw) * cp,
                         std::sin(m_pitch),
                        -std::cos(m_yaw) * cp);
    }

    glm::vec3 CameraController::Right() const {
        return glm::normalize(glm::cross(Forward(), kWorldUp));
    }

    glm::vec3 CameraController::Up() const {
        return glm::cross(Right(), Forward());
    }

    glm::vec3 CameraController::Pivot() const {
        return m_position + Forward() * m_pivotDistance;
    }

    void CameraController::Apply() {
        m_camera->LookAt(m_position, m_position + Forward());
    }

    void CameraController::Update(const platform::Input& input, f32 dt) {
        using platform::Key;
        using platform::MouseButton;

        // --- 0. Tab alterna el mouselook sostenido. Va por el gate de teclado
        // para que escribir en un campo de ImGui no lo dispare.
        if (m_keyboardEnabled && input.WasPressed(Key::Tab)) {
            m_lookToggled = !m_lookToggled;
        }

        // --- 1. Modo de drag: uno solo por frame, con esta precedencia. El
        // mouselook sostenido va último: orbitar y panear lo pisan.
        m_prevDragMode = m_dragMode;
        m_dragMode = DragMode::None;
        if (input.IsMouseDown(MouseButton::Left) && input.IsDown(Key::LeftAlt)) {
            m_dragMode = DragMode::Orbit;
        } else if (input.IsMouseDown(MouseButton::Right)) {
            m_dragMode = DragMode::Look;
        } else if (input.IsMouseDown(MouseButton::Middle)) {
            m_dragMode = DragMode::Pan;
        } else if (m_lookToggled) {
            m_dragMode = DragMode::Look;
        }

        if (!m_mouseEnabled) {
            m_dragMode = DragMode::None;
        }

        if (m_dragMode != DragMode::None) {
            // --- 2. El frame en que ARRANCA un modo no mueve nada: al capturar
            // el cursor GLFW lo reubica y el delta de ese frame es basura.
            // Comparar contra el modo anterior cubre de una el click inicial de
            // un drag, el Tab que prende el mouselook, y volver a mirar después
            // de soltar una órbita.
            const glm::vec2 delta = (m_dragMode != m_prevDragMode)
                                  ? glm::vec2(0.0f)
                                  : input.MouseDelta();

            if (m_dragMode == DragMode::Pan) {
                // --- 3. Panear: el mundo sigue al cursor, de ahí los signos.
                // Escala con la distancia al pivote para que panear de lejos
                // recorra más mundo por pixel.
                m_position += (-delta.x * Right() + delta.y * Up())
                            * m_settings.panSpeed * m_pivotDistance;
            } else {
                // --- 4. Mirar y orbitar comparten los mismos ángulos; sólo
                // cambia alrededor de qué punto giran.
                const f32 dYaw   =  delta.x * m_settings.lookSensitivity
                                 * (m_settings.invertX ? -1.0f : 1.0f);
                // En pantalla la y crece hacia abajo, pero subir el mouse
                // tiene que mirar hacia arriba.
                const f32 dPitch = -delta.y * m_settings.lookSensitivity
                                 * (m_settings.invertY ? -1.0f : 1.0f);

                if (m_dragMode == DragMode::Orbit) {
                    // El pivote se toma ANTES de tocar los ángulos y la
                    // posición se reconstruye desde él: así la distancia se
                    // conserva por construcción, sin normalizar nada.
                    const glm::vec3 pivot = Pivot();
                    m_yaw   += dYaw;
                    m_pitch  = ClampPitch(m_pitch + dPitch);
                    m_position = pivot - Forward() * m_pivotDistance;
                } else {
                    m_yaw   += dYaw;
                    m_pitch  = ClampPitch(m_pitch + dPitch);
                }
            }
        }

        // --- 5. Dolly: no pide drag, sólo que el mouse no lo tenga la UI.
        if (m_mouseEnabled) {
            const f32 scroll = input.ScrollDelta();
            if (scroll != 0.0f) {
                f32 avance = m_pivotDistance * m_settings.dollySpeed * scroll;
                const f32 nueva = m_pivotDistance - avance;
                if (nueva < m_settings.minPivotDistance) {
                    // Recortar el avance con la misma cantidad que el clamp, o
                    // el pivote se despega del punto que se está mirando.
                    avance -= (m_settings.minPivotDistance - nueva);
                    m_pivotDistance = m_settings.minPivotDistance;
                } else {
                    m_pivotDistance = nueva;
                }
                m_position += Forward() * avance;
            }
        }

        // --- 6. Vuelo.
        if (m_keyboardEnabled) {
            glm::vec3 dir { 0.0f };
            if (input.IsDown(Key::W)) dir += Forward();
            if (input.IsDown(Key::S)) dir -= Forward();
            if (input.IsDown(Key::D)) dir += Right();
            if (input.IsDown(Key::A)) dir -= Right();
            if (input.IsDown(Key::E)) dir += kWorldUp;   // Y del MUNDO, no el up de la cámara
            if (input.IsDown(Key::Q)) dir -= kWorldUp;

            if (dir != glm::vec3(0.0f)) {
                f32 paso = m_settings.moveSpeed * dt;
                if (input.IsDown(Key::LeftShift)) {
                    paso *= m_settings.boostMultiplier;
                }
                // Un solo producto por el paso: la diagonal no acelera.
                m_position += glm::normalize(dir) * paso;
            }
        }

        Apply();
    }

    void CameraController::Focus(const glm::vec3& center, f32 radius) {
        // Distancia a la que una esfera de 'radius' entra justo en el fov.
        const f32 r         = std::max(radius, 0.001f);
        const f32 halfFov   = glm::radians(m_camera->Fov()) * 0.5f;
        const f32 distancia = std::max((r / std::sin(halfFov)) * m_settings.focusMargin,
                                       m_settings.minPivotDistance);

        m_pivotDistance = distancia;
        m_position      = center - Forward() * distancia;
        Apply();
    }

    void CameraController::Focus(const glm::vec3& center) {
        m_position = center - Forward() * m_pivotDistance;
        Apply();
    }

    void CameraController::SetPose(const glm::vec3& position, f32 yawRad, f32 pitchRad) {
        m_position = position;
        m_yaw      = yawRad;
        m_pitch    = ClampPitch(pitchRad);
        Apply();
    }
}
}
