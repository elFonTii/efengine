#pragma once
#include <efengine/core/Types.h>

#include <glm/glm.hpp>

namespace efengine {
namespace math {

    inline constexpr f32 kMinKelvin = 1667.0f;
    inline constexpr f32 kMaxKelvin = 25000.0f;

    // Cuerpo negro a Rec.709 LINEAL con luminancia 1: cambiar los Kelvin cambia
    // el tono y no el brillo. Fuera de [kMinKelvin, kMaxKelvin] se recorta, que
    // es donde vale la aproximacion de Kim et al.
    glm::vec3 KelvinToLinearRgb(f32 kelvin);

}
}
