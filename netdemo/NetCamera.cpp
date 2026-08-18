#include "NetCamera.h"

#include <efengine/scene/Camera.h>

#include <cmath>

namespace netdemo {

namespace {
    // Tope del pitch para que la camara no se de vuelta al mirar arriba o abajo.
    constexpr f32 kPitchLimit = 1.4f;   // ~80 grados
}

    NetCamera::NetCamera(efengine::scene::Camera* camera)
        : m_camera(camera) {}

    void NetCamera::Update(const efengine::platform::Input& input, f32 dt) {
        (void)dt;   // el mouse ya viene en pixeles por frame, no por segundo
        if (!m_mouseEnabled) return;

        const glm::vec2 delta = input.MouseDelta();
        m_yaw   -= delta.x * m_sensitivity;
        m_pitch -= delta.y * m_sensitivity;

        if (m_pitch >  kPitchLimit) m_pitch =  kPitchLimit;
        if (m_pitch < -kPitchLimit) m_pitch = -kPitchLimit;
    }

    void NetCamera::Follow(const glm::vec3& target) {
        if (m_camera == null) return;

        // Direccion hacia la que mira el avatar (misma convencion que Step():
        // forward es -Z con yaw 0).
        const f32 s = std::sin(m_yaw);
        const f32 c = std::cos(m_yaw);
        const glm::vec3 forward(s, 0.0f, -c);

        const glm::vec3 foco = target + glm::vec3(0.0f, m_height, 0.0f);

        // La camara va detras y arriba, inclinada segun el pitch.
        const f32 alturaExtra = -std::sin(m_pitch) * m_distance;
        const glm::vec3 pos = foco - forward * (m_distance * std::cos(m_pitch))
                                   + glm::vec3(0.0f, alturaExtra, 0.0f);

        // scene::Camera no tiene SetPosition: la posicion y el objetivo se fijan
        // juntos con LookAt(pos, target, up).
        m_camera->LookAt(pos, foco);
    }

}
