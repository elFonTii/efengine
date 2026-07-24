#pragma once
#include <efengine/core/Types.h>
#include <glm/glm.hpp>

namespace efengine {
namespace renderer {

    class Texture;

    // info de sombra que viajan al Renderer cada frame
    struct ShadowContext {
        const Texture* map               = null;         // depth texture → unit 7
        glm::mat4      lightSpaceMatrix   = glm::mat4(1.0f);
        bool           enabled            = false;
        f32            biasMin            = 0.0005f;
        f32            biasMax            = 0.0025f;
    };

}
}
