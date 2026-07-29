#include <efenet/Simulation.h>

#include <glm/gtc/constants.hpp>

#include <cmath>

namespace efenet {

    void Step(PlayerState& state, const InputCmd& input, f32 dt) {
        // La mirada la decide el cliente; el servidor la copia tal cual.
        state.yaw = input.yaw;

        // Direccion en espacio local. Forward es -Z (convencion de camara del
        // motor); Right es +X.
        f32 localX = 0.0f;
        f32 localZ = 0.0f;
        if (input.buttons & Button::Forward) localZ -= 1.0f;
        if (input.buttons & Button::Back)    localZ += 1.0f;
        if (input.buttons & Button::Left)    localX -= 1.0f;
        if (input.buttons & Button::Right)   localX += 1.0f;

        const f32 lenSq = localX * localX + localZ * localZ;
        if (lenSq > EPSILON) {
            // Normalizar: sin esto, ir en diagonal daria sqrt(2) veces la
            // velocidad. Es el bug clasico, y en red se manifiesta como desync.
            const f32 inv = 1.0f / std::sqrt(lenSq);
            localX *= inv;
            localZ *= inv;

            // Rotar por yaw alrededor de +Y.
            const f32 s = std::sin(state.yaw);
            const f32 c = std::cos(state.yaw);
            const f32 worldX = localX * c + localZ * s;
            const f32 worldZ = -localX * s + localZ * c;

            state.position.x += worldX * kMoveSpeed * dt;
            state.position.z += worldZ * kMoveSpeed * dt;
        }

        // El avatar no sale del area. Y queda fijo: no hay salto ni gravedad.
        if (state.position.x >  kPlayAreaHalfSize) state.position.x =  kPlayAreaHalfSize;
        if (state.position.x < -kPlayAreaHalfSize) state.position.x = -kPlayAreaHalfSize;
        if (state.position.z >  kPlayAreaHalfSize) state.position.z =  kPlayAreaHalfSize;
        if (state.position.z < -kPlayAreaHalfSize) state.position.z = -kPlayAreaHalfSize;
    }

}
