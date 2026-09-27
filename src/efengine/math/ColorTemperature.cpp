#include "efengine/math/ColorTemperature.h"

#include <algorithm>

namespace efengine {
namespace math {

    glm::vec3 KelvinToLinearRgb(f32 kelvin) {
        const f64 t  = static_cast<f64>(std::clamp(kelvin, kMinKelvin, kMaxKelvin));
        const f64 t2 = t * t;
        const f64 t3 = t2 * t;

        // Locus planckiano, cubicas de Kim et al. (2002).
        const f64 x = (t <= 4000.0)
            ? -0.2661239e9 / t3 - 0.2343589e6 / t2 + 0.8776956e3 / t + 0.179910
            : -3.0258469e9 / t3 + 2.1070379e6 / t2 + 0.2226347e3 / t + 0.240390;
        const f64 x2 = x * x;
        const f64 x3 = x2 * x;

        f64 y = 0.0;
        if (t <= 2222.0)      y = -1.1063814 * x3 - 1.34811020 * x2 + 2.18555832 * x - 0.20219683;
        else if (t <= 4000.0) y = -0.9549476 * x3 - 1.37418593 * x2 + 2.09137015 * x - 0.16748867;
        else                  y =  3.0817580 * x3 - 5.87338670 * x2 + 3.75112997 * x - 0.37001483;

        const f64 X = x / y;
        const f64 Z = (1.0 - x - y) / y;

        f64 r = std::max( 3.2404542 * X - 1.5371385 - 0.4985314 * Z, 0.0);
        f64 g = std::max(-0.9692660 * X + 1.8760108 + 0.0415560 * Z, 0.0);
        f64 b = std::max( 0.0556434 * X - 0.2040259 + 1.0572252 * Z, 0.0);

        const f64 lum = 0.2126 * r + 0.7152 * g + 0.0722 * b;
        if (lum <= 0.0) return glm::vec3(1.0f);
        return glm::vec3(static_cast<f32>(r / lum), static_cast<f32>(g / lum), static_cast<f32>(b / lum));
    }

}
}
