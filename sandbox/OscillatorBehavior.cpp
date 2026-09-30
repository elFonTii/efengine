#include "OscillatorBehavior.h"

#include <efengine/math/Transform.h>
#include <efengine/scene/Node.h>

#include <glm/gtc/constants.hpp>

#include <cmath>

namespace sandbox {

using namespace efengine;

    OscillatorBehavior::OscillatorBehavior(const glm::vec3& a, f32 amp, f32 freq)
        : axis(glm::length(a) > 1e-6f ? glm::normalize(a) : glm::vec3(1.0f, 0.0f, 0.0f))
        , amplitude(amp), frequency(freq) {}

    void OscillatorBehavior::OnUpdate(scene::UpdateContext& ctx) {
        if (!m_hasBase) {
            m_base    = ctx.node.local.position;
            m_hasBase = true;
        }
        m_t += ctx.dt;
        math::Transform t = ctx.node.local;
        t.position = m_base + axis * (amplitude * std::sin(glm::two_pi<f32>() * frequency * m_t));
        ctx.SetLocal(t);
    }

}
