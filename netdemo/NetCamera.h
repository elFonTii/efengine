#pragma once
#include <efengine/core/Types.h>
#include <efengine/platform/Input.h>

#include <glm/glm.hpp>

namespace efengine { namespace scene { class Camera; } }

namespace netdemo {

    // Camara de tercera persona del demo de red.
    //
    // scene::CameraController NO sirve acá: es una camara de editor y consume
    // WASD para volar. En el demo WASD tiene que ir al avatar.
    //
    // Es TERCERA persona a proposito, no primera: asi se ve el propio avatar
    // saltar cuando la reconciliacion corrige, que es justamente lo que el PoC
    // quiere hacer visible.
    class NetCamera {
        public:
            explicit NetCamera(efengine::scene::Camera* camera);

            // Solo mouse: yaw y pitch. El teclado no se toca acá.
            void Update(const efengine::platform::Input& input, f32 dt);

            // Reposiciona la camara detras del objetivo y la aplica. Se llama
            // con la posicion PREDICHA del avatar propio, para que la camara
            // responda en el frame igual que el movimiento.
            void Follow(const glm::vec3& target);

            // Radianes. Es la fuente del yaw que viaja en el InputCmd.
            f32 Yaw()   const { return m_yaw; }
            f32 Pitch() const { return m_pitch; }

            void SetMouseEnabled(bool enabled) { m_mouseEnabled = enabled; }

        private:
            efengine::scene::Camera* m_camera;

            f32 m_yaw   = 0.0f;
            f32 m_pitch = -0.35f;   // mirando un poco hacia abajo

            f32 m_distance    = 8.0f;
            f32 m_height      = 2.0f;
            f32 m_sensitivity = 0.005f;   // radianes por pixel

            bool m_mouseEnabled = true;
    };

}
