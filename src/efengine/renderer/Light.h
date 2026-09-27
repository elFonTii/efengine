#pragma once
#include <efengine/core/Types.h>

#include <glm/glm.hpp>

namespace efengine {
namespace renderer {

    // Point y Spot valen lo mismo que el tipo que lee el GLSL (0 y 1).
    enum class LightType : u32 { Point = 0u, Spot = 1u, Directional = 2u };

    // Una luz ya resuelta a mundo. La arma SceneGraph; el renderer no conoce nodos.
    struct Light {
        LightType type = LightType::Point;
        glm::vec3 position  { 0.0f };
        glm::vec3 direction { 0.0f, 0.0f, -1.0f };   // hacia donde VIAJA la luz
        glm::vec3 color     { 0.0f };                // tinte x temperatura x intensidad
        f32  range        = 10.0f;
        f32  cosInner     = 1.0f;
        f32  cosOuter     = 0.0f;
        f32  sourceRadius = 0.0f;
        bool castShadows  = false;
        bool primarySun   = false;
    };

}
}
