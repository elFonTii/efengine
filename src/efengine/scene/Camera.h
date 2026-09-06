#pragma once

#include <efengine/core/Types.h>
#include <glm/glm.hpp>

namespace efengine {
namespace scene {
    class Camera {
        public:
        glm::mat4 ViewMatrix() const;
        glm::mat4 ProjectionMatrix() const;
        const glm::vec3& Position() const;

        void LookAt(const glm::vec3& position, const glm::vec3& target, const glm::vec3& up = glm::vec3(0.0f, 1.0f, 0.0f));
        void SetAspect(f32 aspect);
        void SetExposure(f32 exposure);
        f32 Exposure() const;

        void SetFov(f32 deg);
        void SetClipPlanes(f32 nearPlane, f32 farPlane);

        // Deriva posicion y orientacion de una matriz de mundo: posicion en la
        // 4a columna, forward = -Z local, up = +Y local. El up sale del eje Y
        // LOCAL y no de +Y del mundo por dos razones: conserva el roll del nodo,
        // y up y forward quedan ortogonales por construccion, asi que un nodo
        // que mira recto para abajo no degenera la matriz de vista.
        //
        // Con un eje en escala cero normalizar daria NaN: en ese caso se mueve
        // la camara pero se conserva la orientacion anterior, y se avisa una vez.
        void SetFromWorld(const glm::mat4& world);

        const glm::vec3& Up() const;
        f32 NearPlane() const;
        f32 FarPlane() const;

        // En GRADOS: es como está guardado, ProjectionMatrix le aplica radians().
        f32 Fov() const;
        const glm::vec3& Target() const;


        private:
            glm::vec3 m_position { 0.0f, 0.0f, 0.0f };
            glm::vec3 m_target   { 0.0f, 0.0f, 0.0f };
            glm::vec3 m_up       { 0.0f, 1.0f, 0.0f };
            f32 m_fov    = 45.0f;
            f32 m_aspect = 1.0f;
            f32 m_near   = 0.1f;
            f32 m_far    = 5000.0f;
            // Tuneada contra la Cornell junto con BloomSettings y la intensidad
            // de IBL de la escena: los tres se leen en la misma imagen.
            f32 m_exposure = 1.025f;
    };
}
}