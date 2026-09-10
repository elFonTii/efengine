#include <efengine/physics/ShapeDesc.h>

#include <algorithm>
#include <cmath>

namespace efengine {
namespace physics {

namespace {
    // Por debajo de esto la forma no existe. No es EPSILON: un radio de 1e-6 m
    // es geometria valida en el papel y basura en un motor de fisica.
    constexpr f32 kMinEje = 1e-4f;

    bool casiIgual(f32 a, f32 b) {
        const f32 escala = std::max(1.0f, std::max(std::fabs(a), std::fabs(b)));
        return std::fabs(a - b) <= 1e-5f * escala;
    }
}

ScaledShape ScaleShape(const ShapeDesc& desc, const glm::vec3& scale) {
    ScaledShape out;
    out.desc = desc;

    // Una escala espejada (negativa) estira lo mismo que su positiva: lo unico
    // que cambia es la orientacion, y una forma de colision no la tiene.
    const glm::vec3 s { std::fabs(scale.x), std::fabs(scale.y), std::fabs(scale.z) };

    if (s.x < kMinEje || s.y < kMinEje || s.z < kMinEje) {
        out.degenerate = true;
        return out;
    }

    switch (desc.kind) {
        case ShapeKind::Box:
            out.desc.params = desc.params * s;
            out.degenerate  = out.desc.params.x < kMinEje
                           || out.desc.params.y < kMinEje
                           || out.desc.params.z < kMinEje;
            break;

        case ShapeKind::Sphere: {
            // Los tres ejes se colapsan al mayor: la esfera envuelve la malla
            // en vez de cortarla.
            const f32 factor = std::max(s.x, std::max(s.y, s.z));
            out.approximated  = !(casiIgual(s.x, s.y) && casiIgual(s.y, s.z));
            out.desc.params.x = desc.params.x * factor;
            out.degenerate    = out.desc.params.x < kMinEje;
            break;
        }

        case ShapeKind::Capsule: {
            // El eje de la capsula es Y: X y Z son el radio y tienen que ser
            // iguales, Y es el medio alto y va suelto.
            const f32 radial  = std::max(s.x, s.z);
            out.approximated  = !casiIgual(s.x, s.z);
            out.desc.params.x = desc.params.x * radial;
            out.desc.params.y = desc.params.y * s.y;
            out.degenerate    = out.desc.params.x < kMinEje
                             || out.desc.params.y < kMinEje;
            break;
        }

        case ShapeKind::Mesh:
            // La malla si escala libre en los tres ejes, pero no hay parametro
            // donde meterlo: se lo lleva el ScaledShape de Jolt.
            out.residualScale = s;
            break;
    }

    return out;
}

}
}
