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


        private:
            glm::vec3 m_position { 0.0f, 0.0f, 0.0f };
            glm::vec3 m_target   { 0.0f, 0.0f, 0.0f };
            glm::vec3 m_up       { 0.0f, 1.0f, 0.0f };
            f32 m_fov    = 45.0f;
            f32 m_aspect = 1.0f;
            f32 m_near   = 0.1f;
            f32 m_far    = 5000.0f;
            f32 m_exposure = 1.0f;
    };
}
}