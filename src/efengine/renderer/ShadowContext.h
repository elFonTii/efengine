#pragma once
#include <efengine/core/Types.h>
#include <glm/glm.hpp>

namespace efengine {
namespace renderer {

    class Texture;

    struct ShadowContext {
        const Texture* map               = null;         // depth texture → unit 7
        glm::mat4      lightSpaceMatrix   = glm::mat4(1.0f);
        bool           enabled            = false;
        // En METROS, ya convertido desde ShadowSettings::normalOffsetTexels con
        // el encuadre del frame. El shader no conoce el tamaño del texel, asi
        // que la conversion se hace aca.
        f32            normalOffset       = 0.0f;
        f32            biasMin            = 0.0f;
        f32            biasMax            = 0.0f;
    };

}
}
