#pragma once
#include <efengine/core/Types.h>
#include <efengine/scene/Behavior.h>

#include <glm/glm.hpp>

namespace sandbox {

    // Vaiven senoidal para probar las velocidades de TAA. No esta registrado:
    // al guardar el .efe el serializador lo saltea con un warning.
    class OscillatorBehavior : public efengine::scene::Behavior {
        public:
            OscillatorBehavior(const glm::vec3& axis, f32 amplitude, f32 frequency);

            void OnUpdate(efengine::scene::UpdateContext& ctx) override;

            bool             HasBase() const { return m_hasBase; }
            const glm::vec3& Base() const    { return m_base; }

            glm::vec3 axis;
            f32       amplitude;
            f32       frequency;

        private:
            glm::vec3 m_base    { 0.0f };
            bool      m_hasBase = false;
            f32       m_t       = 0.0f;
    };

}
